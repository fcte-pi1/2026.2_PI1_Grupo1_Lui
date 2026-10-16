import itertools
import json

import pytest
from pistas_de_teste import pista_toda_alcancavel

from app.modelos.telemetria import TIMESTAMP_MAX_MS, TelemetriaMensagem
from app.simulador.__main__ import main
from app.simulador.direcao import Direcao
from app.simulador.gerador import CENARIOS, Pacote, gerar_corrida, linha_gateway
from app.simulador.pistas import pista_de_referencia

# Linhas produzidas pelo telemetriaParaJson do firmware (src/firmware/lib/telemetria),
# compilado nativamente com os mesmos campos. Servem de referência byte a byte.
REFERENCIAS_DO_GATEWAY = [
    (
        Pacote(
            142000,
            "8x4",
            "MAPEANDO",
            False,
            78,
            7420,
            350,
            285,
            1,
            3,
            Direcao.LESTE,
            0x06,
        ),
        42,
        -58,
        (
            '{"seq":42,"rssi":-58,"timestamp_ms":142000,"labirinto":"8x4","status":"MAPEANDO",'
            '"desafio_cumprido":false,"bateria_pct":78,"bateria_volts":7.42,"velocidade_atual":35,'
            '"velocidade_media":28.5,"posicao":{"coluna":"B","linha":4},"orientacao":"LESTE",'
            '"paredes":{"norte":false,"sul":true,"leste":true,"oeste":false}}'
        ),
    ),
    (
        Pacote(
            0, "4x4", "AGUARDANDO", False, 85, 8199, 0, 0, 0, 0, Direcao.LESTE, 0x0B
        ),
        1,
        -60,
        (
            '{"seq":1,"rssi":-60,"timestamp_ms":0,"labirinto":"4x4","status":"AGUARDANDO",'
            '"desafio_cumprido":false,"bateria_pct":85,"bateria_volts":8.199,"velocidade_atual":0,'
            '"velocidade_media":0,"posicao":{"coluna":"A","linha":1},"orientacao":"LESTE",'
            '"paredes":{"norte":true,"sul":true,"leste":false,"oeste":true}}'
        ),
    ),
    (
        Pacote(
            6000, "12x4", "MAPEANDO", False, 84, 8187, 183, 176, 2, 5, Direcao.SUL, 0x09
        ),
        7,
        -49,
        (
            '{"seq":7,"rssi":-49,"timestamp_ms":6000,"labirinto":"12x4","status":"MAPEANDO",'
            '"desafio_cumprido":false,"bateria_pct":84,"bateria_volts":8.187,"velocidade_atual":18.3,'
            '"velocidade_media":17.6,"posicao":{"coluna":"C","linha":6},"orientacao":"SUL",'
            '"paredes":{"norte":true,"sul":false,"leste":false,"oeste":true}}'
        ),
    ),
    (
        Pacote(
            9000, "4x4", "CONCLUIDO", True, 83, 8001, 0, 167, 3, 3, Direcao.NORTE, 0x05
        ),
        9,
        -71,
        (
            '{"seq":9,"rssi":-71,"timestamp_ms":9000,"labirinto":"4x4","status":"CONCLUIDO",'
            '"desafio_cumprido":true,"bateria_pct":83,"bateria_volts":8.001,"velocidade_atual":0,'
            '"velocidade_media":16.7,"posicao":{"coluna":"D","linha":4},"orientacao":"NORTE",'
            '"paredes":{"norte":true,"sul":false,"leste":true,"oeste":false}}'
        ),
    ),
    (
        Pacote(
            3000, "4x4", "MAPEANDO", False, 86, 8203, -15, -7, 1, 0, Direcao.OESTE, 0x00
        ),
        3,
        -55,
        (
            '{"seq":3,"rssi":-55,"timestamp_ms":3000,"labirinto":"4x4","status":"MAPEANDO",'
            '"desafio_cumprido":false,"bateria_pct":86,"bateria_volts":8.203,"velocidade_atual":-1.5,'
            '"velocidade_media":-0.7,"posicao":{"coluna":"B","linha":1},"orientacao":"OESTE",'
            '"paredes":{"norte":false,"sul":false,"leste":false,"oeste":false}}'
        ),
    ),
]

GEOMETRIAS = ["4x4", "8x4", "12x4"]


def mensagens(linhas):
    return [TelemetriaMensagem.model_validate_json(linha) for linha in linhas]


def posicao(mensagem):
    return ("ABCD".index(mensagem.posicao.coluna), mensagem.posicao.linha - 1)


@pytest.mark.parametrize(("pacote", "seq", "rssi", "esperado"), REFERENCIAS_DO_GATEWAY)
def test_linha_identica_a_do_gateway(pacote, seq, rssi, esperado):
    assert linha_gateway(pacote, seq, rssi) == esperado


@pytest.mark.parametrize("cenario", CENARIOS)
@pytest.mark.parametrize("labirinto", GEOMETRIAS)
def test_todas_as_mensagens_sao_validas_e_coerentes(labirinto, cenario):
    pista = pista_de_referencia(labirinto)
    linhas = gerar_corrida(pista, cenario, seq_inicial=10)
    msgs = mensagens(linhas)

    assert [m.seq for m in msgs] == list(range(10, 10 + len(msgs)))
    assert all(m.labirinto == labirinto for m in msgs)
    assert all(-128 <= m.rssi <= 0 for m in msgs)
    tempos = [m.timestamp_ms for m in msgs]
    assert tempos == sorted(tempos)
    assert max(tempos) <= TIMESTAMP_MAX_MS
    assert all(0 < m.bateria_volts <= 8.4 and 0 <= m.bateria_pct <= 100 for m in msgs)

    for anterior, atual in itertools.pairwise(msgs):
        (ca, la), (cb, lb) = posicao(anterior), posicao(atual)
        assert abs(ca - cb) + abs(la - lb) <= 1  # no máximo uma célula por mensagem
        if (ca, la) != (cb, lb):
            assert atual.timestamp_ms - anterior.timestamp_ms == 1000
            direcao = Direcao[atual.orientacao]
            assert not pista.tem_parede(la, ca, direcao)  # só por passagens reais

    for m in msgs:
        coluna, linha = posicao(m)
        verdade = pista.paredes[linha][coluna]
        enviadas = sum(d.mascara for d in Direcao if getattr(m.paredes, d.name.lower()))
        assert enviadas == verdade


