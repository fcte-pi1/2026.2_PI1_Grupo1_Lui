"""Utilitários de log colorido, visualização de telemetria e hints de diagnóstico.

Usado pelo controle remoto (`scripts/controle_remoto.py`) para deixar o terminal
visual, colorido e explicativo (sem emojis), apontando a área provável do problema
(Software, Eletrônica, Energia, Mecânica ou Rádio/RF) e o status de cada porta/sensor.
"""

from __future__ import annotations

import re
import time


class Cor:
    RESET = "\033[0m"
    NEGRITO = "\033[1m"
    ATENUADO = "\033[2m"

    VERMELHO = "\033[91m"
    VERDE = "\033[92m"
    AMARELO = "\033[93m"
    AZUL = "\033[94m"
    MAGENTA = "\033[95m"
    CIANO = "\033[96m"
    BRANCO = "\033[97m"
    CINZA = "\033[90m"

    BG_VERMELHO = "\033[41;97;1m"
    BG_VERDE = "\033[42;30;1m"
    BG_AMARELO = "\033[43;30;1m"
    BG_AZUL = "\033[44;97;1m"
    BG_MAGENTA = "\033[45;97;1m"
    BG_CIANO = "\033[46;30;1m"


BADGES_DOMINIO = {
    "SOFTWARE": f"{Cor.BG_AZUL} SOFTWARE {Cor.RESET}",
    "ELETRONICA": f"{Cor.BG_AMARELO} ELETRÔNICA {Cor.RESET}",
    "ENERGIA": f"{Cor.BG_MAGENTA} ENERGIA {Cor.RESET}",
    "MECANICA": f"{Cor.BG_VERDE} MECÂNICA {Cor.RESET}",
    "RADIO": f"{Cor.BG_CIANO} RÁDIO/RF {Cor.RESET}",
}

COR_STATUS = {
    "AGUARDANDO": f"{Cor.BG_CIANO} AGUARDANDO {Cor.RESET}",
    "MAPEANDO": f"{Cor.BG_AZUL} MAPEANDO {Cor.RESET}",
    "SPEED_RUN": f"{Cor.BG_MAGENTA} SPEED RUN {Cor.RESET}",
    "CONCLUIDO": f"{Cor.BG_VERDE} CONCLUÍDO {Cor.RESET}",
    "TRAVADO": f"{Cor.BG_VERMELHO} TRAVADO {Cor.RESET}",
    "COLISAO": f"{Cor.BG_VERMELHO} COLISÃO {Cor.RESET}",
    "TIMEOUT": f"{Cor.BG_AMARELO} TIMEOUT {Cor.RESET}",
}

SETA_ORIENTACAO = {
    "NORTE": "^ N",
    "SUL": "v S",
    "LESTE": "> L",
    "OESTE": "< O",
}


def hora_agora() -> str:
    return time.strftime("%H:%M:%S")


def barra_bateria(pct: int | float, volts: float) -> str:
    pct_i = max(0, min(100, int(pct or 0)))
    cheios = round(pct_i / 10)
    cor = Cor.VERMELHO if volts < 6.8 else Cor.AMARELO if volts < 7.05 else Cor.VERDE
    barra = f"{'#' * cheios}{'-' * (10 - cheios)}"
    return f"{cor}BAT [{barra}] {pct_i:3d}% ({volts:.2f}V){Cor.RESET}"


def barra_rssi(rssi: int | float | str) -> str:
    if not isinstance(rssi, (int, float)):
        return f"{Cor.CINZA}RF: --{Cor.RESET}"
    val = int(rssi)
    if val >= -60:
        cor, nivel = Cor.VERDE, "||||"
    elif val >= -75:
        cor, nivel = Cor.CIANO, "|||."
    elif val >= -82:
        cor, nivel = Cor.AMARELO, "||.."
    else:
        cor, nivel = Cor.VERMELHO, "|..."
    return f"{cor}RF [{nivel}] {val} dBm{Cor.RESET}"


def radar_paredes(paredes: dict | None) -> str:
    p = paredes or {}

    def fmt(letra: str, ativa: bool) -> str:
        return (
            f"{Cor.VERMELHO}{Cor.NEGRITO}{letra}:1{Cor.RESET}"
            if ativa
            else f"{Cor.CINZA}{letra}:0{Cor.RESET}"
        )

    return " ".join(
        [
            fmt("N", bool(p.get("norte"))),
            fmt("L", bool(p.get("leste"))),
            fmt("S", bool(p.get("sul"))),
            fmt("O", bool(p.get("oeste"))),
        ]
    )


