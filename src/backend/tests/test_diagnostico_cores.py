import importlib.util
from pathlib import Path

MOD_PATH = Path(__file__).resolve().parents[3] / "scripts" / "diagnostico_cores.py"
spec = importlib.util.spec_from_file_location("diagnostico_cores", MOD_PATH)
diag = importlib.util.module_from_spec(spec)
spec.loader.exec_module(diag)


def test_formatar_telemetria_visual_inclui_status_celula_e_barras():
    saida = diag.formatar_telemetria_visual({
        "status": "MAPEANDO",
        "labirinto": "4x4",
        "posicao": {"coluna": "B", "linha": 3},
        "orientacao": "LESTE",
        "timestamp_ms": 14000,
        "velocidade_atual": 120,
        "bateria_pct": 80,
        "bateria_volts": 7.52,
        "rssi": -58,
        "paredes": {"norte": True, "leste": False, "sul": True, "oeste": False},
    })
    assert "MAPEANDO" in saida
    assert "B03" in saida
    assert "> L" in saida
    assert "00:14" in saida
    assert "7.52V" in saida
    assert "-58 dBm" in saida


def test_diagnosticar_telemetria_gera_hint_travado_e_bateria_critica():
    estado = {"ultimo_status": "MAPEANDO", "bat_critica": False, "rssi_fraco": False, "parado_seguidos": 0}
    dicas = diag.diagnosticar_telemetria({
        "status": "TRAVADO",
        "bateria_volts": 6.60,
        "rssi": -88,
        "velocidade_atual": 0,
    }, estado)
    junto = "\n".join(dicas)
    assert "MECÂNICA" in junto
    assert "ELETRÔNICA" in junto
    assert "ENERGIA" in junto
    assert "RÁDIO/RF" in junto
    assert "GPIO 34/35" in junto


def test_diagnosticar_linha_texto_identifica_erro_tof_como_eletronica():
    msg, dica = diag.diagnosticar_linha_texto("ERRO: sensor ToF sem resposta (esq=0 frontal=1 dir=1).")
    assert "ROBÔ" in msg
    assert dica is not None
    assert "ELETRÔNICA" in dica
    assert "GPIO 21" in dica


def test_diagnosticar_linha_hw_detecta_pino_preso():
    msg, dica = diag.diagnosticar_linha_texto("[HW] GPIO 21 (I2C SDA): PU=0 PD=0 -> PRESO_LOW")
    assert "PRESO_LOW" in msg
    assert dica is not None
    assert "ELETRÔNICA" in dica
