export const COLUNAS_PADRAO = ['A', 'B', 'C', 'D'];

export function calcularCellSize(linhas = 4) {
    if (linhas <= 4) return 100;
    if (linhas <= 8) return 70;
    return 50;
}

export function parseLinhas(labirinto = '4x4') {
    if (typeof labirinto === 'number') return labirinto;
    if (typeof labirinto === 'string') {
        const match = labirinto.match(/^(\d+)x/i);
        if (match) return parseInt(match[1], 10);
        const num = parseInt(labirinto, 10);
        if (!isNaN(num)) return num;
    }
    return 4;
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

export function criarEstadoInicial(labirinto = '4x4') {
    const linhas = parseLinhas(labirinto);
    const { cells } = gerarGrid(linhas);
    return {
        linhas,
        geometria: typeof labirinto === 'string' ? labirinto : `${linhas}x4`,
        cells,
        celulas: cells,
        posicao: null,
        roboPosicao: null,
        orientacao: 'NORTE',
        roboOrientacao: 'NORTE',
        rastro: [],
        trajeto: [],
        ultimoStatus: 'AGUARDANDO'
    };
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

/**
 * Função pura que consome mensagens de telemetria no formato da Seção 6.4 do contrato.
 * Registra as paredes da célula atual e a parede simétrica na vizinha,
 * adiciona a posição ao rastro e atualiza posição e orientação.
 * A geometria do mapa vem do campo labirinto.
 * Quando começa uma corrida nova é decidido em painelTelemetria.js
 * (iniciaNovaCorrida); aqui o mapa só é recriado se a geometria mudar.
 */
export function aplicarPacote(estado, pacote) {
    if (!pacote) {
        return estado || criarEstadoInicial('4x4');
    }

    const linhas = parseLinhas(pacote.labirinto || estado?.linhas || 4);

    // Se o estado não existe ou mudou de geometria, inicializa
    let cellsBase;
    const mudouGeometria = !estado || !estado.cells || estado.linhas !== linhas;

    if (mudouGeometria) {
        const gridNovo = gerarGrid(linhas);
        cellsBase = gridNovo.cells;
    } else {
        cellsBase = estado.cells;
    }

    // Clona as células para garantir pureza da função
    const cellsClonadas = cellsBase.map(c => ({
        ...c,
        walls: { ...c.walls }
    }));

    // Determina x e y a partir de pacote.posicao ({ coluna, linha } ou { x, y })
    let x = 0;
    let y = 0;
    if (pacote.posicao) {
        if (typeof pacote.posicao.coluna === 'string') {
            const col = pacote.posicao.coluna.trim().toUpperCase();
            x = col.charCodeAt(0) - 'A'.charCodeAt(0);
        } else if (typeof pacote.posicao.x === 'number') {
            x = pacote.posicao.x;
        }

        if (pacote.posicao.linha !== undefined) {
            y = parseInt(pacote.posicao.linha, 10) - 1;
        } else if (typeof pacote.posicao.y === 'number') {
            y = pacote.posicao.y;
        }
    }

    // Garante que x e y estão dentro dos limites da grade
    x = Math.max(0, Math.min(x, 3));
    y = Math.max(0, Math.min(y, linhas - 1));

    const celulaAtual = cellsClonadas.find(c => c.x === x && c.y === y);

    // Registra paredes da célula atual e na vizinha correspondente
    if (celulaAtual && pacote.paredes) {
        const { norte, sul, leste, oeste } = pacote.paredes;

        if (norte) {
            celulaAtual.walls.top = true;
            if (y > 0) {
                const vizinhaNorte = cellsClonadas.find(c => c.x === x && c.y === y - 1);
                if (vizinhaNorte) vizinhaNorte.walls.bottom = true;
            }
        }

        if (sul) {
            celulaAtual.walls.bottom = true;
            if (y + 1 < linhas) {
                const vizinhaSul = cellsClonadas.find(c => c.x === x && c.y === y + 1);
                if (vizinhaSul) vizinhaSul.walls.top = true;
            }
        }

        if (leste) {
            celulaAtual.walls.right = true;
            if (x + 1 < 4) {
                const vizinhaLeste = cellsClonadas.find(c => c.x === x + 1 && c.y === y);
                if (vizinhaLeste) vizinhaLeste.walls.left = true;
            }
        }

        if (oeste) {
            celulaAtual.walls.left = true;
            if (x > 0) {
                const vizinhaOeste = cellsClonadas.find(c => c.x === x - 1 && c.y === y);
                if (vizinhaOeste) vizinhaOeste.walls.right = true;
            }
        }
    }

    // Atualiza rastro (sem duplicar pontos consecutivos na mesma célula)
    const rastroAnterior = mudouGeometria ? [] : (estado?.rastro || estado?.trajeto || []);
    const ultimoPonto = rastroAnterior[rastroAnterior.length - 1];
    let novoRastro;
    if (!ultimoPonto || ultimoPonto.x !== x || ultimoPonto.y !== y) {
        novoRastro = [...rastroAnterior, { x, y }];
    } else {
        novoRastro = [...rastroAnterior];
    }

    const novaOrientacao = pacote.orientacao || estado?.orientacao || 'NORTE';
    const colunaLetra = COLUNAS_PADRAO[x] || 'A';
    const linhaNum = y + 1;

    return {
        linhas,
        geometria: typeof pacote.labirinto === 'string' ? pacote.labirinto : `${linhas}x4`,
        cells: cellsClonadas,
        celulas: cellsClonadas,
        posicao: { x, y, coluna: colunaLetra, linha: linhaNum },
        roboPosicao: { x, y },
        orientacao: novaOrientacao,
        roboOrientacao: novaOrientacao,
        rastro: novoRastro,
        trajeto: novoRastro,
        ultimoStatus: pacote.status || estado?.ultimoStatus
    };
}

export const acumularPacote = aplicarPacote;

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

/**
 * Sequências simuladas de mensagens no formato 6.4 do contrato.
 * Em cada passo, as paredes enviadas refletem unicamente a célula onde o robô está naquele instante.
 */
export const SEQUENCIAS_SIMULADAS_6_4 = {
    4: [
        {
            timestamp_ms: 1000,
            labirinto: "4x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 98,
            bateria_volts: 8.35,
            velocidade_atual: 30.0,
            velocidade_media: 30.0,
            posicao: { coluna: "A", linha: 1 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 2000,
            labirinto: "4x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 96,
            bateria_volts: 8.30,
            velocidade_atual: 32.0,
            velocidade_media: 31.0,
            posicao: { coluna: "A", linha: 2 },
            orientacao: "LESTE",
            paredes: { norte: false, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 3000,
            labirinto: "4x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 94,
            bateria_volts: 8.24,
            velocidade_atual: 30.0,
            velocidade_media: 30.7,
            posicao: { coluna: "B", linha: 2 },
            orientacao: "LESTE",
            paredes: { norte: true, sul: true, leste: false, oeste: false }
        },
        {
            timestamp_ms: 4000,
            labirinto: "4x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 92,
            bateria_volts: 8.18,
            velocidade_atual: 28.0,
            velocidade_media: 30.0,
            posicao: { coluna: "C", linha: 2 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: false }
        },
        {
            timestamp_ms: 5000,
            labirinto: "4x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 90,
            bateria_volts: 8.12,
            velocidade_atual: 30.0,
            velocidade_media: 30.0,
            posicao: { coluna: "C", linha: 3 },
            orientacao: "LESTE",
            paredes: { norte: false, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 6000,
            labirinto: "4x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 88,
            bateria_volts: 8.05,
            velocidade_atual: 32.0,
            velocidade_media: 30.3,
            posicao: { coluna: "D", linha: 3 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: false }
        },
        {
            timestamp_ms: 7000,
            labirinto: "4x4",
            status: "CONCLUIDO",
            desafio_cumprido: true,
            bateria_pct: 86,
            bateria_volts: 8.00,
            velocidade_atual: 0.0,
            velocidade_media: 29.5,
            posicao: { coluna: "D", linha: 4 },
            orientacao: "SUL",
            paredes: { norte: false, sul: true, leste: true, oeste: false }
        }
    ],
    8: [
        {
            timestamp_ms: 1000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 98,
            bateria_volts: 8.35,
            velocidade_atual: 30.0,
            velocidade_media: 30.0,
            posicao: { coluna: "A", linha: 1 },
            orientacao: "LESTE",
            paredes: { norte: true, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 2000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 96,
            bateria_volts: 8.30,
            velocidade_atual: 30.0,
            velocidade_media: 30.0,
            posicao: { coluna: "B", linha: 1 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: false }
        },
        {
            timestamp_ms: 3000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 94,
            bateria_volts: 8.24,
            velocidade_atual: 32.0,
            velocidade_media: 30.7,
            posicao: { coluna: "B", linha: 2 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 4000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 92,
            bateria_volts: 8.18,
            velocidade_atual: 30.0,
            velocidade_media: 30.5,
            posicao: { coluna: "B", linha: 3 },
            orientacao: "LESTE",
            paredes: { norte: false, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 5000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 90,
            bateria_volts: 8.12,
            velocidade_atual: 28.0,
            velocidade_media: 30.0,
            posicao: { coluna: "C", linha: 3 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: false }
        },
        {
            timestamp_ms: 6000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 88,
            bateria_volts: 8.06,
            velocidade_atual: 32.0,
            velocidade_media: 30.3,
            posicao: { coluna: "C", linha: 4 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 7000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 86,
            bateria_volts: 8.00,
            velocidade_atual: 32.0,
            velocidade_media: 30.5,
            posicao: { coluna: "C", linha: 5 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 8000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 84,
            bateria_volts: 7.94,
            velocidade_atual: 30.0,
            velocidade_media: 30.5,
            posicao: { coluna: "C", linha: 6 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 9000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 82,
            bateria_volts: 7.88,
            velocidade_atual: 30.0,
            velocidade_media: 30.4,
            posicao: { coluna: "C", linha: 7 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 10000,
            labirinto: "8x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 80,
            bateria_volts: 7.82,
            velocidade_atual: 30.0,
            velocidade_media: 30.4,
            posicao: { coluna: "C", linha: 8 },
            orientacao: "LESTE",
            paredes: { norte: false, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 11000,
            labirinto: "8x4",
            status: "CONCLUIDO",
            desafio_cumprido: true,
            bateria_pct: 78,
            bateria_volts: 7.76,
            velocidade_atual: 0.0,
            velocidade_media: 29.8,
            posicao: { coluna: "D", linha: 8 },
            orientacao: "LESTE",
            paredes: { norte: true, sul: true, leste: true, oeste: false }
        }
    ],
    12: [
        {
            timestamp_ms: 1000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 98,
            bateria_volts: 8.35,
            velocidade_atual: 30.0,
            velocidade_media: 30.0,
            posicao: { coluna: "A", linha: 1 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 2000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 96,
            bateria_volts: 8.30,
            velocidade_atual: 30.0,
            velocidade_media: 30.0,
            posicao: { coluna: "A", linha: 2 },
            orientacao: "LESTE",
            paredes: { norte: false, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 3000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 94,
            bateria_volts: 8.24,
            velocidade_atual: 32.0,
            velocidade_media: 30.7,
            posicao: { coluna: "B", linha: 2 },
            orientacao: "LESTE",
            paredes: { norte: true, sul: true, leste: false, oeste: false }
        },
        {
            timestamp_ms: 4000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 92,
            bateria_volts: 8.18,
            velocidade_atual: 30.0,
            velocidade_media: 30.5,
            posicao: { coluna: "C", linha: 2 },
            orientacao: "SUL",
            paredes: { norte: true, sul: false, leste: true, oeste: false }
        },
        {
            timestamp_ms: 5000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 90,
            bateria_volts: 8.12,
            velocidade_atual: 30.0,
            velocidade_media: 30.4,
            posicao: { coluna: "C", linha: 3 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 6000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 88,
            bateria_volts: 8.06,
            velocidade_atual: 30.0,
            velocidade_media: 30.3,
            posicao: { coluna: "C", linha: 4 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 7000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 86,
            bateria_volts: 8.00,
            velocidade_atual: 30.0,
            velocidade_media: 30.3,
            posicao: { coluna: "C", linha: 5 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 8000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 84,
            bateria_volts: 7.94,
            velocidade_atual: 30.0,
            velocidade_media: 30.3,
            posicao: { coluna: "C", linha: 6 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 9000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 82,
            bateria_volts: 7.88,
            velocidade_atual: 30.0,
            velocidade_media: 30.2,
            posicao: { coluna: "C", linha: 7 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 10000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 80,
            bateria_volts: 7.82,
            velocidade_atual: 30.0,
            velocidade_media: 30.2,
            posicao: { coluna: "C", linha: 8 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 11000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 78,
            bateria_volts: 7.76,
            velocidade_atual: 30.0,
            velocidade_media: 30.2,
            posicao: { coluna: "C", linha: 9 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 12000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 76,
            bateria_volts: 7.70,
            velocidade_atual: 30.0,
            velocidade_media: 30.2,
            posicao: { coluna: "C", linha: 10 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 13000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 74,
            bateria_volts: 7.64,
            velocidade_atual: 30.0,
            velocidade_media: 30.1,
            posicao: { coluna: "C", linha: 11 },
            orientacao: "SUL",
            paredes: { norte: false, sul: false, leste: true, oeste: true }
        },
        {
            timestamp_ms: 14000,
            labirinto: "12x4",
            status: "MAPEANDO",
            desafio_cumprido: false,
            bateria_pct: 72,
            bateria_volts: 7.58,
            velocidade_atual: 30.0,
            velocidade_media: 30.1,
            posicao: { coluna: "C", linha: 12 },
            orientacao: "LESTE",
            paredes: { norte: false, sul: true, leste: false, oeste: true }
        },
        {
            timestamp_ms: 15000,
            labirinto: "12x4",
            status: "CONCLUIDO",
            desafio_cumprido: true,
            bateria_pct: 70,
            bateria_volts: 7.52,
            velocidade_atual: 0.0,
            velocidade_media: 29.5,
            posicao: { coluna: "D", linha: 12 },
            orientacao: "LESTE",
            paredes: { norte: true, sul: true, leste: true, oeste: false }
        }
    ]
};

// Mantido para compatibilidade retroativa com testes anteriores
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
