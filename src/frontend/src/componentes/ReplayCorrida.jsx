import React, { useState, useMemo, useEffect } from 'react';
import Labirinto from './Labirinto';
import {
    construirQuadrosReplay,
    obterEstadoNoTempo,
    calcularEstatisticasCorrida
} from '../servicos/replayCorrida';
import { formatarTempo } from '../servicos/formatacao';
import './ReplayCorrida.css';

/**
 * Componente ReplayCorrida (HU-14 / RF-SFT08)
 *
 * Exibe o mapa final ou no instante selecionado da barra de tempo,
 * controles de reprodução (Play/Pause e barra de tempo), estatísticas
 * da corrida e métricas comparativas conforme prototipoHistoricoCorrida.png.
 */
export default function ReplayCorrida({
    corrida = null,
    tempoMsInicial = null,
    intervaloPassoMs = 1000
}) {
    const dadosReplay = useMemo(() => {
        if (!corrida) return null;
        return construirQuadrosReplay(corrida);
    }, [corrida]);

    const estatisticas = useMemo(() => {
        if (!corrida || !dadosReplay) return null;
        return calcularEstatisticasCorrida(corrida, dadosReplay);
    }, [corrida, dadosReplay]);

    const duracaoMaxMs = dadosReplay?.duracaoTotalMs || 0;

    const [tempoAtualMs, setTempoAtualMs] = useState(
        tempoMsInicial !== null ? tempoMsInicial : duracaoMaxMs
    );
    const [reproduzindo, setReproduzindo] = useState(false);

    // Sincroniza quando uma nova corrida é selecionada
    useEffect(() => {
        setReproduzindo(false);
        setTempoAtualMs(tempoMsInicial !== null ? tempoMsInicial : (dadosReplay?.duracaoTotalMs || 0));
    }, [corrida, dadosReplay, tempoMsInicial]);

    // Laço de reprodução temporal quando em Play
    useEffect(() => {
        if (!reproduzindo || !dadosReplay || dadosReplay.quadros.length === 0) {
            return undefined;
        }

        const timer = setInterval(() => {
            setTempoAtualMs((anterior) => {
                const proximo = anterior + 1000;
                if (proximo >= duracaoMaxMs) {
                    setReproduzindo(false);
                    return duracaoMaxMs;
                }
                return proximo;
            });
        }, intervaloPassoMs);

        return () => clearInterval(timer);
    }, [reproduzindo, dadosReplay, duracaoMaxMs, intervaloPassoMs]);

    if (!corrida || !dadosReplay || !estatisticas) {
        return (
            <div className="replay-corrida-painel" data-testid="replay-corrida-vazio">
                <div className="replay-card replay-vazio">
                    Selecione uma corrida para inspecionar o mapa, estatísticas e reproduzir o trajeto
                </div>
            </div>
        );
    }

    const estadoNoTempo = obterEstadoNoTempo(dadosReplay, tempoAtualMs);
    const mapaExibido = estadoNoTempo.mapa;

    const alternarReproducao = () => {
        if (!reproduzindo && tempoAtualMs >= duracaoMaxMs) {
            setTempoAtualMs(0);
        }
        setReproduzindo((prev) => !prev);
    };

    const alterarBarraTempo = (e) => {
        const novoTempo = Number(e.target.value);
        setReproduzindo(false);
        setTempoAtualMs(novoTempo);
    };

    return (
        <div className="replay-corrida-painel" data-testid="replay-corrida">
            {/* Bloco 1: Inspecionar Corrida e Reprodução */}
            <div className="replay-card" data-testid="bloco-inspecionar-corrida">
                <h3 className="replay-card-titulo">
                    {`INSPECIONAR CORRIDA (INSPECT RUN) - ${estatisticas.id}`}
                </h3>

                <div className="replay-mapa-wrapper">
                    <Labirinto
                        linhas={mapaExibido.linhas}
                        cells={mapaExibido.cells}
                        roboPosicao={mapaExibido.roboPosicao}
                        roboOrientacao={mapaExibido.roboOrientacao}
                        trajeto={mapaExibido.trajeto}
                    />
                </div>

                <div className="replay-controles" data-testid="replay-controles">
                    <button
                        type="button"
                        className="botao-play-pause"
                        data-testid="botao-play-pause"
                        onClick={alternarReproducao}
                    >
                        <span>{reproduzindo ? '⏸' : '▶'}</span>
                        <span>{reproduzindo ? 'Pause' : 'Play'}</span>
                    </button>

                    <span className="tempo-replay-label" data-testid="tempo-atual-replay">
                        {formatarTempo(tempoAtualMs)}
                    </span>

                    <input
                        type="range"
                        className="barra-tempo-replay"
                        data-testid="barra-tempo-replay"
                        aria-label="Barra de tempo da reprodução"
                        min={0}
                        max={duracaoMaxMs}
                        step={1000}
                        value={tempoAtualMs}
                        onChange={alterarBarraTempo}
                    />

                    <span className="tempo-replay-label" data-testid="tempo-total-replay">
                        {formatarTempo(duracaoMaxMs)}
                    </span>
                </div>
            </div>

            {/* Bloco 2: Estatísticas da Corrida */}
            <div className="replay-card" data-testid="bloco-estatisticas-corrida">
                <h3 className="replay-card-titulo">ESTATÍSTICAS DA CORRIDA (RUN STATS)</h3>
                <div className="estatisticas-grid">
                    <div className="estatistica-item">
                        <span className="estatistica-rotulo">Tempo Final</span>
                        <span className="estatistica-valor" data-testid="stat-tempo-final">
                            {estatisticas.tempoFinal}
                        </span>
                    </div>
                    <div className="estatistica-item">
                        <span className="estatistica-rotulo">Vel. Média</span>
                        <span className="estatistica-valor" data-testid="stat-vel-media">
                            {estatisticas.velocidadeMedia}
                        </span>
                    </div>
                    <div className="estatistica-item">
                        <span className="estatistica-rotulo">Eficiência de Rota</span>
                        <span className="estatistica-valor" data-testid="stat-eficiencia-rota">
                            {estatisticas.eficienciaRota}
                        </span>
                    </div>
                </div>
            </div>

            {/* Bloco 3: Métricas Comparativas */}
            <div className="replay-card" data-testid="bloco-metricas-comparativas">
                <h3 className="replay-card-titulo">MÉTRICAS COMPARATIVAS (COMPARATIVE METRICS)</h3>
                <div className="metricas-lista">
                    <div className="metrica-linha">
                        <span className="metrica-rotulo">Bateria Inicial/Final</span>
                        <span className="metrica-valor" data-testid="metrica-bateria">
                            {estatisticas.bateriaInicialFinal}
                        </span>
                    </div>
                    <div className="metrica-linha">
                        <span className="metrica-rotulo">Tempo Estimado vs. Real</span>
                        <span className="metrica-valor" data-testid="metrica-tempo-estimado">
                            {estatisticas.tempoEstimadoVsReal}
                        </span>
                    </div>
                    <div className="metrica-linha">
                        <span className="metrica-rotulo">Células Novas Descobertas</span>
                        <span className="metrica-valor" data-testid="metrica-celulas">
                            {estatisticas.celulasDescobertas}
                        </span>
                    </div>
                </div>
            </div>
        </div>
    );
}
