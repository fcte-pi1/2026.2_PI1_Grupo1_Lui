"""Canal de tempo real do painel: WebSocket /ws/telemetria (doc 4.4, seção 6.4)."""

import asyncio

from fastapi import APIRouter, WebSocket

from app.modelos.telemetria import TelemetriaMensagem

router = APIRouter(tags=["telemetria"])


class GerenciadorConexoes:
    """Mantém os navegadores conectados, difunde mensagens e reidrata novos clientes."""

    MAX_HISTORICO_CORRIDA = 512

    def __init__(self) -> None:
        self._conexoes: set[WebSocket] = set()
        self._historico_corrida: list[str] = []
        self._ultima_mensagem: TelemetriaMensagem | None = None

    @property
    def total(self) -> int:
        return len(self._conexoes)

    def _inicia_nova_corrida(self, mensagem: TelemetriaMensagem) -> bool:
        anterior = self._ultima_mensagem
        if anterior is None:
            return True
        if mensagem.labirinto != anterior.labirinto:
            return True
        if mensagem.status == "AGUARDANDO" and anterior.status != "AGUARDANDO":
            return True
        return mensagem.timestamp_ms < anterior.timestamp_ms

    def _registrar_no_historico(self, mensagem: TelemetriaMensagem, texto: str) -> None:
        if self._inicia_nova_corrida(mensagem):
            self._historico_corrida.clear()
        elif (
            mensagem.status == "AGUARDANDO"
            and self._ultima_mensagem is not None
            and self._ultima_mensagem.status == "AGUARDANDO"
        ):
            # Em AGUARDANDO só mantém o pacote mais recente para não acumular repetição.
            self._historico_corrida.clear()
        self._ultima_mensagem = mensagem
        self._historico_corrida.append(texto)
        if len(self._historico_corrida) > self.MAX_HISTORICO_CORRIDA:
            self._historico_corrida = self._historico_corrida[-self.MAX_HISTORICO_CORRIDA :]

    async def conectar(self, websocket: WebSocket) -> None:
        await websocket.accept()
        self._conexoes.add(websocket)
        # Reenvia os pacotes da corrida ativa para que um F5/reconexão reconstrua o mapa.
        for texto in list(self._historico_corrida):
            try:
                await websocket.send_text(texto)
            except BaseException:
                self.desconectar(websocket)
                return

    def desconectar(self, websocket: WebSocket) -> None:
        self._conexoes.discard(websocket)

    async def difundir(self, mensagem: TelemetriaMensagem) -> None:
        """Envia a mensagem a todos os clientes em paralelo.

        Um cliente lento não atrasa os demais, e quem falhar no envio é removido.
        Os campos opcionais ausentes (seq e rssi) não são enviados.
        """
        texto = mensagem.model_dump_json(exclude_none=True)
        self._registrar_no_historico(mensagem, texto)
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
