"""Liga a fonte de telemetria escolhida na configuração ao WebSocket do painel."""

import logging

from app.api.telemetria_ws import GerenciadorConexoes
from app.config import Configuracao
from app.ingestao.fonte import FonteTelemetria
from app.ingestao.fonte_serial import FonteSerial
from app.ingestao.fonte_simulador import FonteSimulador
from app.simulador.pistas import pista_de_referencia

logger = logging.getLogger(__name__)


def criar_fonte(config: Configuracao) -> FonteTelemetria | None:
    if config.fonte == "simulador":
        logger.info(
            "fonte de telemetria: simulador (%s, cenário %s, velocidade %sx)",
            config.simulador_labirinto,
            config.simulador_cenario,
            config.simulador_velocidade,
        )
        return FonteSimulador(
            pista_de_referencia(config.simulador_labirinto),
            config.simulador_cenario,
            fator_velocidade=config.simulador_velocidade,
            repetir=config.simulador_repetir,
            semente=config.simulador_semente,
        )
    if config.fonte == "serial":
        logger.info("fonte de telemetria: gateway na porta %s", config.serial_porta)
        return FonteSerial(config.serial_porta, config.serial_baud)
    logger.warning(
        "nenhuma fonte de telemetria configurada (TELEMETRIA_FONTE=nenhuma): "
        "o WebSocket não vai receber pacotes"
    )
    return None


async def ingerir(fonte: FonteTelemetria, gerenciador: GerenciadorConexoes) -> None:
    """Difunde cada mensagem válida da fonte a todos os clientes do WebSocket."""
    try:
        async for mensagem in fonte.mensagens():
            await gerenciador.difundir(mensagem)
    except Exception:
        logger.exception("ingestão da telemetria interrompida por erro")
        raise
    logger.info("fonte de telemetria encerrada")
