import React from 'react';
import './Labirinto.css';

export default function Labirinto({ linhas = 4 }) {
    // Ajusta o tamanho da célula para caber geometrias maiores sem quebrar a tela
    const cellSize = linhas === 4 ? 100 : (linhas === 8 ? 70 : 50);
    const cols = ['A', 'B', 'C', 'D'];
    const rowsList = Array.from({ length: linhas }, (_, i) => String(i + 1).padStart(2, '0'));

    const cells = Array.from({ length: 4 * linhas }, (_, i) => {
        const x = i % 4;
        const y = Math.floor(i / 4);
        return {
            id: `${cols[x]}${rowsList[y]}`,
            x,
            y,
            isStart: x === 0 && y === 0,
            isFinish: x === 3 && y === linhas - 1,
            walls: {
                top: y === 0,
                bottom: y === linhas - 1,
                left: x === 0,
                right: x === 3
            }
        };
    });

    const findCell = (id) => cells.find(c => c && c.id === id);
    const setWall = (id1, id2, dir1, dir2) => {
        const c1 = findCell(id1);
        const c2 = findCell(id2);
        if(c1) c1.walls[dir1] = true;
        if(c2) c2.walls[dir2] = true;
    };

    let pathPoints = "";
    let roboPos = { x: 0, y: 0 };
    let roboRotacao = "0deg"; // seta padrão ➔ (direita)

    // Mocks específicos para cada geometria (baseado nas imagens)
    if (linhas === 4) {
        setWall('C01', 'C02', 'bottom', 'top');
        setWall('C02', 'D02', 'right', 'left');
        setWall('B03', 'C03', 'right', 'left');
        setWall('C03', 'C04', 'bottom', 'top');
        setWall('A03', 'A04', 'bottom', 'top');
        pathPoints = `${0.5*cellSize},${0.5*cellSize} ${0.5*cellSize},${1.5*cellSize} ${2.5*cellSize},${1.5*cellSize}`;
        roboPos = { x: 2.5 * cellSize, y: 1.5 * cellSize };
        roboRotacao = "0deg"; // Apontando para a direita
    } else if (linhas === 8) {
        setWall('A02', 'A03', 'bottom', 'top');
        setWall('B03', 'B04', 'bottom', 'top');
        setWall('C02', 'D02', 'right', 'left');
        setWall('A05', 'B05', 'right', 'left');
        setWall('C05', 'D05', 'right', 'left');
        setWall('A06', 'A07', 'bottom', 'top');
        pathPoints = `${0.5*cellSize},${0.5*cellSize} ${1.5*cellSize},${0.5*cellSize} ${1.5*cellSize},${2.5*cellSize}`;
        roboPos = { x: 1.5 * cellSize, y: 2.5 * cellSize };
        roboRotacao = "90deg"; // Apontando para baixo
    } else if (linhas === 12) {
        // Paredes corrigidas para não cruzar o trajeto A01 -> A02 -> C02 -> C05
        setWall('B01', 'C01', 'right', 'left');
        setWall('C01', 'C02', 'bottom', 'top');
        setWall('A02', 'A03', 'bottom', 'top');
        setWall('C02', 'D02', 'right', 'left');
        setWall('C04', 'D04', 'right', 'left');
        setWall('A04', 'A05', 'bottom', 'top');
        setWall('B05', 'C05', 'right', 'left');
        setWall('A06', 'A07', 'bottom', 'top');
        setWall('C07', 'C08', 'bottom', 'top');
        setWall('A09', 'A10', 'bottom', 'top');
        setWall('B11', 'B12', 'bottom', 'top');
        pathPoints = `${0.5*cellSize},${0.5*cellSize} ${0.5*cellSize},${1.5*cellSize} ${2.5*cellSize},${1.5*cellSize} ${2.5*cellSize},${4.5*cellSize}`;
        roboPos = { x: 2.5 * cellSize, y: 4.5 * cellSize };
        roboRotacao = "90deg"; // Apontando para baixo
    }

    return (
        <div className="labirinto-container">
            <div className="labirinto-header">
                <div className="header-textos">
                    <h2>MAPEAMENTO EM TEMPO REAL - GRID {linhas}x4</h2>
                    <p>Grid de {linhas}x4 • Escaneando perímetro interno</p>
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
                        style={{ 
                            width: `${cellSize * 4}px`, 
                            height: `${cellSize * linhas}px`,
                            gridTemplateColumns: `repeat(4, ${cellSize}px)`,
                            gridTemplateRows: `repeat(${linhas}, ${cellSize}px)`
                        }}
                    >
                        {cells.map(cell => (
                            <div 
                                key={cell.id} 
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
                        
                        <svg className="trajeto-overlay" viewBox={`0 0 ${cellSize * 4} ${cellSize * linhas}`}>
                            <polyline points={pathPoints} fill="none" stroke="#1a1a1a" strokeWidth="3" strokeDasharray="6,6" />
                        </svg>

                        <div className="robo-icon" style={{ top: `${roboPos.y}px`, left: `${roboPos.x}px` }}>
                            <div className="robo-seta" style={{ transform: `rotate(${roboRotacao})` }}>➔</div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}