def formatar_caixa_dica(
    dominios: list[str], titulo: str, causa: str, passos: list[str]
) -> str:
    badges = " ".join(BADGES_DOMINIO.get(d, f"[{d}]") for d in dominios)
    borda = f"{Cor.AMARELO}{'-' * 74}{Cor.RESET}"
    linhas = [
        f"{borda}",
        f"{Cor.AMARELO}[DICA]{Cor.RESET} {badges}  {Cor.NEGRITO}{Cor.BRANCO}{titulo}{Cor.RESET}",
        f"   {Cor.ATENUADO}Causa:{Cor.RESET} {causa}",
    ]
    for i, passo in enumerate(passos, 1):
        linhas.append(f"   {Cor.CIANO}{i}.{Cor.RESET} {passo}")
    linhas.append(f"{borda}")
    return "\n".join(linhas)


def formatar_telemetria_visual(data: dict) -> str:
    st = str(data.get("status", "---"))
    st_fmt = COR_STATUS.get(st, f"{Cor.BRANCO}{st}{Cor.RESET}")

    pos = data.get("posicao") or {}
    col = pos.get("coluna", "?")
    lin = pos.get("linha", "?")
    celula = f"{col}{int(lin):02d}" if isinstance(lin, int) else f"{col}{lin}"

    ori = str(data.get("orientacao", "?"))
    ori_fmt = SETA_ORIENTACAO.get(ori, ori)

    ts_ms = int(data.get("timestamp_ms") or 0)
    seg = ts_ms // 1000
    tempo_fmt = f"{seg // 60:02d}:{seg % 60:02d}"

    vel = int(data.get("velocidade_atual") or 0)
    vel_cor = Cor.VERDE if vel > 0 else Cor.CINZA
    bat_str = barra_bateria(data.get("bateria_pct", 0), float(data.get("bateria_volts") or 0.0))
    rssi_str = barra_rssi(data.get("rssi"))
    paredes_str = radar_paredes(data.get("paredes"))
    lab = data.get("labirinto", "--")

    return (
        f"{Cor.CINZA}[{hora_agora()}]{Cor.RESET} {st_fmt} "
        f"{Cor.CIANO}{lab}{Cor.RESET} | {Cor.NEGRITO}{celula}{Cor.RESET} ({ori_fmt}) | "
        f"T={tempo_fmt} | {vel_cor}{vel:4d} mm/s{Cor.RESET} | "
        f"{bat_str} | ToF[{paredes_str}] | {rssi_str}"
    )


