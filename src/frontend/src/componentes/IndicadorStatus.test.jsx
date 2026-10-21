import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import IndicadorStatus from './IndicadorStatus';

describe('componentes/IndicadorStatus.jsx (HU-19)', () => {
    it('deve exibir "Travado" em destaque com estilo-alerta, ícone e aviso quando status for TRAVADO', () => {
        const html = ReactDOMServer.renderToString(<IndicadorStatus status="TRAVADO" tentativa={2} />);
        expect(html).toContain('estilo-alerta');
        expect(html).toContain('Travado');
        expect(html).toContain('icone-alerta-status');
        expect(html).toContain('aviso-travamento');
        expect(html).toContain('Corte de tração por travamento das rodas');
        expect(html).toContain('Tentativa 2');
    });

    it.each([
        ['COLISAO', 'Colisão'],
        ['TIMEOUT', 'Tempo esgotado'],
    ])('deve aplicar estilo-alerta e ícone para o estado crítico %s', (status, rotulo) => {
        const html = ReactDOMServer.renderToString(<IndicadorStatus status={status} />);
        expect(html).toContain('estilo-alerta');
        expect(html).toContain(rotulo);
        expect(html).toContain('icone-alerta-status');
        expect(html).not.toContain('aviso-travamento');
    });

    it.each([
        ['AGUARDANDO', 'Em espera'],
        ['MAPEANDO', 'Explorando'],
        ['SPEED_RUN', 'Speed Run'],
        ['CONCLUIDO', 'Concluído'],
    ])('deve aplicar estilo-normal sem ícone de alerta para %s', (status, rotulo) => {
        const html = ReactDOMServer.renderToString(<IndicadorStatus status={status} />);
        expect(html).toContain('estilo-normal');
        expect(html).toContain(rotulo);
        expect(html).not.toContain('icone-alerta-status');
        expect(html).not.toContain('aviso-travamento');
    });
});
