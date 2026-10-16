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
