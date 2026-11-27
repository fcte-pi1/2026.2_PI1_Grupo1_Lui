import pytest
from fastapi.testclient import TestClient

from app.main import criar_app
from app.simulador.pistas import pista_de_referencia
from app.ingestao.fonte_simulador import FonteSimulador


class FonteComandoFalsa:
    def __init__(self, erro: Exception | None = None):
        self.comandos = []
        self.erro = erro

    async def mensagens(self):
        if False:
            yield None

    async def enviar_comando(self, comando: str) -> None:
        if self.erro is not None:
            raise self.erro
        self.comandos.append(comando)


def test_largada_envia_k_quando_fonte_suporta_comando(tmp_path):
    fonte = FonteComandoFalsa()
    app = criar_app(fonte=fonte, database_url=f"sqlite:///{tmp_path / 'cmd.db'}")
    with TestClient(app) as cliente:
        resp = cliente.post("/api/comandos/largada")

    assert resp.status_code == 200
    assert resp.json() == {"enviado": True, "comando": "k"}
    assert fonte.comandos == ["k"]


def test_largada_retorna_503_sem_fonte_ou_com_gateway_desconectado(tmp_path):
    app_sem = criar_app(fonte=None, database_url=f"sqlite:///{tmp_path / 'sem.db'}")
    with TestClient(app_sem) as cliente:
        assert cliente.post("/api/comandos/largada").status_code == 503

    app_err = criar_app(
        fonte=FonteComandoFalsa(erro=RuntimeError("Gateway serial não conectado.")),
        database_url=f"sqlite:///{tmp_path / 'err.db'}",
    )
    with TestClient(app_err) as cliente:
        resp = cliente.post("/api/comandos/largada")
    assert resp.status_code == 503
    assert "não conectado" in resp.json()["detail"]


def test_largada_em_simulador_retorna_200_sem_falha(tmp_path):
    sim = FonteSimulador(pista_de_referencia("4x4"))
    app = criar_app(fonte=sim, database_url=f"sqlite:///{tmp_path / 'sim.db'}")
    with TestClient(app) as cliente:
        resp = cliente.post("/api/comandos/largada")

    assert resp.status_code == 200
    assert resp.json()["enviado"] is False
