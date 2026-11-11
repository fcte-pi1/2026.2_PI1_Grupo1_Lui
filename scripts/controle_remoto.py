#!/usr/bin/env python3
"""Controle Remoto de Bancada via ESP-NOW Gateway.

Permite enviar comandos sem fio ao Micromouse pelo teclado (via porta serial do Gateway)
e exibir a telemetria do robô ao vivo no terminal.

Uso:
    python3 scripts/controle_remoto.py [/dev/ttyUSB0 ou COMx]
"""

import sys
import os
import glob
import time
import threading
import json

try:
    import serial
except ImportError:
    print("[ERRO] Biblioteca 'pyserial' não encontrada.")
    print("Instale com: pip install pyserial  (ou execute com src/backend/.venv/bin/python)")
    sys.exit(1)

def encontrar_porta_serial():
    if len(sys.argv) > 1:
        return sys.argv[1]
    
    # Busca portas comuns no Linux/Mac/Windows
    portas = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if portas:
        return portas[0]
    
    if sys.platform == "win32":
        return "COM3"
    
    return "/dev/ttyUSB0"

def exibir_cabecalho(porta):
    print("\n" + "=" * 65)
    print("   MICROMOUSE PI1 2026.2 - CONTROLE REMOTO SEM FIO (ESP-NOW)    ")
    print("=" * 65)
    print(f"Porta do Gateway: {porta} (115200 bps)")
    print("-" * 65)
    print("COMANDOS DE MOVIMENTO:")
    print("  [4] - Avançar 1 Célula (168 mm) com Controle PD")
    print("  [5] - Curva de 90° à Direita no próprio eixo")
    print("  [6] - Curva de 90° à Esquerda no próprio eixo")
    print("  [7] - Meia-Volta (180°) no próprio eixo")
    print("  [8] - Avanço com Parada Anti-Colisão (< 100 mm)")
    print("  [s] ou [ESPAÇO] - PARADA DE EMERGÊNCIA (Freio Ativo)")
    print("OUTROS MODOS:")
    print("  [1] - Leitura contínua dos ToF   | [3] - Encoders manual")
    print("  [9] - Diagnóstico da Bateria     | [g] - Alternar labirinto")
    print("  [q] - Sair deste programa")
    print("=" * 65)
    print("-> Digite o comando e aperte ENTER (ou digite 's' para frear):\n")

def thread_leitura_serial(ser, stop_event):
    while not stop_event.is_set():
        try:
            linha = ser.readline().decode("utf-8", errors="ignore").strip()
            if not linha:
                continue
            
            if linha.startswith("{"):
                try:
                    data = json.loads(linha)
                    st = data.get("status", "---")
                    bat_pct = data.get("bateria_pct", 0)
                    bat_v = data.get("bateria_volts", 0.0)
                    pos = data.get("posicao", {})
                    col = pos.get("coluna", "?")
                    lin = pos.get("linha", "?")
                    ori = data.get("orientacao", "?")
                    rssi = data.get("rssi", "N/A")
                    vel = data.get("velocidade_atual", 0)
                    paredes = data.get("paredes", {})
                    pf = "N" if paredes.get("norte") else "-"
                    ps = "S" if paredes.get("sul") else "-"
                    pl = "L" if paredes.get("leste") else "-"
                    po = "O" if paredes.get("oeste") else "-"

                    print(f"\r[TELEMETRIA] Status: {st:<10} | Pos: {col}{lin} ({ori}) | Bat: {bat_pct}% ({bat_v:.2f}V) | Vel: {vel} mm/s | Paredes: [{pf}{ps}{pl}{po}] | RSSI: {rssi} dBm\n> ", end="", flush=True)
                except Exception:
                    print(f"\r[JSON] {linha}\n> ", end="", flush=True)
            elif linha.startswith("#"):
                print(f"\r[GATEWAY] {linha}\n> ", end="", flush=True)
            else:
                print(f"\r[ROBO] {linha}\n> ", end="", flush=True)
        except Exception:
            break

def main():
    porta = encontrar_porta_serial()
    print(f"[*] Conectando ao Gateway em {porta} a 115200 bps...")

    try:
        ser = serial.Serial(porta, 115200, timeout=0.5)
    except serial.SerialException as e:
        print(f"[ERRO] Não foi possível abrir a porta {porta}: {e}")
        print("Dica: Especifique a porta manualmente: python3 scripts/controle_remoto.py /dev/ttyUSB0 (ou COMx)")
        sys.exit(1)

    exibir_cabecalho(porta)

    stop_event = threading.Event()
    leitor = threading.Thread(target=thread_leitura_serial, args=(ser, stop_event), daemon=True)
    leitor.start()

    try:
        while True:
            cmd = input("> ").strip()
            if not cmd:
                continue
            
            if cmd.lower() in ('q', 'quit', 'exit'):
                print("[*] Encerrando controle remoto.")
                break

            for char in cmd:
                ser.write(char.encode("utf-8"))
                time.sleep(0.05)
            
            if cmd == '4':
                print(">> [COMANDO ENVIADO] Avançar 1 célula (168 mm)...")
            elif cmd == '5':
                print(">> [COMANDO ENVIADO] Curva 90° à Direita...")
            elif cmd == '6':
                print(">> [COMANDO ENVIADO] Curva 90° à Esquerda...")
            elif cmd == '7':
                print(">> [COMANDO ENVIADO] Meia-volta 180°...")
            elif cmd in ('s', 'S', ' '):
                print(">> [COMANDO ENVIADO] FREIO DE EMERGÊNCIA!")
    except KeyboardInterrupt:
        print("\n[*] Interrompido pelo usuário.")
    finally:
        stop_event.set()
        ser.close()

if __name__ == "__main__":
    main()
