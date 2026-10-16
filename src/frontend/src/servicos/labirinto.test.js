import { describe, it, expect } from 'vitest';
import {
    gerarGrid,
    aplicarParedes,
    calcularCellSize,
    normalizarOrientacao,
    calcularPosicaoPixelRobo,
    gerarPontosSvg,
    obterCenarioMock,
    parseLinhas,
    criarEstadoInicial,
    aplicarPacote,
    acumularPacote,
    SEQUENCIAS_SIMULADAS_6_4
} from './labirinto';

describe('servicos/labirinto.js', () => {
    describe('calcularCellSize', () => {
        it('deve retornar 100 para grid 4x4', () => {
            expect(calcularCellSize(4)).toBe(100);
        });
        it('deve retornar 70 para grid 8x4', () => {
            expect(calcularCellSize(8)).toBe(70);
        });
        it('deve retornar 50 para grid 12x4', () => {
            expect(calcularCellSize(12)).toBe(50);
        });
    });

    describe('parseLinhas', () => {
        it('deve extrair o numero de linhas a partir da string ou numero', () => {
            expect(parseLinhas('4x4')).toBe(4);
            expect(parseLinhas('8x4')).toBe(8);
            expect(parseLinhas('12x4')).toBe(12);
            expect(parseLinhas(8)).toBe(8);
            expect(parseLinhas(null)).toBe(4);
        });
    });

    describe('criarEstadoInicial', () => {
        it('deve inicializar o estado padrão com perímetro e sem robô', () => {
            const estado = criarEstadoInicial('4x4');
            expect(estado.linhas).toBe(4);
            expect(estado.geometria).toBe('4x4');
            expect(estado.cells).toHaveLength(16);
            expect(estado.roboPosicao).toBeNull();
            expect(estado.rastro).toEqual([]);
            expect(estado.ultimoStatus).toBe('AGUARDANDO');
        });
    });

    describe('aplicarPacote (Contrato 6.4)', () => {
        it('deve registrar paredes na célula atual e de forma simétrica na vizinha', () => {
            const estadoInicial = criarEstadoInicial('4x4');
            const pacote = {
                labirinto: '4x4',
                posicao: { coluna: 'B', linha: 2 },
                orientacao: 'LESTE',
                paredes: { norte: true, sul: true, leste: true, oeste: true },
                status: 'MAPEANDO'
            };

            const novoEstado = aplicarPacote(estadoInicial, pacote);
            const celulas = novoEstado.cells;

            // B02: x=1, y=1
            const b02 = celulas.find(c => c.x === 1 && c.y === 1);
            expect(b02.walls.top).toBe(true);
            expect(b02.walls.bottom).toBe(true);
            expect(b02.walls.right).toBe(true);
            expect(b02.walls.left).toBe(true);

            // Vizinha Norte B01: x=1, y=0 deve ter parede bottom
            const b01 = celulas.find(c => c.x === 1 && c.y === 0);
            expect(b01.walls.bottom).toBe(true);

            // Vizinha Sul B03: x=1, y=2 deve ter parede top
            const b03 = celulas.find(c => c.x === 1 && c.y === 2);
            expect(b03.walls.top).toBe(true);

            // Vizinha Leste C02: x=2, y=1 deve ter parede left
            const c02 = celulas.find(c => c.x === 2 && c.y === 1);
            expect(c02.walls.left).toBe(true);

            // Vizinha Oeste A02: x=0, y=1 deve ter parede right
            const a02 = celulas.find(c => c.x === 0 && c.y === 1);
            expect(a02.walls.right).toBe(true);
        });

        it('não deve lançar erro nem acessar fora da matriz ao registrar paredes na borda', () => {
            const estadoInicial = criarEstadoInicial('4x4');
            const pacoteA01 = {
                labirinto: '4x4',
                posicao: { coluna: 'A', linha: 1 },
                orientacao: 'SUL',
                paredes: { norte: true, sul: false, leste: true, oeste: true },
                status: 'MAPEANDO'
            };

            const novoEstado = aplicarPacote(estadoInicial, pacoteA01);
            const a01 = novoEstado.cells.find(c => c.x === 0 && c.y === 0);
            expect(a01.walls.top).toBe(true);
            expect(a01.walls.left).toBe(true);
            expect(a01.walls.right).toBe(true);

            // Vizinha a Leste B01 deve ter recebido parede a Oeste
            const b01 = novoEstado.cells.find(c => c.x === 1 && c.y === 0);
            expect(b01.walls.left).toBe(true);
        });

        it('deve atualizar posição, orientação e rastro', () => {
            const estado0 = criarEstadoInicial('4x4');
            const p1 = {
                labirinto: '4x4',
                posicao: { coluna: 'A', linha: 1 },
                orientacao: 'SUL',
                paredes: { norte: true, sul: false, leste: true, oeste: true },
                status: 'MAPEANDO'
            };
            const estado1 = aplicarPacote(estado0, p1);

            expect(estado1.posicao).toEqual({ x: 0, y: 0, coluna: 'A', linha: 1 });
            expect(estado1.orientacao).toBe('SUL');
            expect(estado1.rastro).toEqual([{ x: 0, y: 0 }]);

            // Avança para A2
            const p2 = {
                labirinto: '4x4',
                posicao: { coluna: 'A', linha: 2 },
                orientacao: 'LESTE',
                paredes: { norte: false, sul: true, leste: false, oeste: true },
                status: 'MAPEANDO'
            };
            const estado2 = aplicarPacote(estado1, p2);

            expect(estado2.posicao).toEqual({ x: 0, y: 1, coluna: 'A', linha: 2 });
            expect(estado2.orientacao).toBe('LESTE');
            expect(estado2.rastro).toEqual([{ x: 0, y: 0 }, { x: 0, y: 1 }]);
        });

        it('não deve duplicar pontos consecutivos na mesma célula no rastro', () => {
            const estado0 = criarEstadoInicial('4x4');
            const p1 = {
                labirinto: '4x4',
                posicao: { coluna: 'A', linha: 1 },
                orientacao: 'NORTE',
                status: 'MAPEANDO'
            };
            const estado1 = aplicarPacote(estado0, p1);
            const p2 = {
                labirinto: '4x4',
                posicao: { coluna: 'A', linha: 1 },
                orientacao: 'LESTE',
                status: 'MAPEANDO'
            };
            const estado2 = aplicarPacote(estado1, p2);

            expect(estado2.rastro).toEqual([{ x: 0, y: 0 }]);
            expect(estado2.orientacao).toBe('LESTE');
        });

        it('deve garantir pureza da função sem mutar o estado anterior', () => {
            const estado0 = criarEstadoInicial('4x4');
            const pacote = {
                labirinto: '4x4',
                posicao: { coluna: 'C', linha: 2 },
                orientacao: 'SUL',
                paredes: { norte: true, sul: false, leste: true, oeste: false },
                status: 'MAPEANDO'
            };

            const estado1 = aplicarPacote(estado0, pacote);

            expect(estado0.rastro).toEqual([]);
            expect(estado0.cells.find(c => c.x === 2 && c.y === 1).walls.right).toBe(false);
            expect(estado1.cells.find(c => c.x === 2 && c.y === 1).walls.right).toBe(true);
        });

        it('deve ajustar a geometria se o pacote indicar outra dimensão', () => {
            const estado0 = criarEstadoInicial('4x4');
            const pacote8 = {
                labirinto: '8x4',
                posicao: { coluna: 'A', linha: 1 },
                orientacao: 'LESTE',
                paredes: { norte: true, sul: false, leste: false, oeste: true },
                status: 'MAPEANDO'
            };

            const estado8 = aplicarPacote(estado0, pacote8);
            expect(estado8.linhas).toBe(8);
            expect(estado8.geometria).toBe('8x4');
            expect(estado8.cells).toHaveLength(32);
        });

        it('deve reiniciar o mapa e rastro se status transicionar de AGUARDANDO para MAPEANDO', () => {
            let estado = criarEstadoInicial('4x4');
            estado.rastro = [{ x: 1, y: 1 }];
            estado.ultimoStatus = 'AGUARDANDO';

            const pacote = {
                labirinto: '4x4',
                posicao: { coluna: 'A', linha: 1 },
                orientacao: 'SUL',
                status: 'MAPEANDO'
            };

            const novoEstado = aplicarPacote(estado, pacote);
            expect(novoEstado.rastro).toEqual([{ x: 0, y: 0 }]);
        });

        it('deve ser exportado também pelo alias acumularPacote', () => {
            expect(acumularPacote).toBe(aplicarPacote);
        });
    });

    describe('gerarGrid', () => {
        it('deve gerar 16 celulas para grid 4x4 com colunas A-D', () => {
            const { cells, cols, rowsList } = gerarGrid(4);
            expect(cols).toEqual(['A', 'B', 'C', 'D']);
            expect(rowsList).toEqual(['01', '02', '03', '04']);
            expect(cells).toHaveLength(16);

            const a01 = cells.find(c => c.id === 'A01');
            expect(a01).toBeDefined();
            expect(a01.isStart).toBe(true);
            expect(a01.isFinish).toBe(false);
            expect(a01.walls.top).toBe(true);
            expect(a01.walls.left).toBe(true);

            const d04 = cells.find(c => c.id === 'D04');
            expect(d04).toBeDefined();
            expect(d04.isStart).toBe(false);
            expect(d04.isFinish).toBe(true);
            expect(d04.walls.bottom).toBe(true);
            expect(d04.walls.right).toBe(true);
        });

        it('deve gerar 32 celulas para grid 8x4 com finish em D08', () => {
            const { cells } = gerarGrid(8);
            expect(cells).toHaveLength(32);
            const d08 = cells.find(c => c.id === 'D08');
            expect(d08.isFinish).toBe(true);
            expect(d08.walls.bottom).toBe(true);
        });

        it('deve gerar 48 celulas para grid 12x4 com finish em D12', () => {
            const { cells } = gerarGrid(12);
            expect(cells).toHaveLength(48);
            const d12 = cells.find(c => c.id === 'D12');
            expect(d12.isFinish).toBe(true);
            expect(d12.walls.bottom).toBe(true);
        });

        it('deve configurar paredes externas corretamente', () => {
            const { cells } = gerarGrid(4);
            const b02 = cells.find(c => c.id === 'B02');
            expect(b02.walls.top).toBe(false);
            expect(b02.walls.bottom).toBe(false);
            expect(b02.walls.left).toBe(false);
            expect(b02.walls.right).toBe(false);
        });
    });

    describe('aplicarParedes', () => {
        it('deve aplicar parede entre duas celulas adjacentes', () => {
            const { cells } = gerarGrid(4);
            const paredes = [
                { id1: 'B02', id2: 'B03', dir1: 'bottom', dir2: 'top' }
            ];
            const resultado = aplicarParedes(cells, paredes);
            const b02 = resultado.find(c => c.id === 'B02');
            const b03 = resultado.find(c => c.id === 'B03');

            expect(b02.walls.bottom).toBe(true);
            expect(b03.walls.top).toBe(true);
            expect(cells.find(c => c.id === 'B02').walls.bottom).toBe(false);
        });

        it('deve suportar formato de parede em celula unica', () => {
            const { cells } = gerarGrid(4);
            const paredes = [{ id: 'B02', dir: 'right' }];
            const resultado = aplicarParedes(cells, paredes);
            expect(resultado.find(c => c.id === 'B02').walls.right).toBe(true);
        });

        it('deve ignorar paredes invalidas ou nulas', () => {
            const { cells } = gerarGrid(4);
            const resultado = aplicarParedes(cells, [null, undefined, {}]);
            expect(resultado).toHaveLength(16);
        });
    });

    describe('normalizarOrientacao', () => {
        it('deve normalizar direcoes cardeais para graus CSS', () => {
            expect(normalizarOrientacao('L')).toBe('0deg');
            expect(normalizarOrientacao('LESTE')).toBe('0deg');
            expect(normalizarOrientacao('E')).toBe('0deg');
            expect(normalizarOrientacao('S')).toBe('90deg');
            expect(normalizarOrientacao('SUL')).toBe('90deg');
            expect(normalizarOrientacao('O')).toBe('180deg');
            expect(normalizarOrientacao('OESTE')).toBe('180deg');
            expect(normalizarOrientacao('W')).toBe('180deg');
            expect(normalizarOrientacao('N')).toBe('270deg');
            expect(normalizarOrientacao('NORTE')).toBe('270deg');
        });

        it('deve aceitar numeros e strings já formatadas', () => {
            expect(normalizarOrientacao(90)).toBe('90deg');
            expect(normalizarOrientacao('180deg')).toBe('180deg');
            expect(normalizarOrientacao(null)).toBe('0deg');
        });
    });

    describe('calcularPosicaoPixelRobo', () => {
        it('deve centralizar o robo na celula com base no cellSize', () => {
            const pos = calcularPosicaoPixelRobo({ x: 2, y: 1 }, 100);
            expect(pos).toEqual({ x: 250, y: 150 });
        });

        it('deve retornar ponto padrao quando pos for nulo', () => {
            const pos = calcularPosicaoPixelRobo(null, 100);
            expect(pos).toEqual({ x: 50, y: 50 });
        });
    });

    describe('gerarPontosSvg', () => {
        it('deve gerar string de coordenadas para polyline SVG', () => {
            const trajeto = [
                { x: 0, y: 0 },
                { x: 0, y: 1 },
                { x: 2, y: 1 }
            ];
            const pontos = gerarPontosSvg(trajeto, 100);
            expect(pontos).toBe('50,50 50,150 250,150');
        });

        it('deve retornar string original se já for string', () => {
            expect(gerarPontosSvg('50,50 150,150', 100)).toBe('50,50 150,150');
        });

        it('deve retornar string vazia para trajeto invalido ou vazio', () => {
            expect(gerarPontosSvg(null, 100)).toBe('');
            expect(gerarPontosSvg([], 100)).toBe('');
        });
    });

    describe('SEQUENCIAS_SIMULADAS_6_4 (Mock coerente com o robô)', () => {
        it('as sequencias simuladas devem ser mensagens coerentes no formato 6.4', () => {
            const seq4 = SEQUENCIAS_SIMULADAS_6_4[4];
            expect(seq4).toBeDefined();
            expect(seq4.length).toBeGreaterThan(0);

            // Primeiro passo está em A1 e suas paredes são da célula A1
            const primeiroPasso = seq4[0];
            expect(primeiroPasso.labirinto).toBe('4x4');
            expect(primeiroPasso.posicao).toEqual({ coluna: 'A', linha: 1 });
            expect(primeiroPasso.paredes).toBeDefined();
            expect(primeiroPasso.paredes.norte).toBe(true);
            expect(primeiroPasso.paredes.oeste).toBe(true);

            // Último passo atinge o objetivo com CONCLUIDO
            const ultimoPasso = seq4[seq4.length - 1];
            expect(ultimoPasso.status).toBe('CONCLUIDO');
            expect(ultimoPasso.desafio_cumprido).toBe(true);
            expect(ultimoPasso.posicao).toEqual({ coluna: 'D', linha: 4 });
        });
    });

    describe('obterCenarioMock', () => {
        it('deve fornecer cenario 4x4 consistente com o prototipo', () => {
            const mock = obterCenarioMock(4);
            expect(mock.paredes.length).toBeGreaterThan(0);
            expect(mock.trajeto).toHaveLength(3);
            expect(mock.roboPos).toEqual({ x: 2, y: 1 });
            expect(mock.roboRotacao).toBe('0deg');
        });
    });
});
