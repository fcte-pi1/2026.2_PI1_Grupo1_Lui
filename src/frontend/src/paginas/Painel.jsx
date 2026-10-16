import React, { useState, useEffect, useMemo } from 'react';
import IndicadorTempo from '../componentes/IndicadorTempo';
import IndicadorVelocidade from '../componentes/IndicadorVelocidade';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import HistoricoCorridas from '../componentes/HistoricoCorridas';
import Labirinto from '../componentes/Labirinto';
import {
    criarEstadoInicial,
    aplicarPacote,
    SEQUENCIAS_SIMULADAS_6_4
} from '../servicos/labirinto';
import '../indicadores.css';

export default function Painel() {
    const [geometria, setGeometria] = useState(4); // 4, 8 ou 12
    const [passoMapeamento, setPassoMapeamento] = useState(0);

    const sequenciaAtual = SEQUENCIAS_SIMULADAS_6_4[geometria] || SEQUENCIAS_SIMULADAS_6_4[4];

    // Estado da telemetria recebida (inicializado a partir do primeiro pacote da simulação)
    const [pacote, setPacote] = useState(() => sequenciaAtual[0] || {
        timestamp_ms: 0,
        velocidade_atual: 0.0,
        velocidade_media: 0.0,
        bateria_pct: 100,
        bateria_volts: 8.4,
        status: "AGUARDANDO"
    });

    // Constrói o estado incremental do mapa acumulando pacotes no formato do contrato 6.4
    const estadoMapa = useMemo(() => {
        let estado = criarEstadoInicial(`${geometria}x4`);
        const passosValidos = sequenciaAtual.slice(0, Math.min(passoMapeamento + 1, sequenciaAtual.length));
        for (const p of passosValidos) {
            estado = aplicarPacote(estado, p);
        }
        return estado;
    }, [geometria, passoMapeamento, sequenciaAtual]);

    // Timer de telemetria em tempo real (1 Hz / 1 segundo)
    useEffect(() => {
        const intervalo = setInterval(() => {
            setPassoMapeamento(passo => {
                if (pacote.status === "MAPEANDO") {
                    const proximoPasso = passo + 1;
                    if (proximoPasso < sequenciaAtual.length) {
                        const novoPacote = sequenciaAtual[proximoPasso];
                        setPacote(novoPacote);
                        return proximoPasso;
                    } else {
                        // Concluiu a exploração
                        setPacote(prev => ({
                            ...prev,
                            status: "CONCLUIDO",
                            desafio_cumprido: true,
                            velocidade_atual: 0.0
                        }));
                        return passo;
                    }
                }
                return passo;
            });
        }, 1000);

        return () => clearInterval(intervalo);
    }, [pacote.status, sequenciaAtual]);

    const handleTrocarGeometria = (linhas) => {
        setGeometria(linhas);
        setPassoMapeamento(0);
        const seq = SEQUENCIAS_SIMULADAS_6_4[linhas] || SEQUENCIAS_SIMULADAS_6_4[4];
        if (seq && seq[0]) {
            setPacote(seq[0]);
        }
    };

    const handleResetar = () => {
        setPassoMapeamento(0);
        if (sequenciaAtual && sequenciaAtual[0]) {
            setPacote(sequenciaAtual[0]);
        }
    };

    return (
        <div className="painel-container">
            <header className="painel-header">
                <h1>PAINEL - MICROMOUSE</h1>
                {/* Indicador de conexão neutro enquanto a telemetria ao vivo da HU-09 não está conectada */}
                <div className="status-conexao neutro" data-testid="status-conexao">
                    <span className="dot"></span> AGUARDANDO CONEXÃO
                </div>
            </header>
            
            <div className="dashboard-layout">
                {/* Lado Esquerdo: Mapeamento Dinâmico do Labirinto */}
                <Labirinto 
                    linhas={geometria} 
                    cells={estadoMapa.cells}
                    roboPosicao={estadoMapa.roboPosicao}
                    roboOrientacao={estadoMapa.roboOrientacao}
                    trajeto={estadoMapa.trajeto}
                />

                {/* Lado Direito: Indicadores de Telemetria */}
                <div className="indicadores-coluna">
                    <IndicadorStatus status={pacote.status} />
                    <IndicadorTempo timestampMs={pacote.timestamp_ms} />
                    <IndicadorVelocidade atual={pacote.velocidade_atual} media={pacote.velocidade_media} />
                    <IndicadorBateria porcentagem={pacote.bateria_pct} volts={pacote.bateria_volts} />
                    
                    {/* Histórico desacoplado, pronto para persistência futura (HU-12) */}
                    <HistoricoCorridas corridas={[]} />
                    
                    <div className="controles-simulacao">
                        <h3>Geometria da Pista</h3>
                        <div style={{ display: 'flex', gap: '8px', marginBottom: '16px' }}>
                            <button 
                                onClick={() => handleTrocarGeometria(4)} 
                                style={{ fontWeight: geometria === 4 ? 'bold' : 'normal' }}
                            >
                                4x4
                            </button>
                            <button 
                                onClick={() => handleTrocarGeometria(8)} 
                                style={{ fontWeight: geometria === 8 ? 'bold' : 'normal' }}
                            >
                                8x4
                            </button>
                            <button 
                                onClick={() => handleTrocarGeometria(12)} 
                                style={{ fontWeight: geometria === 12 ? 'bold' : 'normal' }}
                            >
                                12x4
                            </button>
                        </div>
                        
                        <h3>Controles de Simulação</h3>
                        <button onClick={handleResetar}>Resetar</button>
                        <button onClick={() => setPacote(p => ({ ...p, status: p.status === "MAPEANDO" ? "AGUARDANDO" : "MAPEANDO" }))}>
                            {pacote.status === "MAPEANDO" ? "Pausar Mapeamento" : "Retomar Mapeamento"}
                        </button>
                        <button onClick={() => setPacote(p => ({ ...p, bateria_pct: 15 }))}>Forçar Bateria Baixa</button>
                        <button onClick={() => setPacote(p => ({ ...p, status: "TRAVADO" }))}>Forçar Travamento</button>
                    </div>
                </div>
            </div>
        </div>
    );
}
