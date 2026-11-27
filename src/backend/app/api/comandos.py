"""Endpoints de comando remoto do painel para o gateway/robô."""

from fastapi import APIRouter, HTTPException, Request
import serial

router = APIRouter(prefix="/api/comandos", tags=["comandos"])


@router.post("/largada")
async def enviar_largada(request: Request) -> dict[str, object]:
    """Envia o comando 'k' pela serial do gateway para iniciar a prova no robô."""
    fonte = getattr(request.app.state, "fonte", None)
    if fonte is None:
        raise HTTPException(
            status_code=503, detail="Nenhuma fonte de telemetria configurada."
        )
    if hasattr(fonte, "enviar_comando"):
        try:
            await fonte.enviar_comando("k")
        except (RuntimeError, OSError, serial.SerialException) as erro:
            raise HTTPException(status_code=503, detail=str(erro)) from erro
        return {"enviado": True, "comando": "k"}

    return {
        "enviado": False,
        "comando": "k",
        "detalhe": "Simulador em execução não requer comando serial.",
    }
