"""Consulta de corridas e leituras persistidas no schema MySQL do projeto."""
from fastapi import APIRouter, HTTPException, Request
from sqlalchemy import select

from app.banco import Corrida, Leitura

router = APIRouter(prefix="/api/corridas", tags=["corridas"])
ORIENTACAO_TELEMETRIA = {"N": "NORTE", "S": "SUL", "L": "LESTE", "O": "OESTE"}


def _status_atual(corrida):
    if corrida.status_final:
        return corrida.status_final
    return "SPEED_RUN" if corrida.modo == "SPEED_RUN" else "MAPEANDO"


def _resumo(corrida):
    return {
        "id": corrida.id,
        "labirinto": corrida.labirinto,
        "geometria": corrida.labirinto,
        "status": _status_atual(corrida),
        "modo": corrida.modo,
        "tentativa": corrida.tentativa,
        "iniciada_em": corrida.inicio.isoformat() if corrida.inicio else None,
        "encerrada_em": corrida.fim.isoformat() if corrida.fim else None,
        "tempo_total_ms": corrida.tempo_total_ms,
        "velocidade_media": corrida.velocidade_media,
        "bateria_inicial_pct": corrida.bateria_inicial_pct,
        "bateria_final_pct": corrida.bateria_final_pct,
        "desafio_cumprido": bool(corrida.desafio_cumprido),
        "celulas_visitadas": corrida.celulas_visitadas,
    }


def _leitura(leitura):
    return {
        "timestamp_ms": leitura.timestamp_ms,
        "labirinto": leitura.labirinto,
        "status": leitura.status,
        "desafio_cumprido": bool(leitura.desafio_cumprido),
        "bateria_pct": leitura.bateria_pct,
        "bateria_volts": leitura.bateria_volts,
        "velocidade_atual": leitura.velocidade_atual,
        "velocidade_media": leitura.velocidade_media,
        "posicao": {"coluna": leitura.coluna, "linha": leitura.linha},
        "orientacao": ORIENTACAO_TELEMETRIA[leitura.orientacao],
        "paredes": {
            "norte": None if leitura.parede_norte is None else bool(leitura.parede_norte),
            "sul": None if leitura.parede_sul is None else bool(leitura.parede_sul),
            "leste": None if leitura.parede_leste is None else bool(leitura.parede_leste),
            "oeste": None if leitura.parede_oeste is None else bool(leitura.parede_oeste),
        },
        "seq": leitura.seq,
        "rssi": leitura.rssi,
    }


@router.get("")
def listar_corridas(request: Request):
    with request.app.state.sessoes() as sessao:
        corridas = sessao.scalars(select(Corrida).order_by(Corrida.id.desc())).all()
        return [_resumo(corrida) for corrida in corridas]


@router.get("/{corrida_id}")
def obter_corrida(corrida_id: int, request: Request):
    with request.app.state.sessoes() as sessao:
        corrida = sessao.get(Corrida, corrida_id)
        if corrida is None:
            raise HTTPException(status_code=404, detail="Corrida não encontrada")
        dados = _resumo(corrida)
        dados["leituras"] = [
            _leitura(leitura)
            for leitura in sessao.scalars(
                select(Leitura).where(Leitura.corrida_id == corrida_id).order_by(Leitura.id)
            ).all()
        ]
        return dados
