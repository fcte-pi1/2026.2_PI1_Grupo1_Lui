import { criarEstadoInicial, aplicarPacote, aplicarParedes, parseLinhas } from './labirinto';
import { formatarTempo } from './formatacao';

/**
 * Ordena uma lista de leituras de telemetria por ordem crescente de timestamp_ms
 * (e seq como critério de desempate). Garante imutabilidade.
 */
export function ordenarLeiturasPorTempo(leituras = []) {
    if (!Array.isArray(leituras) || leituras.length === 0) {
        return [];
    }

    return [...leituras]
        .filter(Boolean)
        .sort((a, b) => {
            const ta = typeof a.timestamp_ms === 'number' ? a.timestamp_ms : 0;
            const tb = typeof b.timestamp_ms === 'number' ? b.timestamp_ms : 0;
            if (ta !== tb) return ta - tb;
            const sa = typeof a.seq === 'number' ? a.seq : 0;
            const sb = typeof b.seq === 'number' ? b.seq : 0;
            return sa - sb;
        });
}

/**
 * Extrai a lista de leituras/pacotes de uma corrida salva.
 */
function obterLeiturasDaCorrida(corrida) {
    if (!corrida) return [];
    if (Array.isArray(corrida.leituras)) {
        return corrida.leituras;
    }
    if (Array.isArray(corrida.pacotes)) {
        return corrida.pacotes;
    }
    return [];
}

/**
 * Constrói os quadros (frames) de reprodução em ordem temporal e o mapa final consolidado.
 * Atende aos critérios da HU-14:
 * - Mapa final com todas as paredes descobertas na corrida
 * - Quadros passo a passo em ordem de timestamp_ms
 */
export function construirReproducaoCorrida(corrida) {
    if (!corrida) {
        const estadoVazio = criarEstadoInicial('4x4');
        return {
            geometria: '4x4',
            linhas: 4,
            mapaFinal: estadoVazio,
            quadros: [],
            duracaoTotalMs: 0
        };
    }

    const geometria = corrida.geometria || corrida.grid || corrida.labirinto || '4x4';
    const linhas = parseLinhas(geometria);
    const leiturasOrdenadas = ordenarLeiturasPorTempo(obterLeiturasDaCorrida(corrida));

    let estadoAcumulado = criarEstadoInicial(geometria);
    if (Array.isArray(corrida.paredes) && corrida.paredes.length > 0) {
        estadoAcumulado = {
            ...estadoAcumulado,
            cells: aplicarParedes(estadoAcumulado.cells, corrida.paredes)
        };
    }

    const quadros = [];

    for (const leitura of leiturasOrdenadas) {
        estadoAcumulado = aplicarPacote(estadoAcumulado, {
            ...leitura,
            labirinto: leitura.labirinto || geometria
        });

        quadros.push({
            timestamp_ms: typeof leitura.timestamp_ms === 'number' ? leitura.timestamp_ms : 0,
            roboPosicao: estadoAcumulado.roboPosicao,
            roboOrientacao: estadoAcumulado.roboOrientacao,
            rastro: estadoAcumulado.rastro,
            status: estadoAcumulado.ultimoStatus,
            bateria_pct: leitura.bateria_pct,
            velocidade_media: leitura.velocidade_media,
            cellsParciais: estadoAcumulado.cells
        });
    }

    const ultimoQuadro = quadros[quadros.length - 1];
    const duracaoTotalMs = ultimoQuadro ? ultimoQuadro.timestamp_ms : 0;

    return {
        geometria: `${linhas}x4`,
        linhas,
        mapaFinal: estadoAcumulado,
        quadros,
        duracaoTotalMs
    };
}

/**
 * Retorna os dados prontos para renderização no passo selecionado da barra de tempo.
 * Exibe o mapa final da corrida e posiciona o robô e o rastro até o instante escolhido.
 */
