"""Inicialização do backend de telemetria.

Execução local, a partir de src/backend:
    uvicorn app.main:app --reload

A fonte de telemetria (gateway real ou simulador) é escolhida pelas variáveis de
ambiente descritas em app/config.py.
"""

import asyncio
import contextlib
import logging
import os

from fastapi import FastAPI

from app.api import saude, telemetria_ws
from app.api import corridas as api_corridas
from app.api.telemetria_ws import GerenciadorConexoes
from app.banco import criar_banco, iniciar_esquema
from app.config import Configuracao
from app.ingestao.fonte import FonteTelemetria
from app.ingestao.servico import criar_fonte, ingerir
from app.servicos.corridas import ServicoCorridas


def criar_app(
    fonte: FonteTelemetria | None = None,
    gerenciador: GerenciadorConexoes | None = None,
    *,
    database_url: str | None = None,
    timeout_telemetria_s: float | None = None,
) -> FastAPI:
    timeout_telemetria_s = timeout_telemetria_s or float(os.getenv("TELEMETRIA_TIMEOUT_S", "5"))
    if timeout_telemetria_s <= 0:
        raise ValueError("TELEMETRIA_TIMEOUT_S deve ser positivo")
    gerenciador = gerenciador or GerenciadorConexoes()
    engine, sessoes = criar_banco(database_url)
    servico_corridas = ServicoCorridas(sessoes)

    @contextlib.asynccontextmanager
    async def ciclo_de_vida(_app: FastAPI):
        iniciar_esquema(engine, sessoes)
        tarefa = (
            asyncio.create_task(ingerir(fonte, gerenciador, servico_corridas, timeout_telemetria_s))
            if fonte else None
        )
        try:
            yield
        finally:
            if tarefa is not None:
                tarefa.cancel()
                with contextlib.suppress(asyncio.CancelledError, Exception):
                    await tarefa
            servico_corridas.interromper()

    app = FastAPI(
        title="Micromouse - Telemetria", version="0.1.0", lifespan=ciclo_de_vida
    )
    app.state.gerenciador = gerenciador
    app.state.sessoes = sessoes
    app.state.engine = engine
    app.include_router(saude.router)
    app.include_router(telemetria_ws.router)
    app.include_router(api_corridas.router)
    return app


def _app_da_configuracao() -> FastAPI:
    logging.basicConfig(
        level=logging.INFO, format="%(levelname)s:     %(name)s - %(message)s"
    )
    gerenciador = GerenciadorConexoes()
    fonte = criar_fonte(Configuracao.do_ambiente(), gerenciador)
    return criar_app(fonte, gerenciador)


app = _app_da_configuracao()
