import { aplicarPacote, criarEstadoInicial } from './labirinto';
import { formatarTempo } from './formatacao';

const STATUS_FINAIS = new Set(['CONCLUIDO', 'TRAVADO', 'COLISAO', 'TIMEOUT']);

// Estado do painel alimentado pelo WebSocket de telemetria: o pacote mais recente
// (indicadores da HU-11), o mapa acumulado da corrida atual (HU-10) e o histórico
// de corridas finalizadas na sessão para inspeção e reprodução (HU-13 / HU-14).

export function criarEstadoPainel() {
    return {
        pacote: null,
        mapa: criarEstadoInicial('4x4'),
        corrida: 0,
        leiturasAtuais: [],
        inicioCorridaIso: null,
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

function montarRegistroCorrida(numeroCorrida, leituras, mapa, dataIso) {
    const ultimo = leituras[leituras.length - 1] || {};
    const id = String(numeroCorrida).padStart(3, '0');
    const teveSpeedRun = leituras.some((l) => l.status === 'SPEED_RUN');
    const rastro = mapa?.rastro || [];
    const celulasUnicas = new Set(rastro.map((p) => `${p.x},${p.y}`)).size;
    const passosMovimento = leituras.filter((l) => l.status !== 'AGUARDANDO').length;
    const pctEficiencia = passosMovimento > 0
        ? Math.min(100, Math.round((celulasUnicas / passosMovimento) * 100))
        : 100;

    return {
        id,
        nome: `Tentativa ${numeroCorrida}`,
        data: dataIso || new Date().toISOString(),
        geometria: ultimo.labirinto || '4x4',
        modo: teveSpeedRun ? 'Speed Run' : 'Exploração',
        tentativas: `${numeroCorrida}`,
        tempoFinal: formatarTempo(ultimo.timestamp_ms || 0),
        velMedia: `${ultimo.velocidade_media ?? 0} cm/s`,
        eficienciaRota: `${pctEficiencia}%`,
        status: ultimo.status || 'CONCLUIDO',
        leituras,
    };
}

function atualizarHistorico(historico = [], registro) {
    const idx = historico.findIndex((c) => c.id === registro.id);
    if (idx >= 0) {
        const copia = [...historico];
        copia[idx] = registro;
        return copia;
    }
    return [registro, ...historico];
}

// Aplica um pacote recebido. Deve ser chamado para todo pacote, na ordem de
// chegada: as paredes de cada célula só vêm no pacote em que o robô está nela.
export function reduzirTelemetria(estado, pacote) {
    if (!pacote) return estado;
    const nova = iniciaNovaCorrida(estado.pacote, pacote);

    let historico = estado.historico || [];

    // Se uma nova corrida começou e a anterior saiu de AGUARDANDO sem ter sido salva
    // (ex: cenário perda_conexao), salva a corrida interrompida no histórico.
    if (nova && estado.corrida > 0 && Array.isArray(estado.leiturasAtuais)) {
        const saiuDaLargada = estado.leiturasAtuais.some((l) => l.status !== 'AGUARDANDO');
        const jaSalva = historico.some((c) => c.id === String(estado.corrida).padStart(3, '0'));
        if (saiuDaLargada && !jaSalva) {
            const regAnterior = montarRegistroCorrida(
                estado.corrida,
                estado.leiturasAtuais,
                estado.mapa,
                estado.inicioCorridaIso
            );
            historico = atualizarHistorico(historico, regAnterior);
        }
    }

    const numeroCorrida = estado.corrida + (nova ? 1 : 0);
    const base = nova ? criarEstadoInicial(pacote.labirinto) : estado.mapa;
    const novoMapa = aplicarPacote(base, pacote);
    const novasLeituras = nova ? [pacote] : [...(estado.leiturasAtuais || []), pacote];
    const inicioCorridaIso = nova
        ? new Date().toISOString()
        : (estado.inicioCorridaIso || new Date().toISOString());

    // Quando a corrida atinge um status final (CONCLUIDO, TRAVADO, COLISAO, TIMEOUT),
    // registra ou atualiza imediatamente no histórico para inspeção e reprodução.
    if (STATUS_FINAIS.has(pacote.status)) {
        const regAtual = montarRegistroCorrida(
            numeroCorrida,
            novasLeituras,
            novoMapa,
            inicioCorridaIso
        );
        historico = atualizarHistorico(historico, regAtual);
    }

    return {
        pacote,
        mapa: novoMapa,
        corrida: numeroCorrida,
        leiturasAtuais: novasLeituras,
        inicioCorridaIso,
        historico,
    };
}