export function obterEstadoReproducaoNoPasso(reproducao, indicePasso) {
    if (!reproducao || !Array.isArray(reproducao.quadros) || reproducao.quadros.length === 0) {
        const fallback = reproducao?.mapaFinal || criarEstadoInicial('4x4');
        return {
            linhas: fallback.linhas || 4,
            cells: fallback.cells || [],
            roboPosicao: null,
            roboOrientacao: 'NORTE',
            trajeto: [],
            indiceAtual: 0,
            totalPassos: 0,
            timestampMs: 0,
            tempoAtualFormatado: '00:00',
            tempoTotalFormatado: '00:00'
        };
    }

    const totalPassos = reproducao.quadros.length;
    const indiceSeguro = Math.max(0, Math.min(
        typeof indicePasso === 'number' ? indicePasso : totalPassos - 1,
        totalPassos - 1
    ));

    const quadro = reproducao.quadros[indiceSeguro];

    return {
        linhas: reproducao.linhas,
        cells: reproducao.mapaFinal.cells,
        roboPosicao: quadro.roboPosicao,
        roboOrientacao: quadro.roboOrientacao,
        trajeto: quadro.rastro,
        indiceAtual: indiceSeguro,
        totalPassos,
        timestampMs: quadro.timestamp_ms,
        tempoAtualFormatado: formatarTempo(quadro.timestamp_ms),
        tempoTotalFormatado: formatarTempo(reproducao.duracaoTotalMs)
    };
}

/**
 * Formata a contagem de células visitadas (ex: "10/16") a partir do campo explícito
 * ou calculando as posições únicas presentes nas leituras da corrida.
 */
export function formatarCelulasVisitadas(corrida, reproducao = null) {
    if (!corrida) return '-';
    if (corrida.celulasVisitadas) return String(corrida.celulasVisitadas);
    if (corrida.celulas_visitadas) return String(corrida.celulas_visitadas);

    const geometria = corrida.geometria || corrida.grid || corrida.labirinto || '4x4';
    const linhas = reproducao?.linhas || parseLinhas(geometria);
    const totalCelulasGrid = linhas * 4;

    const ultimoRastro = reproducao?.quadros?.[reproducao.quadros.length - 1]?.rastro;
    if (Array.isArray(ultimoRastro) && ultimoRastro.length > 0) {
        const unicas = new Set(ultimoRastro.map(p => `${p.x},${p.y}`)).size;
        return `${unicas}/${totalCelulasGrid}`;
    }

    const leituras = obterLeiturasDaCorrida(corrida);
    if (leituras.length > 0) {
        const posicoesUnicas = new Set();
        for (const l of leituras) {
            if (l && l.posicao) {
                const col = l.posicao.coluna ?? l.posicao.x;
                const lin = l.posicao.linha ?? l.posicao.y;
                if (col !== undefined && lin !== undefined) {
                    posicoesUnicas.add(`${col}-${lin}`);
                }
            }
        }
        if (posicoesUnicas.size > 0) {
            return `${posicoesUnicas.size}/${totalCelulasGrid}`;
        }
    }

    return '-';
}

/**
 * Extrai as estatísticas e métricas comparativas da corrida selecionada.
 */
export function extrairEstatisticasCorrida(corrida, reproducao = null) {
    if (!corrida) {
        return {
            id: '-',
            tempoFinal: '-',
            velMedia: '-',
            eficienciaRota: '-',
            bateriaInicialFinal: '-',
            celulasNovasDescobertas: '-'
        };
    }

    const rep = reproducao || construirReproducaoCorrida(corrida);
    const quadros = rep.quadros || [];
    const primeiroQuadro = quadros[0];
    const ultimoQuadro = quadros[quadros.length - 1];

    const tempoFinal = corrida.tempoFinal
        || corrida.tempo_total
        || (rep.duracaoTotalMs > 0 ? formatarTempo(rep.duracaoTotalMs) : '-');

    const velMedia = corrida.velMedia
        || (corrida.velocidade_media ? `${corrida.velocidade_media} cm/s` : null)
        || (ultimoQuadro?.velocidade_media ? `${ultimoQuadro.velocidade_media} cm/s` : '-');

    const eficienciaRota = corrida.eficienciaRota
        || corrida.eficiencia
        || '-';

    const batIni = corrida.bateriaInicial ?? primeiroQuadro?.bateria_pct;
    const batFim = corrida.bateriaFinal ?? ultimoQuadro?.bateria_pct;
    const bateriaInicialFinal = corrida.bateriaInicialFinal
        || (batIni !== undefined && batFim !== undefined ? `${batIni}% → ${batFim}%` : '-');

    const celulasNovasDescobertas = corrida.celulasNovasDescobertas
        || formatarCelulasVisitadas(corrida, rep);

    return {
        id: corrida.id || '001',
        tempoFinal,
        velMedia,
        eficienciaRota,
        bateriaInicialFinal,
        celulasNovasDescobertas
    };
}

