import copy
import json

import pytest
from pydantic import ValidationError

from app.modelos.telemetria import TelemetriaMensagem

# Linha emitida pelo gateway (src/firmware/gateway/README.md), com seq e rssi.
LINHA_GATEWAY = (
    '{"seq":42,"rssi":-58,"timestamp_ms":142000,"labirinto":"8x4","status":"MAPEANDO",'
    '"desafio_cumprido":false,"bateria_pct":78,"bateria_volts":7.42,"velocidade_atual":35,'
    '"velocidade_media":28.5,"posicao":{"coluna":"B","linha":4},"orientacao":"LESTE",'
    '"paredes":{"norte":false,"sul":true,"leste":true,"oeste":false}}'
)


def alterar(mensagem, caminho, valor):
    """Copia a mensagem trocando o campo indicado por 'a.b'."""
    nova = copy.deepcopy(mensagem)
    *pais, campo = caminho.split(".")
    alvo = nova
    for pai in pais:
        alvo = alvo[pai]
    alvo[campo] = valor
    return nova


def remover(mensagem, caminho):
    nova = copy.deepcopy(mensagem)
    *pais, campo = caminho.split(".")
    alvo = nova
    for pai in pais:
        alvo = alvo[pai]
    del alvo[campo]
    return nova


def test_exemplo_da_secao_6_4_e_valido(mensagem):
    m = TelemetriaMensagem.model_validate(mensagem)

    assert m.timestamp_ms == 142000
    assert m.labirinto == "8x4"
    assert m.status == "MAPEANDO"
    assert m.desafio_cumprido is False
    assert m.bateria_pct == 78
    assert m.bateria_volts == pytest.approx(7.42)
    assert m.posicao.coluna == "B"
    assert m.posicao.linha == 4
    assert m.orientacao == "LESTE"
    assert m.paredes.sul is True
    assert m.seq is None
    assert m.rssi is None


def test_exemplo_validado_a_partir_do_texto_json(mensagem):
    m = TelemetriaMensagem.model_validate_json(json.dumps(mensagem))

    assert m.model_dump(exclude_none=True) == mensagem


def test_linha_do_gateway_com_seq_e_rssi_e_valida():
    m = TelemetriaMensagem.model_validate_json(LINHA_GATEWAY)

    assert m.seq == 42
    assert m.rssi == -58
    # O ArduinoJson grava 35.0 como 35: inteiro é aceito em campo decimal.
    assert m.velocidade_atual == 35.0


def test_seq_e_rssi_ausentes_nao_sao_serializados(mensagem):
    texto = TelemetriaMensagem.model_validate(mensagem).model_dump_json(
        exclude_none=True
    )

    assert "seq" not in json.loads(texto)
    assert "rssi" not in json.loads(texto)


def test_campo_desconhecido_e_ignorado_sem_invalidar(mensagem):
    mensagem["campo_novo_do_gateway"] = 123
    mensagem["posicao"]["andar"] = 2

    m = TelemetriaMensagem.model_validate(mensagem)

    assert "campo_novo_do_gateway" not in m.model_dump()
    assert "andar" not in m.model_dump()["posicao"]


@pytest.mark.parametrize(
    "status",
    [
        "AGUARDANDO",
        "MAPEANDO",
        "SPEED_RUN",
        "COLISAO",
        "CONCLUIDO",
        "TIMEOUT",
        "TRAVADO",
    ],
)
def test_todos_os_status_do_contrato_sao_aceitos(mensagem, status):
    assert (
        TelemetriaMensagem.model_validate(alterar(mensagem, "status", status)).status
        == status
    )


@pytest.mark.parametrize("orientacao", ["NORTE", "SUL", "LESTE", "OESTE"])
def test_todas_as_orientacoes_sao_aceitas(mensagem, orientacao):
    m = TelemetriaMensagem.model_validate(alterar(mensagem, "orientacao", orientacao))
    assert m.orientacao == orientacao


