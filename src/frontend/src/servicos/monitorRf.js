// Aviso de "Conexão RF perdida" (HU-09): o backend está no ar, mas os pacotes do
// robô pararam de chegar. Todo pacote do WebSocket já foi validado pelo backend,
// então 3 s sem mensagem no WebSocket equivalem a 3 s sem pacote válido do robô.
//
// O monitor só avalia com o WebSocket aberto: com o painel DESCONECTADO do backend
// não há como saber do rádio, e vale apenas aquele indicador. A contagem começa na
// conexão, para avisar também quando o robô nem chegou a transmitir.
// Não depende do React, para poder ser testado com timers falsos.

export const LIMITE_SEM_PACOTE_MS = 3000;
export const INTERVALO_VERIFICACAO_MS = 250;
export const RF_NORMAL = Object.freeze({ perdida: false, segundosSemPacote: 0 });

// aoMudar(estado) é chamado só quando o estado muda: { perdida, segundosSemPacote }.
export function criarMonitorRf({
    aoMudar,
    limiteMs = LIMITE_SEM_PACOTE_MS,
    intervaloMs = INTERVALO_VERIFICACAO_MS,
    agora = () => Date.now(),
}) {
    let referencia = null; // instante do último pacote, ou da conexão; null fechado
    let temporizador = null;
    let atual = RF_NORMAL;

    function publicar(estado) {
        if (estado.perdida === atual.perdida && estado.segundosSemPacote === atual.segundosSemPacote) return;
        atual = estado;
        aoMudar(estado);
    }

    function verificar() {
        if (referencia === null) return publicar(RF_NORMAL);
        const semPacoteMs = agora() - referencia;
        publicar(semPacoteMs >= limiteMs
            ? { perdida: true, segundosSemPacote: Math.floor(semPacoteMs / 1000) }
            : RF_NORMAL);
    }

    function pararTemporizador() {
        if (temporizador !== null) clearInterval(temporizador);
        temporizador = null;
    }

    return {
        // A conexão com o backend abriu (true) ou caiu (false).
        conexao(aberta) {
            pararTemporizador();
            referencia = aberta ? agora() : null;
            if (aberta) temporizador = setInterval(verificar, intervaloMs);
            verificar();
        },
        // Chegou um pacote do robô.
        pacote() {
            if (referencia === null) return;
            referencia = agora();
            verificar();
        },
        fechar() {
            pararTemporizador();
            referencia = null;
        },
    };
}
