#!/usr/bin/env python3
"""Teste de motores sem fio, tecla a tecla, pelo gateway ESP-NOW.

Usa o modo [2] (teste manual de motores) do firmware de bancada (src/firmware/bancada).
Diferenças em relação ao controle_remoto.py:
  - cada tecla vai na hora, sem ENTER;
  - a telemetria fica numa única linha de status (não rola a tela);
  - todo comando de motor é precedido de '2' (garante o modo de motores mesmo
    depois de um reinício do robô) e reenviado, porque o ESP-NOW pode perder pacotes;
  - avisa quando o robô fica mudo e volta (provável reinício por queda de tensão).

Uso:
    python3 scripts/teste_motores_remoto.py [/dev/ttyUSB0 ou COM5]
"""

import json
import os
import shutil
import sys
import threading
import time

if os.name == "nt":
    import msvcrt
else:
    import select
    import termios
    import tty

import serial

REPETICOES = 3          # quantas vezes cada tecla é enviada
INTERVALO_S = 0.04      # pausa entre envios
SILENCIO_REINICIO_S = 2.5  # telemetria é 1 Hz: mais que isso calado = sumiu

TECLAS = {
    "e": ("Esquerda FRENTE", True),
    "E": ("Esquerda RE", True),
    "d": ("Direita FRENTE", True),
    "D": ("Direita RE", True),
    "+": ("PWM +20", True),
    "-": ("PWM -20", True),
    "s": ("FREIO", False),
    " ": ("FREIO", False),
    "x": ("Parar sem freio e sair do modo", False),
}

trava = threading.Lock()
estado = {
    "online": False,
    "ultimo_rx": 0.0,
    "sumiu_em": None,
    "reinicios": 0,
    "bat": None,
    "vel": None,
    "paredes": "----",
    "ultimo_cmd": "-",
    "ultimo_cmd_t": None,
}


def agora_txt():
    return time.strftime("%H:%M:%S")


def linha_status():
    """Linha curta, sem cor, cortada na largura da janela (senão quebra e polui)."""
    if estado["online"]:
        robo = "ONLINE"
    elif estado["ultimo_rx"]:
        robo = "SEM SINAL"
    else:
        robo = "aguardando"
    bat = f"{estado['bat']:.2f}V" if estado["bat"] is not None else "--"
    vel = f"{estado['vel']:.0f}" if estado["vel"] is not None else "--"
    dt = ""
    if estado["ultimo_cmd_t"]:
        dt = f" {time.time() - estado['ultimo_cmd_t']:.0f}s"
    texto = (f"{robo} | bat {bat} | vel {vel} mm/s | reinicios {estado['reinicios']} | "
             f"ultimo: {estado['ultimo_cmd']}{dt}")
    largura = shutil.get_terminal_size((80, 20)).columns - 1
    return texto[:largura]


def desenhar_status():
    sys.stdout.write("\r\033[K" + linha_status())
    sys.stdout.flush()


def evento(texto):
    """Imprime um evento acima da linha de status."""
    with trava:
        sys.stdout.write("\r\033[K" + f"[{agora_txt()}] {texto}\n")
        desenhar_status()


def ler_serial(ser, parar):
    while not parar.is_set():
        try:
            bruto = ser.readline()
        except serial.SerialException:
            evento("\033[1;31mGateway desconectado da USB.\033[0m")
            parar.set()
            return
        if not bruto:
            continue
        linha = bruto.decode("utf-8", errors="replace").strip()
        if linha.startswith("{"):
            try:
                dados = json.loads(linha)
            except ValueError:
                continue
            t = time.time()
            voltou = not estado["online"]
            estado["online"] = True
            estado["ultimo_rx"] = t
            estado["bat"] = dados.get("bateria_volts")
            estado["vel"] = dados.get("velocidade_atual")
            p = dados.get("paredes", {})
            estado["paredes"] = ("N" if p.get("norte") else "-") + ("S" if p.get("sul") else "-") + \
                                ("L" if p.get("leste") else "-") + ("O" if p.get("oeste") else "-")
            if voltou:
                if estado["sumiu_em"] is not None:
                    estado["reinicios"] += 1
                    calado = t - estado["sumiu_em"]
                    evento(f"\033[1;33mRobo VOLTOU depois de {calado:.1f}s calado: provavel REINICIO "
                           f"(queda de tensao). O proximo comando de motor ja reentra no modo 2.\033[0m")
                else:
                    evento("\033[1;32mRobo conectado pelo radio.\033[0m")
            with trava:
                desenhar_status()
        elif linha.startswith("#"):
            evento(f"[gateway] {linha}")


