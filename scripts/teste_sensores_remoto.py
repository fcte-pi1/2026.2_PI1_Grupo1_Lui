#!/usr/bin/env python3
"""Teste dos 3 sensores ToF sem fio, pela telemetria do gateway ESP-NOW.

A telemetria (1 Hz) traz, por sensor, se há parede a menos do limiar
(CAL_LIMIAR_PAREDE_MM, 110 mm): oeste = ESQUERDO, norte = FRONTAL, leste = DIREITO.
Não traz a distância em mm; para isso é preciso o USB no robô (env:teste_tof_*).

Aproxime a mão (a menos de ~10 cm) de UM sensor por vez e segure 2 a 3 s:
o sensor correspondente deve passar de "livre" para "PAREDE".

Uso (Windows):
    python scripts/teste_sensores_remoto.py COM5
"""

import json
import os
import shutil
import sys
import threading
import time

import serial

SILENCIO_S = 2.5
SENSORES = (("oeste", "ESQUERDO"), ("norte", "FRONTAL"), ("leste", "DIREITO"))

trava = threading.Lock()
estado = {"online": False, "ultimo_rx": 0.0, "quedas": 0, "bat": None, "pacotes": 0,
          "paredes": None, "contagem": {n: 0 for _, n in SENSORES}}


def status():
    """Linha curta (cabe numa aba estreita): E=esquerdo, F=frontal, D=direito."""
    if estado["online"]:
        robo = "ONLINE"
    elif estado["ultimo_rx"]:
        robo = "SEM SINAL"
    else:
        robo = "aguardando"
    if estado["paredes"] is None:
        sens = "E:-- F:-- D:--"
    else:
        sens = " ".join(f"{nome[0]}:{'PAREDE' if estado['paredes'][chave] else 'livre'}"
                        for chave, nome in SENSORES)
    vivo = ""
    if estado["ultimo_rx"]:
        vivo = f" | #{estado['pacotes']} {time.strftime('%H:%M:%S', time.localtime(estado['ultimo_rx']))}"
    texto = f"{robo} | {sens} | quedas {estado['quedas']}{vivo}"
    largura = min(shutil.get_terminal_size((80, 20)).columns, 72) - 1
    return texto[:largura]


def redesenhar():
    sys.stdout.write("\r\033[K" + status())
    sys.stdout.flush()


def evento(texto):
    with trava:
        sys.stdout.write("\r\033[K" + f"[{time.strftime('%H:%M:%S')}] {texto}\n")
        redesenhar()


def ler(ser, parar):
    while not parar.is_set():
        try:
            bruto = ser.readline()
        except serial.SerialException:
            evento("Gateway desconectado da USB.")
            parar.set()
            return
        linha = bruto.decode("utf-8", errors="replace").strip()
        if not linha.startswith("{"):
            continue
        try:
            dados = json.loads(linha)
        except ValueError:
            continue
        if not estado["online"]:
            evento("Robo conectado pelo radio." if not estado["ultimo_rx"]
                   else "Robo VOLTOU (reinicio ou perda de radio).")
        estado["online"] = True
        estado["ultimo_rx"] = time.time()
        estado["pacotes"] += 1
        estado["bat"] = dados.get("bateria_volts")
        p = dados.get("paredes", {})
        novo = {chave: bool(p.get(chave)) for chave, _ in SENSORES}
        antigo = estado["paredes"]
        if antigo is not None:
            for chave, nome in SENSORES:
                if novo[chave] != antigo[chave]:
                    if novo[chave]:
                        estado["contagem"][nome] += 1
                    evento(f"{nome:8s} -> {'PAREDE (algo a < 110 mm)' if novo[chave] else 'livre'}")
        estado["paredes"] = novo
        with trava:
            redesenhar()


def vigiar(parar):
    while not parar.is_set():
        time.sleep(0.3)
        if estado["online"] and time.time() - estado["ultimo_rx"] > SILENCIO_S:
            estado["online"] = False
            estado["quedas"] += 1
            evento("Robo SEM SINAL.")
        with trava:
            redesenhar()


def main():
    porta = sys.argv[1] if len(sys.argv) > 1 else "COM5"
    os.system("")
    avisou = False
    while True:
        try:
            ser = serial.Serial(porta, 115200, timeout=0.3)
            break
        except serial.SerialException:
            if not avisou:
                print(f"{porta} ocupada (feche o outro programa com 'q'). Esperando liberar... Ctrl+C cancela.")
                avisou = True
            try:
                time.sleep(1.0)
            except KeyboardInterrupt:
                sys.exit(1)
    print("=" * 72)
    print(" TESTE DOS SENSORES ToF SEM FIO - so observa, nao mexe nos motores")
    print("=" * 72)
    print("  Ponha a mao a menos de ~10 cm de UM sensor por vez e segure 3 s.")
    print("  O sensor deve ir para PAREDE e voltar para livre ao tirar a mao.")
    print("  Telemetria a 1 Hz: espere ~2 s para cada mudanca. Ctrl+C sai.")
    print("=" * 72)
    parar = threading.Event()
    threading.Thread(target=ler, args=(ser, parar), daemon=True).start()
    threading.Thread(target=vigiar, args=(parar,), daemon=True).start()
    try:
        while not parar.is_set():
            time.sleep(0.2)
    except KeyboardInterrupt:
        pass
    finally:
        parar.set()
        ser.close()
        print("\nResumo (vezes que cada sensor detectou a mao):")
        for _, nome in SENSORES:
            print(f"  {nome:8s}: {estado['contagem'][nome]}")


if __name__ == "__main__":
    main()
