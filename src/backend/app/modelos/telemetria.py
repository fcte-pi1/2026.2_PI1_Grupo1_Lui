"""Esquema de validação da mensagem de telemetria.

Espelha o contrato de dados congelado (src/README.md e doc 4.4, seções 6.4 e 6.5).
A mesma mensagem chega do gateway pela serial e é difundida pelo WebSocket.
"""

from typing import Literal

from pydantic import BaseModel, ConfigDict, Field, model_validator

Labirinto = Literal["4x4", "8x4", "12x4"]
Status = Literal[
    "AGUARDANDO",
    "MAPEANDO",
    "SPEED_RUN",
    "COLISAO",
    "CONCLUIDO",
    "TIMEOUT",
    "TRAVADO",
]
Orientacao = Literal["NORTE", "SUL", "LESTE", "OESTE"]
Coluna = Literal["A", "B", "C", "D"]

LINHAS_POR_LABIRINTO: dict[str, int] = {"4x4": 4, "8x4": 8, "12x4": 12}
TIMESTAMP_MAX_MS = 600_000  # Limite de 10 minutos de prova

# Limites herdados dos tipos do pacote binário de 18 bytes (doc 4.4, seção 6.2):
# bateria em uint16 mV e velocidades em int16 mm/s, convertidas pelo gateway.
BATERIA_VOLTS_MAX = 65.535
VELOCIDADE_MIN_CM_S = -3276.8
VELOCIDADE_MAX_CM_S = 3276.7


class _Contrato(BaseModel):
    # Campos desconhecidos são ignorados: se o gateway ganhar um campo novo e o
    # backend não for atualizado, os pacotes continuam válidos durante a prova.
    # Os tipos são estritos: "78" não vale como inteiro nem 1 como booleano.
    model_config = ConfigDict(extra="ignore", strict=True)


class Posicao(_Contrato):
    coluna: Coluna
    linha: int = Field(ge=1, le=12)


class Paredes(_Contrato):
    norte: bool
    sul: bool
    leste: bool
    oeste: bool


class TelemetriaMensagem(_Contrato):
    timestamp_ms: int = Field(ge=0, le=TIMESTAMP_MAX_MS)
    labirinto: Labirinto
    status: Status
    desafio_cumprido: bool
    bateria_pct: int = Field(ge=0, le=100)
    bateria_volts: float = Field(ge=0, le=BATERIA_VOLTS_MAX)
    velocidade_atual: float = Field(ge=VELOCIDADE_MIN_CM_S, le=VELOCIDADE_MAX_CM_S)
    velocidade_media: float = Field(ge=VELOCIDADE_MIN_CM_S, le=VELOCIDADE_MAX_CM_S)
    posicao: Posicao
    orientacao: Orientacao
    paredes: Paredes

    # Acrescentados pelo gateway: número de sequência (começa em 1) e RSSI em dBm.
    seq: int | None = Field(default=None, ge=1, le=2**32 - 1)
    rssi: int | None = Field(default=None, ge=-128, le=127)

    @model_validator(mode="after")
    def _linha_dentro_da_geometria(self) -> "TelemetriaMensagem":
        limite = LINHAS_POR_LABIRINTO[self.labirinto]
        if self.posicao.linha > limite:
            raise ValueError(
                f"linha {self.posicao.linha} fora da geometria {self.labirinto} (1 a {limite})"
            )
        return self
