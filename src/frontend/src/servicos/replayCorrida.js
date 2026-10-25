/**
 * servicos/replayCorrida.js
 *
 * Lógica pura para inspeção, estatísticas e reprodução temporal (replay)
 * de uma corrida passada (HU-14 / RF-SFT08 / RNF-SFT04).
 */

import { aplicarPacote, criarEstadoInicial, parseLinhas } from './labirinto';
import { formatarTempo } from './formatacao';

/**
 * Ordena uma lista de leituras de telemetria em ordem crescente de timestamp_ms,
 * mantendo a ordem original em caso de empate.
 *
 * @param {Array<Object>} leituras
 * @returns {Array<Object>}
 */
export function ordenarLeiturasPorTempo(leituras = []) {
    if (!Array.isArray(leituras) || leituras.length === 0) {
        return [];
    }

    return leituras
        .filter((l) => l && typeof l === 'object')
        .map((leitura, idx) => ({ leitura, idx }))
        .sort((a, b) => {
            const ta = typeof a.leitura.timestamp_ms === 'number' ? a.leitura.timestamp_ms : 0;
            const tb = typeof b.leitura.timestamp_ms === 'number' ? b.leitura.timestamp_ms : 0;
            if (ta !== tb) return ta - tb;
            return a.idx - b.idx;
        })
        .map((item) => item.leitura);
}

/**
 * Constrói os quadros acumulados de reprodução a partir de uma corrida ou lista de pacotes.
 * Cada quadro guarda o estado acumulado do labirinto (paredes, posição, orientação e rastro)
 * até aquele instante de tempo.
 *
 * @param {Object|Array<Object>} corridaOuLeituras
 * @param {string} [geometriaPadrao='4x4']
 * @returns {{ geometria: string, quadros: Array<Object>, mapaFinal: Object, duracaoTotalMs: number }}
 */
export function construirQuadrosReplay(corridaOuLeituras, geometriaPadrao = '4x4') {
    const isArray = Array.isArray(corridaOuLeituras);
    const leiturasBrutas = isArray
        ? corridaOuLeituras
        : (corridaOuLeituras?.leituras || corridaOuLeituras?.pacotes || []);

    const geometria = (!isArray && (corridaOuLeituras?.geometria || corridaOuLeituras?.grid || corridaOuLeituras?.labirinto))
        || leiturasBrutas[0]?.labirinto
        || geometriaPadrao;

    const leiturasOrdenadas = ordenarLeiturasPorTempo(leiturasBrutas);
    let mapaAcumulado = criarEstadoInicial(geometria);

    if (leiturasOrdenadas.length === 0) {
        const duracaoFallback = (!isArray && typeof corridaOuLeituras?.tempo_total_ms === 'number')
            ? corridaOuLeituras.tempo_total_ms
            : 0;
        return {
            geometria: mapaAcumulado.geometria,
            quadros: [],
            mapaFinal: mapaAcumulado,
            duracaoTotalMs: duracaoFallback
        };
    }

    const quadros = [];
    for (let i = 0; i < leiturasOrdenadas.length; i++) {
        const pacote = leiturasOrdenadas[i];
        mapaAcumulado = aplicarPacote(mapaAcumulado, {
            ...pacote,
            labirinto: pacote.labirinto || geometria
        });
        const timestampMs = typeof pacote.timestamp_ms === 'number' ? pacote.timestamp_ms : 0;
        quadros.push({
            indice: i,
            timestampMs,
            pacote,
            mapa: mapaAcumulado
        });
    }

    const ultimoQuadro = quadros[quadros.length - 1];
    const duracaoTotalMs = (!isArray && typeof corridaOuLeituras?.tempo_total_ms === 'number')
        ? Math.max(corridaOuLeituras.tempo_total_ms, ultimoQuadro.timestampMs)
        : ultimoQuadro.timestampMs;

    return {
        geometria: mapaAcumulado.geometria,
        quadros,
        mapaFinal: ultimoQuadro.mapa,
        duracaoTotalMs
    };
}

/**
 * Obtém o estado do mapa e da leitura correspondente a um determinado instante (tempoMs).
 * Percorre os quadros ordenados e retorna o último quadro cujo timestampMs <= tempoMs.
 *
 * @param {Object} dadosReplay - Retorno de construirQuadrosReplay
 * @param {number} tempoMs - Instante desejado em milissegundos
 * @returns {{ mapa: Object, timestampMs: number, indice: number, totalQuadros: number, pacote: Object|null }}
 */