@pytest.mark.parametrize(
    ("caminho", "valor"),
    [
        ("timestamp_ms", 0),
        ("timestamp_ms", 600_000),
        ("bateria_pct", 0),
        ("bateria_pct", 100),
        ("bateria_volts", 0),
        ("velocidade_atual", -3276.8),
        ("velocidade_media", 3276.7),
        ("posicao.coluna", "A"),
        ("posicao.coluna", "D"),
        ("posicao.linha", 1),
        ("seq", 1),
        ("seq", 2**32 - 1),
        ("rssi", -128),
    ],
)
def test_valores_nos_limites_sao_aceitos(mensagem, caminho, valor):
    TelemetriaMensagem.model_validate(alterar(mensagem, caminho, valor))


@pytest.mark.parametrize(
    ("caminho", "valor"),
    [
        ("timestamp_ms", -1),
        ("timestamp_ms", 600_001),
        ("timestamp_ms", 142000.0),
        ("timestamp_ms", "142000"),
        ("timestamp_ms", True),
        ("labirinto", "16x16"),
        ("labirinto", "8X4"),
        ("labirinto", 2),
        ("status", "PARADO"),
        ("status", "mapeando"),
        ("status", 1),
        ("desafio_cumprido", 1),
        ("desafio_cumprido", "true"),
        ("desafio_cumprido", None),
        ("bateria_pct", -1),
        ("bateria_pct", 101),
        ("bateria_pct", 78.5),
        ("bateria_volts", -0.01),
        ("bateria_volts", 65.6),
        ("bateria_volts", "7.42"),
        ("velocidade_atual", 3276.8),
        ("velocidade_atual", -3276.9),
        ("velocidade_media", "28.5"),
        ("orientacao", "NORDESTE"),
        ("orientacao", "leste"),
        ("orientacao", 2),
        ("posicao", "B4"),
        ("posicao.coluna", "E"),
        ("posicao.coluna", "b"),
        ("posicao.coluna", 1),
        ("posicao.linha", 0),
        ("posicao.linha", 13),
        ("posicao.linha", "4"),
        ("paredes", 6),
        ("paredes.norte", 0),
        ("paredes.oeste", "false"),
        ("seq", 0),
        ("seq", 2**32),
        ("seq", "42"),
        ("rssi", -129),
        ("rssi", 128),
        ("rssi", -58.5),
    ],
)
def test_valores_fora_do_contrato_sao_rejeitados(mensagem, caminho, valor):
    with pytest.raises(ValidationError):
        TelemetriaMensagem.model_validate(alterar(mensagem, caminho, valor))


@pytest.mark.parametrize(
    "caminho",
    [
        "timestamp_ms",
        "labirinto",
        "status",
        "desafio_cumprido",
        "bateria_pct",
        "bateria_volts",
        "velocidade_atual",
        "velocidade_media",
        "posicao",
        "posicao.coluna",
        "posicao.linha",
        "orientacao",
        "paredes",
        "paredes.norte",
        "paredes.sul",
        "paredes.leste",
        "paredes.oeste",
    ],
)
def test_campo_obrigatorio_ausente_e_rejeitado(mensagem, caminho):
    with pytest.raises(ValidationError):
        TelemetriaMensagem.model_validate(remover(mensagem, caminho))


@pytest.mark.parametrize(
    ("labirinto", "linha", "valida"),
    [
        ("4x4", 4, True),
        ("4x4", 5, False),
        ("8x4", 8, True),
        ("8x4", 9, False),
        ("12x4", 12, True),
    ],
)
def test_linha_respeita_a_geometria(mensagem, labirinto, linha, valida):
    mensagem = alterar(mensagem, "labirinto", labirinto)
    mensagem = alterar(mensagem, "posicao.linha", linha)

    if valida:
        assert TelemetriaMensagem.model_validate(mensagem).posicao.linha == linha
    else:
        with pytest.raises(ValidationError, match="fora da geometria"):
            TelemetriaMensagem.model_validate(mensagem)


def test_json_malformado_e_rejeitado():
    with pytest.raises(ValidationError):
        TelemetriaMensagem.model_validate_json('{"timestamp_ms": 142000,')
