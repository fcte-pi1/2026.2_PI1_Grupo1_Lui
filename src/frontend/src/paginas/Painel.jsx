import React, { useState, useEffect, useMemo } from 'react';
import IndicadorTempo from '../componentes/IndicadorTempo';
import IndicadorVelocidade from '../componentes/IndicadorVelocidade';
import IndicadorBateria from '../componentes/IndicadorBateria';
import IndicadorStatus from '../componentes/IndicadorStatus';
import Labirinto from '../componentes/Labirinto';
import '../indicadores.css';

// Roteiros de exploração simulados por geometria (atualizados a cada 1s)
const SEQUENCIAS_EXPLORACAO = {
    4: [
        { pos: { x: 0, y: 0 }, dir: 'SUL', parede: { id1: 'C01', id2: 'C02', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 0, y: 1 }, dir: 'LESTE', parede: { id1: 'A03', id2: 'A04', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 1, y: 1 }, dir: 'LESTE', parede: { id1: 'B03', id2: 'C03', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 1 }, dir: 'SUL', parede: { id1: 'C02', id2: 'D02', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 2 }, dir: 'LESTE', parede: { id1: 'C03', id2: 'C04', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 3, y: 2 }, dir: 'SUL', parede: { id1: 'C04', id2: 'D04', dir1: 'right', dir2: 'left' } },
        { pos: { x: 3, y: 3 }, dir: 'SUL', parede: null }
    ],
    8: [
        { pos: { x: 0, y: 0 }, dir: 'LESTE', parede: { id1: 'A02', id2: 'A03', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 1, y: 0 }, dir: 'SUL', parede: { id1: 'B03', id2: 'B04', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 1, y: 1 }, dir: 'SUL', parede: { id1: 'C02', id2: 'D02', dir1: 'right', dir2: 'left' } },
        { pos: { x: 1, y: 2 }, dir: 'LESTE', parede: { id1: 'A05', id2: 'B05', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 2 }, dir: 'SUL', parede: { id1: 'C05', id2: 'D05', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 3 }, dir: 'SUL', parede: { id1: 'A06', id2: 'A07', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 2, y: 4 }, dir: 'SUL', parede: null },
        { pos: { x: 2, y: 5 }, dir: 'SUL', parede: null },
        { pos: { x: 2, y: 6 }, dir: 'SUL', parede: null },
        { pos: { x: 2, y: 7 }, dir: 'LESTE', parede: null },
        { pos: { x: 3, y: 7 }, dir: 'LESTE', parede: null }
    ],
    12: [
        { pos: { x: 0, y: 0 }, dir: 'SUL', parede: { id1: 'B01', id2: 'C01', dir1: 'right', dir2: 'left' } },
        { pos: { x: 0, y: 1 }, dir: 'LESTE', parede: { id1: 'C01', id2: 'C02', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 1, y: 1 }, dir: 'LESTE', parede: { id1: 'A02', id2: 'A03', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 2, y: 1 }, dir: 'SUL', parede: { id1: 'C02', id2: 'D02', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 2 }, dir: 'SUL', parede: { id1: 'C04', id2: 'D04', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 3 }, dir: 'SUL', parede: { id1: 'A04', id2: 'A05', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 2, y: 4 }, dir: 'SUL', parede: { id1: 'B05', id2: 'C05', dir1: 'right', dir2: 'left' } },
        { pos: { x: 2, y: 5 }, dir: 'SUL', parede: { id1: 'A06', id2: 'A07', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 2, y: 6 }, dir: 'OESTE', parede: { id1: 'C07', id2: 'C08', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 1, y: 6 }, dir: 'SUL', parede: null },
        { pos: { x: 1, y: 7 }, dir: 'SUL', parede: null },
        { pos: { x: 1, y: 8 }, dir: 'SUL', parede: { id1: 'A09', id2: 'A10', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 1, y: 9 }, dir: 'SUL', parede: null },
        { pos: { x: 1, y: 10 }, dir: 'LESTE', parede: { id1: 'B11', id2: 'B12', dir1: 'bottom', dir2: 'top' } },
        { pos: { x: 2, y: 10 }, dir: 'LESTE', parede: null },
        { pos: { x: 3, y: 10 }, dir: 'SUL', parede: null },
        { pos: { x: 3, y: 11 }, dir: 'SUL', parede: null }
    ]
};

export default function Painel() {
    const [geometria, setGeometria] = useState(4); // Pode ser 4, 8 ou 12
    const [passoMapeamento, setPassoMapeamento] = useState(3); // Inicia no passo 3 correspondente ao protótipo

    // Estado da telemetria recebida
    const [pacote, setPacote] = useState({
        timestamp_ms: 142000,
        velocidade_atual: 35.0,
        velocidade_media: 28.5,
        bateria_pct: 78,
        bateria_volts: 7.42,
        status: "MAPEANDO"
    });

    const sequenciaAtual = SEQUENCIAS_EXPLORACAO[geometria] || SEQUENCIAS_EXPLORACAO[4];

    // Calcula paredes, robô e rastro com base no passo atual de exploração
    const { paredesAtuais, roboAtual, trajetoAtual } = useMemo(() => {
        const passosValidos = sequenciaAtual.slice(0, Math.min(passoMapeamento + 1, sequenciaAtual.length));
        const paredes = passosValidos.map(p => p.parede).filter(Boolean);
        const trajeto = passosValidos.map(p => p.pos);
        const ultimoPasso = passosValidos[passosValidos.length - 1] || sequenciaAtual[0];

        return {
            paredesAtuais: paredes,
            roboAtual: { pos: ultimoPasso.pos, dir: ultimoPasso.dir },
            trajetoAtual: trajeto
        };
    }, [passoMapeamento, sequenciaAtual]);

    // Timer de telemetria em tempo real (1 Hz / 1 segundo)
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

            // Se estiver mapeando, avança a exploração desenhando a grade a cada segundo
            setPassoMapeamento(passo => {
                if (pacote.status === "MAPEANDO") {
                    if (passo + 1 >= sequenciaAtual.length) {
                        setPacote(p => ({ ...p, status: "CONCLUIDO", velocidade_atual: 0 }));
                        return passo;
                    }
                    return passo + 1;
                }
                return passo;
            });
        }, 1000);

        return () => clearInterval(intervalo);
    }, [pacote.status, sequenciaAtual.length]);

    const handleTrocarGeometria = (linhas) => {
        setGeometria(linhas);
        setPassoMapeamento(0); // Reinicia exploração a partir da nova geometria
    };

    const handleResetar = () => {
        setPacote(p => ({
            ...p,
            status: "MAPEANDO",
            timestamp_ms: 0,
            bateria_pct: 100,
            velocidade_atual: 30.0
        }));
        setPassoMapeamento(0);
    };

    return (
        <div className="painel-container">
            <header className="painel-header">
                <h1>PAINEL - MICROMOUSE</h1>
                <div className="status-online"><span className="dot"></span> ONLINE</div>
            </header>
            
            <div className="dashboard-layout">
                {/* Lado Esquerdo: Mapeamento Dinâmico do Labirinto */}
                <Labirinto 
                    linhas={geometria} 
                    paredes={paredesAtuais}
                    roboPosicao={roboAtual.pos}
                    roboOrientacao={roboAtual.dir}
                    trajeto={trajetoAtual}
                />

                {/* Lado Direito: Indicadores de Telemetria */}
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