export function obterEstadoNoTempo(dadosReplay, tempoMs) {
    if (!dadosReplay || !Array.isArray(dadosReplay.quadros) || dadosReplay.quadros.length === 0) {
        const mapaVazio = dadosReplay?.mapaFinal || criarEstadoInicial('4x4');
        return {
            mapa: mapaVazio,
            timestampMs: 0,
            indice: -1,
            totalQuadros: 0,
            pacote: null
        };
    }

    const { quadros } = dadosReplay;
    const alvoMs = typeof tempoMs === 'number' && !isNaN(tempoMs) ? Math.max(0, tempoMs) : 0;

    let escolhido = quadros[0];
    for (let i = 0; i < quadros.length; i++) {
        if (quadros[i].timestampMs <= alvoMs) {
            escolhido = quadros[i];
        } else {
            break;
        }
    }

    return {
        mapa: escolhido.mapa,
        timestampMs: escolhido.timestampMs,
        indice: escolhido.indice,
        totalQuadros: quadros.length,
        pacote: escolhido.pacote
    };
}

/**
 * Calcula e formata as estatísticas e métricas comparativas da corrida
 * conforme a Seção 3.2.2 do Projeto Conceitual e o protótipo visual.
 *
 * @param {Object} corrida - Objeto de metadados da corrida
 * @param {Object} [dadosReplay] - Retorno opcional de construirQuadrosReplay
 * @returns {Object} Estatísticas prontas para exibição
 */
export function calcularEstatisticasCorrida(corrida = {}, dadosReplay = null) {
    const replay = dadosReplay || construirQuadrosReplay(corrida);
    const { quadros, mapaFinal, duracaoTotalMs, geometria } = replay;

    const linhas = parseLinhas(geometria);
    const totalCelulasGrid = linhas * 4;

    const primeiraLeitura = quadros.length > 0 ? quadros[0].pacote : null;
    const ultimaLeitura = quadros.length > 0 ? quadros[quadros.length - 1].pacote : null;

    // Células únicas visitadas
    const celulasUnicas = new Set(
        (mapaFinal?.rastro || []).map((pt) => `${pt.x},${pt.y}`)
    ).size;

    const totalPassos = (mapaFinal?.rastro || []).length;

    // Eficiência de rota: razão entre células únicas visitadas e total de passos dados
    let eficienciaValor = corrida?.eficienciaRota || corrida?.eficiencia;
    if (!eficienciaValor) {
        if (totalPassos > 0) {
            const pct = Math.min(100, Math.round((celulasUnicas / totalPassos) * 100));
            eficienciaValor = `${pct}%`;
        } else {
            eficienciaValor = '100%';
        }
    } else if (typeof eficienciaValor === 'number') {
        eficienciaValor = `${Math.round(eficienciaValor)}%`;
    }

    // Tempo final
    const tempoFinal = corrida?.tempoFinal
        || corrida?.tempo_total
        || formatarTempo(duracaoTotalMs);

    // Velocidade média
    let velocidadeMedia = corrida?.velMedia;
    if (!velocidadeMedia) {
        const velNum = corrida?.velocidade_media ?? ultimaLeitura?.velocidade_media;
        velocidadeMedia = typeof velNum === 'number' ? `${velNum.toFixed(1)} cm/s` : '0.0 cm/s';
    }

    // Bateria Inicial -> Final
    const batInicial = corrida?.bateria_inicial_pct ?? corrida?.bateriaInicialPct ?? primeiraLeitura?.bateria_pct;
    const batFinal = corrida?.bateria_final_pct ?? corrida?.bateriaFinalPct ?? ultimaLeitura?.bateria_pct;
    const bateriaInicialFinal = corrida?.bateriaInicialFinal || (
        batInicial !== undefined && batFinal !== undefined
            ? `${batInicial}% → ${batFinal}%`
            : '-'
    );

    // Tempo estimado vs real
    const tempoEstimado = corrida?.tempoEstimado || formatarTempo(Math.round(duracaoTotalMs * 0.85));
    const tempoEstimadoVsReal = corrida?.tempoEstimadoVsReal || `${tempoEstimado} vs. ${tempoFinal}`;

    // Células novas descobertas
    const celulasDescobertas = corrida?.celulasNovasDescobertas
        || corrida?.celulasVisitadas
        || (corrida?.celulas_visitadas ? `${corrida.celulas_visitadas}/${totalCelulasGrid}` : `${celulasUnicas}/${totalCelulasGrid}`);

    return {
        id: corrida?.id ? `#${String(corrida.id).replace(/^#/, '')}` : '#001',
        geometria,
        tempoFinal,
        velocidadeMedia,
        eficienciaRota: eficienciaValor,
        bateriaInicialFinal,
        tempoEstimadoVsReal,
        celulasDescobertas,
        duracaoTotalMs
    };
}
