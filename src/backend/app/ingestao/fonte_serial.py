"""Fonte de telemetria que lê as linhas JSON do gateway pela porta serial USB.

Contrato da serial (src/README.md): 115200 bps, 8N1, um objeto JSON por linha
terminada em '\\n'. Linhas vazias, linhas de diagnóstico do gateway (começam com
'#'), bytes que não são UTF-8 e mensagens fora do contrato são descartados.
Se a porta não existir ou cair (cabo do gateway desconectado), a fonte registra o
problema e tenta reabri-la periodicamente.

A porta pode ser um nome comum (COM3, /dev/ttyUSB0) ou uma URL do pyserial, como
socket://localhost:7777, útil para testar o caminho serial sem o gateway.
"""

import asyncio
import logging
from collections.abc import AsyncIterator, Awaitable, Callable
from typing import Any

import serial
from pydantic import ValidationError

from app.modelos.telemetria import TelemetriaMensagem

BAUD_PADRAO = 115200

logger = logging.getLogger(__name__)


def interpretar_linha(bruta: bytes | str) -> TelemetriaMensagem | None:
    """Valida uma linha recebida do gateway; devolve None se ela deve ser descartada."""
    if isinstance(bruta, bytes):
        try:
            bruta = bruta.decode("utf-8")
        except UnicodeDecodeError:
            logger.debug("linha descartada: bytes que não são UTF-8")
            return None
    texto = bruta.strip()
    if not texto or texto.startswith("#"):
        return None
    try:
        return TelemetriaMensagem.model_validate_json(texto)
    except ValidationError as erro:
        logger.debug(
            "linha descartada (%s erro(s) de validação): %s", erro.error_count(), texto
        )
        return None


class FonteSerial:
    def __init__(
        self,
        porta: str,
        baud: int = BAUD_PADRAO,
        *,
        abrir: Callable[[], Any] | None = None,
        espera_reconexao_s: float = 2.0,
        dormir: Callable[[float], Awaitable[None]] = asyncio.sleep,
    ):
        self.porta = porta
        self.baud = baud
        # O timeout faz readline() voltar periodicamente mesmo sem dados.
        self._abrir = abrir or (lambda: serial.serial_for_url(porta, baud, timeout=1))
        self.espera_reconexao_s = espera_reconexao_s
        self._dormir = dormir

    async def mensagens(self) -> AsyncIterator[TelemetriaMensagem]:
        while True:
            try:
                conexao = await asyncio.to_thread(self._abrir)
            except (serial.SerialException, OSError) as erro:
                logger.warning(
                    "não foi possível abrir a porta %s: %s", self.porta, erro
                )
                await self._dormir(self.espera_reconexao_s)
                continue

            logger.info(
                "lendo telemetria do gateway em %s a %s bps", self.porta, self.baud
            )
            try:
                while True:
                    bruta = await asyncio.to_thread(conexao.readline)
                    if not bruta:
                        continue  # timeout sem dados
                    mensagem = interpretar_linha(bruta)
                    if mensagem is not None:
                        yield mensagem
            except (serial.SerialException, OSError) as erro:
                logger.warning(
                    "conexão com o gateway em %s perdida: %s", self.porta, erro
                )
            finally:
                conexao.close()
            await self._dormir(self.espera_reconexao_s)
