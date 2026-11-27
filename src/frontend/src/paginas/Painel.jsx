import React, { useEffect, useReducer, useState, useCallback, useRef } from 'react';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import HistoricoCorridas from '../componentes/HistoricoCorridas';
import AvisoConexaoRf from '../componentes/AvisoConexaoRf';
import Labirinto from '../componentes/Labirinto';
import PainelDiagnosticoLogs from '../componentes/PainelDiagnosticoLogs';
import { useTelemetria } from '../servicos/telemetriaWs';
import { criarEstadoPainel, reduzirTelemetria } from '../servicos/painelTelemetria';
import { listarCorridas, enviarLargadaRobo } from '../servicos/corridasApi';
import {
    NIVEIS,
    diagnosticarStatus,
    diagnosticarAnomaliasTelemetria,
    diagnosticarAcaoResolver,
    formatarTempoRelogio,
} from '../servicos/diagnosticoLogs';
import '../indicadores.css';

// Valores mostrados antes do primeiro pacote chegar. A bateria fica indefinida
// para não disparar o alerta de nível crítico sem dado nenhum.
const SEM_DADOS = {
    bateria_pct: undefined,
    bateria_volts: undefined,
    status: null,
};

export default function Painel() {
    // Cada pacote do WebSocket passa pelo reducer, na ordem de chegada: ele acumula
    // o mapa da corrida atual e recomeça quando começa uma corrida nova.
    const [estado, aplicar] = useReducer(reduzirTelemetria, undefined, criarEstadoPainel);
    const [logsDiag, setLogsDiag] = useState([]);
    const [eventosWebhook, setEventosWebhook] = useState([]);
    const [alturaMapaDesktop, setAlturaMapaDesktop] = useState(null);
    const dashboardRef = useRef(null);
    const diagRef = useRef({
        seqId: 0,
        ultimoStatus: null,
        conectadoAntes: null,
        rfPerdidaAntes: false,
        bateriaCriticaAvisada: false,
        bateriaBaixaAvisada: false,
        rssiFracoAvisado: false,
        pacotesParadosSeguidos: 0,
    });

    const proximoIdLog = useCallback(() => {
        diagRef.current.seqId += 1;
        return `diag-${diagRef.current.seqId}`;
    }, []);

    const adicionarLogs = useCallback((novos) => {
        const validos = novos.filter(Boolean);
        if (validos.length === 0) return;
        setLogsDiag((atual) => [...validos.reverse(), ...atual].slice(0, 60));
    }, []);

    const registrarWebhook = useCallback((direcao, origem, dados) => {
        setEventosWebhook((atual) => [{
            id: proximoIdLog(),
            hora: formatarTempoRelogio(),
            direcao,
            origem,
            payload: typeof dados === 'string' ? dados : JSON.stringify(dados),
        }, ...atual].slice(0, 30));
    }, [proximoIdLog]);

    const aoReceberPacote = useCallback((pacote) => {
        aplicar(pacote);
        registrarWebhook('RX', '/ws/telemetria', pacote);
        const ctx = diagRef.current;
        const gerados = [];

        if (pacote.status && pacote.status !== ctx.ultimoStatus) {
            ctx.ultimoStatus = pacote.status;
            ctx.pacotesParadosSeguidos = 0;
            const logStatus = diagnosticarStatus(pacote, proximoIdLog());
            if (logStatus) gerados.push(logStatus);
        }

        if ((pacote.status === 'MAPEANDO' || pacote.status === 'SPEED_RUN') && pacote.velocidade_atual === 0) {
            ctx.pacotesParadosSeguidos += 1;
        } else {
            ctx.pacotesParadosSeguidos = 0;
        }

        const anomalias = diagnosticarAnomaliasTelemetria(pacote, ctx, proximoIdLog);
        for (const a of anomalias) {
            if (a.dominio === 'energia' && a.nivel === NIVEIS.critico) ctx.bateriaCriticaAvisada = true;
            if (a.dominio === 'energia' && a.nivel === NIVEIS.alerta) ctx.bateriaBaixaAvisada = true;
            if (a.dominio === 'radio') ctx.rssiFracoAvisado = true;
            gerados.push(a);
        }
        if (typeof pacote.bateria_volts === 'number' && pacote.bateria_volts >= 7.15) {
            ctx.bateriaCriticaAvisada = false;
            ctx.bateriaBaixaAvisada = false;
        }
        if (typeof pacote.rssi === 'number' && pacote.rssi >= -78) {
            ctx.rssiFracoAvisado = false;
        }

        adicionarLogs(gerados);
    }, [adicionarLogs, proximoIdLog, registrarWebhook]);

    const { conectado, rf } = useTelemetria({ aoReceber: aoReceberPacote });
    const [historico, setHistorico] = useState(null);
    const [erroHistorico, setErroHistorico] = useState('');
    const [filtros, setFiltros] = useState({});
    const [enviandoLargada, setEnviandoLargada] = useState(false);
    const [feedbackLargada, setFeedbackLargada] = useState('');

    useEffect(() => {
        const ctx = diagRef.current;
        if (ctx.conectadoAntes === null) {
            ctx.conectadoAntes = conectado;
            if (conectado) {
                adicionarLogs([{
                    id: proximoIdLog(),
                    hora: formatarTempoRelogio(),
                    tempoCorrida: null,
                    nivel: NIVEIS.ok,
                    dominio: 'software',
                    secundario: null,
                    titulo: 'Painel conectado ao backend (/ws/telemetria)',
                    detalhe: 'WebSocket ativo aguardando pacotes de telemetria.',
                    dica: null,
                }]);
            }
            return;
        }
        if (ctx.conectadoAntes !== conectado) {
            ctx.conectadoAntes = conectado;
            adicionarLogs([{
                id: proximoIdLog(),
                hora: formatarTempoRelogio(),
                tempoCorrida: null,
                nivel: conectado ? NIVEIS.ok : NIVEIS.critico,
                dominio: 'software',
                secundario: null,
                titulo: conectado ? 'Backend reconectado' : 'Conexão com o backend perdida (DESCONECTADO)',
                detalhe: conectado
                    ? 'Canal WebSocket /ws/telemetria restabelecido.'
                    : 'O navegador não alcança o servidor FastAPI/Uvicorn.',
                dica: conectado
                    ? null
                    : 'Software: verifique se o uvicorn está rodando na porta 8000 e confira VITE_TELEMETRIA_WS_URL.',
            }]);
        }
    }, [conectado, adicionarLogs, proximoIdLog]);

    useEffect(() => {
        const ctx = diagRef.current;
        if (ctx.rfPerdidaAntes !== rf.perdida) {
            ctx.rfPerdidaAntes = rf.perdida;
            adicionarLogs([{
                id: proximoIdLog(),
                hora: formatarTempoRelogio(),
                tempoCorrida: null,
                nivel: rf.perdida ? NIVEIS.alerta : NIVEIS.ok,
                dominio: 'radio',
                secundario: rf.perdida ? 'energia' : null,
                titulo: rf.perdida ? 'Conexão RF perdida (≥ 3 s sem pacotes)' : 'Sinal RF ESP-NOW restabelecido',
                detalhe: rf.perdida
                    ? 'Backend segue no ar, mas o gateway não recebeu pacotes do ESP32.'
                    : 'Pacotes voltaram a chegar pelo gateway.',
                dica: rf.perdida
                    ? 'Energia/Eletrônica: cheque se a LiPo caiu (< 6,8 V = reinício por brownout), confira canal 1 e distância do gateway ESP8266.'
                    : null,
            }]);
        }
    }, [rf.perdida, adicionarLogs, proximoIdLog]);

    const carregarHistorico = useCallback((filtrosAtuais = filtros) => {
        listarCorridas(filtrosAtuais)
            .then(setHistorico)
            .catch((erro) => setErroHistorico(erro.message));
    }, [filtros]);

    const acionarLargada = useCallback(async () => {
        setEnviandoLargada(true);
        setFeedbackLargada('');
        try {
            const resp = await enviarLargadaRobo();
            registrarWebhook('TX', 'POST /api/comandos/largada', resp);
            setFeedbackLargada(resp.enviado ? 'Enviado' : 'Simulador');
            adicionarLogs([diagnosticarAcaoResolver({
                sucesso: true,
                modoSimulador: !resp.enviado,
                detalhe: resp.detalhe,
            }, proximoIdLog())]);
        } catch (erro) {
            registrarWebhook('TX', 'POST /api/comandos/largada (ERRO)', { erro: erro.message });
            setFeedbackLargada(erro.message);
            adicionarLogs([diagnosticarAcaoResolver({
                sucesso: false,
                detalhe: erro.message,
            }, proximoIdLog())]);
        } finally {
            setEnviandoLargada(false);
        }
    }, [adicionarLogs, proximoIdLog, registrarWebhook]);

    useEffect(() => {
        carregarHistorico(filtros);
    }, [estado.historico.length, filtros, carregarHistorico]);

    const pacote = estado.pacote ?? SEM_DADOS;
    const { mapa } = estado;

    useEffect(() => {
        const container = dashboardRef.current;
        const mapaEl = container?.querySelector('[data-testid="labirinto-container"]');
        if (!mapaEl || typeof ResizeObserver === 'undefined') return undefined;

        const atualizarAltura = () => {
            if (window.innerWidth < 768) {
                setAlturaMapaDesktop(null);
                return;
            }
            const h = Math.round(mapaEl.getBoundingClientRect().height);
            setAlturaMapaDesktop(h > 0 ? h : null);
        };

        atualizarAltura();
        const observador = new ResizeObserver(atualizarAltura);
        observador.observe(mapaEl);
        window.addEventListener('resize', atualizarAltura);
        return () => {
            observador.disconnect();
            window.removeEventListener('resize', atualizarAltura);
        };
    }, [mapa.linhas]);

    return (
        <div className="painel-container">
            <header className="painel-header">
                <h1>PAINEL - MICROMOUSE</h1>
                <div className="painel-header-acoes">
                    <button
                        type="button"
                        className="btn-header-discreto"
                        data-testid="btn-resolver"
                        onClick={acionarLargada}
                        disabled={!conectado || enviandoLargada}
                        title={feedbackLargada || "Enviar comando 'k' para o robô iniciar o labirinto"}
                    >
                        {enviandoLargada ? 'Enviando...' : 'Resolver'}
                    </button>
                    {feedbackLargada && (
                        <span className="feedback-resolver" role="status">
                            {feedbackLargada}
                        </span>
                    )}
                    <button
                        type="button"
                        className="btn-header-discreto"
                        onClick={() => carregarHistorico(filtros)}
                    >
                        Atualizar histórico
                    </button>
                    {/* Conexão do painel com o backend; a perda do rádio é o aviso abaixo (HU-09) */}
                    <div
                        className={`status-conexao ${conectado ? 'conectado' : 'desconectado'}`}
                        data-testid="status-conexao"
                    >
                        <span className="dot"></span> {conectado ? 'CONECTADO' : 'DESCONECTADO'}
                    </div>
                </div>
            </header>

            {/* HU-09: backend no ar, mas sem pacotes do robô há 3 s ou mais. O status
                da corrida não muda: o robô não depende do rádio e segue navegando. */}
            <AvisoConexaoRf perdida={rf.perdida} segundosSemPacote={rf.segundosSemPacote} />

            <div className="dashboard-layout" ref={dashboardRef}>
                {/* Lado Esquerdo: Mapeamento Dinâmico do Labirinto */}
                <Labirinto
                    linhas={mapa.linhas}
                    cells={mapa.cells}
                    roboPosicao={mapa.roboPosicao}
                    roboOrientacao={mapa.roboOrientacao}
                    trajeto={mapa.trajeto}
                />

                {/* Lado Direito: Indicadores e Monitor/Webhook com altura sincronizada ao mapa */}
                <div
                    className="indicadores-coluna"
                    style={alturaMapaDesktop ? { height: `${alturaMapaDesktop}px` } : undefined}
                >
                    <IndicadorStatus status={pacote.status} tentativa={estado.corrida || null} />
                    <IndicadorBateria porcentagem={pacote.bateria_pct} volts={pacote.bateria_volts} />

                    {/* Card lateral com 2 abas: Diagnóstico + Portas e Webhook */}
                    <PainelDiagnosticoLogs
                        logs={logsDiag}
                        eventosWebhook={eventosWebhook}
                        pacote={estado.pacote}
                        conectado={conectado}
                        rfPerdida={rf.perdida}
                        aoLimpar={() => setLogsDiag([])}
                        aoLimparWebhook={() => setEventosWebhook([])}
                    />
                    {erroHistorico && <small role="status">Histórico indisponível: {erroHistorico}</small>}
                </div>
            </div>

            {/* Histórico geral com filtros e replay (HU-13/HU-14) em largura total, abaixo
                do mapa e dos indicadores: na coluna da direita a tabela e o mapa do
                replay não cabiam em 1366x768 (RNF-SFT05). */}
            {historico && (
                <section className="secao-historico-geral" data-testid="secao-historico-geral">
                    <HistoricoCorridas
                        corridas={historico}
                        exibirInspecao
                        aoFiltrar={(novos) => setFiltros(novos)}
                    />
                </section>
            )}
        </div>
    );
}
