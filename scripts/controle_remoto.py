#!/usr/bin/env python3
"""Controle Remoto Visual de Bancada via ESP-NOW Gateway.

Envia comandos sem fio ao Micromouse pelo teclado (via porta serial do Gateway)
e exibe a telemetria ao vivo com barras coloridas, radar de paredes e hints de
diagnóstico diferenciando problemas de Software, Eletrônica, Energia, Mecânica e RF.

Uso:
    python3 scripts/controle_remoto.py [/dev/ttyUSB0 ou COMx]
"""

import glob
import json
import os
import sys
import threading
import time

try:
    import serial
except ImportError:
    print("[ERRO] Biblioteca 'pyserial' não encontrada.")
    print("Instale com: pip install pyserial  (ou execute com src/backend/.venv/bin/python)")
    sys.exit(1)

from diagnostico_cores import (  # noqa: E402
    BADGES_DOMINIO,
    Cor,
    diagnosticar_linha_texto,
    diagnosticar_telemetria,
    formatar_caixa_dica,
    formatar_telemetria_visual,
    hora_agora,
)

# Estado global da conexão e diagnóstico do robô
ultimo_sinal_robo = 0.0
robo_online = False
trava_print = threading.Lock()
estado_diag = {
    "ultimo_status": None,
    "bat_critica": False,
    "rssi_fraco": False,
    "parado_seguidos": 0,
}


def imprimir_seguro(texto: str) -> None:
    with trava_print:
        sys.stdout.write(f"\r\033[K{texto}\n{Cor.NEGRITO}{Cor.CIANO}robo>{Cor.RESET} ")
        sys.stdout.flush()


def encontrar_porta_serial():
    if len(sys.argv) > 1:
        return sys.argv[1]

    portas = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if portas:
        return portas[0]

    if sys.platform == "win32":
        return "COM3"

    return "/dev/ttyUSB0"


def exibir_cabecalho(porta: str) -> None:
    os.system("")  # habilita sequências ANSI no console do Windows
    print(f"\n{Cor.CIANO}{'=' * 74}{Cor.RESET}")
    print(f"{Cor.NEGRITO}{Cor.BRANCO}   MICROMOUSE PI1 2026.2 · CONTROLE REMOTO & DEBUG VISUAL (ESP-NOW){Cor.RESET}")
    print(f"{Cor.CIANO}{'=' * 74}{Cor.RESET}")
    print(f"  Porta Gateway USB: {Cor.NEGRITO}{porta}{Cor.RESET} (115200 bps) · "
          f"Legenda: {BADGES_DOMINIO['SOFTWARE']} {BADGES_DOMINIO['ELETRONICA']} "
          f"{BADGES_DOMINIO['ENERGIA']} {BADGES_DOMINIO['MECANICA']} {BADGES_DOMINIO['RADIO']}")
    print(f"{Cor.CINZA}{'-' * 74}{Cor.RESET}")
    print(f"  {Cor.VERDE}[k]{Cor.RESET} Iniciar labirinto (Largada)   "
          f"{Cor.VERMELHO}[s] / [ESPAÇO]{Cor.RESET} Freio de emergência")
    print(f"  {Cor.CIANO}[g]{Cor.RESET} Alternar pista (4x4/8x4/12x4) "
          f"{Cor.CIANO}[r]{Cor.RESET} Reiniciar após fim/falha")
    print(f"  {Cor.AMARELO}[d]{Cor.RESET} Diagnóstico 1-linha           "
          f"{Cor.AMARELO}[i]{Cor.RESET} Análise de portas/I2C (USB)")
    print(f"  {Cor.AMARELO}[9]{Cor.RESET} Relatório de bateria          "
          f"{Cor.AZUL}[4/5/6/7]{Cor.RESET} 1 cél / 90°D / 90°E / 180°")
    print(f"  {Cor.AZUL}[1/2/3/8/0]{Cor.RESET} ToF/Mot/Enc/Col/Tel   "
          f"{Cor.CINZA}[h] Ajuda  |  [q] Sair{Cor.RESET}")
    print(f"{Cor.CIANO}{'=' * 74}{Cor.RESET}")
    print("-> Digite o comando e aperte ENTER:\n")


def thread_leitura_serial(ser, stop_event):
    global ultimo_sinal_robo, robo_online

    while not stop_event.is_set():
        try:
            linha = ser.readline().decode("utf-8", errors="ignore").strip()
            if not linha:
                continue

            agora = time.time()

            if linha.startswith("{"):
                try:
                    data = json.loads(linha)
                    ultimo_sinal_robo = agora
                    if not robo_online:
                        robo_online = True
                        imprimir_seguro(
                            f"{Cor.BG_VERDE} CONECTADO {Cor.RESET} "
                            f"{Cor.VERDE}Robô ESP32 respondeu via rádio ESP-NOW!{Cor.RESET}"
                        )
                    imprimir_seguro(formatar_telemetria_visual(data))
                    for caixa in diagnosticar_telemetria(data, estado_diag):
                        imprimir_seguro(caixa)
                except Exception:
                    imprimir_seguro(f"{Cor.CINZA}[JSON inválido] {linha}{Cor.RESET}")
            elif linha.startswith("#"):
                msg_fmt, dica = diagnosticar_linha_texto(linha, origem="GATEWAY")
                imprimir_seguro(msg_fmt)
                if dica:
                    imprimir_seguro(dica)
            else:
                ultimo_sinal_robo = agora
                if not robo_online:
                    robo_online = True
                    imprimir_seguro(
                        f"{Cor.BG_VERDE} CONECTADO {Cor.RESET} "
                        f"{Cor.VERDE}Robô ESP32 conectado!{Cor.RESET}"
                    )
                msg_fmt, dica = diagnosticar_linha_texto(linha, origem="ROBO")
                imprimir_seguro(msg_fmt)
                if dica:
                    imprimir_seguro(dica)
        except Exception as erro:
            if not stop_event.is_set():
                imprimir_seguro(
                    formatar_caixa_dica(
                        ["ELETRONICA", "SOFTWARE"],
                        f"Conexão serial com Gateway interrompida ({erro})",
                        "Cabo USB desconectado ou porta fechada pelo sistema.",
                        [
                            "Reconecte o Gateway ESP8266 e reinicie o script.",
                        ],
                    )
                )
            break


