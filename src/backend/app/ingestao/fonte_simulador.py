"""Fonte de telemetria que reproduz uma corrida simulada no ritmo do robô (1 Hz).

Como o robô real, que transmite em AGUARDANDO até o operador dar a largada, o
simulador pode ficar em espera na largada até haver um painel conectado (ha_painel).
Sem isso, a corrida começaria assim que o backend sobe e o painel, que leva alguns
segundos para conectar, perderia a largada e as primeiras células.

Também como o robô real, depois de parar (CONCLUIDO ou TRAVADO) o simulador continua
transmitindo a última mensagem no mesmo ritmo, inclusive na pausa entre corridas:
o painel só deve avisar "Conexão RF perdida" (HU-09) quando os pacotes param de
fato. No cenário perda_conexao, a lacuna de pacotes perdidos vira um silêncio de
silencio_perda_s segundos de relógio, que não depende do fator de velocidade.
"""

import asyncio
import logging
from collections.abc import AsyncIterator, Awaitable, Callable

from app.modelos.telemetria import TelemetriaMensagem
from app.simulador.gerador import INTERVALO_MS, gerar_corrida
from app.simulador.pistas import Pista

logger = logging.getLogger(__name__)

# Maior que os 3 s do aviso de RF perdida no painel (HU-09) e menor que os 5 s do
# TELEMETRIA_TIMEOUT_S, para a corrida não ser encerrada no histórico.
SILENCIO_PERDA_S = 4.0


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
        silencio_perda_s: float = SILENCIO_PERDA_S,
        manter_sinal: bool = True,
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
        self.silencio_perda_s = silencio_perda_s
        self.manter_sinal = manter_sinal
        self._dormir = dormir
        self._ha_painel = ha_painel

    async def mensagens(self) -> AsyncIterator[TelemetriaMensagem]:
        # O seq é numerado no envio, como o gateway faz: não reinicia entre corridas
        # nem conta as mensagens da espera de forma diferente.
        seq = 1
        anterior: TelemetriaMensagem | None = None
        # Espera antes do próximo envio; None no primeiro e logo depois da pausa.
        espera: float | None = None

        while True:
            # Mesmo caminho da serial: o texto do gateway é validado pelo schema.
            corrida = [
                TelemetriaMensagem.model_validate_json(linha)
                for linha in gerar_corrida(
                    self.pista, self.cenario, semente=self.semente
                )
            ]
            esperou_painel = False
            for mensagem in self._espera_na_largada(corrida[0]):
                if espera is not None:
                    await self._dormir(espera)
                yield mensagem.model_copy(update={"seq": seq})
                seq, espera, esperou_painel = seq + 1, self.intervalo_s, True
            if esperou_painel:
                logger.info("painel conectado: largada do simulador")

            for mensagem in corrida:
                if espera is not None:
                    await self._dormir(self._espera_antes(anterior, mensagem))
                yield mensagem.model_copy(update={"seq": seq})
                seq, espera, anterior = seq + 1, self.intervalo_s, mensagem

            ultima = corrida[-1]
            if not self.manter_sinal:
                if not self.repetir:
                    return
                await self._dormir(self.pausa_entre_corridas_s)
                espera = None
                continue
            # O robô parado continua transmitindo a última mensagem: para sempre,
            # ou durante a pausa até a próxima corrida.
            repeticoes = round(self.pausa_entre_corridas_s / self.intervalo_s)
            while not self.repetir or repeticoes > 0:
                await self._dormir(self.intervalo_s)
                yield ultima.model_copy(update={"seq": seq})
                seq += 1
                repeticoes -= 1

    def _espera_antes(
        self, anterior: TelemetriaMensagem | None, mensagem: TelemetriaMensagem
    ) -> float:
        """Intervalo normal, ou o silêncio da perda de pacotes do perda_conexao."""
        if anterior is None or "AGUARDANDO" in (anterior.status, mensagem.status):
            return self.intervalo_s
        if mensagem.timestamp_ms - anterior.timestamp_ms > INTERVALO_MS:
            return self.silencio_perda_s
        return self.intervalo_s

    def _espera_na_largada(self, aguardando: TelemetriaMensagem):
        """Repete a mensagem AGUARDANDO da largada enquanto não houver painel."""
        if self._ha_painel is None or self._ha_painel():
            return
        logger.info("simulador em espera na largada até um painel se conectar")
        while not self._ha_painel():
            yield aguardando
