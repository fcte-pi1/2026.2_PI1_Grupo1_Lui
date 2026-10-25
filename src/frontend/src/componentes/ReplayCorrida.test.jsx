import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import ReplayCorrida from './ReplayCorrida';
import corridas4x4 from '../servicos/fixtures/corridas4x4.json';

describe('componentes/ReplayCorrida.jsx', () => {
    it('deve exibir estado vazio quando nenhuma corrida estiver selecionada', () => {
        const html = ReactDOMServer.renderToString(<ReplayCorrida corrida={null} />);
        expect(html).toContain('replay-corrida-vazio');
        expect(html).toContain('Selecione uma corrida para inspecionar');
    });

    it('deve exibir o mapa final, controles de reprodução e estatísticas da corrida selecionada (HU-14)', () => {
        const corrida = {
            id: '042',
            geometria: '4x4',
            tempoFinal: '00:14',
            velMedia: '19.5 cm/s',
            eficienciaRota: '85%',
            bateriaInicialPct: 85,
            bateriaFinalPct: 83,
            tempoEstimado: '00:12',
            celulasVisitadas: '13/16',
            leituras: corridas4x4.completa
        };

        const html = ReactDOMServer.renderToString(<ReplayCorrida corrida={corrida} />);

        // Cabeçalhos dos 3 blocos do protótipo
        expect(html).toContain('INSPECIONAR CORRIDA (INSPECT RUN) - #042');
        expect(html).toContain('ESTATÍSTICAS DA CORRIDA (RUN STATS)');
        expect(html).toContain('MÉTRICAS COMPARATIVAS (COMPARATIVE METRICS)');

        // Controles de reprodução (Play e barra de tempo)
        expect(html).toContain('botao-play-pause');
        expect(html).toContain('Play');
        expect(html).toContain('barra-tempo-replay');
        expect(html).toContain('00:14');

        // Estatísticas e métricas
        expect(html).toContain('19.5 cm/s');
        expect(html).toContain('85%');
        expect(html).toContain('85% → 83%');
        expect(html).toContain('00:12 vs. 00:14');
        expect(html).toContain('13/16');
    });

    it('deve renderizar o mapa no instante especificado por tempoMsInicial', () => {
        const corrida = {
            id: '042',
            geometria: '4x4',
            leituras: corridas4x4.completa
        };

        const htmlInicio = ReactDOMServer.renderToString(
            <ReplayCorrida corrida={corrida} tempoMsInicial={0} />
        );
        expect(htmlInicio).toContain('00:00');
        expect(htmlInicio).toContain('00:14');
    });
});
