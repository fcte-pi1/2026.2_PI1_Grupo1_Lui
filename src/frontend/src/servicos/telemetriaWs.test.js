import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

// O projeto não tem DOM nos testes; para exercitar o hook basta uma execução
// mínima de useState e useEffect, guardada aqui e usada pelo mock do React.
const react = vi.hoisted(() => ({ valores: [], indice: 0, efeitos: [] }));

vi.mock('react', () => ({
    useState(inicial) {
        const i = react.indice++;
        if (!(i in react.valores)) react.valores[i] = inicial;
        return [react.valores[i], (novo) => { react.valores[i] = novo; }];
    },
    useEffect(efeito, dependencias) {
        react.efeitos.push({ efeito, dependencias });
    },
}));

// Importado com outro nome porque aqui o hook é chamado fora de um componente, de
// propósito, sobre o React simulado acima.
import {
    INTERVALO_RECONEXAO_MS,
    criarConexaoTelemetria,
    urlTelemetria,
    useTelemetria as executarHookTelemetria,
} from './telemetriaWs';

class WebSocketFalso {
    static instancias = [];

    constructor(url) {
        this.url = url;
        this.fechado = false;
        WebSocketFalso.instancias.push(this);
    }

    close() {
        this.fechado = true;
        this.onclose?.({ code: 1000 });
    }

    abrir() {
        this.onopen?.({});
    }

    receber(dados) {
        this.onmessage?.({ data: typeof dados === 'string' ? dados : JSON.stringify(dados) });
    }

    cair() {
        this.onclose?.({ code: 1006 });
    }
}

const PACOTE = {
    seq: 7,
    rssi: -58,
    timestamp_ms: 142000,
    labirinto: '8x4',
    status: 'MAPEANDO',
    desafio_cumprido: false,
    bateria_pct: 78,
    bateria_volts: 7.42,
    velocidade_atual: 35,
    velocidade_media: 28.5,
    posicao: { coluna: 'B', linha: 4 },
    orientacao: 'LESTE',
    paredes: { norte: false, sul: true, leste: true, oeste: false },
};

const URL = 'ws://backend:8000/ws/telemetria';

function ultimoSocket() {
    return WebSocketFalso.instancias[WebSocketFalso.instancias.length - 1];
}

beforeEach(() => {
    vi.useFakeTimers();
    WebSocketFalso.instancias = [];
});

afterEach(() => {
    vi.useRealTimers();
    vi.unstubAllEnvs();
    vi.unstubAllGlobals();
});

describe('urlTelemetria', () => {
    it('usa VITE_TELEMETRIA_WS_URL quando definida', () => {
        const env = { VITE_TELEMETRIA_WS_URL: 'ws://192.168.0.10:9000/ws/telemetria' };
        expect(urlTelemetria(env, { protocol: 'http:', hostname: 'outro' })).toBe(env.VITE_TELEMETRIA_WS_URL);
    });

    it('sem a variável, usa o host da página na porta 8000', () => {
        expect(urlTelemetria({}, { protocol: 'http:', hostname: '192.168.0.42' }))
            .toBe('ws://192.168.0.42:8000/ws/telemetria');
    });

    it('usa wss quando a página é https', () => {
        expect(urlTelemetria({}, { protocol: 'https:', hostname: 'painel.local' }))
            .toBe('wss://painel.local:8000/ws/telemetria');
    });

    it('fora do navegador usa localhost', () => {
        expect(urlTelemetria({}, undefined)).toBe('ws://localhost:8000/ws/telemetria');
    });
});

