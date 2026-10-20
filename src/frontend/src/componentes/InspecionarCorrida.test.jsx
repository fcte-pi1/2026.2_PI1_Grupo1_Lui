import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import InspecionarCorrida from './InspecionarCorrida';

const CORRIDA_EXEMPLO = {
    id: '042',
    data: '25/10/2023 15:30',
    geometria: '8x4',
    modo: 'Exploração',
    tentativas: '2 de 3',
    tempoFinal: '05:12.30',
    velMedia: '11.2 cm/s',
    celulasVisitadas: '32/32',
    status: 'SUCESSO',
    eficienciaRota: '85%',
    bateriaInicial: 98,
    bateriaFinal: 89,
    celulasNovasDescobertas: '10/32',
    leituras: [
        { timestamp_ms: 0, labirinto: '8x4', status: 'AGUARDANDO', bateria_pct: 98, velocidade_media: 0, posicao: { coluna: 'A', linha: 1 }, orientacao: 'LESTE', paredes: { norte: true, sul: true, leste: false, oeste: true } },
        { timestamp_ms: 60000, labirinto: '8x4', status: 'MAPEANDO', bateria_pct: 93, velocidade_media: 11.0, posicao: { coluna: 'B', linha: 1 }, orientacao: 'SUL', paredes: { norte: true, sul: false, leste: true, oeste: false } },
        { timestamp_ms: 312300, labirinto: '8x4', status: 'CONCLUIDO', bateria_pct: 89, velocidade_media: 11.2, posicao: { coluna: 'D', linha: 8 }, orientacao: 'LESTE', paredes: { norte: true, sul: true, leste: true, oeste: false } }
    ]
};

describe('componentes/InspecionarCorrida.jsx (HU-14 / CT-12)', () => {
    it('deve exibir estado vazio quando nenhuma corrida estiver selecionada', () => {
        const html = ReactDOMServer.renderToString(<InspecionarCorrida corrida={null} />);
        expect(html).toContain('inspecionar-vazio');
        expect(html).toContain('INSPECIONAR CORRIDA');
        expect(html).toContain('Selecione uma corrida na tabela');
    });

    it('deve exibir o mapa final, controles de reprodução, estatísticas e métricas ao selecionar uma corrida', () => {
        const html = ReactDOMServer.renderToString(<InspecionarCorrida corrida={CORRIDA_EXEMPLO} />);

        // Cabeçalho com ID da corrida sem termos em inglês
        expect(html).toContain('INSPECIONAR CORRIDA - #042');
        expect(html).not.toContain('INSPECT RUN');

        // Mapa final renderizado com robô e rastro
        expect(html).toContain('labirinto-grid');
        expect(html).toContain('robo-icon');
        expect(html).toContain('trajeto-overlay');

        // Controles de reprodução (botão Reproduzir e barra de tempo)
        expect(html).toContain('botao-play-pause');
        expect(html).toContain('▶ Reproduzir');
        expect(html).toContain('barra-tempo-reproducao');

        // Bloco de Estatísticas da Corrida
        expect(html).toContain('ESTATÍSTICAS DA CORRIDA');
        expect(html).not.toContain('RUN STATS');
        expect(html).toContain('05:12.30');
        expect(html).toContain('11.2 cm/s');
        expect(html).toContain('85%');

        // Bloco de Métricas Comparativas (sem Tempo Estimado vs. Real)
        expect(html).toContain('MÉTRICAS COMPARATIVAS');
        expect(html).not.toContain('COMPARATIVE METRICS');
        expect(html).not.toContain('Tempo Estimado');
        expect(html).toContain('98% → 89%');
        expect(html).toContain('10/32');
    });

    it('deve posicionar o robô no passo correspondente quando passoInicial for informado', () => {
        // No passo 0, o robô está em A1 (x=0, y=0) no grid 8x4 (cellSize = 70 -> 35px, 35px)
        const htmlPasso0 = ReactDOMServer.renderToString(
            <InspecionarCorrida corrida={CORRIDA_EXEMPLO} passoInicial={0} />
        );
        expect(htmlPasso0).toContain('left:35px');
        expect(htmlPasso0).toContain('top:35px');
        expect(htmlPasso0).toContain('00:00');
    });
});
