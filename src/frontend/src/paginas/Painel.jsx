import React, { useReducer, useState } from 'react';
import IndicadorTempo from '../componentes/IndicadorTempo';
import IndicadorVelocidade from '../componentes/IndicadorVelocidade';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import HistoricoCorridas from '../componentes/HistoricoCorridas';
import Labirinto from '../componentes/Labirinto';
import { useTelemetria } from '../servicos/telemetriaWs';
import { criarEstadoPainel, reduzirTelemetria } from '../servicos/painelTelemetria';
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

export default function Painel({ corridasHistorico = null }) {
    // Cada pacote do WebSocket passa pelo reducer, na ordem de chegada: ele acumula
    // o mapa da corrida atual, recomeça quando começa uma corrida nova e salva as
    // corridas concluídas em estado.historico para inspeção e reprodução.
    const [estado, aplicar] = useReducer(reduzirTelemetria, undefined, criarEstadoPainel);
    const { conectado } = useTelemetria({ aoReceber: aplicar });
    const [corridaSelecionadaId, setCorridaSelecionadaId] = useState(null);

    const pacote = estado.pacote ?? SEM_DADOS;
    const { mapa } = estado;
    const corridas = corridasHistorico ?? estado.historico ?? [];

    return (
        <div className="painel-container">
            <header className="painel-header">
                <h1>PAINEL - MICROMOUSE</h1>
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
                    <IndicadorStatus status={pacote.status} />
                    <IndicadorTempo timestampMs={pacote.timestamp_ms} />
                    <IndicadorVelocidade atual={pacote.velocidade_atual} media={pacote.velocidade_media} />
                    <IndicadorBateria porcentagem={pacote.bateria_pct} volts={pacote.bateria_volts} />

                    {/* Resumo lateral das corridas registradas */}
                    <HistoricoCorridas
                        corridas={corridas}
                        modo="compacto"
                        aoSelecionarCorrida={(corrida) => setCorridaSelecionadaId(corrida.id)}
                    />
                </div>
            </div>

            {/* Seção de Histórico de Corridas: Tabela + Filtros (HU-13) + Inspeção e Reprodução (HU-14) */}
            <section style={{ marginTop: '36px' }} data-testid="secao-historico-corridas">
                <header className="painel-header" style={{ marginBottom: '20px' }}>
                    <h1>HISTÓRICO DE CORRIDAS</h1>
                </header>
                <HistoricoCorridas
                    corridas={corridas}
                    modo="tabela"
                    corridaSelecionadaId={corridaSelecionadaId}
                    aoSelecionarCorrida={(corrida) => setCorridaSelecionadaId(corrida.id)}
                />
            </section>
        </div>
    );
}
