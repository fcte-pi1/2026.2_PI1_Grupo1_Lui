#!/usr/bin/env python3
"""Monitor ao vivo do WebSocket de telemetria do Micromouse.

Conecta em ws://localhost:8000/ws/telemetria e imprime cada pacote recebido,
além de salvar um histórico contínuo em telemetria_log.jsonl.
"""
import asyncio
import json
import os
import sys
from datetime import datetime

try:
    import websockets
except ImportError:
    print("Instale websockets ou rode com o ambiente virtual do backend (.venv/bin/python)")
    sys.exit(1)

LOG_FILE = os.path.join(os.path.dirname(__file__), "telemetria_log.jsonl")

async def monitor():
    uri = os.getenv("WS_URI", "ws://localhost:8000/ws/telemetria")
    print(f"[*] Conectando ao WebSocket de telemetria em {uri}...")
    print(f"[*] Salvando cópia de todos os pacotes em: {LOG_FILE}\n")
    print("=" * 80)
    print("  HORA     | STATUS     | BAT (%) | VEL (mm/s) | PAREDES (F/E/D) | POS  | RSSI")
    print("=" * 80)

    reconnect_delay = 2.0
    while True:
        try:
            async with websockets.connect(uri) as ws:
                print(f"[+] Conectado ao servidor! Aguardando transmissões do robô...")
                async for raw in ws:
                    agora = datetime.now().strftime("%H:%M:%S")
                    with open(LOG_FILE, "a", encoding="utf-8") as f:
                        f.write(raw + "\n")

                    try:
                        data = json.loads(raw)
                        st = data.get("status", "---")[:10].ljust(10)
                        bat_pct = data.get("bateria_pct", 0)
                        bat_v = data.get("bateria_volts", 0.0)
                        vel_atual = data.get("velocidade_atual", 0)
                        vel_med = data.get("velocidade_media", 0)
                        rssi = data.get("rssi", "N/A")
                        
                        pos = data.get("posicao", {})
                        col = pos.get("coluna", "?")
                        lin = pos.get("linha", "?")
                        pos_str = f"{col}{lin}".ljust(4)

                        paredes = data.get("paredes", {})
                        pf = "SIM" if paredes.get("norte") else "---"
                        pe = "SIM" if paredes.get("oeste") else "---"
                        pd = "SIM" if paredes.get("leste") else "---"
                        paredes_str = f"F:{pf} E:{pe} D:{pd}"

                        print(f"[{agora}] | {st} | {bat_pct:3d}% ({bat_v:.2f}V) | {vel_atual:4d} (med {vel_med:4d}) | {paredes_str} | {pos_str} | {rssi} dBm")
                    except Exception as e:
                        print(f"[{agora}] RAW: {raw}")
        except (websockets.ConnectionClosed, OSError) as e:
            print(f"[!] Conexão perdida com o WebSocket: {e}. Reconectando em {reconnect_delay}s...")
            await asyncio.sleep(reconnect_delay)

if __name__ == "__main__":
    try:
        asyncio.run(monitor())
    except KeyboardInterrupt:
        print("\n[*] Monitor encerrado.")
