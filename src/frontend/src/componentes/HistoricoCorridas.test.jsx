import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import HistoricoCorridas from './HistoricoCorridas';

describe('componentes/HistoricoCorridas.jsx', () => {
    describe('Modo compacto (para o painel de monitoramento)', () => {
        it('deve exibir "Nenhuma corrida registrada" quando a lista estiver vazia no modo compacto', () => {
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={[]} modo="compacto" />);
            expect(html).toContain('HISTÓRICO');
            expect(html).toContain('Nenhuma corrida registrada');
        });

        it('deve listar corridas fornecidas via props no modo compacto', () => {
            const corridas = [
                { nome: 'Tentativa 1', status: 'Concluído' },
                { nome: 'Tentativa 2', status: 'Travado' }
            ];
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={corridas} modo="compacto" />);
            expect(html).not.toContain('Nenhuma corrida registrada');
            expect(html).toContain('Tentativa 1');
            expect(html).toContain('Concluído');
            expect(html).toContain('Tentativa 2');
            expect(html).toContain('Travado');
        });

        it('deve exibir o modo de cada corrida e comparar o tempo do Speed Run com a Exploração no modo compacto (HU-16)', () => {
            const corridas = [
                {
                    id: 1,
                    nome: 'Tentativa 1',
                    labirinto: '4x4',
                    modo: 'EXPLORACAO',
                    status: 'CONCLUIDO',
                    resultado: 'Concluído',
                    tempo_total_ms: 14000,
                },
                {
                    id: 2,
                    nome: 'Tentativa 2',
                    labirinto: '4x4',
                    modo: 'SPEED_RUN',
                    status: 'CONCLUIDO',
                    resultado: 'Concluído',
                    tempo_total_ms: 10000,
                },
            ];
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={corridas} modo="compacto" />);
            expect(html).toContain('Tentativa 1 · Exploração');
            expect(html).toContain('00:14 · Concluído');
            expect(html).toContain('Tentativa 2 · Speed Run');
            expect(html).toContain('00:10 · Concluído');
            expect(html).toContain('-29% vs. Exploração (00:14)');
        });
    });

    describe('Modo tabela (HU-13 / CT-11)', () => {
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
                id: '040',
                data: '24/10/2023 10:05',
                geometria: '4x4',
                modo: 'Exploração',
                tentativas: '1 de 3',
                tempoFinal: '02:45.00',
                velMedia: '10.8 cm/s',
                celulasVisitadas: '20/32',
                status: 'COLISAO'
            }
        ];

        it('deve renderizar o cabeçalho e todas as colunas exigidas', () => {
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={mockCorridas} />);
            expect(html).toContain('Histórico Geral (General History)');
            expect(html).toContain('Grid');
            expect(html).toContain('Modo');
            expect(html).toContain('Tentativas');
            expect(html).toContain('Tempo Final');
            expect(html).toContain('Vel. Média');
            expect(html).toContain('Células Visitadas');
            expect(html).toContain('Status');
        });

        it('deve renderizar os dados das corridas nas linhas', () => {
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={mockCorridas} />);
            expect(html).toContain('042');
            expect(html).toContain('25/10/2023 15:30');
            expect(html).toContain('8x4');
            expect(html).toContain('05:12.30');
            expect(html).toContain('SUCESSO');
            
            expect(html).toContain('040');
            expect(html).toContain('4x4');
            expect(html).toContain('COLISÃO');
        });

        it('deve renderizar mensagem de "Nenhuma corrida encontrada" se a lista estiver vazia', () => {
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={[]} />);
            expect(html).toContain('Nenhuma corrida encontrada');
        });

        it('deve renderizar os controles de filtro por geometria e data', () => {
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={mockCorridas} />);
            expect(html).toContain('filtro-geometria');
            expect(html).toContain('filtro-data-inicio');
            expect(html).toContain('filtro-data-fim');
            expect(html).toContain('Todas');
            expect(html).toContain('4x4');
            expect(html).toContain('8x4');
            expect(html).toContain('12x4');
        });

        it('deve exibir o painel de inspeção e replay quando exibirInspecao for verdadeiro (HU-14)', () => {
            const html = ReactDOMServer.renderToString(
                <HistoricoCorridas corridas={mockCorridas} corridaSelecionadaId="042" exibirInspecao={true} />
            );
            expect(html).toContain('secao-inspecao-historico');
            expect(html).toContain('INSPECIONAR CORRIDA (INSPECT RUN) - #042');
            expect(html).toContain('05:12.30');
        });

        it('deve formatar e renderizar corridas reais persistidas no backend (HU-12 / HU-13)', () => {
            const corridasReais = [
                {
                    id: 1,
                    labirinto: '8x4',
                    geometria: '8x4',
                    status: 'CONCLUIDO',
                    modo: 'EXPLORACAO',
                    tentativa: 1,
                    iniciada_em: '2026-10-25T14:30:00Z',
                    encerrada_em: '2026-10-25T14:32:15Z',
                    tempo_total_ms: 135000,
                    velocidade_media: 28.5,
                    bateria_inicial_pct: 100,
                    bateria_final_pct: 82,
                    desafio_cumprido: true,
                    celulas_visitadas: 32,
                },
                {
                    id: 2,
                    labirinto: '4x4',
                    geometria: '4x4',
                    status: 'TRAVADO',
                    modo: 'EXPLORACAO',
                    tentativa: null,
                    iniciada_em: '2026-10-24T09:15:00Z',
                    encerrada_em: '2026-10-24T09:15:45Z',
                    tempo_total_ms: 45000,
                    velocidade_media: 12.0,
                    bateria_inicial_pct: 80,
                    bateria_final_pct: 75,
                    desafio_cumprido: false,
                    celulas_visitadas: 8,
                },
            ];

            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={corridasReais} />);
            expect(html).toContain('001');
            expect(html).toContain('8x4');
            expect(html).toContain('Exploração');
            expect(html).toContain('Tentativa 1');
            expect(html).toContain('02:15'); // 135000 ms formatado
            expect(html).toContain('28.5 cm/s');
            expect(html).toContain('32');
            expect(html).toContain('SUCESSO');

            expect(html).toContain('002');
            expect(html).toContain('4x4');
            expect(html).toContain('—'); // tentativa nula
            expect(html).toContain('00:45'); // 45000 ms formatado
            expect(html).toContain('12 cm/s');
            expect(html).toContain('TRAVADO');
        });

        it('atende ao RNF-SFT04 renderizando 500 registros de dados reais em menos de 2 segundos', () => {
            const baseData = new Date('2026-10-01T12:00:00Z').getTime();
            const geometrias = ['4x4', '8x4', '12x4'];
            const muitasCorridasReais = [];

            for (let i = 0; i < 500; i++) {
                muitasCorridasReais.push({
                    id: i + 1,
                    labirinto: geometrias[i % 3],
                    geometria: geometrias[i % 3],
                    status: i % 2 === 0 ? 'CONCLUIDO' : 'TRAVADO',
                    modo: i % 3 === 0 ? 'SPEED_RUN' : 'EXPLORACAO',
                    tentativa: (i % 3) + 1,
                    iniciada_em: new Date(baseData + i * 3600000).toISOString(),
                    tempo_total_ms: 60000 + i * 100,
                    velocidade_media: 25.0,
                    celulas_visitadas: 16,
                });
            }

            const t0 = performance.now();
            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={muitasCorridasReais} />);
            const duracao = performance.now() - t0;

            expect(duracao).toBeLessThan(1000); // Muito abaixo dos 2000 ms exigidos
            expect(html).toContain('001');
            expect(html).toContain('500');
        });

        it('deve exibir o modo Speed Run e a comparação de tempo com a exploração na tabela (HU-16)', () => {
            const corridasComSpeedRun = [
                {
                    id: 2,
                    labirinto: '12x4',
                    geometria: '12x4',
                    status: 'CONCLUIDO',
                    modo: 'SPEED_RUN',
                    tentativa: 2,
                    iniciada_em: '2026-10-25T14:35:00Z',
                    tempo_total_ms: 20000,
                    velocidade_media: 22.0,
                    celulas_visitadas: 21,
                },
                {
                    id: 1,
                    labirinto: '12x4',
                    geometria: '12x4',
                    status: 'CONCLUIDO',
                    modo: 'EXPLORACAO',
                    tentativa: 1,
                    iniciada_em: '2026-10-25T14:30:00Z',
                    tempo_total_ms: 30000,
                    velocidade_media: 15.0,
                    celulas_visitadas: 28,
                },
            ];

            const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={corridasComSpeedRun} />);
            expect(html).toContain('Speed Run');
            expect(html).toContain('Exploração');
            expect(html).toContain('00:20');
            expect(html).toContain('(-33% vs. 00:30)');
        });
    });
});
