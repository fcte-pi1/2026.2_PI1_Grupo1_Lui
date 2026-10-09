import React, { useState, useEffect } from 'react';
import IndicadorTempo from '../componentes/IndicadorTempo';
import IndicadorVelocidade from '../componentes/IndicadorVelocidade';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import '../indicadores.css';

export default function Painel() {
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
            <h1>Painel de Controle - Micromouse</h1>
            <div className="indicadores-container">
                <IndicadorTempo timestampMs={pacote.timestamp_ms} />
                <IndicadorVelocidade atual={pacote.velocidade_atual} media={pacote.velocidade_media} />
                <IndicadorBateria porcentagem={pacote.bateria_pct} volts={pacote.bateria_volts} />
                <IndicadorStatus status={pacote.status} />
            </div>
            
            <div className="controles-simulacao">
                <h3>Controles de Simulação</h3>
                <button onClick={() => setPacote(p => ({ ...p, status: "AGUARDANDO", timestamp_ms: 0, bateria_pct: 100 }))}>Resetar</button>
                <button onClick={() => setPacote(p => ({ ...p, bateria_pct: 15 }))}>Forçar Bateria Baixa</button>
                <button onClick={() => setPacote(p => ({ ...p, status: "TRAVADO" }))}>Forçar Travamento</button>
            </div>
        </div>
    );
}
