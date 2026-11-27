export const LIMIAR_BATERIA_BAIXA = 20;
export const LIMITE_PROVA_MS = 600000;

export function formatarTempo(ms) {
    if (typeof ms !== 'number' || ms < 0) return "00:00";
    const totalSeconds = Math.floor(ms / 1000);
    const minutes = Math.floor(totalSeconds / 60);
    const seconds = totalSeconds % 60;
    
    return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
}

export function rotuloStatus(status) {
    const mapaStatus = {
        "AGUARDANDO": "Em espera",
        "MAPEANDO": "Explorando",
        "SPEED_RUN": "Speed Run",
        "CONCLUIDO": "Concluído",
        "COLISAO": "Colisão",
        "TIMEOUT": "Tempo esgotado",
        "TRAVADO": "Travado",
        "INTERROMPIDA": "Interrompida"
    };
    return mapaStatus[status] || "Desconhecido";
}

export function bateriaBaixa(porcentagem) {
    return porcentagem <= LIMIAR_BATERIA_BAIXA;
}

export function estiloStatus(status) {
    if (status === "TRAVADO" || status === "COLISAO" || status === "TIMEOUT" || status === "INTERROMPIDA") {
        return "alerta";
    }
    return "normal";
}

const STATUS_CONCLUIDO = new Set(['CONCLUIDO', 'SUCESSO', 'CONCLUÍDO']);

export function ehCorridaConcluida(status) {
    return STATUS_CONCLUIDO.has(String(status || '').toUpperCase());
}

export function rotuloModo(modo) {
    return modo === "SPEED_RUN" ? "Speed Run" : "Exploração";
}

export function compararTemposSpeedRun(tempoExploracaoMs, tempoSpeedRunMs) {
    if (
        typeof tempoExploracaoMs !== 'number' ||
        typeof tempoSpeedRunMs !== 'number' ||
        tempoExploracaoMs <= 0 ||
        tempoSpeedRunMs < 0
    ) {
        return null;
    }
    const diferencaMs = tempoExploracaoMs - tempoSpeedRunMs;
    const reducaoPct = Math.round((diferencaMs / tempoExploracaoMs) * 100);
    return {
        tempoExploracaoMs,
        tempoSpeedRunMs,
        diferencaMs,
        reducaoPct,
        tempoExploracaoFormatado: formatarTempo(tempoExploracaoMs),
        tempoSpeedRunFormatado: formatarTempo(tempoSpeedRunMs),
        metaAtingida: reducaoPct >= 20,
    };
}

export function formatarComparacao(comp) {
    if (!comp || typeof comp.reducaoPct !== 'number' || Number.isNaN(comp.reducaoPct)) {
        return '';
    }
    if (comp.reducaoPct > 0) {
        return `-${comp.reducaoPct}%`;
    }
    if (comp.reducaoPct < 0) {
        return `+${Math.abs(comp.reducaoPct)}%`;
    }
    return '0%';
}

export function compararComExploracaoAnterior(corridas) {
    const mapa = new Map();
    if (!Array.isArray(corridas) || corridas.length === 0) return mapa;

    const ordenadas = corridas
        .map((corrida, idx) => ({ corrida, idx }))
        .filter(({ corrida }) => Boolean(corrida))
        .sort((a, b) => {
            const idA = Number(a.corrida.id);
            const idB = Number(b.corrida.id);
            if (!Number.isNaN(idA) && !Number.isNaN(idB)) {
                return idA - idB || a.idx - b.idx;
            }
            return a.idx - b.idx;
        });

    const ultimaExploracaoPorGeometria = new Map();

    for (const { corrida: c, idx } of ordenadas) {
        const chave = c.id ?? idx;
        const geo = c.labirinto || c.geometria || c.grid || '4x4';
        const modo = rotuloModo(c.modo);
        const tempoMs = typeof c.tempo_total_ms === 'number' ? c.tempo_total_ms : c.timestamp_ms;

        if (!ehCorridaConcluida(c.status || c.resultado) || typeof tempoMs !== 'number' || tempoMs <= 0) {
            continue;
        }

        if (modo === 'Exploração') {
            ultimaExploracaoPorGeometria.set(geo, tempoMs);
        } else if (modo === 'Speed Run') {
            const tempoExp = ultimaExploracaoPorGeometria.get(geo);
            if (typeof tempoExp === 'number' && tempoExp > 0) {
                const comp = compararTemposSpeedRun(tempoExp, tempoMs);
                if (comp) {
                    mapa.set(chave, comp);
                }
            }
        }
    }

    return mapa;
}
