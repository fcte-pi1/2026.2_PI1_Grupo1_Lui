import { describe, expect, it } from 'vitest';
import {
    diagnosticarStatus,
    diagnosticarAnomaliasTelemetria,
    diagnosticarAcaoResolver,
} from './diagnosticoLogs';

describe('diagnosticoLogs', () => {
    it('classifica TRAVADO como Mecânica/Eletrônica com dica prática', () => {
        const log = diagnosticarStatus({
            status: 'TRAVADO',
            posicao: { coluna: 'B', linha: 3 },
            orientacao: 'LESTE',
            timestamp_ms: 12000,
        }, 'id-1');

        expect(log.nivel).toBe('critico');
        expect(log.dominio).toBe('mecanica');
        expect(log.secundario).toBe('eletronica');
        expect(log.dica).toContain('GPIO 34/39');
        expect(log.tempoCorrida).toBe('00:12');
    });

    it('detecta bateria crítica, RSSI fraco e velocidade zero persistente', () => {
        const logs = diagnosticarAnomaliasTelemetria(
            {
                status: 'MAPEANDO',
                bateria_volts: 6.65,
                bateria_pct: 0,
                rssi: -88,
                velocidade_atual: 0,
                posicao: { coluna: 'A', linha: 2 },
                timestamp_ms: 5000,
            },
            {
                bateriaCriticaAvisada: false,
                bateriaBaixaAvisada: false,
                rssiFracoAvisado: false,
                pacotesParadosSeguidos: 3,
            },
            (() => {
                let n = 0;
                return () => `a-${++n}`;
            })(),
        );

        expect(logs.map((l) => l.dominio)).toEqual(['energia', 'radio', 'eletronica']);
        expect(logs[0].nivel).toBe('critico');
        expect(logs[1].nivel).toBe('alerta');
        expect(logs[2].nivel).toBe('alerta');
    });

    it('gera log de erro com dica de porta serial quando Resolver falha', () => {
        const log = diagnosticarAcaoResolver({
            sucesso: false,
            detalhe: 'Gateway serial não conectado.',
        }, 'cmd-1');

        expect(log.nivel).toBe('critico');
        expect(log.dominio).toBe('software');
        expect(log.secundario).toBe('eletronica');
        expect(log.dica).toContain('TELEMETRIA_FONTE=serial');
    });
});
