import { describe, it, expect } from 'vitest';
import { criarEstadoPainel, iniciaNovaCorrida, reduzirTelemetria } from './painelTelemetria';
import corridas from './fixtures/corridas4x4.json';

// Corridas reais do simulador do backend na pista 4x4 dos slides.
const { completa, perda_conexao: perdaConexao, travado, paredes: PAREDES_DA_PISTA } = corridas;

// Como o simulador com SIMULADOR_REPETIR=1: o seq continua entre as corridas.
function emSequencia(...listas) {
    let seq = 1;
    return listas.flat().map((pacote) => ({ ...pacote, seq: seq++ }));
}

function aplicarTodos(pacotes, estado = criarEstadoPainel()) {
    return pacotes.reduce(reduzirTelemetria, estado);
}

function celula(mapa, id) {
    const x = 'ABCD'.indexOf(id[0]);
    const y = Number(id.slice(1)) - 1;
    return mapa.cells.find((c) => c.x === x && c.y === y);
}

const LADOS = { norte: 'top', sul: 'bottom', leste: 'right', oeste: 'left' };

function visitadas(mapa) {
    return new Set(mapa.rastro.map((p) => `${'ABCD'[p.x]}${p.y + 1}`));
}

const base = {
    labirinto: '4x4',
    status: 'MAPEANDO',
    timestamp_ms: 5000,
    posicao: { coluna: 'B', linha: 2 },
    orientacao: 'SUL',
    paredes: { norte: false, sul: false, leste: false, oeste: false },
};

