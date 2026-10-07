import React, { useState, useEffect } from 'react';
import IndicadorTempo from '../componentes/IndicadorTempo';
import IndicadorVelocidade from '../componentes/IndicadorVelocidade';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import Labirinto from '../componentes/Labirinto';
import '../indicadores.css';

export default function Painel() {
    const [geometria, setGeometria] = useState(4); // Pode ser 4, 8 ou 12

    // Estado mockado para demonstração no painel
    const [pacote, setPacote] = useState({
        timestamp_ms: 142000,
        velocidade_atual: 35.0,
        velocidade_media: 28.5,
        bateria_pct: 78,
        bateria_volts: 7.42,
        status: "MAPEANDO"
    });

    // Simular o progresso do tempo e bateria caindo (apenas para teste visual)
    useEffect(() => {
        const intervalo = setInterval(() => {
            setPacote(prev => {
                let novoStatus = prev.status;
                if (prev.timestamp_ms > 200000) novoStatus = "TRAVADO";
                return {
                    ...prev,
                    timestamp_ms: prev.timestamp_ms + 1000,
                    bateria_pct: Math.max(0, prev.bateria_pct - 1),
                    status: novoStatus
                };
            });
        }, 1000);
        return () => clearInterval(intervalo);
    }, []);

    return (
        <div className="painel-container">
            <header className="painel-header">
                <h1>PAINEL - MICROMOUSE</h1>
                <div className="status-online"><span className="dot"></span> ONLINE</div>
            </header>
            
            <div className="dashboard-layout">
                {/* Lado Esquerdo: Mapeamento do Labirinto */}
                <Labirinto linhas={geometria} />

                {/* Lado Direito: Indicadores */}
                <div className="indicadores-coluna">
                    <IndicadorStatus status={pacote.status} />
                    <IndicadorTempo timestampMs={pacote.timestamp_ms} />
                    <IndicadorVelocidade atual={pacote.velocidade_atual} media={pacote.velocidade_media} />
                    <IndicadorBateria porcentagem={pacote.bateria_pct} volts={pacote.bateria_volts} />
                    
                    <div className="indicador historico">
                        <div className="cabecalho">HISTÓRICO</div>
                        <div className="historico-itens">
                            <p>Execuções Anteriores</p>
                            <div className="historico-item">
                                <span>Tentativa 1</span>
                                <strong>Em andamento</strong>
                            </div>
                        </div>
                    </div>
                    
                    <div className="controles-simulacao">
                        <h3>Geometria da Pista</h3>
                        <div style={{ display: 'flex', gap: '8px', marginBottom: '16px' }}>
                            <button onClick={() => setGeometria(4)} style={{ fontWeight: geometria === 4 ? 'bold' : 'normal' }}>4x4</button>
                            <button onClick={() => setGeometria(8)} style={{ fontWeight: geometria === 8 ? 'bold' : 'normal' }}>8x4</button>
                            <button onClick={() => setGeometria(12)} style={{ fontWeight: geometria === 12 ? 'bold' : 'normal' }}>12x4</button>
                        </div>
                        
                        <h3>Controles de Simulação</h3>
                        <button onClick={() => setPacote(p => ({ ...p, status: "AGUARDANDO", timestamp_ms: 0, bateria_pct: 100 }))}>Resetar</button>
                        <button onClick={() => setPacote(p => ({ ...p, bateria_pct: 15 }))}>Forçar Bateria Baixa</button>
                        <button onClick={() => setPacote(p => ({ ...p, status: "TRAVADO" }))}>Forçar Travamento</button>
                    </div>
                </div>
            </div>
        </div>
    );
}
