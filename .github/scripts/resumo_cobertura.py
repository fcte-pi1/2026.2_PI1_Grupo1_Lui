"""Converte um relatório de cobertura em tabela markdown para o resumo do GitHub Actions.

Formatos aceitos:
  gcovr     saída de `gcovr --json-summary` (firmware)
  istanbul  coverage/coverage-summary.json do Vitest (frontend)

Uso:
  python resumo_cobertura.py gcovr cobertura.json --titulo "Firmware" >> "$GITHUB_STEP_SUMMARY"
"""

import argparse
import json
import os
import sys

METRICAS = ("linhas", "funcoes", "ramos")


def _contagens_gcovr(item):
    return {
        "linhas": (item["line_covered"], item["line_total"]),
        "funcoes": (item["function_covered"], item["function_total"]),
        "ramos": (item["branch_covered"], item["branch_total"]),
    }


def _contagens_istanbul(item):
    return {
        "linhas": (item["lines"]["covered"], item["lines"]["total"]),
        "funcoes": (item["functions"]["covered"], item["functions"]["total"]),
        "ramos": (item["branches"]["covered"], item["branches"]["total"]),
    }


def ler_gcovr(dados):
    arquivos = [(f["filename"], _contagens_gcovr(f)) for f in dados["files"]]
    return _contagens_gcovr(dados), sorted(arquivos)


def ler_istanbul(dados, base):
    # O Vitest grava caminhos absolutos; a tabela mostra-os relativos ao projeto.
    arquivos = [
        (os.path.relpath(caminho, base).replace(os.sep, "/"), _contagens_istanbul(item))
        for caminho, item in dados.items()
        if caminho != "total"
    ]
    return _contagens_istanbul(dados["total"]), sorted(arquivos)


def _celula(cobertos, total):
    if total == 0:
        return "-"
    return f"{cobertos}/{total} ({100 * cobertos / total:.1f}%)"


def gerar_markdown(titulo, total, arquivos):
    linhas = [
        f"### {titulo}",
        "",
        "| Arquivo | Linhas | Funções | Ramos |",
        "|:---|---:|---:|---:|",
    ]
    for nome, contagens in arquivos:
        celulas = " | ".join(_celula(*contagens[m]) for m in METRICAS)
        linhas.append(f"| `{nome}` | {celulas} |")
    celulas = " | ".join(f"**{_celula(*total[m])}**" for m in METRICAS)
    linhas.append(f"| **Total** | {celulas} |")
    return "\n".join(linhas) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("formato", choices=["gcovr", "istanbul"])
    parser.add_argument("arquivo")
    parser.add_argument("--titulo", required=True)
    parser.add_argument(
        "--base", default=os.getcwd(), help="raiz para caminhos relativos (istanbul)"
    )
    args = parser.parse_args()

    with open(args.arquivo, encoding="utf-8") as f:
        dados = json.load(f)
    if args.formato == "gcovr":
        total, arquivos = ler_gcovr(dados)
    else:
        total, arquivos = ler_istanbul(dados, args.base)
    sys.stdout.reconfigure(encoding="utf-8")
    print(gerar_markdown(args.titulo, total, arquivos))


if __name__ == "__main__":
    main()
