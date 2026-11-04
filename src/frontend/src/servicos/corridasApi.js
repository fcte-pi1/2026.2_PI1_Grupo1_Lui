const baseApi = (env = import.meta.env, local = globalThis.location) => {
    if (env?.VITE_API_URL) return env.VITE_API_URL.replace(/\/$/, '') + '/api/corridas';
    if (env?.VITE_TELEMETRIA_WS_URL) {
        const ws = new URL(env.VITE_TELEMETRIA_WS_URL);
        ws.protocol = ws.protocol === 'wss:' ? 'https:' : 'http:';
        return `${ws.origin}/api/corridas`;
    }
    const protocolo = local?.protocol === 'https:' ? 'https:' : 'http:';
    return `${protocolo}//${local?.hostname || 'localhost'}:8000/api/corridas`;
};

async function requisitar(url) {
    const resposta = await fetch(url);
    if (!resposta.ok) throw new Error(`Falha ao consultar corridas (${resposta.status})`);
    return resposta.json();
}

export function construirUrlCorridas(urlBase = baseApi(), filtros = {}) {
    const [baseUrl, queryExistente] = urlBase.split('?');
    const params = new URLSearchParams(queryExistente || '');

    if (filtros.geometria && filtros.geometria !== 'todas' && filtros.geometria !== 'todos') {
        params.set('geometria', filtros.geometria);
    }
    if (filtros.data) {
        params.set('data', filtros.data);
    }
    if (filtros.dataInicio || filtros.data_inicio) {
        params.set('data_inicio', filtros.dataInicio || filtros.data_inicio);
    }
    if (filtros.dataFim || filtros.data_fim) {
        params.set('data_fim', filtros.dataFim || filtros.data_fim);
    }

    const query = params.toString();
    return query ? `${baseUrl}?${query}` : baseUrl;
}

export async function listarCorridas(urlOuFiltros = baseApi(), talvezFiltros) {
    let urlBase;
    let filtros = {};

    if (typeof urlOuFiltros === 'string') {
        urlBase = urlOuFiltros;
        filtros = talvezFiltros || {};
    } else if (urlOuFiltros && typeof urlOuFiltros === 'object') {
        urlBase = baseApi();
        filtros = urlOuFiltros;
    } else {
        urlBase = baseApi();
    }

    const urlComFiltros = construirUrlCorridas(urlBase, filtros);
    const corridas = await requisitar(urlComFiltros);
    const urlDetalhe = urlBase.split('?')[0];

    return Promise.all(corridas.map(async (corrida) => ({
        ...corrida,
        data: corrida.iniciada_em,
        leituras: (await requisitar(`${urlDetalhe}/${corrida.id}`)).leituras,
    })));
}

export async function obterCorrida(corridaId, urlBase = baseApi()) {
    const urlDetalhe = urlBase.split('?')[0];
    const dados = await requisitar(`${urlDetalhe}/${corridaId}`);
    return {
        ...dados,
        data: dados.iniciada_em,
    };
}

export { baseApi as urlApiCorridas };
