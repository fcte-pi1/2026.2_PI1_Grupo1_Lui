import { afterEach, describe, expect, it, vi } from 'vitest';
import { listarCorridas, urlApiCorridas, construirUrlCorridas, obterCorrida } from './corridasApi';

afterEach(() => vi.restoreAllMocks());

describe('corridasApi', () => {
    it('consulta histórico da API', async () => {
        vi.stubGlobal('fetch', vi.fn()
            .mockResolvedValueOnce({ ok: true, json: async () => [{ id: 7, iniciada_em: '2026-10-31T12:00:00Z' }] }));

        await expect(listarCorridas('http://localhost/api/corridas')).resolves.toEqual([
            { id: 7, iniciada_em: '2026-10-31T12:00:00Z', data: '2026-10-31T12:00:00Z' },
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

    it('constrói URL com filtros de geometria e datas', () => {
        const urlBase = 'http://localhost:8000/api/corridas';
        expect(construirUrlCorridas(urlBase, { geometria: '8x4' }))
            .toBe('http://localhost:8000/api/corridas?geometria=8x4');
        expect(construirUrlCorridas(urlBase, { geometria: 'todas' }))
            .toBe('http://localhost:8000/api/corridas');
        expect(construirUrlCorridas(urlBase, { data: '2026-10-25' }))
            .toBe('http://localhost:8000/api/corridas?data=2026-10-25');
        expect(construirUrlCorridas(urlBase, { dataInicio: '2026-10-01', dataFim: '2026-10-31' }))
            .toBe('http://localhost:8000/api/corridas?data_inicio=2026-10-01&data_fim=2026-10-31');
        expect(construirUrlCorridas(urlBase, { geometria: '12x4', data: '2026-10-25' }))
            .toBe('http://localhost:8000/api/corridas?geometria=12x4&data=2026-10-25');
    });

    it('permite consultar corridas com objeto de filtros', async () => {
        const mockFetch = vi.fn()
            .mockResolvedValueOnce({ ok: true, json: async () => [{ id: 10, iniciada_em: '2026-10-25T15:00:00Z' }] });
        vi.stubGlobal('fetch', mockFetch);

        await listarCorridas('http://localhost/api/corridas', { geometria: '8x4' });
        expect(mockFetch).toHaveBeenCalledWith('http://localhost/api/corridas?geometria=8x4');
    });

    it('consulta dados de uma corrida específica via obterCorrida', async () => {
        vi.stubGlobal('fetch', vi.fn().mockResolvedValueOnce({
            ok: true,
            json: async () => ({ id: 42, labirinto: '8x4', iniciada_em: '2026-10-25T15:00:00Z', leituras: [] }),
        }));

        const corrida = await obterCorrida(42, 'http://localhost/api/corridas');
        expect(corrida).toEqual({
            id: 42,
            labirinto: '8x4',
            iniciada_em: '2026-10-25T15:00:00Z',
            data: '2026-10-25T15:00:00Z',
            leituras: [],
        });
    });
});
