"""Pistas do simulador: leitura do desenho em texto e validação das regras da prova.

O formato está descrito no cabeçalho de pistas.txt. Regras verificadas ao carregar:
4 colunas e 4, 8 ou 12 linhas, perímetro fechado, largada em A1 num beco com a única
abertura para o leste e chegada (canto oposto) alcançável a partir da largada.
Regiões fechadas, sem acesso a partir da largada, são permitidas.
"""

from collections import deque
from dataclasses import dataclass
from pathlib import Path

from app.modelos.telemetria import LINHAS_POR_LABIRINTO
from app.simulador.direcao import Direcao

COLUNAS = 4
LARGURA_DESENHO = 4 * COLUNAS + 1
ARQUIVO_PISTAS = Path(__file__).with_name("pistas.txt")


class PistaInvalida(ValueError):
    pass


@dataclass(frozen=True)
class Pista:
    labirinto: str
    linhas: int
    # Bitmask de paredes de cada célula, indexado por [linha][coluna] a partir de 0.
    paredes: tuple[tuple[int, ...], ...]

    colunas = COLUNAS
    largada = (0, 0)

    @property
    def chegada(self) -> tuple[int, int]:
        return (self.linhas - 1, COLUNAS - 1)

    def tem_parede(self, linha: int, coluna: int, direcao: Direcao) -> bool:
        return bool(self.paredes[linha][coluna] & direcao.mascara)

    def vizinha(
        self, linha: int, coluna: int, direcao: Direcao
    ) -> tuple[int, int] | None:
        dl, dc = direcao.delta
        nl, nc = linha + dl, coluna + dc
        if 0 <= nl < self.linhas and 0 <= nc < COLUNAS:
            return (nl, nc)
        return None

    def alcancaveis(self) -> set[tuple[int, int]]:
        """Células acessíveis a partir da largada, andando só por passagens."""
        vistas = {self.largada}
        fila = deque([self.largada])
        while fila:
            linha, coluna = fila.popleft()
            for direcao in Direcao:
                destino = self.vizinha(linha, coluna, direcao)
                if (
                    destino is not None
                    and not self.tem_parede(linha, coluna, direcao)
                    and destino not in vistas
                ):
                    vistas.add(destino)
                    fila.append(destino)
        return vistas


def interpretar_desenho(desenho: list[str], labirinto: str) -> Pista:
    """Converte o desenho de uma pista em paredes por célula e valida as regras."""
    if labirinto not in LINHAS_POR_LABIRINTO:
        raise PistaInvalida(f"geometria desconhecida: {labirinto!r}")
    linhas = LINHAS_POR_LABIRINTO[labirinto]
    # Editores costumam apagar espaços no fim da linha; completar com espaços faz uma
    # parede leste ausente aparecer como perímetro aberto, e não como linha curta.
    texto = [linha.rstrip().ljust(LARGURA_DESENHO) for linha in desenho]
    if len(texto) != 2 * linhas + 1:
        raise PistaInvalida(
            f"{labirinto}: o desenho deve ter {2 * linhas + 1} linhas, tem {len(texto)}"
        )
    for numero, linha in enumerate(texto):
        if len(linha) > LARGURA_DESENHO:
            raise PistaInvalida(
                f"{labirinto}: linha {numero + 1} do desenho deve ter no máximo "
                f"{LARGURA_DESENHO} caracteres, tem {len(linha)}"
            )
        _validar_caracteres(labirinto, numero, linha)

    paredes = []
    for linha in range(linhas):
        acima, meio, abaixo = (
            texto[2 * linha],
            texto[2 * linha + 1],
            texto[2 * linha + 2],
        )
        celulas = []
        for coluna in range(COLUNAS):
            x = 4 * coluna
            mascara = 0
            if acima[x + 1] == "-":
                mascara |= Direcao.NORTE.mascara
            if abaixo[x + 1] == "-":
                mascara |= Direcao.SUL.mascara
            if meio[x + 4] == "|":
                mascara |= Direcao.LESTE.mascara
            if meio[x] == "|":
                mascara |= Direcao.OESTE.mascara
            celulas.append(mascara)
        paredes.append(tuple(celulas))

    pista = Pista(labirinto=labirinto, linhas=linhas, paredes=tuple(paredes))
    _validar_regras(pista)
    return pista


def interpretar_pistas(texto: str) -> dict[str, Pista]:
    """Lê várias pistas no formato de pistas.txt, separadas por [geometria]."""
    desenhos: dict[str, list[str]] = {}
    atual = None
    for linha in texto.splitlines():
        conteudo = linha.strip()
        if not conteudo or conteudo.startswith("#"):
            continue
        if conteudo.startswith("[") and conteudo.endswith("]"):
            atual = conteudo[1:-1].strip()
            if atual in desenhos:
                raise PistaInvalida(f"pista {atual} definida mais de uma vez")
            desenhos[atual] = []
        elif atual is None:
            raise PistaInvalida("desenho fora de uma seção [geometria]")
        else:
            desenhos[atual].append(linha)
    return {
        nome: interpretar_desenho(desenho, nome) for nome, desenho in desenhos.items()
    }


def carregar_pistas(caminho: Path = ARQUIVO_PISTAS) -> dict[str, Pista]:
    return interpretar_pistas(caminho.read_text(encoding="utf-8"))


def pista_de_referencia(labirinto: str) -> Pista:
    pistas = carregar_pistas()
    if labirinto not in pistas:
        raise PistaInvalida(f"não há pista de referência para {labirinto!r}")
    return pistas[labirinto]


def _validar_caracteres(labirinto: str, numero: int, linha: str) -> None:
    for coluna in range(COLUNAS + 1):
        x = 4 * coluna
        if numero % 2 == 0:
            if linha[x] != "+":
                raise PistaInvalida(f"{labirinto}: esperado '+' na linha {numero + 1}")
            if coluna < COLUNAS and linha[x + 1 : x + 4] not in ("---", "   "):
                raise PistaInvalida(
                    f"{labirinto}: parede horizontal inválida na linha {numero + 1}"
                )
        elif linha[x] not in ("|", " "):
            raise PistaInvalida(
                f"{labirinto}: parede vertical inválida na linha {numero + 1}"
            )


def _validar_regras(pista: Pista) -> None:
    for linha in range(pista.linhas):
        for coluna in range(COLUNAS):
            bordas = []
            if linha == 0:
                bordas.append(Direcao.NORTE)
            if linha == pista.linhas - 1:
                bordas.append(Direcao.SUL)
            if coluna == 0:
                bordas.append(Direcao.OESTE)
            if coluna == COLUNAS - 1:
                bordas.append(Direcao.LESTE)
            if any(not pista.tem_parede(linha, coluna, d) for d in bordas):
                raise PistaInvalida(
                    f"{pista.labirinto}: perímetro aberto na célula {linha}, {coluna}"
                )

    abertas = [d for d in Direcao if not pista.tem_parede(0, 0, d)]
    if abertas != [Direcao.LESTE]:
        raise PistaInvalida(
            f"{pista.labirinto}: a largada (A1) deve ser um beco com a única abertura para o leste"
        )
    if pista.chegada not in pista.alcancaveis():
        raise PistaInvalida(
            f"{pista.labirinto}: a chegada não é alcançável a partir da largada"
        )
