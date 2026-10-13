"""Inicialização do backend de telemetria.

Execução local, a partir de src/backend:
    uvicorn app.main:app --reload
"""

from fastapi import FastAPI

from app.api import saude, telemetria_ws
from app.api.telemetria_ws import GerenciadorConexoes


def criar_app() -> FastAPI:
    app = FastAPI(title="Micromouse - Telemetria", version="0.1.0")
    app.state.gerenciador = GerenciadorConexoes()
    app.include_router(saude.router)
    app.include_router(telemetria_ws.router)
    return app


app = criar_app()
