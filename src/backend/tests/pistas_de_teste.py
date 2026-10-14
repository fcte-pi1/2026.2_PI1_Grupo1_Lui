"""Pistas usadas só nos testes, além das de referência dos slides."""

from app.simulador.pistas import interpretar_desenho

# Todas as 16 células são alcançáveis e há um ciclo entre C1, D1, D2 e C2
# (as pistas dos slides de 8x4 e 12x4 têm regiões fechadas).
PISTA_TODA_ALCANCAVEL = """\
+---+---+---+---+
| L     |       |
+---+   +   +   +
|       |       |
+   +---+   +   +
|   |       |   |
+   +   +---+   +
|       |     C |
+---+---+---+---+
"""


def desenho(texto: str) -> list[str]:
    return texto.strip("\n").splitlines()


def pista_toda_alcancavel():
    return interpretar_desenho(desenho(PISTA_TODA_ALCANCAVEL), "4x4")
