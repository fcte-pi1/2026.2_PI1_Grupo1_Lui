import asyncio
import contextlib

import pytest

from app.ingestao.fonte_simulador import FonteSimulador
from app.modelos.telemetria import TelemetriaMensagem
from app.simulador.gerador import gerar_corrida
from app.simulador.pistas import pista_de_referencia


class Relogio:
    def __init__(self):
        self.esperas = []

    async def dormir(self, segundos):
        self.esperas.append(segundos)


async def coletar(fonte, limite=None):
    recebidas = []
    async with contextlib.aclosing(fonte.mensagens()) as mensagens:
        async for mensagem in mensagens:
            recebidas.append(mensagem)
            if limite is not None and len(recebidas) == limite:
                break
    return recebidas


def test_entrega_a_corrida_do_gerador_validada():
    pista = pista_de_referencia("8x4")
    relogio = Relogio()
    fonte = FonteSimulador(pista, "travado", dormir=relogio.dormir)

    recebidas = asyncio.run(coletar(fonte))

    esperadas = [
        TelemetriaMensagem.model_validate_json(linha)
        for linha in gerar_corrida(pista, "travado")
    ]
    assert recebidas == esperadas


def test_ritmo_de_uma_mensagem_por_segundo_dividido_pelo_fator():
    pista = pista_de_referencia("4x4")
    for fator, intervalo in ((1.0, 1.0), (4.0, 0.25)):
        relogio = Relogio()
        fonte = FonteSimulador(pista, fator_velocidade=fator, dormir=relogio.dormir)
        recebidas = asyncio.run(coletar(fonte))
        # Nenhuma espera antes da primeira mensagem nem depois da última.
        assert relogio.esperas == [intervalo] * (len(recebidas) - 1)


def test_repetir_recomeca_a_corrida_sem_reiniciar_o_seq():
    pista = pista_de_referencia("4x4")
    tamanho = len(gerar_corrida(pista, "perda_conexao"))
    relogio = Relogio()
    fonte = FonteSimulador(
        pista,
        "perda_conexao",
        repetir=True,
        fator_velocidade=2,
        pausa_entre_corridas_s=6,
        dormir=relogio.dormir,
    )

    recebidas = asyncio.run(coletar(fonte, limite=tamanho + 2))

    assert [m.seq for m in recebidas] == list(range(1, tamanho + 3))
    assert recebidas[tamanho].status == "AGUARDANDO"
    assert recebidas[tamanho].timestamp_ms == 0
    assert 3.0 in relogio.esperas  # pausa entre corridas também segue o fator


def test_fator_de_velocidade_deve_ser_positivo():
    with pytest.raises(ValueError, match="positivo"):
        FonteSimulador(pista_de_referencia("4x4"), fator_velocidade=0)


class PainelFalso:
    """ha_painel que só fica verdadeiro depois de algumas consultas."""

    def __init__(self, consultas_sem_painel):
        self.restantes = consultas_sem_painel

    def __call__(self):
        if self.restantes > 0:
            self.restantes -= 1
            return False
        return True


def test_espera_na_largada_ate_haver_painel():
    pista = pista_de_referencia("4x4")
    relogio = Relogio()
    fonte = FonteSimulador(pista, ha_painel=PainelFalso(4), dormir=relogio.dormir)

    recebidas = asyncio.run(coletar(fonte))

    corrida = [
        TelemetriaMensagem.model_validate_json(linha) for linha in gerar_corrida(pista)
    ]
    # Antes do painel: "Em espera" em A1 repetido (a primeira consulta só decide
    # se é preciso esperar). Depois, a corrida inteira, desde a largada.
    espera = recebidas[:3]
    assert all(m.status == "AGUARDANDO" and m.timestamp_ms == 0 for m in espera)
    assert all((m.posicao.coluna, m.posicao.linha) == ("A", 1) for m in espera)
    resto = [m.model_dump(exclude={"seq"}) for m in recebidas[3:]]
    assert resto == [m.model_dump(exclude={"seq"}) for m in corrida]
    assert [m.seq for m in recebidas] == list(range(1, len(recebidas) + 1))
    assert relogio.esperas == [1.0] * (len(recebidas) - 1)


def test_com_painel_ja_conectado_comeca_na_hora():
    pista = pista_de_referencia("8x4")
    fonte = FonteSimulador(pista, ha_painel=lambda: True, dormir=Relogio().dormir)

    recebidas = asyncio.run(coletar(fonte))

    assert recebidas == [
        TelemetriaMensagem.model_validate_json(linha) for linha in gerar_corrida(pista)
    ]


def test_espera_de_novo_a_cada_corrida_sem_painel():
    pista = pista_de_referencia("4x4")
    tamanho = len(gerar_corrida(pista))
    painel = {"conectado": True}
    fonte = FonteSimulador(
        pista,
        repetir=True,
        ha_painel=lambda: painel["conectado"],
        dormir=Relogio().dormir,
    )

    async def cenario():
        recebidas = []
        async with contextlib.aclosing(fonte.mensagens()) as mensagens:
            async for mensagem in mensagens:
                recebidas.append(mensagem)
                if len(recebidas) == tamanho:
                    painel["conectado"] = False  # o painel fecha no fim da corrida
                if len(recebidas) == tamanho + 5:
                    break
        return recebidas

    recebidas = asyncio.run(cenario())

    # Sem painel, a segunda corrida não sai da largada.
    assert all(m.status == "AGUARDANDO" for m in recebidas[tamanho:])
    assert [m.seq for m in recebidas] == list(range(1, tamanho + 6))
