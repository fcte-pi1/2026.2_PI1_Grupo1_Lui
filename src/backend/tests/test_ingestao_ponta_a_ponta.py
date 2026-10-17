import asyncio
import logging
import time

import pytest
from fastapi import FastAPI
from fastapi.testclient import TestClient

from app import main
from app.api.telemetria_ws import GerenciadorConexoes
from app.config import Configuracao
from app.ingestao.fonte_serial import FonteSerial
from app.ingestao.fonte_simulador import FonteSimulador
from app.ingestao.servico import criar_fonte, ingerir
from app.main import criar_app
from app.modelos.telemetria import TelemetriaMensagem
from app.simulador.gerador import gerar_corrida
from app.simulador.pistas import pista_de_referencia


def test_mensagem_do_simulador_chega_ao_cliente_websocket():
    pista = pista_de_referencia("12x4")
    # Corrida acelerada e repetida, para o cliente receber algo mesmo conectando depois
    # do início da ingestão.
    fonte = FonteSimulador(pista, fator_velocidade=50, repetir=True)
    # Ao repetir, o seq continua crescendo; o resto da mensagem é igual ao do gerador.
    geradas = [
        TelemetriaMensagem.model_validate_json(linha).model_dump(exclude={"seq"})
        for linha in gerar_corrida(pista)
    ]

    with (
        TestClient(criar_app(fonte)) as cliente,
        cliente.websocket_connect("/ws/telemetria") as ws,
    ):
        recebidas = [ws.receive_json() for _ in range(3)]

    for dados in recebidas:
        mensagem = TelemetriaMensagem.model_validate(dados)
        assert mensagem.labirinto == "12x4"
        assert mensagem.model_dump(exclude={"seq"}) in geradas
    seqs = [dados["seq"] for dados in recebidas]
    assert seqs == sorted(seqs)


class GerenciadorRegistrando(GerenciadorConexoes):
    """Registra o status de cada pacote difundido antes do primeiro cliente."""

    def __init__(self):
        super().__init__()
        self.antes_do_painel = []
        self._houve_cliente = False

    async def difundir(self, mensagem):
        self._houve_cliente = self._houve_cliente or self.total > 0
        if not self._houve_cliente:
            self.antes_do_painel.append(mensagem.status)
        await super().difundir(mensagem)


def test_cliente_que_conecta_depois_recebe_a_corrida_desde_a_largada():
    # Mesmo arranjo de _app_da_configuracao: o simulador consulta o gerenciador e
    # fica em espera na largada até o painel conectar.
    pista = pista_de_referencia("4x4")
    gerenciador = GerenciadorRegistrando()
    # Com repetição, sempre há mensagens: sem a espera o teste falha em vez de travar.
    config = Configuracao(
        fonte="simulador", simulador_velocidade=50, simulador_repetir=True
    )
    fonte = criar_fonte(config, gerenciador)
    corrida = [
        TelemetriaMensagem.model_validate_json(linha).model_dump(exclude={"seq"})
        for linha in gerar_corrida(pista)
    ]

    with TestClient(criar_app(fonte, gerenciador)) as cliente:
        time.sleep(0.5)  # o simulador já está rodando, sem ninguém ouvindo
        with cliente.websocket_connect("/ws/telemetria") as ws:
            recebidas = []
            while not recebidas or recebidas[-1]["status"] != "CONCLUIDO":
                recebidas.append(ws.receive_json())

    # Sem painel, o simulador ficou em espera na largada: nada da corrida se perdeu.
    assert gerenciador.antes_do_painel
    assert set(gerenciador.antes_do_painel) == {"AGUARDANDO"}

    # O cliente recebe a corrida inteira, a partir da largada.
    sem_seq = [
        TelemetriaMensagem.model_validate(d).model_dump(exclude={"seq"})
        for d in recebidas
    ]
    inicio = next(i for i, d in enumerate(recebidas) if d["status"] == "MAPEANDO")
    assert all(d["status"] == "AGUARDANDO" for d in recebidas[:inicio])
    assert sem_seq[inicio:] == corrida[2 : 2 + len(sem_seq) - inicio]
    assert recebidas[inicio]["posicao"] == {"coluna": "B", "linha": 1}


def test_criar_fonte_com_gerenciador_espera_painel():
    gerenciador = GerenciadorConexoes()
    fonte = criar_fonte(Configuracao(fonte="simulador"), gerenciador)

    assert fonte._ha_painel() is False
    gerenciador._conexoes.add(object())
    assert fonte._ha_painel() is True
    assert criar_fonte(Configuracao(fonte="simulador"))._ha_painel is None


def test_app_sem_fonte_nao_inicia_ingestao():
    with TestClient(criar_app()) as cliente:
        assert cliente.get("/health").json() == {"status": "ok"}


def test_criar_fonte_pela_configuracao(caplog):
    with caplog.at_level(logging.WARNING):
        assert criar_fonte(Configuracao()) is None
    assert "nenhuma fonte de telemetria" in caplog.text

    simulador = criar_fonte(
        Configuracao(
            fonte="simulador", simulador_labirinto="8x4", simulador_cenario="travado"
        )
    )
    assert isinstance(simulador, FonteSimulador)
    assert simulador.pista.labirinto == "8x4"
    assert simulador.cenario == "travado"

    serial = criar_fonte(
        Configuracao(fonte="serial", serial_porta="COM3", serial_baud=9600)
    )
    assert isinstance(serial, FonteSerial)
    assert (serial.porta, serial.baud) == ("COM3", 9600)


class FonteDeLista:
    def __init__(self, itens):
        self.itens = itens

    async def mensagens(self):
        for item in self.itens:
            if isinstance(item, Exception):
                raise item
            yield item


class GerenciadorFalso:
    def __init__(self):
        self.difundidas = []

    async def difundir(self, mensagem):
        self.difundidas.append(mensagem)


def test_ingerir_difunde_cada_mensagem(mensagem):
    telemetria = TelemetriaMensagem.model_validate(mensagem)
    gerenciador = GerenciadorFalso()

    asyncio.run(ingerir(FonteDeLista([telemetria, telemetria]), gerenciador))

    assert gerenciador.difundidas == [telemetria, telemetria]


def test_erro_na_fonte_e_registrado(mensagem, caplog):
    telemetria = TelemetriaMensagem.model_validate(mensagem)
    gerenciador = GerenciadorFalso()

    with pytest.raises(RuntimeError), caplog.at_level(logging.ERROR):
        asyncio.run(
            ingerir(FonteDeLista([telemetria, RuntimeError("falha")]), gerenciador)
        )

    assert gerenciador.difundidas == [telemetria]
    assert "ingestão da telemetria interrompida" in caplog.text


def test_app_da_configuracao_usa_as_variaveis_de_ambiente(monkeypatch):
    monkeypatch.setenv("TELEMETRIA_FONTE", "simulador")
    monkeypatch.setenv("SIMULADOR_VELOCIDADE", "100")

    assert isinstance(main._app_da_configuracao(), FastAPI)


def test_configuracao_invalida_impede_a_inicializacao(monkeypatch):
    monkeypatch.setenv("TELEMETRIA_FONTE", "serial")
    monkeypatch.delenv("SERIAL_PORTA", raising=False)

    with pytest.raises(ValueError, match="exige SERIAL_PORTA"):
        main._app_da_configuracao()
