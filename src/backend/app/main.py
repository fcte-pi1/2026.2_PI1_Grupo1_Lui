"""Inicialização do backend de telemetria.

Execução local, a partir de src/backend:
    uvicorn app.main:app --reload

A fonte de telemetria (gateway real ou simulador) é escolhida pelas variáveis de
ambiente descritas em app/config.py.
"""

import asyncio
import contextlib
import logging

from fastapi import FastAPI

from app.api import saude, telemetria_ws
from app.api.telemetria_ws import GerenciadorConexoes
from app.config import Configuracao
from app.ingestao.fonte import FonteTelemetria
from app.ingestao.servico import criar_fonte, ingerir


def criar_app(
    fonte: FonteTelemetria | None = None,
    gerenciador: GerenciadorConexoes | None = None,
) -> FastAPI:
    gerenciador = gerenciador or GerenciadorConexoes()

    @contextlib.asynccontextmanager
    async def ciclo_de_vida(_app: FastAPI):
        tarefa = asyncio.create_task(ingerir(fonte, gerenciador)) if fonte else None
        try:
            yield
        finally:
            if tarefa is not None:
                tarefa.cancel()
                with contextlib.suppress(asyncio.CancelledError, Exception):
                    await tarefa

    app = FastAPI(
        title="Micromouse - Telemetria", version="0.1.0", lifespan=ciclo_de_vida
    )
    app.state.gerenciador = gerenciador
    app.include_router(saude.router)
    app.include_router(telemetria_ws.router)
    return app


def _app_da_configuracao() -> FastAPI:
    logging.basicConfig(
        level=logging.INFO, format="%(levelname)s:     %(name)s - %(message)s"
    )
    gerenciador = GerenciadorConexoes()
    fonte = criar_fonte(Configuracao.do_ambiente(), gerenciador)
    return criar_app(fonte, gerenciador)


app = _app_da_configuracao()
