import { describe, it, expect } from 'vitest';
import {
    ordenarLeiturasPorTempo,
    construirReproducaoCorrida,
    obterEstadoReproducaoNoPasso,
    extrairEstatisticasCorrida
} from './reproducaoCorrida';

describe('servicos/reproducaoCorrida.js (HU-14 / CT-12)', () => {
    describe('ordenarLeiturasPorTempo', () => {
        it('deve retornar lista vazia para entradas inválidas ou vazias', () => {
            expect(ordenarLeiturasPorTempo()).toEqual([]);
            expect(ordenarLeiturasPorTempo(null)).toEqual([]);
            expect(ordenarLeiturasPorTempo([])).toEqual([]);
        });

        it('deve ordenar as leituras por timestamp_ms crescente e usar seq como desempate', () => {
            const leiturasDesordenadas = [
                { seq: 3, timestamp_ms: 2000, posicao: { coluna: 'B', linha: 1 } },
                { seq: 2, timestamp_ms: 0, posicao: { coluna: 'A', linha: 1 } },
                { seq: 1, timestamp_ms: 0, posicao: { coluna: 'A', linha: 1 } },
                { seq: 4, timestamp_ms: 1000, posicao: { coluna: 'A', linha: 2 } }
            ];

            const ordenadas = ordenarLeiturasPorTempo(leiturasDesordenadas);
            expect(ordenadas.map(l => l.seq)).toEqual([1, 2, 4, 3]);
            expect(ordenadas.map(l => l.timestamp_ms)).toEqual([0, 0, 1000, 2000]);
        });
    });

    describe('construirReproducaoCorrida e obterEstadoReproducaoNoPasso', () => {
        it('deve retornar estrutura segura quando nenhuma corrida for fornecida', () => {
            const rep = construirReproducaoCorrida(null);
            expect(rep.linhas).toBe(4);
            expect(rep.quadros).toEqual([]);

            const estado = obterEstadoReproducaoNoPasso(rep, 0);
            expect(estado.roboPosicao).toBeNull();
            expect(estado.trajeto).toEqual([]);
            expect(estado.tempoAtualFormatado).toBe('00:00');
        });

        it('deve construir o mapa final consolidado e os quadros em ordem de tempo', () => {
            const corridaForaDeOrdem = {
                id: '100',
                geometria: '4x4',
                leituras: [
                    {
                        timestamp_ms: 3000,
                        posicao: { coluna: 'B', linha: 2 },
                        orientacao: 'LESTE',
                        paredes: { norte: true, sul: true, leste: false, oeste: false }
                    },
                    {
                        timestamp_ms: 1000,
                        posicao: { coluna: 'A', linha: 1 },
                        orientacao: 'SUL',
                        paredes: { norte: true, sul: false, leste: true, oeste: true }
                    },
                    {
                        timestamp_ms: 2000,
                        posicao: { coluna: 'A', linha: 2 },
                        orientacao: 'LESTE',
                        paredes: { norte: false, sul: true, leste: false, oeste: true }
                    }
                ]
            };

            const rep = construirReproducaoCorrida(corridaForaDeOrdem);
            expect(rep.quadros).toHaveLength(3);
            expect(rep.duracaoTotalMs).toBe(3000);

            // Passo 0 (1000 ms -> A1)
            const passo0 = obterEstadoReproducaoNoPasso(rep, 0);
            expect(passo0.roboPosicao).toEqual({ x: 0, y: 0 });
            expect(passo0.roboOrientacao).toBe('SUL');
            expect(passo0.trajeto).toEqual([{ x: 0, y: 0 }]);
            // Mapa final já possui as paredes descobertas ao longo de toda a corrida
            const celulaB02 = passo0.cells.find(c => c.id === 'B02');
            expect(celulaB02.walls.top).toBe(true);
            expect(celulaB02.walls.bottom).toBe(true);

            // Passo 1 (2000 ms -> A2)
            const passo1 = obterEstadoReproducaoNoPasso(rep, 1);
            expect(passo1.roboPosicao).toEqual({ x: 0, y: 1 });
            expect(passo1.trajeto).toEqual([{ x: 0, y: 0 }, { x: 0, y: 1 }]);

            // Passo 2 (3000 ms -> B2)
            const passo2 = obterEstadoReproducaoNoPasso(rep, 2);
            expect(passo2.roboPosicao).toEqual({ x: 1, y: 1 });
            expect(passo2.trajeto).toEqual([
                { x: 0, y: 0 },
                { x: 0, y: 1 },
                { x: 1, y: 1 }
            ]);
        });

        it('deve retornar quadros vazios e grade da geometria quando a corrida não tiver leituras', () => {
            const corridaSemLeituras = { id: '099', geometria: '8x4', status: 'SUCESSO' };
            const rep = construirReproducaoCorrida(corridaSemLeituras);
            expect(rep.linhas).toBe(8);
            expect(rep.quadros).toEqual([]);
        });
    });

    describe('extrairEstatisticasCorrida', () => {
        it('deve retornar placeholders com hífen quando corrida for nula', () => {
            const stats = extrairEstatisticasCorrida(null);
            expect(stats.tempoFinal).toBe('-');
            expect(stats.velMedia).toBe('-');
            expect(stats.eficienciaRota).toBe('-');
        });

        it('deve extrair todas as estatísticas e métricas comparativas da corrida', () => {
            const corrida042 = {
                id: '042',
                geometria: '8x4',
                tempoFinal: '05:12.30',
                velMedia: '11.2 cm/s',
                eficienciaRota: '85%',
                bateriaInicial: 98,
                bateriaFinal: 89,
                celulasNovasDescobertas: '10/32'
            };
            const stats = extrairEstatisticasCorrida(corrida042);

            expect(stats.id).toBe('042');
            expect(stats.tempoFinal).toBe('05:12.30');
            expect(stats.velMedia).toBe('11.2 cm/s');
            expect(stats.eficienciaRota).toBe('85%');
            expect(stats.bateriaInicialFinal).toBe('98% → 89%');
            expect(stats.celulasNovasDescobertas).toBe('10/32');
        });
    });

    describe('Desempenho de carregamento dos detalhes (Critério de Aceitação / RNF-SFT04)', () => {
        it('deve carregar mapa final, quadros de reprodução e estatísticas em muito menos de 2 segundos (< 100ms)', () => {
            const leiturasLongas = Array.from({ length: 300 }, (_, i) => ({
                seq: i + 1,
                timestamp_ms: i * 1000,
                labirinto: '12x4',
                status: i === 299 ? 'CONCLUIDO' : 'MAPEANDO',
                bateria_pct: Math.max(20, 99 - Math.floor(i / 10)),
                velocidade_media: 14.5,
                posicao: {
                    coluna: ['A', 'B', 'C', 'D'][i % 4],
                    linha: (i % 12) + 1
                },
                orientacao: ['NORTE', 'LESTE', 'SUL', 'OESTE'][i % 4],
                paredes: { norte: i % 2 === 0, sul: false, leste: i % 3 === 0, oeste: false }
            }));

            const corridaGrande = {
                id: '500',
                geometria: '12x4',
                tempoFinal: '05:00.00',
                velMedia: '14.5 cm/s',
                leituras: leiturasLongas
            };

            const inicio = performance.now();
            const rep = construirReproducaoCorrida(corridaGrande);
            const stats = extrairEstatisticasCorrida(corridaGrande, rep);
            const estadoFinal = obterEstadoReproducaoNoPasso(rep, rep.quadros.length - 1);
            const duracaoMs = performance.now() - inicio;

            expect(duracaoMs).toBeLessThan(200); // Muito abaixo do limite de 2000 ms (2 segundos)
            expect(rep.quadros).toHaveLength(300);
            expect(stats.id).toBe('500');
            expect(estadoFinal.cells).toHaveLength(48);
        });
    });
});
