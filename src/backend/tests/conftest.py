import copy
import os

import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine, text

from app.main import criar_app

# URL de um MySQL com migrations/armazena.sql já aplicado, por exemplo
# mysql+pymysql://root:senha@127.0.0.1:3306/armazena. Sem ela, os testes de
# persistência rodam só no SQLite e os exclusivos do MySQL são pulados.
MYSQL_TEST_URL = os.getenv("MYSQL_TEST_URL", "").strip()
sem_mysql = pytest.mark.skipif(not MYSQL_TEST_URL, reason="MYSQL_TEST_URL não definida")


def limpar_mysql():
    """Esvazia as tabelas do banco de teste, mantendo a estrutura do armazena.sql."""
    engine = create_engine(MYSQL_TEST_URL)
    try:
        with engine.begin() as conexao:
            conexao.execute(text("DELETE FROM Leitura"))
            conexao.execute(text("DELETE FROM Corrida"))
    finally:
        engine.dispose()


@pytest.fixture(
    params=["sqlite", pytest.param("mysql", marks=[pytest.mark.mysql, sem_mysql])]
)
def url_banco(request, tmp_path):
    """Banco dos testes de persistência: SQLite em arquivo e, se configurado, MySQL."""
    if request.param == "sqlite":
        return f"sqlite:///{tmp_path / 'corridas.db'}"
    limpar_mysql()
    return MYSQL_TEST_URL

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
    return criar_app(database_url="sqlite://")


@pytest.fixture
def cliente(app):
    with TestClient(app) as cliente:
        yield cliente
