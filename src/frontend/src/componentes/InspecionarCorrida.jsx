import React, { useState, useEffect, useMemo } from 'react';
import Labirinto from './Labirinto';
import {
    construirReproducaoCorrida,
    obterEstadoReproducaoNoPasso,
    extrairEstatisticasCorrida
} from '../servicos/reproducaoCorrida';

export default function InspecionarCorrida({
    corrida = null,
    passoInicial = undefined,
    intervaloMs = 700
}) {
    const reproducao = useMemo(() => construirReproducaoCorrida(corrida), [corrida]);
    const estatisticas = useMemo(
        () => extrairEstatisticasCorrida(corrida, reproducao),
        [corrida, reproducao]
    );

    const ultimoIndice = Math.max(0, reproducao.quadros.length - 1);
    const [indicePasso, setIndicePasso] = useState(
        typeof passoInicial === 'number' ? passoInicial : ultimoIndice
    );
    const [reproduzindo, setReproduzindo] = useState(false);

    // Ao trocar de corrida selecionada, exibe o estado final completo e pausa a reprodução
    useEffect(() => {
        setReproduzindo(false);
        setIndicePasso(
            typeof passoInicial === 'number'
                ? passoInicial
                : Math.max(0, reproducao.quadros.length - 1)
        );
    }, [corrida, reproducao.quadros.length, passoInicial]);

    // Avança os quadros em ordem temporal enquanto estiver reproduzindo
    useEffect(() => {
        if (!reproduzindo || reproducao.quadros.length <= 1) return undefined;

        const timer = setInterval(() => {
            setIndicePasso((atual) => {
                if (atual >= reproducao.quadros.length - 1) {
                    setReproduzindo(false);
                    return atual;
                }
                return atual + 1;
            });
        }, intervaloMs);

        return () => clearInterval(timer);
    }, [reproduzindo, reproducao.quadros.length, intervaloMs]);

    if (!corrida) {
        return (
            <div className="inspecao-coluna" data-testid="inspecionar-vazio">
                <div className="inspecao-card">
                    <h3 className="inspecao-titulo">INSPECIONAR CORRIDA</h3>
                    <p className="inspecao-mensagem-vazia">
                        Selecione uma corrida na tabela para visualizar o mapa final, reproduzir o trajeto e consultar as estatísticas.
                    </p>
                </div>
            </div>
        );
    }

    const estadoPasso = obterEstadoReproducaoNoPasso(reproducao, indicePasso);

    const alternarReproducao = () => {
        if (reproduzindo) {
            setReproduzindo(false);
        } else {
            // Se já estiver no último quadro, reinicia do começo para percorrer em ordem de tempo
            if (estadoPasso.indiceAtual >= estadoPasso.totalPassos - 1) {
                setIndicePasso(0);
            }
            setReproduzindo(true);
        }
    };

    const aoMudarBarraTempo = (e) => {
        const novoIndice = parseInt(e.target.value, 10);
        setReproduzindo(false);
        setIndicePasso(Number.isNaN(novoIndice) ? 0 : novoIndice);
    };

    return (
        <div className="inspecao-coluna" data-testid="inspecionar-corrida">
            {/* Bloco 1: Mapa e Reprodução do Trajeto */}
            <div className="inspecao-card" data-testid="card-inspecionar-mapa">
                <div className="inspecao-card-header">
                    <h3 className="inspecao-titulo">
                        {`INSPECIONAR CORRIDA - #${estatisticas.id}`}
                    </h3>
                    <span className="inspecao-dot">•</span>
                </div>

                <div className="inspecao-mapa-wrapper">
                    <Labirinto
                        linhas={estadoPasso.linhas}
                        cells={estadoPasso.cells}
                        roboPosicao={estadoPasso.roboPosicao}
                        roboOrientacao={estadoPasso.roboOrientacao}
                        trajeto={estadoPasso.trajeto}
                        ocultarCabecalho={true}
                    />
                </div>

                <div className="reproducao-controles" data-testid="reproducao-controles">
                    <button
                        type="button"
                        className="botao-play-pause"
                        data-testid="botao-play-pause"
                        onClick={alternarReproducao}
                    >
                        {reproduzindo ? '⏸ Pausar' : '▶ Reproduzir'}
                    </button>

                    <span className="reproducao-tempo" data-testid="tempo-atual-reproducao">
                        {estadoPasso.tempoAtualFormatado}
                    </span>

                    <input
                        type="range"
                        aria-label="Barra de tempo da reprodução"
                        className="barra-tempo-reproducao"
                        data-testid="barra-tempo-reproducao"
                        min={0}
                        max={Math.max(0, estadoPasso.totalPassos - 1)}
                        value={estadoPasso.indiceAtual}
                        onChange={aoMudarBarraTempo}
                    />

                    <span className="reproducao-tempo" data-testid="tempo-total-reproducao">
                        {estadoPasso.tempoTotalFormatado}
                    </span>
                </div>
            </div>

            {/* Bloco 2: Estatísticas da Corrida */}
            <div className="inspecao-card" data-testid="estatisticas-corrida">
                <h3 className="inspecao-titulo">ESTATÍSTICAS DA CORRIDA</h3>
                <div className="estatisticas-grid">
                    <div className="estatistica-box" data-testid="stat-tempo-final">
                        <span className="estatistica-label">Tempo Final</span>
                        <strong className="estatistica-valor">{estatisticas.tempoFinal}</strong>
                    </div>
                    <div className="estatistica-box" data-testid="stat-vel-media">
                        <span className="estatistica-label">Vel. Média</span>
                        <strong className="estatistica-valor">{estatisticas.velMedia}</strong>
                    </div>
                    <div className="estatistica-box" data-testid="stat-eficiencia">
                        <span className="estatistica-label">Eficiência de Rota</span>
                        <strong className="estatistica-valor">{estatisticas.eficienciaRota}</strong>
                    </div>
                </div>
            </div>

            {/* Bloco 3: Métricas Comparativas */}
            <div className="inspecao-card" data-testid="metricas-comparativas">
                <h3 className="inspecao-titulo">MÉTRICAS COMPARATIVAS</h3>
                <div className="metricas-lista">
                    <div className="metrica-linha" data-testid="metrica-bateria">
                        <span>Bateria Inicial/Final</span>
                        <strong>{estatisticas.bateriaInicialFinal}</strong>
                    </div>
                    <div className="metrica-linha" data-testid="metrica-celulas-descobertas">
                        <span>Células Novas Descobertas</span>
                        <strong>{estatisticas.celulasNovasDescobertas}</strong>
                    </div>
                </div>
            </div>
        </div>
    );
}
