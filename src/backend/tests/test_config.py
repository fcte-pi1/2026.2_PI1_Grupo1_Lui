import pytest

from app.config import Configuracao


def test_padrao_e_nenhuma_fonte():
    config = Configuracao.do_ambiente({})

    assert config == Configuracao()
    assert config.fonte == "nenhuma"


def test_simulador_configurado_pelo_ambiente():
    config = Configuracao.do_ambiente(
        {
            "TELEMETRIA_FONTE": "Simulador",
            "SIMULADOR_LABIRINTO": "12X4",
            "SIMULADOR_CENARIO": "perda_conexao",
            "SIMULADOR_VELOCIDADE": "2.5",
            "SIMULADOR_REPETIR": "1",
            "SIMULADOR_SEMENTE": "7",
        }
    )

    assert config.fonte == "simulador"
    assert config.simulador_labirinto == "12x4"
    assert config.simulador_cenario == "perda_conexao"
    assert config.simulador_velocidade == 2.5
    assert config.simulador_repetir is True
    assert config.simulador_semente == 7


def test_serial_configurada_pelo_ambiente():
    config = Configuracao.do_ambiente(
        {"TELEMETRIA_FONTE": "serial", "SERIAL_PORTA": "COM3", "SERIAL_BAUD": "9600"}
    )

    assert (config.fonte, config.serial_porta, config.serial_baud) == (
        "serial",
        "COM3",
        9600,
    )


def test_le_as_variaveis_do_processo(monkeypatch):
    monkeypatch.setenv("TELEMETRIA_FONTE", "simulador")
    monkeypatch.setenv("SIMULADOR_REPETIR", "true")

    config = Configuracao.do_ambiente()

    assert config.fonte == "simulador"
    assert config.simulador_repetir is True


@pytest.mark.parametrize(
    ("ambiente", "mensagem"),
    [
        ({"TELEMETRIA_FONTE": "radio"}, "TELEMETRIA_FONTE inválida"),
        ({"TELEMETRIA_FONTE": "serial"}, "exige SERIAL_PORTA"),
        ({"SERIAL_BAUD": "rapido"}, "SERIAL_BAUD inválido"),
        ({"SIMULADOR_LABIRINTO": "16x16"}, "SIMULADOR_LABIRINTO inválido"),
        ({"SIMULADOR_CENARIO": "explodiu"}, "SIMULADOR_CENARIO inválido"),
        ({"SIMULADOR_VELOCIDADE": "0"}, "deve ser positivo"),
        ({"SIMULADOR_VELOCIDADE": "x"}, "SIMULADOR_VELOCIDADE inválido"),
        ({"SIMULADOR_REPETIR": "talvez"}, "SIMULADOR_REPETIR deve ser 0 ou 1"),
        ({"SIMULADOR_SEMENTE": "1.5"}, "SIMULADOR_SEMENTE inválido"),
    ],
)
def test_valores_invalidos_sao_rejeitados(ambiente, mensagem):
    with pytest.raises(ValueError, match=mensagem):
        Configuracao.do_ambiente(ambiente)
