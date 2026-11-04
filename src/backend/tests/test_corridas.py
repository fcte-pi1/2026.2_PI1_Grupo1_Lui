import asyncio
import time

from fastapi.testclient import TestClient

from app.main import criar_app
from app.modelos.telemetria import TelemetriaMensagem
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
    from app.servicos.corridas import ServicoCorridas

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

    app = criar_app(FontePausada(), database_url="sqlite://", timeout_telemetria_s=0.05)
    with TestClient(app) as cliente:
        corridas = aguardar_corridas(
            cliente, lambda itens: bool(itens and itens[0]["status"] == "INTERROMPIDA")
        )
        assert corridas, "Nenhuma corrida encontrada antes do tempo limite"
        assert corridas[0]["status"] == "INTERROMPIDA"
        assert cliente.get(f"/api/corridas/{corridas[0]['id']}").json()["leituras"]


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


def test_filtrar_corridas_por_geometria_e_data(tmp_path):
    from datetime import datetime
    from app.banco import Corrida, iniciar_esquema

    database_url = f"sqlite:///{tmp_path / 'filtro.db'}"
    app = criar_app(database_url=database_url)
    iniciar_esquema(app.state.engine, app.state.sessoes)

    # Popula com corridas em diferentes datas e geometrias
    with app.state.sessoes.begin() as sessao:
        c1 = Corrida(
            labirinto="4x4",
            inicio=datetime(2026, 10, 24, 10, 0, 0),
            fim=datetime(2026, 10, 24, 10, 2, 0),
            tempo_total_ms=120000,
            status_final="CONCLUIDO",
            modo="EXPLORACAO",
            desafio_cumprido=True,
        )
        c2 = Corrida(
            labirinto="8x4",
            inicio=datetime(2026, 10, 25, 14, 0, 0),
            fim=datetime(2026, 10, 25, 14, 5, 0),
            tempo_total_ms=300000,
            status_final="CONCLUIDO",
            modo="EXPLORACAO",
            desafio_cumprido=True,
        )
        c3 = Corrida(
            labirinto="8x4",
            inicio=datetime(2026, 10, 25, 16, 0, 0),
            fim=datetime(2026, 10, 25, 16, 1, 30),
            tempo_total_ms=90000,
            status_final="CONCLUIDO",
            modo="SPEED_RUN",
            desafio_cumprido=True,
        )
        c4 = Corrida(
            labirinto="12x4",
            inicio=datetime(2026, 10, 26, 9, 0, 0),
            fim=datetime(2026, 10, 26, 9, 8, 0),
            tempo_total_ms=480000,
            status_final="TRAVADO",
            modo="EXPLORACAO",
            desafio_cumprido=False,
        )
        sessao.add_all([c1, c2, c3, c4])

    with TestClient(app) as cliente:
        # Sem filtro retorna todas (4)
        todas = cliente.get("/api/corridas").json()
        assert len(todas) == 4

        # Filtro por geometria 8x4
        res_8x4 = cliente.get("/api/corridas?geometria=8x4").json()
        assert len(res_8x4) == 2
        assert all(c["geometria"] == "8x4" for c in res_8x4)

        # Filtro por geometria 4x4
        res_4x4 = cliente.get("/api/corridas?geometria=4x4").json()
        assert len(res_4x4) == 1
        assert res_4x4[0]["geometria"] == "4x4"

        # Filtro com "todas" não restringe
        res_todas = cliente.get("/api/corridas?geometria=todas").json()
        assert len(res_todas) == 4

        # Filtro por data específica
        res_data = cliente.get("/api/corridas?data=2026-10-25").json()
        assert len(res_data) == 2
        assert all(c["iniciada_em"].startswith("2026-10-25") for c in res_data)

        # Filtro por formato brasileiro
        res_data_br = cliente.get("/api/corridas?data=25/10/2026").json()
        assert len(res_data_br) == 2

        # Filtro por intervalo de datas
        res_intervalo = cliente.get("/api/corridas?data_inicio=2026-10-24&data_fim=2026-10-25").json()
        assert len(res_intervalo) == 3

        # Filtro combinado: geometria 8x4 e data 2026-10-25
        res_combinado = cliente.get("/api/corridas?geometria=8x4&data=2026-10-25").json()
        assert len(res_combinado) == 2

        # Filtro combinado sem correspondência: 4x4 na data 2026-10-25
        res_vazio = cliente.get("/api/corridas?geometria=4x4&data=2026-10-25").json()
        assert len(res_vazio) == 0


def test_performance_consulta_500_corridas(tmp_path):
    """RNF-SFT04: Resposta inferior a 2,0 s para base com até 500 registros."""
    from datetime import datetime, timedelta
    from app.banco import Corrida, iniciar_esquema

    database_url = f"sqlite:///{tmp_path / 'perf.db'}"
    app = criar_app(database_url=database_url)
    iniciar_esquema(app.state.engine, app.state.sessoes)

    geometrias = ["4x4", "8x4", "12x4"]
    corridas = []
    base_data = datetime(2026, 10, 1, 8, 0, 0)
    for i in range(500):
        corridas.append(
            Corrida(
                labirinto=geometrias[i % 3],
                inicio=base_data + timedelta(hours=i),
                fim=base_data + timedelta(hours=i, minutes=3),
                tempo_total_ms=180000,
                status_final="CONCLUIDO" if i % 2 == 0 else "TRAVADO",
                modo="EXPLORACAO",
                desafio_cumprido=i % 2 == 0,
            )
        )

    with app.state.sessoes.begin() as sessao:
        sessao.add_all(corridas)

    with TestClient(app) as cliente:
        inicio = time.monotonic()
        resposta = cliente.get("/api/corridas?geometria=8x4&data_inicio=2026-10-05&data_fim=2026-10-15")
        duracao = time.monotonic() - inicio

        assert resposta.status_code == 200
        dados = resposta.json()
        assert len(dados) > 0
        assert duracao < 2.0  # RNF-SFT04 estrito (< 2s)