def diagnosticar_telemetria(data: dict, estado_diag: dict) -> list[str]:
    """Analisa o pacote JSON e retorna caixas de dica para anomalias/falhas novas."""
    dicas: list[str] = []
    st = data.get("status")
    ultimo_st = estado_diag.get("ultimo_status")
    estado_diag["ultimo_status"] = st

    if st != ultimo_st:
        estado_diag["parado_seguidos"] = 0
        if st == "TRAVADO":
            dicas.append(
                formatar_caixa_dica(
                    ["MECANICA", "ELETRONICA"],
                    "Robô entrou em TRAVADO (corte de tração <= 1,2 s)",
                    "Os motores receberam PWM, mas os encoders não registraram pulsos suficientes.",
                    [
                        "Mecânica: verifique se roda/O-ring raspou na parede ou prendeu na trava do motor.",
                        "Eletrônica: teste encoders com [3] (GPIO 34/35 esq, 32/33 dir) e motores com [2] (STBY GPIO 13 / TB6612).",
                        "Software: se o motor patina ou não vence atrito, revise CAL_PWM_MINIMO em CalibracaoRobo.h.",
                    ],
                )
            )
        elif st == "COLISAO":
            dicas.append(
                formatar_caixa_dica(
                    ["ELETRONICA", "SOFTWARE"],
                    "Emergência frontal / COLISÃO detectada",
                    "O sensor ToF frontal leu distância abaixo do limite seguro (15 mm) em movimento.",
                    [
                        "Eletrônica: verifique o ToF frontal (GPIO 16, end. 0x31) enviando 'd' ou teste [1].",
                        "Software: ajuste CAL_TOF_OFFSET_FRONTAL_MM ou CAL_DIST_ENCOSTO_FRONTAL_MM em CalibracaoRobo.h.",
                    ],
                )
            )
        elif st == "TIMEOUT":
            dicas.append(
                formatar_caixa_dica(
                    ["SOFTWARE", "ENERGIA"],
                    "Tempo máximo de 10 minutos excedido (TIMEOUT)",
                    "O robô não concluiu o percurso dentro do limite da prova.",
                    [
                        "Software: cheque se houve falhas repetidas de leitura ToF ou loop em bifurcações.",
                        "Energia: confira se a bateria baixa derrubou a velocidade dos motores.",
                    ],
                )
            )

    volts = data.get("bateria_volts")
    if isinstance(volts, (int, float)):
        if volts < 6.8 and not estado_diag.get("bat_critica"):
            estado_diag["bat_critica"] = True
            dicas.append(
                formatar_caixa_dica(
                    ["ENERGIA", "ELETRONICA"],
                    f"Bateria crítica ({volts:.2f} V < 6,80 V)",
                    "Abaixo do corte mínimo de largada; o ESP32 pode reiniciar ao acionar os motores.",
                    [
                        "Energia: recarregue a bateria LiPo 2S antes de enviar 'k'.",
                        "Eletrônica: se no multímetro estiver > 7,4 V, cheque divisor 30k/7.5k no GPIO 36 ou CAL_BATERIA_FATOR_AJUSTE.",
                    ],
                )
            )
        elif volts >= 7.0:
            estado_diag["bat_critica"] = False

    rssi = data.get("rssi")
    if isinstance(rssi, (int, float)) and rssi < -82 and not estado_diag.get("rssi_fraco"):
        estado_diag["rssi_fraco"] = True
        dicas.append(
            formatar_caixa_dica(
                ["RADIO"],
                f"Sinal ESP-NOW fraco ({int(rssi)} dBm)",
                "Atenuação alta entre o ESP32 do robô e o gateway ESP8266.",
                [
                    "Rádio: aproxime o gateway da pista e evite cabos/caixas metálicas cobrindo a antena.",
                ],
            )
        )
    elif isinstance(rssi, (int, float)) and rssi >= -78:
        estado_diag["rssi_fraco"] = False

    vel = data.get("velocidade_atual")
    if st in ("MAPEANDO", "SPEED_RUN") and vel == 0:
        estado_diag["parado_seguidos"] = estado_diag.get("parado_seguidos", 0) + 1
        if estado_diag["parado_seguidos"] == 3:
            dicas.append(
                formatar_caixa_dica(
                    ["ELETRONICA", "MECANICA"],
                    "Velocidade 0 mm/s por 3 pacotes seguidos durante corrida",
                    "O robô está em execução, mas a odometria não registrou deslocamento linear.",
                    [
                        "Se não estiver fazendo giro no eixo: cheque cabos dos encoders (GPIO 34/35/32/33) e pino STBY (GPIO 13).",
                        "Use 'd' para ler o diagnóstico completo de ToF, encoders e bateria.",
                    ],
                )
            )
    else:
        estado_diag["parado_seguidos"] = 0

    return dicas


