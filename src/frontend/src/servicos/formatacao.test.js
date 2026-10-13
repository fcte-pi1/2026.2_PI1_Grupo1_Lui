import { describe, it, expect } from 'vitest';
import { formatarTempo, rotuloStatus, bateriaBaixa, estiloStatus, LIMITE_PROVA_MS } from './formatacao';

describe('formatacao.js', () => {
    describe('formatarTempo', () => {
        it('deve formatar 0 ms', () => {
            expect(formatarTempo(0)).toBe('00:00');
        });
        it('deve formatar 59.999 ms como 00:59', () => {
            expect(formatarTempo(59999)).toBe('00:59');
        });
        it('deve formatar 60.000 ms como 01:00', () => {
            expect(formatarTempo(60000)).toBe('01:00');
        });
        it('deve formatar LIMITE_PROVA_MS (600.000 ms) como 10:00', () => {
            expect(formatarTempo(LIMITE_PROVA_MS)).toBe('10:00');
        });
        it('deve retornar 00:00 para ms invalido', () => {
            expect(formatarTempo(-100)).toBe('00:00');
            expect(formatarTempo(null)).toBe('00:00');
        });
    });

    describe('rotuloStatus', () => {
        it('deve mapear os 7 estados corretamente', () => {
            expect(rotuloStatus('AGUARDANDO')).toBe('Em espera');
            expect(rotuloStatus('MAPEANDO')).toBe('Explorando');
            expect(rotuloStatus('SPEED_RUN')).toBe('Speed Run');
            expect(rotuloStatus('CONCLUIDO')).toBe('Concluído');
            expect(rotuloStatus('COLISAO')).toBe('Colisão');
            expect(rotuloStatus('TIMEOUT')).toBe('Tempo esgotado');
            expect(rotuloStatus('TRAVADO')).toBe('Travado');
        });
        it('deve retornar Desconhecido para status invalido', () => {
            expect(rotuloStatus('STATUS_INVENTADO')).toBe('Desconhecido');
            expect(rotuloStatus(null)).toBe('Desconhecido');
        });
    });

    describe('bateriaBaixa', () => {
        it('deve retornar true nos limites', () => {
            expect(bateriaBaixa(20)).toBe(true);
            expect(bateriaBaixa(19)).toBe(true);
            expect(bateriaBaixa(0)).toBe(true);
        });
        it('deve retornar false se estiver bem', () => {
            expect(bateriaBaixa(21)).toBe(false);
            expect(bateriaBaixa(100)).toBe(false);
        });
    });

    describe('estiloStatus', () => {
        it('deve retornar alerta para status criticos', () => {
            expect(estiloStatus('TRAVADO')).toBe('alerta');
            expect(estiloStatus('COLISAO')).toBe('alerta');
            expect(estiloStatus('TIMEOUT')).toBe('alerta');
        });
        it('deve retornar normal para o resto', () => {
            expect(estiloStatus('AGUARDANDO')).toBe('normal');
            expect(estiloStatus('MAPEANDO')).toBe('normal');
            expect(estiloStatus('CONCLUIDO')).toBe('normal');
        });
    });
});
