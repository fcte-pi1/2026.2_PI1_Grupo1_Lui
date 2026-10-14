import pytest
from pistas_de_teste import PISTA_TODA_ALCANCAVEL, desenho

from app.simulador.direcao import Direcao
from app.simulador.pistas import (
    ARQUIVO_PISTAS,
    PistaInvalida,
    carregar_pistas,
    interpretar_desenho,
    interpretar_pistas,
    pista_de_referencia,
)


def test_pistas_de_referencia_dos_slides():
    pistas = carregar_pistas()

    assert set(pistas) == {"4x4", "8x4", "12x4"}
    assert [pistas[n].linhas for n in ("4x4", "8x4", "12x4")] == [4, 8, 12]
    # Regiões fechadas fiéis aos slides: 7 células no 8x4 e 8 no 12x4.
    assert len(pistas["4x4"].alcancaveis()) == 16
    assert len(pistas["8x4"].alcancaveis()) == 32 - 7
    assert len(pistas["12x4"].alcancaveis()) == 48 - 8


@pytest.mark.parametrize("labirinto", ["4x4", "8x4", "12x4"])
def test_regras_da_largada_e_da_chegada(labirinto):
    pista = pista_de_referencia(labirinto)

    assert pista.largada == (0, 0)
    assert pista.chegada == (pista.linhas - 1, 3)
    # A1 é um beco com a única abertura para o leste.
    assert [d for d in Direcao if not pista.tem_parede(0, 0, d)] == [Direcao.LESTE]
    assert pista.chegada in pista.alcancaveis()


@pytest.mark.parametrize("labirinto", ["4x4", "8x4", "12x4"])
def test_paredes_sao_simetricas_e_perimetro_fechado(labirinto):
    pista = pista_de_referencia(labirinto)
    for linha in range(pista.linhas):
        for coluna in range(4):
            for direcao in Direcao:
                vizinha = pista.vizinha(linha, coluna, direcao)
                if vizinha is None:
                    assert pista.tem_parede(linha, coluna, direcao)
                else:
                    assert pista.tem_parede(linha, coluna, direcao) == pista.tem_parede(
                        *vizinha, direcao.oposta
                    )


def test_paredes_lidas_do_desenho():
    pista = interpretar_desenho(desenho(PISTA_TODA_ALCANCAVEL), "4x4")

    norte, sul, leste, oeste = (d.mascara for d in Direcao)
    assert pista.paredes[0][0] == norte | sul | oeste
    assert pista.paredes[0][1] == norte | leste
    assert pista.paredes[1][1] == sul | leste
    assert pista.paredes[3][3] == sul | leste
    assert len(pista.alcancaveis()) == 16


def test_varias_pistas_com_comentarios():
    texto = "# comentário\n\n[4x4]\n" + PISTA_TODA_ALCANCAVEL
    assert list(interpretar_pistas(texto)) == ["4x4"]


def test_arquivo_padrao_existe():
    assert ARQUIVO_PISTAS.name == "pistas.txt"
    assert ARQUIVO_PISTAS.exists()


def trocar(texto, linha, coluna, caractere):
    linhas = desenho(texto)
    linhas[linha] = linhas[linha][:coluna] + caractere + linhas[linha][coluna + 1 :]
    return linhas


def sem_parede_sul_da_largada():
    linhas = desenho(PISTA_TODA_ALCANCAVEL)
    linhas[2] = "+   " + linhas[2][4:]
    return linhas


@pytest.mark.parametrize(
    ("alteracao", "mensagem"),
    [
        (lambda: desenho(PISTA_TODA_ALCANCAVEL)[:-1], "deve ter 9 linhas"),
        (lambda: trocar(PISTA_TODA_ALCANCAVEL, 3, 16, "||"), "no máximo 17 caracteres"),
        (lambda: trocar(PISTA_TODA_ALCANCAVEL, 0, 4, "-"), r"esperado '\+'"),
        (
            lambda: trocar(PISTA_TODA_ALCANCAVEL, 2, 6, "-"),
            "parede horizontal inválida",
        ),
        (lambda: trocar(PISTA_TODA_ALCANCAVEL, 3, 8, "x"), "parede vertical inválida"),
        (lambda: trocar(PISTA_TODA_ALCANCAVEL, 5, 16, " "), "perímetro aberto"),
        (
            lambda: trocar(PISTA_TODA_ALCANCAVEL, 1, 4, "|"),
            "única abertura para o leste",
        ),
        (lambda: sem_parede_sul_da_largada(), "única abertura para o leste"),
    ],
)
def test_desenho_invalido_e_rejeitado(alteracao, mensagem):
    with pytest.raises(PistaInvalida, match=mensagem):
        interpretar_desenho(alteracao(), "4x4")


def test_chegada_inalcancavel_e_rejeitada():
    fechada = """\
+---+---+---+---+
| L             |
+---+---+---+   +
|               |
+   +   +   +---+
|               |
+---+---+---+---+
|             C |
+---+---+---+---+
"""
    with pytest.raises(PistaInvalida, match="não é alcançável"):
        interpretar_desenho(desenho(fechada), "4x4")


def test_geometria_desconhecida_e_rejeitada():
    with pytest.raises(PistaInvalida, match="geometria desconhecida"):
        interpretar_desenho(desenho(PISTA_TODA_ALCANCAVEL), "16x16")


def test_secao_repetida_ou_ausente_e_rejeitada():
    with pytest.raises(PistaInvalida, match="mais de uma vez"):
        interpretar_pistas(
            "[4x4]\n" + PISTA_TODA_ALCANCAVEL + "[4x4]\n" + PISTA_TODA_ALCANCAVEL
        )
    with pytest.raises(PistaInvalida, match="fora de uma seção"):
        interpretar_pistas(PISTA_TODA_ALCANCAVEL)


def test_pista_de_referencia_inexistente(monkeypatch):
    monkeypatch.setattr(
        "app.simulador.pistas.carregar_pistas",
        lambda: interpretar_pistas("[4x4]\n" + PISTA_TODA_ALCANCAVEL),
    )
    with pytest.raises(PistaInvalida, match="não há pista de referência"):
        pista_de_referencia("8x4")
