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
            expect(html).toContain('Histórico Geral');
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
    });
});
