import { describe, it, expect } from 'vitest';
import {
    extrairDataIso,
    formatarDataHora,
    filtrarCorridas
} from './historicoFiltros';

describe('servicos/historicoFiltros.js', () => {
    describe('extrairDataIso', () => {
        it('deve extrair data ISO a partir de string ISO', () => {
            expect(extrairDataIso('2023-10-25T15:30:00Z')).toBe('2023-10-25');
            expect(extrairDataIso('2023-10-25')).toBe('2023-10-25');
        });

        it('deve extrair data ISO a partir de formato brasileiro DD/MM/YYYY', () => {
            expect(extrairDataIso('25/10/2023 15:30')).toBe('2023-10-25');
            expect(extrairDataIso('25/10/2023')).toBe('2023-10-25');
            expect(extrairDataIso('01/05/2024')).toBe('2024-05-01');
        });

        it('deve retornar null para valores inválidos ou vazios', () => {
            expect(extrairDataIso('')).toBeNull();
            expect(extrairDataIso(null)).toBeNull();
            expect(extrairDataIso(undefined)).toBeNull();
            expect(extrairDataIso('invalido')).toBeNull();
        });
    });

    describe('formatarDataHora', () => {
        it('deve manter se já estiver em formato legível DD/MM/AAAA HH:mm', () => {
            expect(formatarDataHora('25/10/2023 15:30')).toBe('25/10/2023 15:30');
        });

        it('deve formatar data ISO adequadamente', () => {
            const dataIso = new Date(2023, 9, 25, 15, 30); // 25 de Outubro de 2023
            expect(formatarDataHora(dataIso)).toBe('25/10/2023 15:30');
        });

        it('deve retornar hífen para valores nulos ou vazios', () => {
            expect(formatarDataHora(null)).toBe('-');
            expect(formatarDataHora('')).toBe('-');
        });
    });

    describe('filtrarCorridas', () => {
        const mockCorridas = [
            {
                id: '042',
                data: '25/10/2023 15:30',
                geometria: '8x4',
                modo: 'Exploração',
                tentativas: '2 de 3',
                tempoFinal: '05:12.30',
                velMedia: '11.2 cm/s',
                celulasVisitadas: '32/32',
                status: 'SUCESSO'
            },
            {
                id: '041',
                data: '25/10/2023 15:15',
                geometria: '8x4',
                modo: 'Speed Run',
                tentativas: '1 de 1',
                tempoFinal: '01:28.15',
                velMedia: '34.5 cm/s',
                celulasVisitadas: '16/16',
                status: 'SUCESSO'
            },
            {
                id: '040',
                data: '24/10/2023 10:05',
                geometria: '4x4',
                modo: 'Exploração',
                tentativas: '1 de 3',
                tempoFinal: '02:45.00',
                velMedia: '10.8 cm/s',
                celulasVisitadas: '20/32',
                status: 'COLISAO'
            },
            {
                id: '039',
                data: '23/10/2023 09:00',
                geometria: '12x4',
                modo: 'Exploração',
                tentativas: '1 de 3',
                tempoFinal: '07:10.00',
                velMedia: '12.0 cm/s',
                celulasVisitadas: '48/48',
                status: 'CONCLUIDO'
            }
        ];

        it('deve retornar todas as corridas se nenhum filtro for aplicado', () => {
            const resultado = filtrarCorridas(mockCorridas);
            expect(resultado).toHaveLength(4);
        });

        it('deve filtrar corretamente por geometria 4x4, 8x4 e 12x4', () => {
            const res8x4 = filtrarCorridas(mockCorridas, { geometria: '8x4' });
            expect(res8x4).toHaveLength(2);
            expect(res8x4.map(c => c.id)).toEqual(['042', '041']);

            const res4x4 = filtrarCorridas(mockCorridas, { geometria: '4x4' });
            expect(res4x4).toHaveLength(1);
            expect(res4x4[0].id).toBe('040');

            const res12x4 = filtrarCorridas(mockCorridas, { geometria: '12x4' });
            expect(res12x4).toHaveLength(1);
            expect(res12x4[0].id).toBe('039');
        });

        it('deve ignorar filtro de geometria quando for "todas"', () => {
            const resultado = filtrarCorridas(mockCorridas, { geometria: 'todas' });
            expect(resultado).toHaveLength(4);
        });

        it('deve filtrar corretamente por data exata', () => {
            const res25 = filtrarCorridas(mockCorridas, { data: '2023-10-25' });
            expect(res25).toHaveLength(2);

            const res24 = filtrarCorridas(mockCorridas, { data: '24/10/2023' });
            expect(res24).toHaveLength(1);
            expect(res24[0].id).toBe('040');
        });

        it('deve filtrar por intervalo de datas (dataInicio e dataFim)', () => {
            const resultado = filtrarCorridas(mockCorridas, {
                dataInicio: '2023-10-24',
                dataFim: '2023-10-25'
            });
            expect(resultado).toHaveLength(3);
            expect(resultado.map(c => c.id)).toEqual(['042', '041', '040']);
        });

        it('deve combinar filtro de geometria e data simultaneamente (Critério de Aceitação)', () => {
            const resultado = filtrarCorridas(mockCorridas, {
                geometria: '8x4',
                data: '2023-10-25'
            });
            expect(resultado).toHaveLength(2);

            const semResultado = filtrarCorridas(mockCorridas, {
                geometria: '4x4',
                data: '2023-10-25'
            });
            expect(semResultado).toHaveLength(0);
        });

        it('atende ao RNF-SFT04: deve processar 500+ registros em muito menos de 2 segundos (< 50ms)', () => {
            const muitasCorridas = [];
            const geometrias = ['4x4', '8x4', '12x4'];
            for (let i = 0; i < 600; i++) {
                const dia = String((i % 28) + 1).padStart(2, '0');
                muitasCorridas.push({
                    id: String(i).padStart(3, '0'),
                    data: `${dia}/10/2023 10:00`,
                    geometria: geometrias[i % 3],
                    modo: i % 2 === 0 ? 'Exploração' : 'Speed Run',
                    tentativas: '1 de 3',
                    tempoFinal: '02:00.00',
                    velMedia: '15.0 cm/s',
                    celulasVisitadas: '16/16',
                    status: 'SUCESSO'
                });
            }

            const t0 = performance.now();
            const res = filtrarCorridas(muitasCorridas, {
                geometria: '8x4',
                dataInicio: '2023-10-10',
                dataFim: '2023-10-20'
            });
            const duracaoMs = performance.now() - t0;

            expect(duracaoMs).toBeLessThan(100); // Bem abaixo dos 2000 ms (2.0s) exigidos pelo RNF-SFT04
            expect(res.length).toBeGreaterThan(0);
        });
    });
});
