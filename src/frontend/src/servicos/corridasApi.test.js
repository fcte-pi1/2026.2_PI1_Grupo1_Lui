import { afterEach, describe, expect, it, vi } from 'vitest';
import { listarCorridas, urlApiCorridas } from './corridasApi';

afterEach(() => vi.restoreAllMocks());

describe('corridasApi', () => {
    it('consulta histórico e leituras para montar os dados do replay', async () => {
        vi.stubGlobal('fetch', vi.fn()
            .mockResolvedValueOnce({ ok: true, json: async () => [{ id: 7, iniciada_em: '2026-10-31T12:00:00Z' }] })
            .mockResolvedValueOnce({ ok: true, json: async () => ({ leituras: [{ status: 'INTERROMPIDA' }] }) }));

        await expect(listarCorridas('http://localhost/api/corridas')).resolves.toEqual([
            { id: 7, iniciada_em: '2026-10-31T12:00:00Z', data: '2026-10-31T12:00:00Z', leituras: [{ status: 'INTERROMPIDA' }] },
        ]);
    });

    it('monta a URL HTTP para o host do painel', () => {
        expect(urlApiCorridas({}, { protocol: 'https:', hostname: 'exemplo.local' }))
            .toBe('https://exemplo.local:8000/api/corridas');
    });

    it('permite configurar a origem da API separadamente', () => {
        expect(urlApiCorridas({ VITE_API_URL: 'https://api.exemplo.local/' }))
            .toBe('https://api.exemplo.local/api/corridas');
    });
});