describe('criarConexaoTelemetria', () => {
    function conectar(opcoes = {}) {
        const recebidos = [];
        const estados = [];
        const conexao = criarConexaoTelemetria({
            url: URL,
            aoReceber: (pacote) => recebidos.push(pacote),
            aoMudarConexao: (conectado) => estados.push(conectado),
            WebSocketImpl: WebSocketFalso,
            ...opcoes,
        });
        return { conexao, recebidos, estados };
    }

    it('conecta na URL e informa quando a conexão abre', () => {
        const { estados } = conectar();

        expect(WebSocketFalso.instancias).toHaveLength(1);
        expect(ultimoSocket().url).toBe(URL);
        expect(estados).toEqual([]);

        ultimoSocket().abrir();
        expect(estados).toEqual([true]);
    });

    it('entrega cada pacote JSON e ignora mensagens que não são objetos JSON', () => {
        const { recebidos } = conectar();
        ultimoSocket().abrir();

        ultimoSocket().receber(PACOTE);
        ultimoSocket().receber('isso não é JSON');
        ultimoSocket().receber('42');
        ultimoSocket().receber('null');
        ultimoSocket().receber({ ...PACOTE, seq: 8 });

        expect(recebidos.map((p) => p.seq)).toEqual([7, 8]);
        expect(recebidos[0]).toEqual(PACOTE);
    });

    it('reconecta depois do intervalo quando a conexão cai', () => {
        const { estados } = conectar();
        ultimoSocket().abrir();

        ultimoSocket().cair();
        expect(estados).toEqual([true, false]);

        vi.advanceTimersByTime(INTERVALO_RECONEXAO_MS - 1);
        expect(WebSocketFalso.instancias).toHaveLength(1);
        vi.advanceTimersByTime(1);
        expect(WebSocketFalso.instancias).toHaveLength(2);

        ultimoSocket().abrir();
        expect(estados).toEqual([true, false, true]);
    });

    it('continua tentando enquanto o backend estiver fora do ar', () => {
        conectar({ intervaloReconexaoMs: 500 });

        for (let tentativa = 1; tentativa <= 3; tentativa++) {
            ultimoSocket().cair(); // falha ao conectar: o navegador dispara 'close'
            vi.advanceTimersByTime(500);
        }

        expect(WebSocketFalso.instancias).toHaveLength(4);
    });

    it('tenta de novo se o WebSocket não puder ser criado', () => {
        let falhas = 1;
        class WebSocketQueFalha extends WebSocketFalso {
            constructor(url) {
                if (falhas-- > 0) throw new SyntaxError('endereço inválido');
                super(url);
            }
        }
        const { estados } = conectar({ WebSocketImpl: WebSocketQueFalha });

        expect(estados).toEqual([false]);
        vi.advanceTimersByTime(INTERVALO_RECONEXAO_MS);
        expect(WebSocketFalso.instancias).toHaveLength(1);
    });

    it('fechar encerra a conexão e não reconecta', () => {
        const { conexao, estados } = conectar();
        ultimoSocket().abrir();

        conexao.fechar();
        vi.advanceTimersByTime(10 * INTERVALO_RECONEXAO_MS);

        expect(ultimoSocket().fechado).toBe(true);
        expect(WebSocketFalso.instancias).toHaveLength(1);
        expect(estados).toEqual([true]);
    });

    it('fechar durante a espera cancela a reconexão agendada', () => {
        const { conexao } = conectar();
        ultimoSocket().cair();

        conexao.fechar();
        vi.advanceTimersByTime(10 * INTERVALO_RECONEXAO_MS);

        expect(WebSocketFalso.instancias).toHaveLength(1);
    });
});

describe('useTelemetria', () => {
    beforeEach(() => {
        react.valores = [];
        vi.stubGlobal('WebSocket', WebSocketFalso);
    });

    function renderizar(opcoes) {
        react.indice = 0;
        react.efeitos = [];
        return executarHookTelemetria(opcoes);
    }

    function montar(opcoes) {
        renderizar(opcoes);
        const [{ efeito, dependencias }] = react.efeitos;
        return { desmontar: efeito(), dependencias };
    }

    it('começa sem pacote e desconectado, e conecta ao montar', () => {
        expect(renderizar({ url: URL })).toEqual({ pacote: null, conectado: false });

        const { dependencias } = montar({ url: URL });

        expect(WebSocketFalso.instancias).toHaveLength(1);
        expect(ultimoSocket().url).toBe(URL);
        expect(dependencias).toEqual([URL, INTERVALO_RECONEXAO_MS]);
    });

    it('devolve o pacote mais recente e o estado da conexão', () => {
        montar({ url: URL });

        ultimoSocket().abrir();
        expect(renderizar({ url: URL })).toEqual({ pacote: null, conectado: true });

        ultimoSocket().receber(PACOTE);
        ultimoSocket().receber({ ...PACOTE, seq: 8, timestamp_ms: 143000 });
        const { pacote, conectado } = renderizar({ url: URL });
        expect(conectado).toBe(true);
        expect(pacote.seq).toBe(8);
        expect(pacote.timestamp_ms).toBe(143000);

        ultimoSocket().cair();
        expect(renderizar({ url: URL }).conectado).toBe(false);
        // O último pacote continua disponível enquanto reconecta.
        expect(renderizar({ url: URL }).pacote.seq).toBe(8);
    });

    it('reconecta sozinho quando a conexão cai', () => {
        montar({ url: URL, intervaloReconexaoMs: 1000 });
        ultimoSocket().abrir();
        ultimoSocket().cair();

        vi.advanceTimersByTime(1000);
        ultimoSocket().abrir();

        expect(WebSocketFalso.instancias).toHaveLength(2);
        expect(renderizar({ url: URL }).conectado).toBe(true);
    });

    it('fecha a conexão ao desmontar', () => {
        const { desmontar } = montar({ url: URL });
        ultimoSocket().abrir();

        desmontar();
        vi.advanceTimersByTime(10 * INTERVALO_RECONEXAO_MS);

        expect(ultimoSocket().fechado).toBe(true);
        expect(WebSocketFalso.instancias).toHaveLength(1);
    });

    it('usa o endereço de VITE_TELEMETRIA_WS_URL por padrão', () => {
        vi.stubEnv('VITE_TELEMETRIA_WS_URL', 'ws://robo.local:9000/ws/telemetria');

        montar();

        expect(ultimoSocket().url).toBe('ws://robo.local:9000/ws/telemetria');
    });
});