def thread_monitor_conexao(stop_event):
    """Monitora perda de sinal do robô se passar mais de 4 segundos sem telemetria."""
    global robo_online

    time.sleep(3.0)
    if not robo_online and not stop_event.is_set():
        imprimir_seguro(
            formatar_caixa_dica(
                ["ENERGIA", "RADIO"],
                "Robô ainda não detectado após 3 s",
                "Nenhum pacote ESP-NOW recebido pelo Gateway até agora.",
                [
                    "Energia: verifique se a chave da LiPo 2S do robô está ligada.",
                    "Software/Rádio: confirme se o robô está com firmware robo/geral (canal 1) e se o gateway está gravado.",
                ],
            )
        )

    while not stop_event.is_set():
        time.sleep(2.0)
        agora = time.time()
        if robo_online and (agora - ultimo_sinal_robo > 4.0):
            robo_online = False
            imprimir_seguro(
                formatar_caixa_dica(
                    ["RADIO", "ENERGIA"],
                    "Sinal do robô PERDIDO há mais de 4 s",
                    "O Gateway deixou de receber telemetria de 1 Hz do ESP32.",
                    [
                        "Energia: queda brusca da LiPo (< 6,8 V) pode ter reiniciado o ESP32.",
                        "Rádio: aproxime o Gateway ou confira se o robô entrou em modo de teste bloqueante.",
                    ],
                )
            )


DESCRICOES_CMD = {
    "k": ("Largada remota ('k') enviada ao robô", Cor.VERDE),
    "K": ("Largada remota ('k') enviada ao robô", Cor.VERDE),
    "s": ("FREIO DE EMERGÊNCIA enviado!", Cor.VERMELHO),
    "S": ("FREIO DE EMERGÊNCIA enviado!", Cor.VERMELHO),
    " ": ("FREIO DE EMERGÊNCIA enviado!", Cor.VERMELHO),
    "d": ("Diagnóstico rápido solicitado", Cor.CIANO),
    "D": ("Diagnóstico rápido solicitado", Cor.CIANO),
    "g": ("Alternar geometria (4x4 / 8x4 / 12x4)", Cor.CIANO),
    "G": ("Alternar geometria (4x4 / 8x4 / 12x4)", Cor.CIANO),
    "r": ("Reinício remoto para AGUARDANDO", Cor.AMARELO),
    "R": ("Reinício remoto para AGUARDANDO", Cor.AMARELO),
    "4": ("Avançar 1 célula", Cor.AZUL),
    "5": ("Curva 90° à Direita", Cor.AZUL),
    "6": ("Curva 90° à Esquerda", Cor.AZUL),
    "7": ("Meia-volta 180°", Cor.AZUL),
}


def main():
    porta = encontrar_porta_serial()
    print(f"{Cor.CIANO}[*]{Cor.RESET} Conectando ao Gateway em {Cor.NEGRITO}{porta}{Cor.RESET} a 115200 bps...")

    try:
        ser = serial.Serial(porta, 115200, timeout=0.5)
    except serial.SerialException as e:
        print(
            formatar_caixa_dica(
                ["SOFTWARE", "ELETRONICA"],
                f"Não foi possível abrir a porta serial {porta}",
                str(e),
                [
                    "Feche o Monitor Serial, backend (`uvicorn`) ou outro script que esteja ocupando a porta.",
                    "Confira a porta correta: `ls /dev/ttyUSB*` (Linux) ou Gerenciador de Dispositivos (COMx).",
                ],
            )
        )
        sys.exit(1)

    exibir_cabecalho(porta)
    print(f"{Cor.CINZA}[{hora_agora()}]{Cor.RESET} Aguardando telemetria ESP-NOW do ESP32...\n")

    stop_event = threading.Event()
    threading.Thread(target=thread_leitura_serial, args=(ser, stop_event), daemon=True).start()
    threading.Thread(target=thread_monitor_conexao, args=(stop_event,), daemon=True).start()

    try:
        while True:
            cmd = input(f"{Cor.NEGRITO}{Cor.CIANO}robo>{Cor.RESET} ").strip()
            if not cmd:
                continue

            if cmd.lower() in ("q", "quit", "exit"):
                print(f"{Cor.CINZA}[*] Encerrando controle remoto.{Cor.RESET}")
                break

            if cmd.lower() in ("h", "?"):
                exibir_cabecalho(porta)
                continue

            for char in cmd:
                ser.write(char.encode("utf-8"))
                time.sleep(0.05)

            if cmd in DESCRICOES_CMD:
                texto_cmd, cor_cmd = DESCRICOES_CMD[cmd]
                imprimir_seguro(f"{cor_cmd}>> [ENVIO] {texto_cmd}{Cor.RESET}")
            else:
                imprimir_seguro(f"{Cor.CINZA}>> [ENVIO] Comando '{cmd}' transmitido via Gateway.{Cor.RESET}")
    except KeyboardInterrupt:
        print(f"\n{Cor.CINZA}[*] Interrompido pelo usuário.{Cor.RESET}")
    finally:
        stop_event.set()
        ser.close()


if __name__ == "__main__":
    main()
