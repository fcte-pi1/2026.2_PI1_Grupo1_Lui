import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { criarMonitorRf, LIMITE_SEM_PACOTE_MS, RF_NORMAL } from './monitorRf';

describe('servicos/monitorRf.js (HU-09)', () => {
    let estados;
    let monitor;

    beforeEach(() => {
        vi.useFakeTimers();
        vi.setSystemTime(0);
        estados = [];
        monitor = criarMonitorRf({ aoMudar: (estado) => estados.push(estado) });
    });

    afterEach(() => {
        monitor.fechar();
        vi.useRealTimers();
    });

    const ultimo = () => estados.at(-1) ?? RF_NORMAL;

    it('o limite do critério de aceite é 3 s', () => {
        expect(LIMITE_SEM_PACOTE_MS).toBe(3000);
    });

    it('não avisa antes de 3 s sem pacote e avisa ao completar 3 s', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(2900);
        expect(ultimo().perdida).toBe(false);
        vi.advanceTimersByTime(100);
        expect(ultimo()).toEqual({ perdida: true, segundosSemPacote: 3 });
    });

    it('cada pacote reinicia a contagem', () => {
        monitor.conexao(true);
        for (let i = 0; i < 10; i += 1) {
            vi.advanceTimersByTime(1000);
            monitor.pacote();
        }
        vi.advanceTimersByTime(2750);
        expect(estados).toEqual([]);
    });

    it('volta ao normal assim que um pacote chega', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(4000);
        expect(ultimo().perdida).toBe(true);
        monitor.pacote();
        expect(ultimo()).toEqual(RF_NORMAL);
    });

    it('atualiza os segundos sem pacote enquanto o aviso está ativo', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(5000);
        expect(estados.filter((e) => e.perdida).map((e) => e.segundosSemPacote)).toEqual([3, 4, 5]);
    });

    it('não avisa com o painel desconectado do backend, só o indicador DESCONECTADO vale', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(3500);
        expect(ultimo().perdida).toBe(true);
        monitor.conexao(false);
        expect(ultimo()).toEqual(RF_NORMAL);
        vi.advanceTimersByTime(10_000);
        expect(ultimo()).toEqual(RF_NORMAL);
        monitor.pacote(); // não deveria acontecer fechado, mas não liga a contagem
        vi.advanceTimersByTime(10_000);
        expect(ultimo()).toEqual(RF_NORMAL);
    });

    it('a contagem recomeça quando a conexão com o backend volta', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(1000);
        monitor.conexao(false);
        vi.advanceTimersByTime(60_000);
        monitor.conexao(true);
        vi.advanceTimersByTime(2900);
        expect(ultimo().perdida).toBe(false);
        vi.advanceTimersByTime(100);
        expect(ultimo().perdida).toBe(true);
    });

    it('avisa também se o robô nunca transmitiu desde a conexão', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(3000);
        expect(ultimo().perdida).toBe(true);
    });

    it('só notifica mudanças de estado', () => {
        monitor.conexao(true);
        vi.advanceTimersByTime(2500);
        expect(estados).toEqual([]);
    });

    it('fechar() para a verificação', () => {
        monitor.conexao(true);
        monitor.fechar();
        vi.advanceTimersByTime(10_000);
        expect(estados).toEqual([]);
        expect(vi.getTimerCount()).toBe(0);
    });

    it('aceita outro limite', () => {
        const outros = [];
        const curto = criarMonitorRf({ aoMudar: (e) => outros.push(e), limiteMs: 1000 });
        curto.conexao(true);
        vi.advanceTimersByTime(1000);
        expect(outros.at(-1)).toEqual({ perdida: true, segundosSemPacote: 1 });
        curto.fechar();
    });
});