def vigiar_silencio(parar):
    while not parar.is_set():
        time.sleep(0.3)
        if estado["online"] and time.time() - estado["ultimo_rx"] > SILENCIO_REINICIO_S:
            estado["online"] = False
            estado["sumiu_em"] = estado["ultimo_rx"]
            evento(f"\033[1;31mRobo SEM SINAL (ultimo comando: {estado['ultimo_cmd']}).\033[0m")
        with trava:
            desenhar_status()


def enviar(ser, tecla):
    nome, precisa_modo = TECLAS[tecla]
    # +/- somam 20 a cada recebimento: vão uma vez só para não pular 60 de PWM.
    repeticoes = 1 if tecla in "+-" else REPETICOES
    for _ in range(repeticoes):
        if precisa_modo:
            ser.write(b"2")
            time.sleep(INTERVALO_S)
        ser.write(tecla.encode("ascii"))
        time.sleep(INTERVALO_S)
    estado["ultimo_cmd"] = f"'{tecla.strip() or 'espaco'}' {nome}"
    estado["ultimo_cmd_t"] = time.time()
    evento(f"Enviado: {estado['ultimo_cmd']}")


def ler_tecla_nao_bloqueante():
    if os.name == "nt":
        if not msvcrt.kbhit():
            return None
        tecla = msvcrt.getwch()
        if tecla in ("\x00", "\xe0"):
            msvcrt.getwch()
            return ""
        return tecla
    if select.select([sys.stdin], [], [], 0.02)[0]:
        return sys.stdin.read(1)
    return None


def main():
    porta_padrao = "COM5" if os.name == "nt" else "/dev/ttyUSB0"
    porta = sys.argv[1] if len(sys.argv) > 1 else porta_padrao
    os.system("")  # liga as sequências ANSI no console do Windows
    try:
        ser = serial.Serial(porta, 115200, timeout=0.3)
    except serial.SerialException as erro:
        print(f"Nao abriu {porta}: {erro}")
        sys.exit(1)

    print("=" * 78)
    print(" TESTE DE MOTORES SEM FIO (ESP-NOW) - aperte a tecla, sem ENTER")
    print("=" * 78)
    print("  e / E : roda ESQUERDA frente / re     d / D : roda DIREITA frente / re")
    print("  s ou ESPACO : FREIO                    x : para sem freio")
    print("  + / - : PWM +20 / -20                  q : freia e sai")
    print(f"  Gateway: {porta}. Cada comando de motor vai {REPETICOES}x (+/- vao 1x) e ja reentra no modo 2.")
    print("=" * 78)

    parar = threading.Event()
    threading.Thread(target=ler_serial, args=(ser, parar), daemon=True).start()
    threading.Thread(target=vigiar_silencio, args=(parar,), daemon=True).start()

    fd_stdin = None
    attrs_orig = None
    if os.name != "nt" and sys.stdin.isatty():
        fd_stdin = sys.stdin.fileno()
        attrs_orig = termios.tcgetattr(fd_stdin)
        tty.setcbreak(fd_stdin)

    try:
        while not parar.is_set():
            tecla = ler_tecla_nao_bloqueante()
            if tecla is None:
                if os.name == "nt":
                    time.sleep(0.02)
                continue
            if tecla == "":
                continue
            if tecla in ("q", "Q", "\x03"):
                enviar(ser, "s")
                break
            if tecla == "S":
                tecla = "s"
            if tecla == "X":
                tecla = "x"
            if tecla in TECLAS:
                enviar(ser, tecla)
            else:
                evento(f"Tecla '{tecla}' sem uso aqui.")
    except KeyboardInterrupt:
        enviar(ser, "s")
    finally:
        if attrs_orig is not None:
            termios.tcsetattr(fd_stdin, termios.TCSADRAIN, attrs_orig)
        parar.set()
        time.sleep(0.2)
        ser.close()
        print("\nEncerrado (freio enviado).")


if __name__ == "__main__":
    main()
