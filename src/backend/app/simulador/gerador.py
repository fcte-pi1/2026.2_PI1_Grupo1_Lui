"""Gera as linhas JSON de uma corrida simulada, idênticas às do gateway.

Cada pacote é montado nas unidades do pacote binário de 18 bytes (índices, mm/s, mV
e bitmask) e convertido com as mesmas regras de TelemetriaJson.h: mesma ordem de
campos, seq e rssi primeiro, e números decimais sem parte fracionária escritos como
inteiros (o ArduinoJson grava 35.0 como 35). Uma mensagem por célula percorrida,
com timestamp_ms avançando 1000 ms por mensagem (1 Hz).

Cenários:
  completa       termina em CONCLUIDO na chegada
  travado        vai para TRAVADO no meio do percurso
  perda_conexao  para de enviar no meio do percurso, ainda em MAPEANDO
Em CONCLUIDO e TRAVADO a última mensagem se repete com o cronômetro congelado,
como o robô real continua transmitindo depois de parar.
"""

import json
import random
from dataclasses import dataclass

from app.modelos.telemetria import TIMESTAMP_MAX_MS
from app.simulador.direcao import Direcao
from app.simulador.pistas import Pista
from app.simulador.robo_virtual import CURVA, MEIA_VOLTA, RETA, RoboVirtual

CENARIOS = ("completa", "travado", "perda_conexao")
INTERVALO_MS = 1000

_LETRAS_COLUNA = "ABCD"
_STATUS_ID = {
    "AGUARDANDO": 0,
    "MAPEANDO": 1,
    "SPEED_RUN": 2,
    "COLISAO": 3,
    "CONCLUIDO": 4,
    "TIMEOUT": 5,
    "TRAVADO": 6,
}

# Modelo simples de bateria LiPo 2S (doc 4.3): 8,4 V cheia; 0% em 7,0 V.
_BATERIA_INICIAL_MV = 8200
_BATERIA_CHEIA_MV = 8400
_BATERIA_VAZIA_MV = 7000
_DESCARGA_MV_POR_S = 2

# Uma célula de 18 cm por segundo, mais devagar quando há curva ou meia-volta
# (exploração de 15 a 30 cm/s, doc 4.3).
_VELOCIDADE_MM_S = {RETA: 180, CURVA: 140, MEIA_VOLTA: 100}


@dataclass(frozen=True)
class Pacote:
    """Campos do pacote binário de 18 bytes (doc 4.4, seção 6.2)."""

    timestamp_ms: int
    labirinto: str
    status: str
    desafio_cumprido: bool
    bateria_pct: int
    bateria_mv: int
    velocidade_atual_mm_s: int
    velocidade_media_mm_s: int
    coluna: int
    linha: int
    orientacao: Direcao
    paredes: int


def _numero(valor: float) -> int | float:
    return int(valor) if float(valor).is_integer() else valor


def linha_gateway(pacote: Pacote, seq: int, rssi: int) -> str:
    """Converte o pacote na linha JSON escrita pelo gateway na serial (sem o '\\n')."""
    dados = {
        "seq": seq,
        "rssi": rssi,
        "timestamp_ms": pacote.timestamp_ms,
        "labirinto": pacote.labirinto,
        "status": pacote.status,
        "desafio_cumprido": pacote.desafio_cumprido,
        "bateria_pct": pacote.bateria_pct,
        "bateria_volts": _numero(pacote.bateria_mv / 1000),
        "velocidade_atual": _numero(pacote.velocidade_atual_mm_s / 10),
        "velocidade_media": _numero(pacote.velocidade_media_mm_s / 10),
        "posicao": {"coluna": _LETRAS_COLUNA[pacote.coluna], "linha": pacote.linha + 1},
        "orientacao": pacote.orientacao.name,
        "paredes": {d.name.lower(): bool(pacote.paredes & d.mascara) for d in Direcao},
    }
    return json.dumps(dados, separators=(",", ":"), ensure_ascii=False)


def gerar_corrida(
    pista: Pista,
    cenario: str = "completa",
    *,
    semente: int = 141,
    seq_inicial: int = 1,
    mensagens_iniciais: int = 2,
    mensagens_finais: int = 2,
    fracao_interrupcao: float = 0.5,
) -> list[str]:
    """Simula uma corrida inteira e devolve as linhas JSON na ordem de envio."""
    if cenario not in CENARIOS:
        raise ValueError(
            f"cenário desconhecido: {cenario!r} (use {', '.join(CENARIOS)})"
        )

    sorteio = random.Random(semente)
    robo = RoboVirtual(pista)
    estado_inicial = (
        robo.linha,
        robo.coluna,
        robo.orientacao,
        robo.paredes_da_celula_atual(),
    )
    passos = list(robo.explorar())
    if cenario != "completa":
        # Interrompe no meio do percurso, nunca na largada nem na chegada.
        corte = min(max(1, int(len(passos) * fracao_interrupcao)), len(passos) - 1)
        passos = passos[:corte]

    linhas: list[str] = []
    seq = seq_inicial
    velocidades: list[int] = []

    def emitir(tempo_ms, status, posicao, velocidade_mm_s, desafio=False):
        nonlocal seq
        linha, coluna, orientacao, paredes = posicao
        tempo_ms = min(tempo_ms, TIMESTAMP_MAX_MS)
        bateria_mv = (
            _BATERIA_INICIAL_MV
            - _DESCARGA_MV_POR_S * (tempo_ms // 1000)
            - sorteio.randint(0, 4)
        )
        faixa = _BATERIA_CHEIA_MV - _BATERIA_VAZIA_MV
        bateria_pct = round((bateria_mv - _BATERIA_VAZIA_MV) * 100 / faixa)
        media = round(sum(velocidades) / len(velocidades)) if velocidades else 0
        pacote = Pacote(
            timestamp_ms=tempo_ms,
            labirinto=pista.labirinto,
            status=status,
            desafio_cumprido=desafio,
            bateria_pct=max(0, min(100, bateria_pct)),
            bateria_mv=bateria_mv,
            velocidade_atual_mm_s=velocidade_mm_s,
            velocidade_media_mm_s=media,
            coluna=coluna,
            linha=linha,
            orientacao=orientacao,
            paredes=paredes,
        )
        linhas.append(linha_gateway(pacote, seq, sorteio.randint(-72, -48)))
        seq += 1

    for _ in range(mensagens_iniciais):
        emitir(0, "AGUARDANDO", estado_inicial, 0)

    posicao = estado_inicial
    tempo_ms = 0
    for indice, passo in enumerate(passos, start=1):
        tempo_ms = indice * INTERVALO_MS
        posicao = (passo.linha, passo.coluna, passo.orientacao, passo.paredes)
        chegou = cenario == "completa" and indice == len(passos)
        if chegou:
            emitir(tempo_ms, "CONCLUIDO", posicao, 0, desafio=True)
        else:
            velocidade = _VELOCIDADE_MM_S[passo.manobra] + sorteio.randint(-8, 8)
            velocidades.append(velocidade)
            emitir(tempo_ms, "MAPEANDO", posicao, velocidade)

    if cenario == "travado":
        tempo_ms += INTERVALO_MS
        emitir(tempo_ms, "TRAVADO", posicao, 0)

    if cenario in ("completa", "travado"):
        status_final = "CONCLUIDO" if cenario == "completa" else "TRAVADO"
        for _ in range(mensagens_finais):
            emitir(tempo_ms, status_final, posicao, 0, desafio=cenario == "completa")

    return linhas