def diagnosticar_linha_texto(linha: str, origem: str = "ROBO") -> tuple[str, str | None]:
    """Formata uma linha de log não-JSON do robô ou gateway e gera dica se aplicável."""
    texto = linha.strip()
    prefixo = (
        f"{Cor.CIANO}[{hora_agora()}] [GATEWAY]{Cor.RESET}"
        if origem == "GATEWAY"
        else f"{Cor.VERDE}[{hora_agora()}] [ROBÔ]{Cor.RESET}"
    )

    if "[HW]" in texto:
        if any(tag in texto for tag in ("PRESO_LOW", "PRESO_HIGH", "FALHA", "SEM_RESPOSTA", "BAIXA/SEM_BAT", "ABAIXO_MINIMO", "PARCIAL")):
            return (
                f"{prefixo} {Cor.VERMELHO}{Cor.NEGRITO}{texto}{Cor.RESET}",
                formatar_caixa_dica(
                    ["ELETRONICA"],
                    "Anomalia detectada em porta/sensor no relatório [HW]",
                    texto,
                    [
                        "PRESO_LOW/HIGH em pino IO indica curto com GND/VCC ou periférico segurando o barramento.",
                        "SEM_RESPOSTA ou init=FALHA no ToF: cheque SDA (GPIO 21), SCL (GPIO 22), alimentação 3,3V e XSHUT (4/16/17).",
                    ],
                ),
            )
        if "SAUDAVEL" in texto or "-> OK" in texto or ": OK" in texto:
            return f"{prefixo} {Cor.VERDE}{texto}{Cor.RESET}", None
        return f"{prefixo} {Cor.CIANO}{texto}{Cor.RESET}", None

    if re.search(r"ERRO.*ToF|sensor ToF sem resposta", texto, re.IGNORECASE):
        return (
            f"{prefixo} {Cor.VERMELHO}{Cor.NEGRITO}{texto}{Cor.RESET}",
            formatar_caixa_dica(
                ["ELETRONICA"],
                "Sensor VL53L0X não respondeu no barramento I2C",
                "Falha de alimentação 3,3V, fio SDA/SCL solto ou pino XSHUT desconectado/trocado.",
                [
                    "Confira SDA=GPIO 21, SCL=GPIO 22 e os XSHUT: Esq=GPIO 4, Frontal=GPIO 16, Dir=GPIO 17.",
                    "No firmware geral/bancada, envie 'i' via USB para rodar o scan completo de portas e XSHUT.",
                ],
            ),
        )

    if "Largada recusada" in texto:
        return (
            f"{prefixo} {Cor.AMARELO}{Cor.NEGRITO}{texto}{Cor.RESET}",
            formatar_caixa_dica(
                ["ENERGIA", "ELETRONICA"],
                "Largada bloqueada por tensão mínima (< 6,80 V)",
                "Proteção automática do CicloProva impediu o início da prova.",
                [
                    "Carregue a bateria LiPo 2S ou confira o divisor no GPIO 36 com o comando '9' / 'd'.",
                ],
            ),
        )

    if "Falha de leitura das paredes" in texto:
        return (
            f"{prefixo} {Cor.AMARELO}{texto}{Cor.RESET}",
            formatar_caixa_dica(
                ["ELETRONICA", "SOFTWARE"],
                "Timeout na leitura dos sensores ToF durante a prova",
                "Um dos 3 VL53L0X não entregou amostra válida dentro da janela de 250 ms.",
                [
                    "Verifique mau contato no barramento I2C (GPIO 21/22) causado por vibração dos motores.",
                    "Envie 'd' ou teste no modo [1] para ver qual dos 3 slots (ok=E/F/D) zerou.",
                ],
            ),
        )

    if "ESP-NOW" in texto and ("falha" in texto.lower() or "erro" in texto.lower()):
        return (
            f"{prefixo} {Cor.VERMELHO}{texto}{Cor.RESET}",
            formatar_caixa_dica(
                ["SOFTWARE", "RADIO"],
                "Falha ao iniciar rádio ESP-NOW no ESP32",
                "Erro na inicialização do Wi-Fi STA / canal 1.",
                [
                    "Reinicie o ESP32 e confira TELEMETRIA_CANAL_WIFI em TelemetriaConfig.h.",
                ],
            ),
        )

    if origem == "GATEWAY" and ("descartado" in texto or "erro" in texto.lower()):
        return (
            f"{prefixo} {Cor.AMARELO}{texto}{Cor.RESET}",
            formatar_caixa_dica(
                ["SOFTWARE", "RADIO"],
                "Gateway descartou pacote recebido",
                "Tamanho diferente de 18 bytes, magic/CRC inválido ou versão antiga de firmware.",
                [
                    "Garanta que robô e gateway foram compilados com o mesmo TelemetriaPacket.h (18 bytes).",
                ],
            ),
        )

    if "[DIAG]" in texto:
        return f"{prefixo} {Cor.CIANO}{Cor.NEGRITO}{texto}{Cor.RESET}", None

    if "erro" in texto.lower() or "falh" in texto.lower():
        return f"{prefixo} {Cor.VERMELHO}{texto}{Cor.RESET}", None

    return f"{prefixo} {texto}", None
