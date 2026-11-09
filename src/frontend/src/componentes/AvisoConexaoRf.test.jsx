import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import AvisoConexaoRf from './AvisoConexaoRf';

describe('componentes/AvisoConexaoRf.jsx (HU-09)', () => {
    it('não renderiza nada com o rádio normal', () => {
        expect(ReactDOMServer.renderToString(<AvisoConexaoRf />)).toBe('');
        expect(ReactDOMServer.renderToString(<AvisoConexaoRf perdida={false} segundosSemPacote={0} />)).toBe('');
    });

    it('exibe "Conexão RF perdida" como alerta, com o tempo sem pacotes', () => {
        const html = ReactDOMServer.renderToString(<AvisoConexaoRf perdida segundosSemPacote={4} />);
        expect(html).toContain('role="alert"');
        expect(html).toContain('data-testid="aviso-rf"');
        expect(html).toContain('Conexão RF perdida');
        expect(html).toContain('Nenhum pacote do robô há 4 s');
        expect(html).toContain('último pacote recebido');
    });
});
