import asyncio
import contextlib
import json
import socketserver
import threading

import pytest
import serial

from app.ingestao import fonte_serial
from app.ingestao.fonte_serial import FonteSerial, interpretar_linha


def linha_valida(seq=1, **extras):
    dados = {
        "seq": seq,
        "rssi": -58,
        "timestamp_ms": 142000,
        "labirinto": "8x4",
        "status": "MAPEANDO",
        "desafio_cumprido": False,
        "bateria_pct": 78,
        "bateria_volts": 7.42,
        "velocidade_atual": 35,
        "velocidade_media": 28.5,
        "posicao": {"coluna": "B", "linha": 4},
        "orientacao": "LESTE",
        "paredes": {"norte": False, "sul": True, "leste": True, "oeste": False},
        **extras,
    }
    return (json.dumps(dados, separators=(",", ":")) + "\n").encode()


class PortaFalsa:
    """Imita serial.Serial: cada readline() devolve o próximo item ou levanta o erro."""

    def __init__(self, itens):
        self.itens = list(itens)
        self.fechada = False

    def readline(self):
        if not self.itens:
            return b""  # timeout sem dados
        item = self.itens.pop(0)
        if isinstance(item, Exception):
            raise item
        return item

    def close(self):
        self.fechada = True


async def coletar(fonte, quantidade):
    recebidas = []
    async with contextlib.aclosing(fonte.mensagens()) as mensagens:
        async for mensagem in mensagens:
            recebidas.append(mensagem)
            if len(recebidas) == quantidade:
                break
    return recebidas


@pytest.mark.parametrize("final", [b"\n", b"\r\n", b""])
def test_linha_valida_e_aceita(final):
    mensagem = interpretar_linha(linha_valida().rstrip(b"\n") + final)

    assert mensagem is not None
    assert mensagem.seq == 1
    assert mensagem.posicao.coluna == "B"


def test_linha_em_texto_tambem_e_aceita():
    assert interpretar_linha(linha_valida().decode()).labirinto == "8x4"


@pytest.mark.parametrize(
    "linha",
    [
        b"",
        b"   \r\n",
        b"# gateway: pacote descartado (TAMANHO)\n",
        b"#MAC 24:6F:28:AA:BB:CC\n",
        b"\xff\xfe lixo\n",
        b'{"seq": 1, "timestamp_ms": \n',
        b"texto qualquer\n",
        linha_valida(posicao={"coluna": "B", "linha": 9}),  # linha 9 numa pista 8x4
        linha_valida(status="PARADO"),
    ],
    ids=[
        "vazia",
        "so-espacos",
        "debug",
        "debug-mac",
        "nao-utf8",
        "json-incompleto",
        "nao-json",
        "fora-da-geometria",
        "status-invalido",
    ],
)
def test_linha_invalida_ou_de_debug_e_descartada(linha):
    assert interpretar_linha(linha) is None


def test_le_so_as_linhas_validas_da_porta():
    porta = PortaFalsa(
        [
            b"#MAC 24:6F:28:AA:BB:CC\n",
            linha_valida(seq=1),
            b"lixo\n",
            b"",
            linha_valida(seq=2),
        ]
    )
    fonte = FonteSerial("COM9", abrir=lambda: porta)

    recebidas = asyncio.run(coletar(fonte, 2))

    assert [m.seq for m in recebidas] == [1, 2]
    assert porta.fechada


def test_reabre_a_porta_depois_de_perder_a_conexao():
    portas = [
        PortaFalsa([linha_valida(seq=1), serial.SerialException("cabo desconectado")]),
        PortaFalsa([linha_valida(seq=2)]),
    ]
    esperas = []

    async def dormir(segundos):
        esperas.append(segundos)

    fonte = FonteSerial(
        "COM9", abrir=lambda: portas.pop(0), espera_reconexao_s=3, dormir=dormir
    )
    primeira, segunda = portas

    recebidas = asyncio.run(coletar(fonte, 2))

    assert [m.seq for m in recebidas] == [1, 2]
    assert primeira.fechada and segunda.fechada
    assert esperas == [3]


def test_tenta_de_novo_quando_a_porta_nao_existe():
    tentativas = []

    def abrir():
        tentativas.append(1)
        if len(tentativas) < 3:
            raise serial.SerialException("could not open port 'COM9'")
        return PortaFalsa([linha_valida(seq=5)])

    async def dormir(_segundos):
        pass

    fonte = FonteSerial("COM9", abrir=abrir, dormir=dormir)

    assert [m.seq for m in asyncio.run(coletar(fonte, 1))] == [5]
    assert len(tentativas) == 3


def test_abre_a_porta_a_115200_bps_por_padrao(monkeypatch):
    aberturas = []

    def serial_falso(porta, baud, timeout):
        aberturas.append((porta, baud, timeout))
        return PortaFalsa([linha_valida()])

    monkeypatch.setattr(fonte_serial.serial, "serial_for_url", serial_falso)

    asyncio.run(coletar(FonteSerial("/dev/ttyUSB0"), 1))

    assert aberturas == [("/dev/ttyUSB0", 115200, 1)]


def test_le_um_gateway_emulado_por_socket_com_o_pyserial_real():
    # Um servidor TCP local faz o papel do ESP32: o pyserial abre socket:// do mesmo
    # jeito que abriria a porta COM do gateway.
    linhas = [
        b"#MAC 24:6F:28:AA:BB:CC\n",
        linha_valida(seq=1),
        b"lixo\n",
        linha_valida(seq=2),
    ]
    servidor = socketserver.TCPServer(("127.0.0.1", 0), _gateway_emulado(linhas))
    thread = threading.Thread(target=servidor.handle_request, daemon=True)
    thread.start()
    try:
        porta = f"socket://127.0.0.1:{servidor.server_address[1]}"
        recebidas = asyncio.run(coletar(FonteSerial(porta), 2))
    finally:
        thread.join(timeout=5)
        servidor.server_close()

    assert [m.seq for m in recebidas] == [1, 2]


def _gateway_emulado(linhas):
    # Como o gateway real, repete o envio até o cliente desconectar: o pyserial
    # descarta o que já chegou ao abrir a porta (reset_input_buffer).
    class Gateway(socketserver.BaseRequestHandler):
        def handle(self):
            self.request.settimeout(0.1)
            while True:
                try:
                    self.request.sendall(b"".join(linhas))
                    if self.request.recv(1) == b"":
                        return  # cliente fechou a conexão
                except TimeoutError:
                    continue
                except OSError:
                    return

    return Gateway
