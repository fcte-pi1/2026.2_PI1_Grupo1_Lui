"""Envia ao painel a prova do robô virtual, no lugar do gateway.

O teste test_robo_virtual grava as linhas JSON que o gateway geraria (mesma conversão,
GatewayBridge da lib/telemetria) quando a variável ROBO_VIRTUAL_SAIDA aponta para uma pasta.
Este utilitário serve essas linhas por TCP, uma por segundo, como a porta serial do gateway.
O backend lê pela FonteSerial com uma URL do pyserial, sem mudança no backend nem no
frontend:

    TELEMETRIA_FONTE=serial
    SERIAL_PORTA=socket://localhost:7777

Uso (PowerShell, a partir de src/firmware/robo):

    $env:ROBO_VIRTUAL_SAIDA = "$PWD\\.pio\\robo_virtual"
    New-Item -ItemType Directory -Force $env:ROBO_VIRTUAL_SAIDA
    ..\\.venv\\Scripts\\python.exe -m platformio test -e native -f test_robo_virtual
    python tools\\painel_virtual.py .pio\\robo_virtual\\robo_virtual_12x4.jsonl

Só biblioteca padrão do Python. O servidor espera o backend conectar, envia as linhas no
ritmo dos pacotes (1 s) e, no fim, mantém a conexão aberta sem enviar nada, como um gateway
ligado com o robô parado; o painel acusa a perda de sinal pelo watchdog. Ctrl+C encerra.
"""

import argparse
import socket
import sys
import time
from pathlib import Path


def ler_linhas(caminho: Path) -> list[bytes]:
    linhas = [linha.strip() for linha in caminho.read_text(encoding="utf-8").splitlines()]
    return [(linha + "\n").encode("utf-8") for linha in linhas if linha]


def servir(linhas: list[bytes], porta: int, intervalo: float, repetir: bool) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as servidor:
        servidor.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        servidor.bind(("127.0.0.1", porta))
        servidor.listen(1)
        print(f"Aguardando o backend em socket://localhost:{porta} ({len(linhas)} pacotes)...")
        while True:
            conexao, endereco = servidor.accept()
            print(f"Backend conectado de {endereco[0]}:{endereco[1]}.")
            try:
                with conexao:
                    while True:
                        for numero, linha in enumerate(linhas, start=1):
                            conexao.sendall(linha)
                            print(f"  pacote {numero}/{len(linhas)}", end="\r", flush=True)
                            time.sleep(intervalo)
                        print(f"\nProva enviada ({len(linhas)} pacotes).")
                        if not repetir:
                            break
                    print("Conexão mantida sem envio (Ctrl+C encerra).")
                    while conexao.recv(1024):
                        pass  # o backend não envia nada; recv volta vazio quando ele fecha
            except (ConnectionResetError, BrokenPipeError, ConnectionAbortedError):
                pass
            print("Backend desconectado; aguardando nova conexão.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("arquivo", type=Path, help="arquivo .jsonl gravado pelo test_robo_virtual")
    parser.add_argument("--porta", type=int, default=7777, help="porta TCP (padrão: 7777)")
    parser.add_argument("--intervalo", type=float, default=1.0,
                        help="segundos entre pacotes (padrão: 1,0, o ritmo da telemetria)")
    parser.add_argument("--repetir", action="store_true", help="reenviar a prova em laço")
    args = parser.parse_args()

    if not args.arquivo.is_file():
        print(f"Arquivo não encontrado: {args.arquivo}", file=sys.stderr)
        return 1
    linhas = ler_linhas(args.arquivo)
    if not linhas:
        print(f"Arquivo vazio: {args.arquivo}", file=sys.stderr)
        return 1
    try:
        servir(linhas, args.porta, args.intervalo, args.repetir)
    except KeyboardInterrupt:
        print("\nEncerrado.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
