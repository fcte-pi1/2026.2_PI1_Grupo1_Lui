import React from 'react';
import ReactDOMServer from 'react-dom/server';
import { describe, expect, it } from 'vitest';
import PainelDiagnosticoLogs from './PainelDiagnosticoLogs';

describe('PainelDiagnosticoLogs', () => {
    it('renderiza abas de diagnóstico, webhook e portas ESP32 sem emojis', () => {
        const html = ReactDOMServer.renderToStaticMarkup(
            <PainelDiagnosticoLogs
                logs={[
                    {
                        id: '1',
                        hora: '11:20:00',
                        tempoCorrida: '00:12',
                        nivel: 'critico',
                        dominio: 'mecanica',
                        secundario: 'eletronica',
                        titulo: 'Corte de tração por travamento',
                        detalhe: 'Sem pulsos de encoder.',
                        dica: 'Verifique GPIO 34/35.',
                    },
                ]}
                eventosWebhook={[
                    {
                        id: 'w1',
                        hora: '11:20:00',
                        direcao: 'RX',
                        origem: '/ws/telemetria',
                        payload: '{"status":"TRAVADO"}',
                    },
                ]}
                pacote={{
                    status: 'TRAVADO',
                    bateria_volts: 7.42,
                    rssi: -58,
                    paredes: { norte: true, leste: false, sul: true, oeste: false },
                }}
                conectado
                rfPerdida={false}
            />,
        );

        expect(html).toContain('MONITOR &amp; WEBHOOK');
        expect(html).toContain('Diagnóstico + Portas (1)');
        expect(html).toContain('Webhook (1)');
        expect(html).toContain('data-testid="card-diagnostico-logs"');
        expect(html).toContain('data-testid="card-portas-esp"');
        expect(html).toContain('3x ToF VL53L0X (I2C)');
        expect(html).toContain('Corte de tração por travamento');
        expect(html).toContain('Verifique GPIO 34/35.');
    });
});
