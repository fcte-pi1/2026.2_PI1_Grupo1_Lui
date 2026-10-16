"""Interface comum das fontes de telemetria (gateway real ou simulador)."""

from collections.abc import AsyncIterator
from typing import Protocol

from app.modelos.telemetria import TelemetriaMensagem


class FonteTelemetria(Protocol):
    def mensagens(self) -> AsyncIterator[TelemetriaMensagem]:
        """Entrega cada mensagem válida assim que ela chega, na ordem de recebimento."""
        ...
