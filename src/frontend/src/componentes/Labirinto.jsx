import React from 'react';
import './Labirinto.css';
import {
    calcularCellSize,
    gerarGrid,
    aplicarParedes,
    normalizarOrientacao,
    calcularPosicaoPixelRobo,
    gerarPontosSvg
} from '../servicos/labirinto';

export default function Labirinto({
    linhas = 4,
    paredes = [],
    cells: celulasProp,
    celulas,
    roboPosicao,
    posicao,
    roboOrientacao,
    orientacao,
    trajeto,
    rastro
}) {
    const cellSize = calcularCellSize(linhas);
    const { cells: celulasBase, cols, rowsList } = gerarGrid(linhas);

    // Se celulas pré-computadas forem passadas (ex: por aplicarPacote), usa-as diretamente.
    // Senão, aplica as paredes recebidas sobre o grid base (sem fallback para mock).
    let cellsAtivas = celulasProp || celulas;
    if (!cellsAtivas) {
        cellsAtivas = (paredes && paredes.length > 0)
            ? aplicarParedes(celulasBase, paredes)
            : celulasBase;
    }

    const posicaoRoboAtiva = roboPosicao !== undefined ? roboPosicao : posicao;
    const orientacaoAtiva = roboOrientacao !== undefined ? roboOrientacao : orientacao;
    const trajetoAtivo = rastro !== undefined ? rastro : (trajeto !== undefined ? trajeto : []);

    // Cálculos de renderização
    const roboPosPixel = posicaoRoboAtiva ? calcularPosicaoPixelRobo(posicaoRoboAtiva, cellSize) : null;
    const roboRotacao = normalizarOrientacao(orientacaoAtiva || 'NORTE');
    const pathPoints = gerarPontosSvg(trajetoAtivo, cellSize);

    return (
        <div className="labirinto-container" data-testid="labirinto-container">
            <div className="labirinto-header">
                <div className="header-textos">
                    <h2>{`MAPEAMENTO EM TEMPO REAL - GRID ${linhas}x4`}</h2>
                    <p>{`Grid de ${linhas}x4 • Escaneando perímetro interno`}</p>
                </div>
                <div className="legendas">
                    <span className="legenda-start"><span className="box-start"></span> START</span>
                    <span className="legenda-finish"><span className="box-finish"></span> FINISH</span>
                </div>
            </div>

            <div className="labirinto-wrapper">
                {/* Headers das Colunas (A, B, C, D) */}
                <div className="grid-header-cols" style={{ width: `${cellSize * 4}px`, marginLeft: '40px' }}>
                    {cols.map(col => <div key={col}>{col}</div>)}
                </div>

                <div className="grid-body">
                    {/* Headers das Linhas */}
                    <div className="grid-header-rows">
                        {rowsList.map(row => <div key={row} style={{ height: `${cellSize}px` }}>{row}</div>)}
                    </div>

                    {/* O Labirinto em si */}
                    <div 
                        className="labirinto-grid" 
                        data-testid="labirinto-grid"
                        style={{ 
                            width: `${cellSize * 4}px`, 
                            height: `${cellSize * linhas}px`,
                            gridTemplateColumns: `repeat(4, ${cellSize}px)`,
                            gridTemplateRows: `repeat(${linhas}, ${cellSize}px)`
                        }}
                    >
                        {cellsAtivas.map(cell => (
                            <div 
                                key={cell.id} 
                                data-testid={`celula-${cell.id}`}
                                className={`celula ${cell.isStart ? 'start' : ''} ${cell.isFinish ? 'finish' : ''}`}
                                style={{
                                    borderTop: cell.walls.top ? '3px solid #1a1a1a' : '1px solid #e2e8f0',
                                    borderBottom: cell.walls.bottom ? '3px solid #1a1a1a' : '1px solid #e2e8f0',
                                    borderLeft: cell.walls.left ? '3px solid #1a1a1a' : '1px solid #e2e8f0',
                                    borderRight: cell.walls.right ? '3px solid #1a1a1a' : '1px solid #e2e8f0',
                                }}
                            >
                                {cell.isStart && <span className="label-start">START</span>}
                                {cell.isFinish && <span className="label-finish">FINISH</span>}
                            </div>
                        ))}
                        
                        {pathPoints ? (
                            <svg className="trajeto-overlay" viewBox={`0 0 ${cellSize * 4} ${cellSize * linhas}`}>
                                <polyline points={pathPoints} fill="none" stroke="#1a1a1a" strokeWidth="3" strokeDasharray="6,6" />
                            </svg>
                        ) : null}

                        {posicaoRoboAtiva && roboPosPixel && (
                            <div 
                                className="robo-icon" 
                                data-testid="robo-icon"
                                style={{ top: `${roboPosPixel.y}px`, left: `${roboPosPixel.x}px` }}
                            >
                                <div className="robo-seta" style={{ transform: `rotate(${roboRotacao})` }}>➔</div>
                            </div>
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
}
