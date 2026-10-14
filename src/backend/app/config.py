"""Configuração do backend lida das variáveis de ambiente.

TELEMETRIA_FONTE     nenhuma (padrão), simulador ou serial
SERIAL_PORTA         porta do gateway, por exemplo COM3 ou /dev/ttyUSB0 (fonte serial)
SERIAL_BAUD          padrão 115200
SIMULADOR_LABIRINTO  4x4 (padrão), 8x4 ou 12x4
SIMULADOR_CENARIO    completa (padrão), travado ou perda_conexao
SIMULADOR_VELOCIDADE fator de velocidade; 1 = uma mensagem por segundo (padrão)
SIMULADOR_REPETIR    1 para recomeçar a corrida ao terminar (padrão 0)
SIMULADOR_SEMENTE    semente dos valores aleatórios (padrão 141)

O padrão é nenhuma fonte, para que um esquecimento no dia da prova não exiba uma
corrida simulada como se fosse real.
"""

import os
from collections.abc import Mapping
from dataclasses import dataclass

from app.modelos.telemetria import LINHAS_POR_LABIRINTO
from app.simulador.gerador import CENARIOS

FONTES = ("nenhuma", "simulador", "serial")


@dataclass(frozen=True)
class Configuracao:
    fonte: str = "nenhuma"
    serial_porta: str | None = None
    serial_baud: int = 115200
    simulador_labirinto: str = "4x4"
    simulador_cenario: str = "completa"
    simulador_velocidade: float = 1.0
    simulador_repetir: bool = False
    simulador_semente: int = 141

    @classmethod
    def do_ambiente(cls, ambiente: Mapping[str, str] | None = None) -> "Configuracao":
        env = os.environ if ambiente is None else ambiente

        def valor(nome: str, padrao: str) -> str:
            return env.get(nome, "").strip() or padrao

        fonte = valor("TELEMETRIA_FONTE", "nenhuma").lower()
        _exigir(
            fonte in FONTES,
            f"TELEMETRIA_FONTE inválida: {fonte!r} (use {', '.join(FONTES)})",
        )

        porta = valor("SERIAL_PORTA", "") or None
        _exigir(
            fonte != "serial" or porta is not None,
            "TELEMETRIA_FONTE=serial exige SERIAL_PORTA",
        )

        labirinto = valor("SIMULADOR_LABIRINTO", "4x4").lower()
        _exigir(
            labirinto in LINHAS_POR_LABIRINTO,
            f"SIMULADOR_LABIRINTO inválido: {labirinto!r}",
        )

        cenario = valor("SIMULADOR_CENARIO", "completa").lower()
        _exigir(
            cenario in CENARIOS,
            f"SIMULADOR_CENARIO inválido: {cenario!r} (use {', '.join(CENARIOS)})",
        )

        velocidade = _numero(
            valor("SIMULADOR_VELOCIDADE", "1"), float, "SIMULADOR_VELOCIDADE"
        )
        _exigir(velocidade > 0, "SIMULADOR_VELOCIDADE deve ser positivo")

        repetir = valor("SIMULADOR_REPETIR", "0").lower()
        _exigir(
            repetir in ("0", "1", "false", "true"), "SIMULADOR_REPETIR deve ser 0 ou 1"
        )

        return cls(
            fonte=fonte,
            serial_porta=porta,
            serial_baud=_numero(valor("SERIAL_BAUD", "115200"), int, "SERIAL_BAUD"),
            simulador_labirinto=labirinto,
            simulador_cenario=cenario,
            simulador_velocidade=velocidade,
            simulador_repetir=repetir in ("1", "true"),
            simulador_semente=_numero(
                valor("SIMULADOR_SEMENTE", "141"), int, "SIMULADOR_SEMENTE"
            ),
        )


def _exigir(condicao: bool, mensagem: str) -> None:
    if not condicao:
        raise ValueError(mensagem)


def _numero(texto: str, tipo, nome: str):
    try:
        return tipo(texto)
    except ValueError:
        raise ValueError(f"{nome} inválido: {texto!r}") from None