describe('painelTelemetria.js', () => {
    describe('iniciaNovaCorrida', () => {
        it('o primeiro pacote começa uma corrida', () => {
            expect(iniciaNovaCorrida(null, base)).toBe(true);
        });

        it('começa ao mudar a geometria', () => {
            expect(iniciaNovaCorrida(base, { ...base, labirinto: '8x4' })).toBe(true);
        });

        it.each(['CONCLUIDO', 'TRAVADO', 'COLISAO', 'TIMEOUT', 'MAPEANDO', 'SPEED_RUN'])(
            'começa ao voltar para AGUARDANDO depois de %s',
            (status) => {
                const anterior = { ...base, status };
                // Mesmo tempo do pacote anterior: vale mesmo que o cronômetro não seja zerado.
                expect(iniciaNovaCorrida(anterior, { ...base, status: 'AGUARDANDO' })).toBe(true);
            }
        );

        it('começa quando o tempo de prova volta para trás', () => {
            expect(iniciaNovaCorrida(base, { ...base, timestamp_ms: 1000 })).toBe(true);
        });

        it('não começa ao sair de AGUARDANDO para MAPEANDO', () => {
            const emEspera = { ...base, status: 'AGUARDANDO', timestamp_ms: 0 };
            expect(iniciaNovaCorrida(emEspera, { ...base, timestamp_ms: 1000 })).toBe(false);
        });

        it('não começa com pacotes AGUARDANDO repetidos', () => {
            const emEspera = { ...base, status: 'AGUARDANDO', timestamp_ms: 0 };
            expect(iniciaNovaCorrida(emEspera, { ...emEspera })).toBe(false);
        });

        it('não começa com o cronômetro congelado depois da chegada', () => {
            const concluido = { ...base, status: 'CONCLUIDO', timestamp_ms: 14000 };
            expect(iniciaNovaCorrida(concluido, { ...concluido })).toBe(false);
        });

        it('não começa quando só o seq volta (gateway religado)', () => {
            expect(iniciaNovaCorrida({ ...base, seq: 500 }, { ...base, seq: 1, timestamp_ms: 6000 })).toBe(false);
        });
    });

    describe('reduzirTelemetria', () => {
        it('ignora pacote nulo', () => {
            const estado = criarEstadoPainel();
            expect(reduzirTelemetria(estado, null)).toBe(estado);
        });

        it('guarda o pacote mais recente para os indicadores', () => {
            const estado = aplicarTodos(completa);
            expect(estado.pacote).toBe(completa[completa.length - 1]);
            expect(estado.pacote.status).toBe('CONCLUIDO');
        });

        it('desenha a corrida completa igual à pista nas células visitadas', () => {
            const { mapa } = aplicarTodos(completa);

            expect(mapa.rastro[0]).toEqual({ x: 0, y: 0 }); // começa na largada
            expect(mapa.roboPosicao).toEqual({ x: 3, y: 3 }); // termina em D4
            for (const id of visitadas(mapa)) {
                for (const [lado, chave] of Object.entries(LADOS)) {
                    expect(celula(mapa, id).walls[chave], `${id} ${lado}`).toBe(PAREDES_DA_PISTA[id][lado]);
                }
            }
        });

        it('mantém as paredes da largada vistas em AGUARDANDO', () => {
            const { mapa } = aplicarTodos(completa);
            // A1 é um beco aberto só para o leste; a parede sul só vem nos pacotes AGUARDANDO.
            expect(celula(mapa, 'A1').walls).toEqual({ top: true, bottom: true, left: true, right: false });
        });

        it('recomeça limpo quando a corrida se repete', () => {
            const umaVez = aplicarTodos(emSequencia(completa));
            const pacotes = emSequencia(completa, completa);
            const inicioSegunda = completa.length;

            // Primeiro pacote da segunda corrida (AGUARDANDO em A1): o mapa antigo some.
            const noReinicio = aplicarTodos(pacotes.slice(0, inicioSegunda + 1));
            expect(noReinicio.corrida).toBe(2);
            expect(noReinicio.mapa.rastro).toEqual([{ x: 0, y: 0 }]);
            expect(celula(noReinicio.mapa, 'D4').walls.top).toBe(false);

            // Ao fim, o mapa da segunda corrida é igual ao de uma corrida só.
            const duasVezes = aplicarTodos(pacotes);
            expect(duasVezes.mapa.rastro).toEqual(umaVez.mapa.rastro);
            expect(duasVezes.mapa.cells).toEqual(umaVez.mapa.cells);
        });

        it.each([
            ['perda de conexão', perdaConexao],
            ['travamento', travado],
        ])('recomeça depois de uma corrida interrompida por %s', (_nome, interrompida) => {
            const parcial = aplicarTodos(emSequencia(interrompida));
            const final = aplicarTodos(emSequencia(interrompida, completa));
            const sozinha = aplicarTodos(emSequencia(completa));

            expect(parcial.mapa.roboPosicao).not.toEqual({ x: 3, y: 3 });
            expect(final.corrida).toBe(2);
            expect(final.mapa.rastro).toEqual(sozinha.mapa.rastro);
            expect(final.mapa.cells).toEqual(sozinha.mapa.cells);
        });

        it('recomeça quando os pacotes AGUARDANDO se perdem entre as corridas', () => {
            const semEspera = completa.filter((p) => p.status !== 'AGUARDANDO');
            const final = aplicarTodos(emSequencia(completa, semEspera));

            expect(final.corrida).toBe(2);
            expect(final.mapa.rastro[0]).toEqual({ x: 1, y: 0 }); // primeiro pacote recebido: B1
        });

        it('recomeça ao trocar de geometria', () => {
            const oitoPorQuatro = {
                ...base,
                labirinto: '8x4',
                status: 'AGUARDANDO',
                timestamp_ms: 0,
                posicao: { coluna: 'A', linha: 1 },
            };
            const final = aplicarTodos([...completa, oitoPorQuatro]);

            expect(final.corrida).toBe(2);
            expect(final.mapa.linhas).toBe(8);
            expect(final.mapa.cells).toHaveLength(32);
            expect(final.mapa.rastro).toEqual([{ x: 0, y: 0 }]);
        });

        it('salva corridas finalizadas e interrompidas no histórico com leituras para reprodução (HU-14)', () => {
            const final = aplicarTodos(emSequencia(perdaConexao, completa));
            expect(final.historico).toHaveLength(2);
            expect(final.historico[0].id).toBe('002');
            expect(final.historico[0].status).toBe('CONCLUIDO');
            expect(final.historico[0].leituras.length).toBeGreaterThan(0);
            expect(final.historico[1].id).toBe('001');
        });
    });
});

