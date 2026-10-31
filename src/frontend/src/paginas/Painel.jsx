import React, { useEffect, useReducer, useState } from 'react';
import IndicadorTempo from '../componentes/IndicadorTempo';
import IndicadorVelocidade from '../componentes/IndicadorVelocidade';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import HistoricoCorridas from '../componentes/HistoricoCorridas';
import Labirinto from '../componentes/Labirinto';
import { useTelemetria } from '../servicos/telemetriaWs';
import { criarEstadoPainel, reduzirTelemetria } from '../servicos/painelTelemetria';
import { listarCorridas } from '../servicos/corridasApi';
import '../indicadores.css';

// Valores mostrados antes do primeiro pacote chegar. A bateria fica indefinida
// para não disparar o alerta de nível crítico sem dado nenhum.
const SEM_DADOS = {
    timestamp_ms: 0,
    velocidade_atual: 0,
    velocidade_media: 0,
    bateria_pct: undefined,
    bateria_volts: undefined,
    status: null,
};

export default function Painel() {
    // Cada pacote do WebSocket passa pelo reducer, na ordem de chegada: ele acumula
    // o mapa da corrida atual e recomeça quando começa uma corrida nova.
    const [estado, aplicar] = useReducer(reduzirTelemetria, undefined, criarEstadoPainel);
    const { conectado } = useTelemetria({ aoReceber: aplicar });
    const [historico, setHistorico] = useState(null);
    const [erroHistorico, setErroHistorico] = useState('');
    useEffect(() => {
        listarCorridas().then(setHistorico).catch((erro) => setErroHistorico(erro.message));
    }, [estado.historico.length]);

    const pacote = estado.pacote ?? SEM_DADOS;
    const { mapa } = estado;

    return (
        <div className="painel-container">
            <header className="painel-header">
                <h1>PAINEL - MICROMOUSE</h1>
                <button type="button" onClick={() => listarCorridas().then(setHistorico).catch((erro) => setErroHistorico(erro.message))}>
                    Atualizar histórico
                </button>
                {/* Conexão do painel com o backend; o aviso de perda do rádio é a HU-09 */}
                <div
                    className={`status-conexao ${conectado ? 'conectado' : 'desconectado'}`}
                    data-testid="status-conexao"
                >
                    <span className="dot"></span> {conectado ? 'CONECTADO' : 'DESCONECTADO'}
                </div>
            </header>

            <div className="dashboard-layout">
                {/* Lado Esquerdo: Mapeamento Dinâmico do Labirinto */}
                <Labirinto
                    linhas={mapa.linhas}
                    cells={mapa.cells}
                    roboPosicao={mapa.roboPosicao}
                    roboOrientacao={mapa.roboOrientacao}
                    trajeto={mapa.trajeto}
                />

                {/* Lado Direito: Indicadores de Telemetria */}
                <div className="indicadores-coluna">
                    <IndicadorStatus status={pacote.status} tentativa={estado.corrida || null} />
                    <IndicadorTempo timestampMs={pacote.timestamp_ms} />
                    <IndicadorVelocidade atual={pacote.velocidade_atual} media={pacote.velocidade_media} />
                    <IndicadorBateria porcentagem={pacote.bateria_pct} volts={pacote.bateria_volts} />

                    {/* Histórico da sessão, registrando corridas encerradas inclusive por TRAVADO (HU-19) */}
                    <HistoricoCorridas corridas={estado.historico} modo="compacto" />
                    {erroHistorico && <small role="status">Histórico indisponível: {erroHistorico}</small>}
                    {historico && <HistoricoCorridas corridas={historico} exibirInspecao />}
                </div>
            </div>
        </div>
    );
}
