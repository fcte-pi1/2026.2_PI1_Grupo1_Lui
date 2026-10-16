"""Executa o simulador sozinho, escrevendo as linhas JSON do gateway na saída padrão.

Exemplos, a partir de src/backend:
    python -m app.simulador --labirinto 8x4 --cenario travado
    python -m app.simulador --labirinto 12x4 --tempo-real --velocidade 4
"""

import argparse
import sys
import time

from app.modelos.telemetria import LINHAS_POR_LABIRINTO
from app.simulador.gerador import CENARIOS, gerar_corrida
from app.simulador.pistas import pista_de_referencia


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="python -m app.simulador",
        description="Gera a telemetria de uma corrida simulada no formato do gateway.",
    )
    parser.add_argument(
        "--labirinto", choices=list(LINHAS_POR_LABIRINTO), default="4x4"
    )
    parser.add_argument("--cenario", choices=CENARIOS, default="completa")
    parser.add_argument("--semente", type=int, default=141)
    parser.add_argument(
        "--tempo-real",
        action="store_true",
        help="espera entre as linhas, como o robô (1 Hz)",
    )
    parser.add_argument(
        "--velocidade", type=float, default=1.0, help="fator de velocidade"
    )
    args = parser.parse_args(argv)
    if args.velocidade <= 0:
        parser.error("--velocidade deve ser positivo")

    linhas = gerar_corrida(
        pista_de_referencia(args.labirinto), args.cenario, semente=args.semente
    )
    for indice, linha in enumerate(linhas):
        if args.tempo_real and indice > 0:
            time.sleep(1.0 / args.velocidade)
        print(linha, flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
