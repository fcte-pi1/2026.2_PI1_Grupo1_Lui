"""Liga a fonte de telemetria escolhida na configuração ao WebSocket do painel."""

import logging

from app.api.telemetria_ws import GerenciadorConexoes
from app.config import Configuracao
from app.ingestao.fonte import FonteTelemetria
from app.ingestao.fonte_serial import FonteSerial
from app.ingestao.fonte_simulador import FonteSimulador
from app.simulador.pistas import pista_de_referencia

logger = logging.getLogger(__name__)


def criar_fonte(
    config: Configuracao, gerenciador: GerenciadorConexoes | None = None
) -> FonteTelemetria | None:
    """Cria a fonte configurada. Com o gerenciador, o simulador espera na largada
    até haver um painel conectado ao WebSocket."""
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
            ha_painel=(lambda: gerenciador.total > 0) if gerenciador else None,
        )
    if config.fonte == "serial":
        logger.info("fonte de telemetria: gateway na porta %s", config.serial_porta)
        return FonteSerial(config.serial_porta, config.serial_baud)
    logger.warning(
        "nenhuma fonte de telemetria configurada (TELEMETRIA_FONTE=nenhuma): "
        "o WebSocket não vai receber pacotes"
    )
    return None


async def ingerir(
    fonte: FonteTelemetria, gerenciador: GerenciadorConexoes,
    corridas=None, timeout_telemetria_s: float = 5.0,
) -> None:
    """Difunde a telemetria e persiste as leituras sem interromper o painel."""
    import asyncio
    import contextlib

    iterador = fonte.mensagens().__aiter__()
    proxima = None
    fila_persistencia = asyncio.Queue() if corridas is not None else None
    tarefa_persistencia = None
    persistencia_em_andamento = False

    async def persistir_em_segundo_plano():
        nonlocal persistencia_em_andamento
        while True:
            mensagem = await fila_persistencia.get()
            persistencia_em_andamento = True
            try:
                await asyncio.to_thread(corridas.processar, mensagem)
            except Exception:
                logger.exception("falha ao persistir telemetria; ingestão mantida")
            finally:
                persistencia_em_andamento = False
                fila_persistencia.task_done()

    if corridas is not None:
        tarefa_persistencia = asyncio.create_task(persistir_em_segundo_plano())
    try:
        while True:
            try:
                if proxima is None:
                    proxima = asyncio.create_task(iterador.__anext__())
                aguardando_persistencia = (
                    corridas is not None
                    and (
                        corridas.tem_corrida_ativa
                        or persistencia_em_andamento
                        or not fila_persistencia.empty()
                    )
                )
                if aguardando_persistencia:
                    pronto, _ = await asyncio.wait({proxima}, timeout=timeout_telemetria_s)
                    if not pronto:
                        logger.warning("telemetria sem atualização por %.1f s", timeout_telemetria_s)
                        try:
                            await asyncio.to_thread(corridas.interromper)
                        except Exception:
                            logger.exception("falha ao interromper a corrida por timeout")
                        continue
                mensagem = await proxima
                proxima = None
            except StopAsyncIteration:
                break
            await gerenciador.difundir(mensagem)
            if fila_persistencia is not None:
                await fila_persistencia.put(mensagem)
    except Exception:
        logger.exception("ingestão da telemetria interrompida por erro")
        raise
    finally:
        if fila_persistencia is not None:
            await fila_persistencia.join()
            tarefa_persistencia.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await tarefa_persistencia
        if proxima is not None and not proxima.done():
            proxima.cancel()
            with contextlib.suppress(asyncio.CancelledError, StopAsyncIteration):
                await proxima
        if corridas is not None:
            try:
                await asyncio.to_thread(corridas.interromper)
            except Exception:
                logger.exception("falha ao interromper a corrida no encerramento")
    logger.info("fonte de telemetria encerrada")
