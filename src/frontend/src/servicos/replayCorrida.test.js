import { describe, it, expect } from 'vitest';
import {
    ordenarLeiturasPorTempo,
    construirQuadrosReplay,
    obterEstadoNoTempo,
    calcularEstatisticasCorrida
} from './replayCorrida';
import corridas4x4 from './fixtures/corridas4x4.json';

describe('servicos/replayCorrida.js', () => {
    describe('ordenarLeiturasPorTempo', () => {
        it('deve retornar lista vazia para entrada inválida ou vazia', () => {
            expect(ordenarLeiturasPorTempo()).toEqual([]);
            expect(ordenarLeiturasPorTempo(null)).toEqual([]);
            expect(ordenarLeiturasPorTempo([])).toEqual([]);
        });

        it('deve ordenar leituras fora de ordem pelo timestamp_ms crescente', () => {
            const desordenadas = [
                { timestamp_ms: 3000, posicao: { coluna: 'C', linha: 1 } },
                { timestamp_ms: 1000, posicao: { coluna: 'A', linha: 1 } },
                { timestamp_ms: 2000, posicao: { coluna: 'B', linha: 1 } },
                null
            ];
            const ordenadas = ordenarLeiturasPorTempo(desordenadas);
            expect(ordenadas.map((l) => l.timestamp_ms)).toEqual([1000, 2000, 3000]);
            expect(ordenadas.map((l) => l.posicao.coluna)).toEqual(['A', 'B', 'C']);
        });

        it('deve preservar a ordem original quando timestamp_ms for igual ou ausente', () => {
            const empatadas = [
                { seq: 1, timestamp_ms: 1000 },
                { seq: 2, timestamp_ms: 1000 },
                { seq: 3 }
            ];
            const ordenadas = ordenarLeiturasPorTempo(empatadas);
            expect(ordenadas.map((l) => l.seq)).toEqual([3, 1, 2]);
        });
    });

    describe('construirQuadrosReplay', () => {
        it('deve construir estado vazio seguro quando não houver leituras', () => {
            const res = construirQuadrosReplay({ geometria: '8x4', tempo_total_ms: 15000 });
            expect(res.geometria).toBe('8x4');
            expect(res.quadros).toHaveLength(0);
            expect(res.duracaoTotalMs).toBe(15000);
            expect(res.mapaFinal.linhas).toBe(8);
        });

        it('deve acumular o mapa passo a passo para uma corrida real (corridas4x4.completa)', () => {
            const res = construirQuadrosReplay(corridas4x4.completa);
            expect(res.geometria).toBe('4x4');
            expect(res.quadros.length).toBe(corridas4x4.completa.length);
            expect(res.duracaoTotalMs).toBe(14000);

            // Primeiro quadro na largada A1
            expect(res.quadros[0].mapa.roboPosicao).toEqual({ x: 0, y: 0 });
            // Último quadro no objetivo D4
            expect(res.mapaFinal.roboPosicao).toEqual({ x: 3, y: 3 });
        });

        it('deve aceitar objeto de corrida com campo pacotes e tempo_total_ms maior', () => {
            const corrida = {
                labirinto: '12x4',
                tempo_total_ms: 25000,
                pacotes: [
                    { timestamp_ms: 1000, posicao: { coluna: 'A', linha: 1 }, orientacao: 'SUL' }
                ]
            };
            const res = construirQuadrosReplay(corrida);
            expect(res.geometria).toBe('12x4');
            expect(res.duracaoTotalMs).toBe(25000);
            expect(res.quadros).toHaveLength(1);
        });
    });

    describe('obterEstadoNoTempo', () => {
        it('deve retornar estado padrão quando dadosReplay for nulo ou sem quadros', () => {
            const estado = obterEstadoNoTempo(null, 5000);
            expect(estado.indice).toBe(-1);
            expect(estado.totalQuadros).toBe(0);
            expect(estado.pacote).toBeNull();
        });

        it('deve localizar o quadro correspondente a cada instante da barra de tempo', () => {
            const replay = construirQuadrosReplay(corridas4x4.completa);

            // No início (0 ms)
            const inicio = obterEstadoNoTempo(replay, 0);
            expect(inicio.timestampMs).toBe(0);
            expect(inicio.mapa.roboPosicao).toEqual({ x: 0, y: 0 });

            // No meio da corrida (5500 ms -> pega leitura de 5000 ms)
            const meio = obterEstadoNoTempo(replay, 5500);
            expect(meio.timestampMs).toBe(5000);
            expect(meio.mapa.rastro.length).toBeLessThan(replay.mapaFinal.rastro.length);

            // No final da corrida (14000 ms)
            const fim = obterEstadoNoTempo(replay, 14000);
            expect(fim.timestampMs).toBe(14000);
            expect(fim.mapa.roboPosicao).toEqual({ x: 3, y: 3 });
            expect(fim.pacote.status).toBe('CONCLUIDO');
        });
    });

    describe('calcularEstatisticasCorrida', () => {
        it('deve calcular estatísticas a partir das leituras da corrida', () => {
            const corrida = {
                id: '042',
                geometria: '4x4',
                leituras: corridas4x4.completa
            };
            const stats = calcularEstatisticasCorrida(corrida);
            expect(stats.id).toBe('#042');
            expect(stats.geometria).toBe('4x4');
            expect(stats.tempoFinal).toBe('00:14');
            expect(stats.velocidadeMedia).toContain('cm/s');
            expect(stats.eficienciaRota).toMatch(/^\d+%$/);
            expect(stats.bateriaInicialFinal).toBe('85% → 84%');
            expect(stats.celulasDescobertas).toMatch(/^\d+\/16$/);
        });

        it('deve respeitar campos explícitos de estatísticas e formatar eficiência numérica', () => {
            const corrida = {
                id: '#007',
                geometria: '8x4',
                tempoFinal: '05:12.30',
                velMedia: '11.2 cm/s',
                eficiencia: 85,
                bateriaInicialPct: 98,
                bateriaFinalPct: 89,
                tempoEstimado: '04:30',
                celulas_visitadas: 32
            };
            const stats = calcularEstatisticasCorrida(corrida);
            expect(stats.id).toBe('#007');
            expect(stats.tempoFinal).toBe('05:12.30');
            expect(stats.velocidadeMedia).toBe('11.2 cm/s');
            expect(stats.eficienciaRota).toBe('85%');
            expect(stats.bateriaInicialFinal).toBe('98% → 89%');
            expect(stats.tempoEstimadoVsReal).toBe('04:30 vs. 05:12.30');
            expect(stats.celulasDescobertas).toBe('32/32');
        });

        it('deve lidar com corrida vazia sem falhar', () => {
            const stats = calcularEstatisticasCorrida({});
            expect(stats.id).toBe('#001');
            expect(stats.eficienciaRota).toBe('100%');
            expect(stats.velocidadeMedia).toBe('0.0 cm/s');
            expect(stats.bateriaInicialFinal).toBe('-');
        });

        it('atende ao RNF-SFT04: constrói replay e estatísticas de corrida longa (600 leituras) em < 2s', () => {
            const leiturasLongas = Array.from({ length: 600 }, (_, i) => ({
                timestamp_ms: i * 1000,
                labirinto: '12x4',
                status: i === 599 ? 'CONCLUIDO' : 'MAPEANDO',
                bateria_pct: Math.max(20, 100 - Math.floor(i / 10)),
                velocidade_media: 25.4,
                posicao: {
                    coluna: ['A', 'B', 'C', 'D'][i % 4],
                    linha: (i % 12) + 1
                },
                orientacao: 'SUL',
                paredes: { norte: false, sul: false, leste: true, oeste: true }
            }));

            const t0 = performance.now();
            const replay = construirQuadrosReplay({ id: '500', leituras: leiturasLongas });
            const stats = calcularEstatisticasCorrida({ id: '500' }, replay);
            const estadoMeio = obterEstadoNoTempo(replay, 300000);
            const duracaoMs = performance.now() - t0;

            expect(duracaoMs).toBeLessThan(500); // Bem abaixo dos 2000 ms (RNF-SFT04)
            expect(replay.quadros).toHaveLength(600);
            expect(estadoMeio.timestampMs).toBe(300000);
            expect(stats.id).toBe('#500');
        });
    });
});
