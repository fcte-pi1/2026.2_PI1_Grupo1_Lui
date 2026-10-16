"""Robô virtual que explora uma pista com flood fill até a chegada.

Serve só ao simulador; o algoritmo de navegação do firmware é outra issue (HU-06).
Como o robô real, ele só detecta as paredes da célula onde está (frente, esquerda e
direita) e o lado por onde entrou é passagem. O próprio mapa começa só com o
perímetro, e o flood fill trata paredes ainda não observadas como abertas.
"""

from collections import deque
from collections.abc import Iterator
from dataclasses import dataclass

from app.simulador.direcao import Direcao
from app.simulador.pistas import COLUNAS, Pista

# Manobra feita para chegar à célula: define a velocidade plausível do passo.
RETA = "reta"
CURVA = "curva"
MEIA_VOLTA = "meia_volta"


class ExploracaoSemSaida(RuntimeError):
    pass


@dataclass(frozen=True)
class Passo:
    linha: int
    coluna: int
    orientacao: Direcao
    paredes: int  # paredes da célula atual no mapa do robô (bitmask do contrato)
    manobra: str


class RoboVirtual:
    def __init__(self, pista: Pista, orientacao: Direcao = Direcao.LESTE):
        self.pista = pista
        self.linha, self.coluna = pista.largada
        self.orientacao = orientacao
        self.visitadas = {pista.largada}
        self.mapa = [[0] * COLUNAS for _ in range(pista.linhas)]
        for linha in range(pista.linhas):
            for coluna in range(COLUNAS):
                for direcao in Direcao:
                    if pista.vizinha(linha, coluna, direcao) is None:
                        self.mapa[linha][coluna] |= direcao.mascara
        self._sentir()

    @property
    def chegou(self) -> bool:
        return (self.linha, self.coluna) == self.pista.chegada

    def paredes_da_celula_atual(self) -> int:
        return self.mapa[self.linha][self.coluna]

    def distancias(self) -> list[list[int | None]]:
        """Flood fill a partir da chegada sobre o mapa do robô."""
        distancia: list[list[int | None]] = [
            [None] * COLUNAS for _ in range(self.pista.linhas)
        ]
        destino = self.pista.chegada
        distancia[destino[0]][destino[1]] = 0
        fila = deque([destino])
        while fila:
            linha, coluna = fila.popleft()
            for direcao in Direcao:
                vizinha = self.pista.vizinha(linha, coluna, direcao)
                if vizinha is None or self.mapa[linha][coluna] & direcao.mascara:
                    continue
                vl, vc = vizinha
                if distancia[vl][vc] is None:
                    distancia[vl][vc] = distancia[linha][coluna] + 1
                    fila.append(vizinha)
        return distancia

    def proxima_direcao(self) -> Direcao:
        """Vizinha aberta com menor distância; empate: frente, direita, esquerda, trás."""
        distancia = self.distancias()
        candidatas = (
            self.orientacao,
            self.orientacao.direita,
            self.orientacao.esquerda,
            self.orientacao.oposta,
        )
        melhor = None
        for direcao in candidatas:
            if self.paredes_da_celula_atual() & direcao.mascara:
                continue
            vl, vc = self.pista.vizinha(self.linha, self.coluna, direcao)
            if distancia[vl][vc] is None:
                continue
            if melhor is None or distancia[vl][vc] < distancia[melhor[1]][melhor[2]]:
                melhor = (direcao, vl, vc)
        if melhor is None:
            raise ExploracaoSemSaida(
                f"sem caminho conhecido até a chegada em {self.pista.labirinto}"
            )
        return melhor[0]

    def avancar(self) -> Passo:
        direcao = self.proxima_direcao()
        if direcao == self.orientacao:
            manobra = RETA
        elif direcao == self.orientacao.oposta:
            manobra = MEIA_VOLTA
        else:
            manobra = CURVA
        self.linha, self.coluna = self.pista.vizinha(self.linha, self.coluna, direcao)
        self.orientacao = direcao
        self.visitadas.add((self.linha, self.coluna))
        # O lado por onde entrou é passagem; os outros três vêm dos sensores.
        self._registrar(self.linha, self.coluna, direcao.oposta, parede=False)
        self._sentir()
        return Passo(
            self.linha, self.coluna, direcao, self.paredes_da_celula_atual(), manobra
        )

    def explorar(self, limite_passos: int | None = None) -> Iterator[Passo]:
        """Avança até a chegada, entregando um passo por célula percorrida."""
        limite = limite_passos or 10 * self.pista.linhas * COLUNAS
        passos = 0
        while not self.chegou:
            if passos >= limite:
                raise ExploracaoSemSaida(f"limite de {limite} passos atingido")
            passos += 1
            yield self.avancar()

    def _sentir(self) -> None:
        for direcao in (
            self.orientacao,
            self.orientacao.esquerda,
            self.orientacao.direita,
        ):
            parede = self.pista.tem_parede(self.linha, self.coluna, direcao)
            self._registrar(self.linha, self.coluna, direcao, parede)

    def _registrar(
        self, linha: int, coluna: int, direcao: Direcao, parede: bool
    ) -> None:
        vizinha = self.pista.vizinha(linha, coluna, direcao)
        if vizinha is None:
            return  # perímetro: já registrado como parede
        for (l, c), lado in (((linha, coluna), direcao), (vizinha, direcao.oposta)):
            if parede:
                self.mapa[l][c] |= lado.mascara
            else:
                self.mapa[l][c] &= ~lado.mascara
