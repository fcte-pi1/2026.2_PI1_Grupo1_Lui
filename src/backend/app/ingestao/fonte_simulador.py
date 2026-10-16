"""Fonte de telemetria que reproduz uma corrida simulada no ritmo do robô (1 Hz)."""

import asyncio
from collections.abc import AsyncIterator, Awaitable, Callable

from app.modelos.telemetria import TelemetriaMensagem
from app.simulador.gerador import gerar_corrida
from app.simulador.pistas import Pista


class FonteSimulador:
    def __init__(
        self,
        pista: Pista,
        cenario: str = "completa",
        *,
        fator_velocidade: float = 1.0,
        repetir: bool = False,
        semente: int = 141,
        intervalo_s: float = 1.0,
        pausa_entre_corridas_s: float = 5.0,
        dormir: Callable[[float], Awaitable[None]] = asyncio.sleep,
    ):
        if fator_velocidade <= 0:
            raise ValueError("o fator de velocidade deve ser positivo")
        self.pista = pista
        self.cenario = cenario
        self.repetir = repetir
        self.semente = semente
        self.intervalo_s = intervalo_s / fator_velocidade
        self.pausa_entre_corridas_s = pausa_entre_corridas_s / fator_velocidade
        self._dormir = dormir

    async def mensagens(self) -> AsyncIterator[TelemetriaMensagem]:
        seq = 1
        while True:
            linhas = gerar_corrida(
                self.pista, self.cenario, semente=self.semente, seq_inicial=seq
            )
            for indice, linha in enumerate(linhas):
                if indice > 0:
                    await self._dormir(self.intervalo_s)
                # Mesmo caminho da serial: o texto do gateway é validado pelo schema.
                yield TelemetriaMensagem.model_validate_json(linha)
            if not self.repetir:
                return
            # O gateway não reinicia o seq entre corridas.
            seq += len(linhas)
            await self._dormir(self.pausa_entre_corridas_s)
