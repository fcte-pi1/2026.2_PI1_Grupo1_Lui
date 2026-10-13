"""Canal de tempo real do painel: WebSocket /ws/telemetria (doc 4.4, seção 6.4)."""

import asyncio

from fastapi import APIRouter, WebSocket

from app.modelos.telemetria import TelemetriaMensagem

router = APIRouter(tags=["telemetria"])


class GerenciadorConexoes:
    """Mantém os navegadores conectados e difunde cada mensagem a todos eles."""

    def __init__(self) -> None:
        self._conexoes: set[WebSocket] = set()

    @property
    def total(self) -> int:
        return len(self._conexoes)

    async def conectar(self, websocket: WebSocket) -> None:
        await websocket.accept()
        self._conexoes.add(websocket)

    def desconectar(self, websocket: WebSocket) -> None:
        self._conexoes.discard(websocket)

    async def difundir(self, mensagem: TelemetriaMensagem) -> None:
        """Envia a mensagem a todos os clientes em paralelo.

        Um cliente lento não atrasa os demais, e quem falhar no envio é removido.
        Os campos opcionais ausentes (seq e rssi) não são enviados.
        """
        texto = mensagem.model_dump_json(exclude_none=True)
        conexoes = list(self._conexoes)
        resultados = await asyncio.gather(
            *(websocket.send_text(texto) for websocket in conexoes),
            return_exceptions=True,
        )
        for websocket, resultado in zip(conexoes, resultados):
            if isinstance(resultado, BaseException):
                self.desconectar(websocket)


@router.websocket("/ws/telemetria")
async def telemetria(websocket: WebSocket) -> None:
    gerenciador: GerenciadorConexoes = websocket.app.state.gerenciador
    await gerenciador.conectar(websocket)
    try:
        # O painel só observa: o que o cliente enviar é ignorado, pois a
        # aplicação web nunca envia comandos ao robô (doc 4.4, seção 4.1).
        while True:
            mensagem = await websocket.receive()
            if mensagem["type"] == "websocket.disconnect":
                break
    finally:
        gerenciador.desconectar(websocket)
