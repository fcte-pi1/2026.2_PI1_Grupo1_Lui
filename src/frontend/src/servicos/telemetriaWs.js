import { useEffect, useState } from 'react';

// Cliente do WebSocket de telemetria do backend (ws://<host>:<porta>/ws/telemetria).
// O formato de cada pacote é o do contrato de dados, documentado em src/README.md.
// A conexão não depende do React, para poder ser testada sem DOM; o hook
// useTelemetria só liga essa conexão ao estado de um componente.

export const PORTA_BACKEND_PADRAO = 8000;
export const CAMINHO_TELEMETRIA = '/ws/telemetria';
export const INTERVALO_RECONEXAO_MS = 2000;

// Endereço do WebSocket: a variável VITE_TELEMETRIA_WS_URL, se definida; senão,
// o mesmo host da página na porta padrão do backend (wss quando a página é https).
export function urlTelemetria(env = import.meta.env, local = globalThis.location) {
    const configurada = env?.VITE_TELEMETRIA_WS_URL;
    if (configurada) return configurada;
    const protocolo = local?.protocol === 'https:' ? 'wss' : 'ws';
    const host = local?.hostname || 'localhost';
    return `${protocolo}://${host}:${PORTA_BACKEND_PADRAO}${CAMINHO_TELEMETRIA}`;
}

// Abre a conexão e reconecta sozinha quando ela cai, até fechar() ser chamado.
// aoReceber(pacote) recebe cada mensagem JSON; aoMudarConexao(bool) informa se
// a conexão está aberta.
export function criarConexaoTelemetria({
    url,
    aoReceber,
    aoMudarConexao,
    intervaloReconexaoMs = INTERVALO_RECONEXAO_MS,
    WebSocketImpl = globalThis.WebSocket,
}) {
    let socket = null;
    let temporizador = null;
    let encerrada = false;

    function agendarReconexao() {
        if (!encerrada) temporizador = setTimeout(conectar, intervaloReconexaoMs);
    }

    function conectar() {
        temporizador = null;
        try {
            socket = new WebSocketImpl(url);
        } catch {
            aoMudarConexao(false);
            agendarReconexao();
            return;
        }
        socket.onopen = () => aoMudarConexao(true);
        socket.onmessage = (evento) => {
            let pacote;
            try {
                pacote = JSON.parse(evento.data);
            } catch {
                return; // mensagem que não é JSON: ignorada
            }
            if (pacote !== null && typeof pacote === 'object') aoReceber(pacote);
        };
        // Em caso de erro o navegador também dispara 'close', que agenda a reconexão.
        socket.onclose = () => {
            aoMudarConexao(false);
            agendarReconexao();
        };
    }

    function fechar() {
        encerrada = true;
        if (temporizador !== null) clearTimeout(temporizador);
        temporizador = null;
        if (socket) {
            socket.onopen = null;
            socket.onmessage = null;
            socket.onclose = null;
            socket.close();
        }
    }

    conectar();
    return { fechar };
}

// Hook para os componentes do painel: devolve o pacote mais recente (null até o
// primeiro chegar) e se a conexão com o backend está aberta. A conexão é fechada
// quando o componente é desmontado.
export function useTelemetria({ url, intervaloReconexaoMs = INTERVALO_RECONEXAO_MS } = {}) {
    const endereco = url ?? urlTelemetria();
    const [pacote, setPacote] = useState(null);
    const [conectado, setConectado] = useState(false);

    useEffect(() => {
        const conexao = criarConexaoTelemetria({
            url: endereco,
            aoReceber: setPacote,
            aoMudarConexao: setConectado,
            intervaloReconexaoMs,
        });
        return () => conexao.fechar();
    }, [endereco, intervaloReconexaoMs]);

    return { pacote, conectado };
}
