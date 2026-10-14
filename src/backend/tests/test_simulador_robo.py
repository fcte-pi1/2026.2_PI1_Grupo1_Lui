import pytest
from pistas_de_teste import pista_toda_alcancavel

from app.simulador.direcao import Direcao
from app.simulador.pistas import pista_de_referencia
from app.simulador.robo_virtual import (
    CURVA,
    MEIA_VOLTA,
    RETA,
    ExploracaoSemSaida,
    RoboVirtual,
)


def pistas_para_testar():
    return [pista_de_referencia(n) for n in ("4x4", "8x4", "12x4")] + [
        pista_toda_alcancavel()
    ]


@pytest.fixture(
    params=pistas_para_testar(), ids=["4x4", "8x4", "12x4", "4x4-toda-alcancavel"]
)
def pista(request):
    return request.param


def test_flood_fill_chega_ao_destino(pista):
    robo = RoboVirtual(pista)
    passos = list(robo.explorar())

    assert robo.chegou
    assert (passos[-1].linha, passos[-1].coluna) == pista.chegada


def test_robo_so_anda_por_passagens_da_pista(pista):
    robo = RoboVirtual(pista)
    anterior = pista.largada
    for passo in robo.explorar():
        destino = pista.vizinha(*anterior, passo.orientacao)
        assert destino == (passo.linha, passo.coluna)
        assert not pista.tem_parede(*anterior, passo.orientacao)
        assert (passo.linha, passo.coluna) in pista.alcancaveis()
        anterior = destino


def test_mapa_do_robo_bate_com_a_pista_nas_celulas_visitadas(pista):
    robo = RoboVirtual(pista)
    for passo in robo.explorar():
        # Frente, esquerda e direita vêm dos sensores; o lado de entrada é passagem.
        assert passo.paredes == pista.paredes[passo.linha][passo.coluna]

    for linha in range(pista.linhas):
        for coluna in range(4):
            mapa = robo.mapa[linha][coluna]
            verdade = pista.paredes[linha][coluna]
            # O robô nunca registra uma parede que não existe.
            assert mapa & ~verdade == 0
            if (linha, coluna) in robo.visitadas:
                assert mapa == verdade


def test_regioes_fechadas_nunca_sao_visitadas():
    for labirinto in ("8x4", "12x4"):
        pista = pista_de_referencia(labirinto)
        robo = RoboVirtual(pista)
        list(robo.explorar())
        assert robo.visitadas <= pista.alcancaveis()


def test_largada_e_sentida_antes_do_primeiro_passo(pista):
    robo = RoboVirtual(pista)

    assert (robo.linha, robo.coluna) == (0, 0)
    assert robo.orientacao == Direcao.LESTE
    assert robo.paredes_da_celula_atual() == pista.paredes[0][0]


def test_manobras_registradas():
    robo = RoboVirtual(pista_de_referencia("4x4"))
    manobras = [p.manobra for p in robo.explorar()]

    # A pista 4x4 dos slides tem becos: o robô faz retas, curvas e meias-voltas.
    assert {RETA, CURVA, MEIA_VOLTA} <= set(manobras)


def test_explorar_sem_passos_quando_ja_esta_na_chegada():
    robo = RoboVirtual(pista_de_referencia("4x4"))
    list(robo.explorar())
    assert list(robo.explorar()) == []


def test_limite_de_passos_interrompe_a_exploracao():
    robo = RoboVirtual(pista_de_referencia("12x4"))
    with pytest.raises(ExploracaoSemSaida, match="limite de 3 passos"):
        list(robo.explorar(limite_passos=3))


def test_sem_caminho_conhecido_e_detectado():
    robo = RoboVirtual(pista_de_referencia("4x4"))
    # Simula um mapa corrompido em que a largada ficou fechada.
    robo.mapa[0][0] |= Direcao.LESTE.mascara
    with pytest.raises(ExploracaoSemSaida, match="sem caminho"):
        robo.proxima_direcao()
