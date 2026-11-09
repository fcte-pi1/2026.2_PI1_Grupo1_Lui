import asyncio
import contextlib

import pytest

from app.ingestao.fonte_simulador import SILENCIO_PERDA_S, FonteSimulador
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
    fonte = FonteSimulador(pista, "travado", manter_sinal=False, dormir=relogio.dormir)

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
        fonte = FonteSimulador(
            pista, fator_velocidade=fator, manter_sinal=False, dormir=relogio.dormir
        )
        recebidas = asyncio.run(coletar(fonte))
        # Nenhuma espera antes da primeira mensagem nem depois da última.
        assert relogio.esperas == [intervalo] * (len(recebidas) - 1)


def test_repetir_recomeca_a_corrida_sem_reiniciar_o_seq():
    pista = pista_de_referencia("4x4")
    corrida = gerar_corrida(pista)
    tamanho = len(corrida)
    relogio = Relogio()
    fonte = FonteSimulador(
        pista,
        repetir=True,
        fator_velocidade=2,
        pausa_entre_corridas_s=6,
        dormir=relogio.dormir,
    )

    # Pausa de 3 s (6 s / fator 2) a 0,5 s por mensagem: 6 repetições do fim.
    recebidas = asyncio.run(coletar(fonte, limite=tamanho + 6 + 2))

    assert [m.seq for m in recebidas] == list(range(1, tamanho + 9))
    final = TelemetriaMensagem.model_validate_json(corrida[-1])
    repeticoes = recebidas[tamanho:tamanho + 6]
    assert all(m.model_dump(exclude={"seq"}) == final.model_dump(exclude={"seq"}) for m in repeticoes)
    assert recebidas[tamanho + 6].status == "AGUARDANDO"
    assert recebidas[tamanho + 6].timestamp_ms == 0
    # Sem silêncio entre as corridas: o ritmo segue o fator o tempo todo.
    assert relogio.esperas == [0.5] * (len(recebidas) - 1)


def test_sem_manter_sinal_a_pausa_entre_corridas_e_silencio():
    pista = pista_de_referencia("4x4")
    tamanho = len(gerar_corrida(pista))
    relogio = Relogio()
    fonte = FonteSimulador(
        pista,
        repetir=True,
        manter_sinal=False,
        fator_velocidade=2,
        pausa_entre_corridas_s=6,
        dormir=relogio.dormir,
    )

    recebidas = asyncio.run(coletar(fonte, limite=tamanho + 2))

    assert recebidas[tamanho].status == "AGUARDANDO"
    assert relogio.esperas.count(3.0) == 1  # pausa entre corridas também segue o fator
    assert relogio.esperas.count(0.5) == len(recebidas) - 2


def test_depois_de_parar_continua_transmitindo_a_ultima_mensagem():
    """Como o robô real: parado em CONCLUIDO ou TRAVADO, segue enviando a 1 Hz (HU-09)."""
    pista = pista_de_referencia("8x4")
    for cenario in ("completa", "travado"):
        corrida = gerar_corrida(pista, cenario)
        relogio = Relogio()
        fonte = FonteSimulador(pista, cenario, dormir=relogio.dormir)

        recebidas = asyncio.run(coletar(fonte, limite=len(corrida) + 20))

        final = TelemetriaMensagem.model_validate_json(corrida[-1])
        depois = recebidas[len(corrida):]
        assert len(depois) == 20
        assert all(m.model_dump(exclude={"seq"}) == final.model_dump(exclude={"seq"}) for m in depois)
        assert [m.seq for m in recebidas] == list(range(1, len(recebidas) + 1))
        assert relogio.esperas == [1.0] * (len(recebidas) - 1)


def test_perda_de_conexao_vira_silencio_em_tempo_real():
    """O silêncio dos pacotes perdidos não encolhe com o fator de velocidade."""
    pista = pista_de_referencia("4x4")
    relogio = Relogio()
    fonte = FonteSimulador(
        pista, "perda_conexao", fator_velocidade=5, manter_sinal=False, dormir=relogio.dormir
    )

    recebidas = asyncio.run(coletar(fonte))

    assert recebidas[-1].status == "CONCLUIDO"
    assert relogio.esperas.count(SILENCIO_PERDA_S) == 1
    assert relogio.esperas.count(0.2) == len(recebidas) - 2
    assert 3.0 < SILENCIO_PERDA_S < 5.0  # aviso do painel < silêncio < timeout do backend


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
    fonte = FonteSimulador(
        pista, ha_painel=PainelFalso(4), manter_sinal=False, dormir=relogio.dormir
    )

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
    fonte = FonteSimulador(
        pista, ha_painel=lambda: True, manter_sinal=False, dormir=Relogio().dormir
    )

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
        manter_sinal=False,
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
