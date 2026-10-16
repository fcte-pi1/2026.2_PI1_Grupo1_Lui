"""Direções absolutas do labirinto, com os mesmos valores do contrato de dados.

Espelha src/firmware/robo/src/mapa/Direcao.h: o valor é o do campo 'orientacao' do
pacote binário e o bit de parede de cada direção é 1 << valor (norte 0x01, sul 0x02,
leste 0x04, oeste 0x08). O norte é o topo da grade; ir para o sul aumenta a linha e
ir para o leste aumenta a coluna.
"""

from enum import IntEnum


class Direcao(IntEnum):
    NORTE = 0
    SUL = 1
    LESTE = 2
    OESTE = 3

    @property
    def mascara(self) -> int:
        return 1 << self.value

    @property
    def oposta(self) -> "Direcao":
        return _OPOSTA[self]

    @property
    def direita(self) -> "Direcao":
        return _DIREITA[self]

    @property
    def esquerda(self) -> "Direcao":
        return _ESQUERDA[self]

    @property
    def delta(self) -> tuple[int, int]:
        """Deslocamento (linha, coluna) ao avançar uma célula."""
        return _DELTA[self]


_OPOSTA = {
    Direcao.NORTE: Direcao.SUL,
    Direcao.SUL: Direcao.NORTE,
    Direcao.LESTE: Direcao.OESTE,
    Direcao.OESTE: Direcao.LESTE,
}
_DIREITA = {
    Direcao.NORTE: Direcao.LESTE,
    Direcao.LESTE: Direcao.SUL,
    Direcao.SUL: Direcao.OESTE,
    Direcao.OESTE: Direcao.NORTE,
}
_ESQUERDA = {valor: chave for chave, valor in _DIREITA.items()}
_DELTA = {
    Direcao.NORTE: (-1, 0),
    Direcao.SUL: (1, 0),
    Direcao.LESTE: (0, 1),
    Direcao.OESTE: (0, -1),
}