def test_corrida_comeca_aguardando_na_largada():
    msgs = mensagens(gerar_corrida(pista_de_referencia("8x4")))

    for m in msgs[:2]:
        assert m.status == "AGUARDANDO"
        assert m.timestamp_ms == 0
        assert m.velocidade_atual == 0
        assert posicao(m) == (0, 0)
        assert m.orientacao == "LESTE"
    assert msgs[2].status == "MAPEANDO"
    assert msgs[2].timestamp_ms == 1000


@pytest.mark.parametrize("labirinto", GEOMETRIAS)
def test_corrida_completa_termina_concluida_na_chegada(labirinto):
    pista = pista_de_referencia(labirinto)
    msgs = mensagens(gerar_corrida(pista, "completa"))

    finais = msgs[-3:]
    assert all(m.status == "CONCLUIDO" and m.desafio_cumprido for m in finais)
    assert all((posicao(m)[1], posicao(m)[0]) == pista.chegada for m in finais)
    assert all(m.velocidade_atual == 0 for m in finais)
    # Cronômetro congelado depois da chegada.
    assert len({m.timestamp_ms for m in finais}) == 1
    assert not any(m.desafio_cumprido for m in msgs[:-3])
    assert {m.status for m in msgs[:-3]} == {"AGUARDANDO", "MAPEANDO"}


@pytest.mark.parametrize("labirinto", GEOMETRIAS)
def test_corrida_interrompida_termina_travada_no_meio(labirinto):
    pista = pista_de_referencia(labirinto)
    completa = mensagens(gerar_corrida(pista, "completa"))
    msgs = mensagens(gerar_corrida(pista, "travado"))

    finais = msgs[-3:]
    assert all(m.status == "TRAVADO" and not m.desafio_cumprido for m in finais)
    assert all(m.velocidade_atual == 0 for m in finais)
    assert len({posicao(m) for m in finais}) == 1
    assert (posicao(finais[0])[1], posicao(finais[0])[0]) != pista.chegada
    assert "CONCLUIDO" not in {m.status for m in msgs}
    assert len(msgs) < len(completa)


@pytest.mark.parametrize("labirinto", GEOMETRIAS)
def test_perda_de_conexao_para_de_enviar_no_meio(labirinto):
    pista = pista_de_referencia(labirinto)
    completa = mensagens(gerar_corrida(pista, "completa"))
    msgs = mensagens(gerar_corrida(pista, "perda_conexao"))

    assert msgs[-1].status == "MAPEANDO"
    assert (posicao(msgs[-1])[1], posicao(msgs[-1])[0]) != pista.chegada
    assert {m.status for m in msgs} == {"AGUARDANDO", "MAPEANDO"}
    # Até o corte, a corrida é igual à completa.
    assert [m.model_dump() for m in msgs] == [
        m.model_dump() for m in completa[: len(msgs)]
    ]
    assert len(msgs) < len(completa) - 1


def test_pista_toda_alcancavel_tambem_gera_corrida_completa():
    pista = pista_toda_alcancavel()
    msgs = mensagens(gerar_corrida(pista))
    assert msgs[-1].status == "CONCLUIDO"
    assert msgs[-1].posicao.coluna == "D" and msgs[-1].posicao.linha == 4


def test_interrupcao_nunca_na_largada_nem_na_chegada():
    pista = pista_de_referencia("4x4")
    for fracao in (0.0, 1.0):
        msgs = mensagens(gerar_corrida(pista, "travado", fracao_interrupcao=fracao))
        parada = posicao(msgs[-1])
        assert parada != (0, 0)
        assert (parada[1], parada[0]) != pista.chegada


def test_mesma_semente_gera_a_mesma_corrida():
    pista = pista_de_referencia("4x4")
    assert gerar_corrida(pista, semente=7) == gerar_corrida(pista, semente=7)
    assert gerar_corrida(pista, semente=7) != gerar_corrida(pista, semente=8)


def test_cenario_desconhecido_e_rejeitado():
    with pytest.raises(ValueError, match="cenário desconhecido"):
        gerar_corrida(pista_de_referencia("4x4"), "atropelado")


def test_linha_de_comando_escreve_as_linhas_do_gateway(capsys):
    assert main(["--labirinto", "8x4", "--cenario", "travado"]) == 0

    linhas = capsys.readouterr().out.splitlines()
    assert linhas == gerar_corrida(pista_de_referencia("8x4"), "travado")
    assert json.loads(linhas[-1])["status"] == "TRAVADO"


def test_linha_de_comando_em_tempo_real(monkeypatch, capsys):
    esperas = []
    monkeypatch.setattr("app.simulador.__main__.time.sleep", esperas.append)

    main(["--cenario", "perda_conexao", "--tempo-real", "--velocidade", "4"])

    linhas = capsys.readouterr().out.splitlines()
    assert esperas == [0.25] * (len(linhas) - 1)


def test_linha_de_comando_rejeita_velocidade_invalida():
    with pytest.raises(SystemExit):
        main(["--velocidade", "0"])
