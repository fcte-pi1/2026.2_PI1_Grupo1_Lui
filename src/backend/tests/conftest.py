import copy

import pytest
from fastapi.testclient import TestClient

from app.main import criar_app

# Exemplo de mensagem da seção 6.4 do doc 4.4.
MENSAGEM_EXEMPLO = {
    "timestamp_ms": 142000,
    "labirinto": "8x4",
    "status": "MAPEANDO",
    "desafio_cumprido": False,
    "bateria_pct": 78,
    "bateria_volts": 7.42,
    "velocidade_atual": 35.0,
    "velocidade_media": 28.5,
    "posicao": {"coluna": "B", "linha": 4},
    "orientacao": "LESTE",
    "paredes": {"norte": False, "sul": True, "leste": True, "oeste": False},
}


@pytest.fixture
def mensagem():
    return copy.deepcopy(MENSAGEM_EXEMPLO)


@pytest.fixture
def app():
    return criar_app()


@pytest.fixture
def cliente(app):
    with TestClient(app) as cliente:
        yield cliente
