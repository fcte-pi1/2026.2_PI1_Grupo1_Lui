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

export async function listarCorridas(url = baseApi()) {
    const corridas = await requisitar(url);
    return Promise.all(corridas.map(async (corrida) => ({
        ...corrida,
        data: corrida.iniciada_em,
        leituras: (await requisitar(`${url}/${corrida.id}`)).leituras,
    })));
}

export { baseApi as urlApiCorridas };
