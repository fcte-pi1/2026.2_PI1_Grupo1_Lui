"""Fonte de telemetria que reproduz uma corrida simulada no ritmo do robô (1 Hz).

Como o robô real, que transmite em AGUARDANDO até o operador dar a largada, o
simulador pode ficar em espera na largada até haver um painel conectado (ha_painel).
Sem isso, a corrida começaria assim que o backend sobe e o painel, que leva alguns
segundos para conectar, perderia a largada e as primeiras células.
"""

import asyncio
import logging
from collections.abc import AsyncIterator, Awaitable, Callable

from app.modelos.telemetria import TelemetriaMensagem
from app.simulador.gerador import gerar_corrida
from app.simulador.pistas import Pista

logger = logging.getLogger(__name__)


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
        ha_painel: Callable[[], bool] | None = None,
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
        self._ha_painel = ha_painel

    async def mensagens(self) -> AsyncIterator[TelemetriaMensagem]:
        # O seq é numerado no envio, como o gateway faz: não reinicia entre corridas
        # nem conta as mensagens da espera de forma diferente.
        seq = 1
        while True:
            # Mesmo caminho da serial: o texto do gateway é validado pelo schema.
            corrida = [
                TelemetriaMensagem.model_validate_json(linha)
                for linha in gerar_corrida(
                    self.pista, self.cenario, semente=self.semente
                )
            ]
            enviadas = 0
            for mensagem in self._espera_na_largada(corrida[0]):
                if enviadas:
                    await self._dormir(self.intervalo_s)
                yield mensagem.model_copy(update={"seq": seq})
                seq += 1
                enviadas += 1
            if enviadas:
                logger.info("painel conectado: largada do simulador")
            for mensagem in corrida:
                if enviadas:
                    await self._dormir(self.intervalo_s)
                yield mensagem.model_copy(update={"seq": seq})
                seq += 1
                enviadas += 1
            if not self.repetir:
                return
            await self._dormir(self.pausa_entre_corridas_s)

    def _espera_na_largada(self, aguardando: TelemetriaMensagem):
        """Repete a mensagem AGUARDANDO da largada enquanto não houver painel."""
        if self._ha_painel is None or self._ha_painel():
            return
        logger.info("simulador em espera na largada até um painel se conectar")
        while not self._ha_painel():
            yield aguardando
