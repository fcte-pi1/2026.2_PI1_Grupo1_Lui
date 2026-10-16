import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import HistoricoCorridas from './HistoricoCorridas';

describe('componentes/HistoricoCorridas.jsx', () => {
    it('deve exibir "Nenhuma corrida registrada" quando a lista estiver vazia', () => {
        const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={[]} />);
        expect(html).toContain('HISTÓRICO');
        expect(html).toContain('Nenhuma corrida registrada');
    });

    it('deve exibir mensagem de vazio quando corridas não for fornecido', () => {
        const html = ReactDOMServer.renderToString(<HistoricoCorridas />);
        expect(html).toContain('Nenhuma corrida registrada');
    });

    it('deve listar corridas fornecidas via props', () => {
        const corridas = [
            { nome: 'Tentativa 1', status: 'Concluído' },
            { nome: 'Tentativa 2', status: 'Travado' }
        ];
        const html = ReactDOMServer.renderToString(<HistoricoCorridas corridas={corridas} />);
        expect(html).not.toContain('Nenhuma corrida registrada');
        expect(html).toContain('Tentativa 1');
        expect(html).toContain('Concluído');
        expect(html).toContain('Tentativa 2');
        expect(html).toContain('Travado');
    });
});
