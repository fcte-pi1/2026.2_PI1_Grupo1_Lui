import { aplicarPacote, criarEstadoInicial } from './labirinto';
import { rotuloStatus, rotuloModo, compararTemposSpeedRun } from './formatacao';

// Estado do painel alimentado pelo WebSocket de telemetria: o pacote mais recente
// (indicadores da HU-11 e HU-19), o mapa acumulado da corrida atual (HU-10) e o
// histórico das corridas encerradas na sessão.

export const STATUS_TERMINAIS = new Set(['CONCLUIDO', 'TRAVADO', 'COLISAO', 'TIMEOUT', 'INTERROMPIDA']);

export function ehStatusTerminal(status) {
    return STATUS_TERMINAIS.has(status);
}

export function criarEstadoPainel() {
    return {
        pacote: null,
        mapa: criarEstadoInicial('4x4'),
        corrida: 0,
        modo: 'EXPLORACAO',
        historico: [],
    };
}

// Decide se o pacote começa uma corrida nova, para o mapa não desenhar uma corrida
// por cima da outra. Começa quando:
//  - a geometria (labirinto) muda;
//  - chega AGUARDANDO depois de qualquer outro status: o robô voltou à largada
//    depois de concluir, travar ou ter a transmissão cortada;
//  - o tempo de prova volta para trás, caso os pacotes AGUARDANDO tenham se perdido.
// AGUARDANDO -> MAPEANDO não reinicia, para manter o que foi visto na largada, e
// um seq que volta a 1 (gateway religado) também não, pois a corrida é a mesma.
export function iniciaNovaCorrida(anterior, pacote) {
    if (!anterior) return true;
    if (pacote.labirinto !== anterior.labirinto) return true;
    if (pacote.status === 'AGUARDANDO' && anterior.status !== 'AGUARDANDO') return true;
    return pacote.timestamp_ms < anterior.timestamp_ms;
}

function encontrarExploracaoAnterior(historico, idCorrida, labirinto) {
    for (let i = historico.length - 1; i >= 0; i--) {
        const item = historico[i];
        if (
            item.id < idCorrida &&
            item.labirinto === labirinto &&
            item.modo === 'EXPLORACAO' &&
            item.status === 'CONCLUIDO' &&
            typeof item.tempo_total_ms === 'number' &&
            item.tempo_total_ms > 0
        ) {
            return item;
        }
    }
    return null;
}

// Registra ou atualiza a corrida encerrada no histórico da sessão (HU-19 e HU-16).
function atualizarHistoricoSessao(historico = [], idCorrida, pacote, modoCorrida = 'EXPLORACAO') {
    if (!ehStatusTerminal(pacote.status)) {
        return historico;
    }

    const modo = modoCorrida === 'SPEED_RUN' ? 'SPEED_RUN' : 'EXPLORACAO';
    const exploracaoAnterior =
        modo === 'SPEED_RUN' ? encontrarExploracaoAnterior(historico, idCorrida, pacote.labirinto) : null;
    const comparacao = exploracaoAnterior
        ? compararTemposSpeedRun(exploracaoAnterior.tempo_total_ms, pacote.timestamp_ms)
        : null;

    const registro = {
        id: idCorrida,
        tentativa: idCorrida,
        nome: `Tentativa ${idCorrida}`,
        labirinto: pacote.labirinto,
        geometria: pacote.labirinto,
        modo,
        rotuloModo: rotuloModo(modo),
        status: pacote.status,
        resultado: rotuloStatus(pacote.status),
        timestamp_ms: pacote.timestamp_ms,
        tempo_total_ms: pacote.timestamp_ms,
        ...(comparacao
            ? {
                  tempo_exploracao_ms: comparacao.tempoExploracaoMs,
                  reducao_pct: comparacao.reducaoPct,
                  comparacao,
              }
            : {}),
    };

    const idxExistente = historico.findIndex((item) => item.id === idCorrida);
    if (idxExistente >= 0) {
        const copia = [...historico];
        copia[idxExistente] = registro;
        return copia;
    }
    return [...historico, registro];
}

// Aplica um pacote recebido. Deve ser chamado para todo pacote, na ordem de
// chegada: as paredes de cada célula só vêm no pacote em que o robô está nela.
// Quando a corrida entra em estado terminal (ex.: TRAVADO na HU-19), o status
// permanece destacado até o início da próxima corrida.
export function reduzirTelemetria(estado, pacote) {
    if (!pacote) return estado;
    const nova = iniciaNovaCorrida(estado.pacote, pacote);
    const base = nova ? criarEstadoInicial(pacote.labirinto) : estado.mapa;
    const corrida = estado.corrida + (nova ? 1 : 0);
    const modoAnterior = nova ? 'EXPLORACAO' : (estado.modo || 'EXPLORACAO');
    const modo = pacote.status === 'SPEED_RUN' ? 'SPEED_RUN' : modoAnterior;

    const mantemStatusTerminal =
        !nova &&
        ehStatusTerminal(estado.pacote?.status) &&
        !ehStatusTerminal(pacote.status);

    const pacoteEfetivo = mantemStatusTerminal
        ? { ...pacote, status: estado.pacote.status }
        : pacote;

    return {
        pacote: pacoteEfetivo,
        mapa: aplicarPacote(base, pacoteEfetivo),
        corrida,
        modo,
        historico: atualizarHistoricoSessao(estado.historico, corrida, pacoteEfetivo, modo),
    };
}
