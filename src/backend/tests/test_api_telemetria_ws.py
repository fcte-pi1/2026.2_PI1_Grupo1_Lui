import asyncio

from app.api.telemetria_ws import GerenciadorConexoes
from app.modelos.telemetria import TelemetriaMensagem

# Os handlers do WebSocket rodam no laço de eventos do TestClient. Consultas e
# difusões passam por cliente.portal para executar nesse mesmo laço, depois que
# a conexão já foi registrada.


def total_conectados(cliente, gerenciador):
    return cliente.portal.call(lambda: gerenciador.total)


def test_difunde_para_todos_os_clientes(app, cliente, mensagem):
    gerenciador = app.state.gerenciador
    telemetria = TelemetriaMensagem.model_validate({**mensagem, "seq": 7, "rssi": -60})

    with (
        cliente.websocket_connect("/ws/telemetria") as ws1,
        cliente.websocket_connect("/ws/telemetria") as ws2,
    ):
        assert total_conectados(cliente, gerenciador) == 2

        cliente.portal.call(gerenciador.difundir, telemetria)

        esperado = {**mensagem, "seq": 7, "rssi": -60}
        assert ws1.receive_json() == esperado
        assert ws2.receive_json() == esperado


def test_mensagem_sem_seq_e_rssi_chega_sem_esses_campos(app, cliente, mensagem):
    gerenciador = app.state.gerenciador

    with cliente.websocket_connect("/ws/telemetria") as ws:
        assert total_conectados(cliente, gerenciador) == 1
        cliente.portal.call(
            gerenciador.difundir, TelemetriaMensagem.model_validate(mensagem)
        )

        assert ws.receive_json() == mensagem


def test_cliente_desconectado_e_removido(app, cliente, mensagem):
    gerenciador = app.state.gerenciador

    with cliente.websocket_connect("/ws/telemetria") as ws1:
        with cliente.websocket_connect("/ws/telemetria"):
            assert total_conectados(cliente, gerenciador) == 2

        assert total_conectados(cliente, gerenciador) == 1
        cliente.portal.call(
            gerenciador.difundir, TelemetriaMensagem.model_validate(mensagem)
        )
        assert ws1.receive_json()["timestamp_ms"] == mensagem["timestamp_ms"]

    assert gerenciador.total == 0


def test_mensagens_do_cliente_sao_ignoradas(app, cliente, mensagem):
    gerenciador = app.state.gerenciador

    with cliente.websocket_connect("/ws/telemetria") as ws:
        ws.send_text("comando qualquer")
        ws.send_bytes(b"\x00\x01")

        assert total_conectados(cliente, gerenciador) == 1
        cliente.portal.call(
            gerenciador.difundir, TelemetriaMensagem.model_validate(mensagem)
        )
        assert ws.receive_json()["status"] == "MAPEANDO"


class WebSocketFalso:
    def __init__(self, falha=False):
        self.falha = falha
        self.recebidas = []

    async def accept(self):
        pass

    async def send_text(self, texto):
        if self.falha:
            raise RuntimeError("conexão encerrada")
        self.recebidas.append(texto)


def test_difundir_remove_quem_falha_e_entrega_aos_demais(mensagem):
    gerenciador = GerenciadorConexoes()
    ativo, quebrado = WebSocketFalso(), WebSocketFalso(falha=True)
    telemetria = TelemetriaMensagem.model_validate(mensagem)

    async def cenario():
        await gerenciador.conectar(ativo)
        await gerenciador.conectar(quebrado)
        await gerenciador.difundir(telemetria)

    asyncio.run(cenario())

    assert gerenciador.total == 1
    assert ativo.recebidas == [telemetria.model_dump_json(exclude_none=True)]


def test_difundir_sem_clientes_nao_falha(mensagem):
    gerenciador = GerenciadorConexoes()

    asyncio.run(gerenciador.difundir(TelemetriaMensagem.model_validate(mensagem)))

    assert gerenciador.total == 0
