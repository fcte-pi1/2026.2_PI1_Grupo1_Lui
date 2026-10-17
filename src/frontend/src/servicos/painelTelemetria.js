import { aplicarPacote, criarEstadoInicial } from './labirinto';

// Estado do painel alimentado pelo WebSocket de telemetria: o pacote mais recente
// (indicadores da HU-11) e o mapa acumulado da corrida atual (HU-10).

export function criarEstadoPainel() {
    return {
        pacote: null,
        mapa: criarEstadoInicial('4x4'),
        corrida: 0,
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

// Aplica um pacote recebido. Deve ser chamado para todo pacote, na ordem de
// chegada: as paredes de cada célula só vêm no pacote em que o robô está nela.
export function reduzirTelemetria(estado, pacote) {
    if (!pacote) return estado;
    const nova = iniciaNovaCorrida(estado.pacote, pacote);
    const base = nova ? criarEstadoInicial(pacote.labirinto) : estado.mapa;
    return {
        pacote,
        mapa: aplicarPacote(base, pacote),
        corrida: estado.corrida + (nova ? 1 : 0),
    };
}
