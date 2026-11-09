"""Consulta de corridas e leituras persistidas no schema MySQL do projeto."""
import re
from datetime import datetime
from fastapi import APIRouter, HTTPException, Request
from sqlalchemy import select

from app.banco import Corrida, Leitura

router = APIRouter(prefix="/api/corridas", tags=["corridas"])


def _extrair_data(valor: str | None) -> str | None:
    """Extrai e normaliza data para YYYY-MM-DD suportando ISO e DD/MM/YYYY."""
    if not valor:
        return None
    val = valor.strip()
    match_iso = re.match(r"^(\d{4})-(\d{2})-(\d{2})", val)
    if match_iso:
        return f"{match_iso.group(1)}-{match_iso.group(2)}-{match_iso.group(3)}"
    match_br = re.match(r"^(\d{2})/(\d{2})/(\d{4})", val)
    if match_br:
        return f"{match_br.group(3)}-{match_br.group(2)}-{match_br.group(1)}"
    return None


def _utc_iso(valor):
    """Serializa o UTC ingênuo armazenado no banco sem perder o fuso."""
    return f"{valor.isoformat()}Z" if valor else None


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
        "iniciada_em": _utc_iso(corrida.inicio),
        "encerrada_em": _utc_iso(corrida.fim),
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
        "orientacao": leitura.orientacao,
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
def listar_corridas(
    request: Request,
    geometria: str | None = None,
    data: str | None = None,
    data_inicio: str | None = None,
    data_fim: str | None = None,
):
    query = select(Corrida).order_by(Corrida.id.desc())

    if geometria and geometria.lower() not in ("todas", "todos", ""):
        query = query.where(Corrida.labirinto == geometria)

    data_val = _extrair_data(data)
    if data_val:
        inicio_dt = datetime.fromisoformat(f"{data_val}T00:00:00")
        fim_dt = datetime.fromisoformat(f"{data_val}T23:59:59.999999")
        query = query.where(Corrida.inicio >= inicio_dt, Corrida.inicio <= fim_dt)

    inicio_val = _extrair_data(data_inicio)
    if inicio_val:
        inicio_dt = datetime.fromisoformat(f"{inicio_val}T00:00:00")
        query = query.where(Corrida.inicio >= inicio_dt)

    fim_val = _extrair_data(data_fim)
    if fim_val:
        fim_dt = datetime.fromisoformat(f"{fim_val}T23:59:59.999999")
        query = query.where(Corrida.inicio <= fim_dt)

    with request.app.state.sessoes() as sessao:
        corridas = sessao.scalars(query).all()
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
