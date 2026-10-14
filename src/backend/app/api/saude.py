"""Verificação de disponibilidade do backend."""

from fastapi import APIRouter

router = APIRouter(tags=["saude"])


@router.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}
