import { describe, it, expect } from 'vitest';
import {
    gerarGrid,
    aplicarParedes,
    calcularCellSize,
    normalizarOrientacao,
    calcularPosicaoPixelRobo,
    gerarPontosSvg,
    obterCenarioMock
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
            // Celula interna B02 nao deve ter paredes externas
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
            // Confirma imutabilidade do grid original
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

    describe('obterCenarioMock', () => {
        it('deve fornecer cenario 4x4 consistente com o prototipo', () => {
            const mock = obterCenarioMock(4);
            expect(mock.paredes.length).toBeGreaterThan(0);
            expect(mock.trajeto).toHaveLength(3);
            expect(mock.roboPos).toEqual({ x: 2, y: 1 });
            expect(mock.roboRotacao).toBe('0deg');
        });

        it('deve fornecer cenario 8x4 e 12x4 com orientacao e trajeto', () => {
            const mock8 = obterCenarioMock(8);
            expect(mock8.roboRotacao).toBe('90deg');

            const mock12 = obterCenarioMock(12);
            expect(mock12.paredes.length).toBeGreaterThan(5);
            expect(mock12.trajeto).toHaveLength(4);
        });
    });
});
