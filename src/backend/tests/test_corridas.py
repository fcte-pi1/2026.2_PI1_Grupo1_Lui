import asyncio
import time

from fastapi.testclient import TestClient

from app.main import criar_app
from app.banco import Corrida, Leitura
from app.modelos.telemetria import TelemetriaMensagem
from app.servicos.corridas import ServicoCorridas
from app.simulador.gerador import gerar_corrida
from app.simulador.pistas import pista_de_referencia


class Fonte:
    def __init__(self, itens):
        self.itens = itens

    async def mensagens(self):
        for item in self.itens:
            if isinstance(item, BaseException):
                raise item
            yield item


def pacotes(cenario="completa"):
    return [
        TelemetriaMensagem.model_validate_json(linha)
        for linha in gerar_corrida(pista_de_referencia("4x4"), cenario)
    ]


def aguardar_corridas(cliente, predicado):
    limite = time.monotonic() + 5
    corridas = []
    while time.monotonic() < limite:
        corridas = cliente.get("/api/corridas").json()
        if predicado(corridas):
            return corridas
        time.sleep(0.05)
    return corridas


def test_corrida_normal_e_travada_persistem_todas_as_leituras_e_consultam_api(tmp_path):
    mensagens = pacotes("completa") + pacotes("travado")
    database_url = f"sqlite:///{tmp_path / 'corridas.db'}"
    app = criar_app(Fonte(mensagens), database_url=database_url)
    with TestClient(app) as cliente:
        corridas = aguardar_corridas(
            cliente, lambda itens: [c["status"] for c in itens] == ["TRAVADO", "CONCLUIDO"]
        )
        assert [c["status"] for c in corridas] == ["TRAVADO", "CONCLUIDO"]

        # O status final pode ficar visível na API antes da última transação
        # de leitura terminar de ser observável pela conexão do teste.
        time.sleep(0.1)
        limite = time.monotonic() + 5
        detalhes = None

        while time.monotonic() < limite:
            detalhes = cliente.get(f"/api/corridas/{corridas[-1]['id']}").json()
            if len(detalhes["leituras"]) == len(pacotes("completa")) - 2:
                break
            time.sleep(0.05)

        assert detalhes is not None
        assert len(detalhes["leituras"]) == len(pacotes("completa")) - 2
        assert detalhes["leituras"][-1]["status"] == "CONCLUIDO"
        assert set(detalhes["leituras"][-1]["paredes"]) == {"norte", "sul", "leste", "oeste"}
        assert corridas[-1]["tentativa"] is None

def test_corrida_ativa_tem_fim_e_status_final_nulos_e_modo_valido():
    app = criar_app(database_url="sqlite://")
    with TestClient(app) as cliente:
        servico = ServicoCorridas(app.state.sessoes)
        servico.processar(pacotes()[2])
        corrida = cliente.get("/api/corridas").json()[0]
        assert corrida["status"] == "MAPEANDO"
        assert corrida["iniciada_em"].endswith("Z")
        assert corrida["encerrada_em"] is None
        assert corrida["modo"] == "EXPLORACAO"
        assert corrida["tentativa"] is None
        leitura = cliente.get(f"/api/corridas/{corrida['id']}").json()["leituras"][0]
        assert leitura["orientacao"] in {"NORTE", "SUL", "LESTE", "OESTE"}


def test_mysql_persiste_orientacao_com_nome_completo(mysql_app):
    app, cliente = mysql_app
    ids_anteriores = {corrida["id"] for corrida in cliente.get("/api/corridas").json()}
    corrida_id = None

    try:
        ServicoCorridas(app.state.sessoes).processar(pacotes()[2])
        corrida = next(
            corrida
            for corrida in cliente.get("/api/corridas").json()
            if corrida["id"] not in ids_anteriores
        )
        corrida_id = corrida["id"]
        leitura = cliente.get(f"/api/corridas/{corrida_id}").json()["leituras"][0]

        assert leitura["orientacao"] == "LESTE"
    finally:
        if corrida_id is not None:
            with app.state.sessoes.begin() as sessao:
                sessao.query(Leitura).filter(Leitura.corrida_id == corrida_id).delete()
                sessao.query(Corrida).filter(Corrida.id == corrida_id).delete()


def test_fim_inesperado_marca_corrida_interrompida_e_preserva_leituras():
    mensagens = pacotes("perda_conexao")
    app = criar_app(Fonte(mensagens), database_url="sqlite://")
    with TestClient(app) as cliente:
        corrida = aguardar_corridas(
            cliente, lambda itens: bool(itens and itens[0]["status"] == "INTERROMPIDA")
        )[0]
        assert corrida["status"] == "INTERROMPIDA"
        assert cliente.get(f"/api/corridas/{corrida['id']}").json()["leituras"]


def test_perda_de_telemetria_finaliza_corrida_ativa():
    async def sem_fim():
        yield pacotes()[2]
        await asyncio.Event().wait()

    class FontePausada:
        mensagens = staticmethod(sem_fim)

    app = criar_app(FontePausada(), database_url="sqlite://", timeout_telemetria_s=0.02)
    with TestClient(app) as cliente:
        limite = time.monotonic() + 1
        corrida = []
        while time.monotonic() < limite:
            corrida = cliente.get("/api/corridas").json()
            if corrida and corrida[0]["status"] == "INTERROMPIDA":
                break
            time.sleep(0.01)
        assert corrida[0]["status"] == "INTERROMPIDA"
        assert cliente.get(f"/api/corridas/{corrida[0]['id']}").json()["leituras"]


def test_reinicio_marca_corrida_aberta_interrompida_e_consulta_registros(tmp_path):
    url = f"sqlite:///{tmp_path / 'corridas.db'}"
    primeira = criar_app(database_url=url)
    with TestClient(primeira):
        corrida = TelemetriaMensagem.model_validate_json(gerar_corrida(pista_de_referencia("4x4"))[2])
        from app.servicos.corridas import ServicoCorridas
        ServicoCorridas(primeira.state.sessoes).processar(corrida)

    segunda = criar_app(database_url=url)
    with TestClient(segunda) as cliente:
        dados = cliente.get("/api/corridas").json()
        assert dados[0]["status"] == "INTERROMPIDA"
        assert cliente.get(f"/api/corridas/{dados[0]['id']}").json()["leituras"]


def test_corrida_inexistente_retorna_404():
    with TestClient(criar_app(database_url="sqlite://")) as cliente:
        assert cliente.get("/api/corridas/999").status_code == 404
