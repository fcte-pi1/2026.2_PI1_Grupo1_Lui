export const COLUNAS_PADRAO = ['A', 'B', 'C', 'D'];

export function calcularCellSize(linhas = 4) {
    if (linhas <= 4) return 100;
    if (linhas <= 8) return 70;
    return 50;
}

export function gerarGrid(linhas = 4, cols = COLUNAS_PADRAO) {
    const totalLinhas = Math.max(1, linhas);
    const rowsList = Array.from({ length: totalLinhas }, (_, i) => String(i + 1).padStart(2, '0'));

    const cells = Array.from({ length: cols.length * totalLinhas }, (_, i) => {
        const x = i % cols.length;
        const y = Math.floor(i / cols.length);
        return {
            id: `${cols[x]}${rowsList[y]}`,
            x,
            y,
            isStart: x === 0 && y === 0,
            isFinish: x === cols.length - 1 && y === totalLinhas - 1,
            walls: {
                top: y === 0,
                bottom: y === totalLinhas - 1,
                left: x === 0,
                right: x === cols.length - 1
            }
        };
    });

    return { cells, cols, rowsList };
}

export function aplicarParedes(cells, paredes = []) {
    // Clona células profundamente para manter função pura
    const celulasAtualizadas = cells.map(c => ({
        ...c,
        walls: { ...c.walls }
    }));

    const findCell = (id) => celulasAtualizadas.find(c => c && c.id === id);

    for (const parede of paredes) {
        if (!parede) continue;

        // Formato entre duas células adjacentes: { id1, id2, dir1, dir2 }
        if (parede.id1 && parede.id2 && parede.dir1 && parede.dir2) {
            const c1 = findCell(parede.id1);
            const c2 = findCell(parede.id2);
            if (c1) c1.walls[parede.dir1] = true;
            if (c2) c2.walls[parede.dir2] = true;
        } else if (parede.id && parede.dir) {
            // Formato de parede em célula única: { id, dir }
            const c = findCell(parede.id);
            if (c) c.walls[parede.dir] = true;
        }
    }

    return celulasAtualizadas;
}

export function normalizarOrientacao(orientacao) {
    if (typeof orientacao === 'number') {
        return `${orientacao}deg`;
    }
    if (typeof orientacao === 'string') {
        const o = orientacao.trim().toUpperCase();
        if (o.endsWith('DEG')) return orientacao.trim();
        if (o === 'N' || o === 'NORTE') return '270deg';
        if (o === 'S' || o === 'SUL') return '90deg';
        if (o === 'L' || o === 'LESTE' || o === 'E' || o === 'EAST') return '0deg';
        if (o === 'O' || o === 'OESTE' || o === 'W' || o === 'WEST') return '180deg';
    }
    return '0deg';
}

export function calcularPosicaoPixelRobo(roboPos, cellSize) {
    if (!roboPos) return { x: 0.5 * cellSize, y: 0.5 * cellSize };
    // Coordenadas em unidades de célula (0..cols-1, 0..linhas-1)
    const x = (roboPos.x + 0.5) * cellSize;
    const y = (roboPos.y + 0.5) * cellSize;
    return { x, y };
}

export function gerarPontosSvg(trajeto, cellSize) {
    if (!trajeto) return '';
    if (typeof trajeto === 'string') return trajeto;
    if (Array.isArray(trajeto)) {
        return trajeto
            .map(pt => `${(pt.x + 0.5) * cellSize},${(pt.y + 0.5) * cellSize}`)
            .join(' ');
    }
    return '';
}

export function obterCenarioMock(linhas = 4) {
    if (linhas === 8) {
        return {
            paredes: [
                { id1: 'A02', id2: 'A03', dir1: 'bottom', dir2: 'top' },
                { id1: 'B03', id2: 'B04', dir1: 'bottom', dir2: 'top' },
                { id1: 'C02', id2: 'D02', dir1: 'right', dir2: 'left' },
                { id1: 'A05', id2: 'B05', dir1: 'right', dir2: 'left' },
                { id1: 'C05', id2: 'D05', dir1: 'right', dir2: 'left' },
                { id1: 'A06', id2: 'A07', dir1: 'bottom', dir2: 'top' }
            ],
            trajeto: [
                { x: 0, y: 0 },
                { x: 1, y: 0 },
                { x: 1, y: 2 }
            ],
            roboPos: { x: 1, y: 2 },
            roboRotacao: '90deg'
        };
    }

    if (linhas === 12) {
        return {
            paredes: [
                { id1: 'B01', id2: 'C01', dir1: 'right', dir2: 'left' },
                { id1: 'C01', id2: 'C02', dir1: 'bottom', dir2: 'top' },
                { id1: 'A02', id2: 'A03', dir1: 'bottom', dir2: 'top' },
                { id1: 'C02', id2: 'D02', dir1: 'right', dir2: 'left' },
                { id1: 'C04', id2: 'D04', dir1: 'right', dir2: 'left' },
                { id1: 'A04', id2: 'A05', dir1: 'bottom', dir2: 'top' },
                { id1: 'B05', id2: 'C05', dir1: 'right', dir2: 'left' },
                { id1: 'A06', id2: 'A07', dir1: 'bottom', dir2: 'top' },
                { id1: 'C07', id2: 'C08', dir1: 'bottom', dir2: 'top' },
                { id1: 'A09', id2: 'A10', dir1: 'bottom', dir2: 'top' },
                { id1: 'B11', id2: 'B12', dir1: 'bottom', dir2: 'top' }
            ],
            trajeto: [
                { x: 0, y: 0 },
                { x: 0, y: 1 },
                { x: 2, y: 1 },
                { x: 2, y: 4 }
            ],
            roboPos: { x: 2, y: 4 },
            roboRotacao: '90deg'
        };
    }

    // Default 4x4
    return {
        paredes: [
            { id1: 'C01', id2: 'C02', dir1: 'bottom', dir2: 'top' },
            { id1: 'C02', id2: 'D02', dir1: 'right', dir2: 'left' },
            { id1: 'B03', id2: 'C03', dir1: 'right', dir2: 'left' },
            { id1: 'C03', id2: 'C04', dir1: 'bottom', dir2: 'top' },
            { id1: 'A03', id2: 'A04', dir1: 'bottom', dir2: 'top' },
            { id1: 'C04', id2: 'D04', dir1: 'right', dir2: 'left' }
        ],
        trajeto: [
            { x: 0, y: 0 },
            { x: 0, y: 1 },
            { x: 2, y: 1 }
        ],
        roboPos: { x: 2, y: 1 },
        roboRotacao: '0deg'
    };
}
