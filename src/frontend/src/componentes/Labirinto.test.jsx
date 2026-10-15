import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, it, expect } from 'vitest';
import Labirinto from './Labirinto';

describe('componentes/Labirinto.jsx', () => {
    it('sem dados, deve exibir só a grade com o perímetro e sem robô/rastro', () => {
        const html = ReactDOMServer.renderToString(<Labirinto linhas={4} />);
        expect(html).toContain('MAPEAMENTO EM TEMPO REAL - GRID 4x4');
        expect(html).toContain('Grid de 4x4');
        expect(html).toContain('START');
        expect(html).toContain('FINISH');
        // Deve conter 16 celulas
        const cellCount = (html.match(/class="celula/g) || []).length;
        expect(cellCount).toBe(16);
        // Sem fallback de mock: sem robo e sem overlay de trajeto
        expect(html).not.toContain('robo-icon');
        expect(html).not.toContain('trajeto-overlay');
    });

    it('deve renderizar 32 celulas para geometria 8x4 sem fallback de mock', () => {
        const html = ReactDOMServer.renderToString(<Labirinto linhas={8} />);
        expect(html).toContain('MAPEAMENTO EM TEMPO REAL - GRID 8x4');
        const cellCount = (html.match(/class="celula/g) || []).length;
        expect(cellCount).toBe(32);
        expect(html).not.toContain('robo-icon');
    });

    it('deve renderizar 48 celulas para geometria 12x4 sem fallback de mock', () => {
        const html = ReactDOMServer.renderToString(<Labirinto linhas={12} />);
        expect(html).toContain('MAPEAMENTO EM TEMPO REAL - GRID 12x4');
        const cellCount = (html.match(/class="celula/g) || []).length;
        expect(cellCount).toBe(48);
        expect(html).not.toContain('robo-icon');
    });

    it('deve renderizar o robo com posicao e orientacao dinamicas quando fornecidos', () => {
        const html = ReactDOMServer.renderToString(
            <Labirinto 
                linhas={4} 
                roboPosicao={{ x: 1, y: 2 }} 
                roboOrientacao="SUL" 
            />
        );
        expect(html).toContain('robo-icon');
        // Coordenadas calculadas: (1 + 0.5) * 100 = 150px, (2 + 0.5) * 100 = 250px
        expect(html).toContain('left:150px');
        expect(html).toContain('top:250px');
        // Rotação normalizada de SUL -> 90deg
        expect(html).toContain('rotate(90deg)');
    });

    it('deve renderizar trajeto percorrido customizado no overlay SVG', () => {
        const trajeto = [
            { x: 0, y: 0 },
            { x: 1, y: 0 },
            { x: 1, y: 1 }
        ];
        const html = ReactDOMServer.renderToString(
            <Labirinto linhas={4} trajeto={trajeto} />
        );
        expect(html).toContain('trajeto-overlay');
        expect(html).toContain('points="50,50 150,50 150,150"');
    });

    it('deve aplicar paredes dinamicas customizadas', () => {
        const paredes = [
            { id1: 'A01', id2: 'B01', dir1: 'right', dir2: 'left' }
        ];
        const html = ReactDOMServer.renderToString(
            <Labirinto linhas={4} paredes={paredes} />
        );
        // Deve renderizar borda escura de parede (3px solid #1a1a1a)
        expect(html).toContain('3px solid #1a1a1a');
    });

    it('deve aceitar lista de celulas acumuladas diretamente', () => {
        const celulasCustom = [
            { id: 'A01', x: 0, y: 0, isStart: true, isFinish: false, walls: { top: true, bottom: true, left: true, right: true } }
        ];
        const html = ReactDOMServer.renderToString(
            <Labirinto linhas={4} cells={celulasCustom} />
        );
        expect(html).toContain('celula-A01');
    });
});
