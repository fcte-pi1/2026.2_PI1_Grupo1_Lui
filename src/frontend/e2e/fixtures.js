import { spawn } from 'node:child_process';
import { existsSync, mkdirSync, mkdtempSync } from 'node:fs';
import { createServer } from 'node:net';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { test as base, expect } from '@playwright/test';

// Fixtures dos testes E2E: um backend com o simulador por teste, numa porta livre,
// e o painel apontado para ele.

const RAIZ = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../..');
export const PASTA_BACKEND = path.join(RAIZ, 'src', 'backend');
export const PASTA_EVIDENCIAS = path.join(RAIZ, 'docs', 'evidencias', '5.3.2.1');

// Uma mensagem a cada 200 ms: rápido, mas ainda dá para ver o mapa crescer.
const VELOCIDADE_PADRAO = 5;

// Python do backend: E2E_PYTHON (usado no CI) ou o do .venv de src/backend.
export function pythonDoBackend() {
    if (process.env.E2E_PYTHON) return process.env.E2E_PYTHON;
    const venv = process.platform === 'win32'
        ? path.join(PASTA_BACKEND, '.venv', 'Scripts', 'python.exe')
        : path.join(PASTA_BACKEND, '.venv', 'bin', 'python');
    return existsSync(venv) ? venv : 'python';
}

function portaLivre() {
    return new Promise((resolve, reject) => {
        const servidor = createServer();
        servidor.unref();
        servidor.on('error', reject);
        servidor.listen(0, '127.0.0.1', () => {
            const { port } = servidor.address();
            servidor.close(() => resolve(port));
        });
    });
}

async function esperarSaude(porta, processo, prazoMs = 20_000) {
    const limite = Date.now() + prazoMs;
    while (Date.now() < limite) {
        if (processo.exitCode !== null) throw new Error(`backend encerrou com código ${processo.exitCode}`);
        try {
            const resposta = await fetch(`http://127.0.0.1:${porta}/health`);
            if (resposta.ok) return;
        } catch {
            // ainda subindo
        }
        await new Promise((r) => setTimeout(r, 200));
    }
    throw new Error(`backend não respondeu em ${prazoMs} ms na porta ${porta}`);
}

// Sobe o uvicorn com o simulador. ligar()/desligar() permitem derrubar e subir de
// novo na mesma porta (CT-08). `fonte` e `ambiente` trocam a fonte da telemetria e
// acrescentam variáveis (ex.: DATABASE_URL de um banco já populado).
async function criarBackend({
    labirinto = '4x4', cenario = 'completa', repetir = false, velocidade = VELOCIDADE_PADRAO,
    fonte = 'simulador', ambiente = {},
}) {
    const porta = await portaLivre();
    // Cada backend grava num SQLite próprio: os testes rodam em paralelo e o
    // histórico de um não aparece no painel do outro. ligar() de novo (CT-08)
    // reaproveita o mesmo arquivo, como um backend reiniciado.
    const pastaBanco = mkdtempSync(path.join(tmpdir(), 'e2e-backend-'));
    const bancoPadrao = `sqlite:///${path.join(pastaBanco, 'corridas.db').replaceAll('\\', '/')}`;
    let processo = null;
    let log = '';

    async function ligar() {
        processo = spawn(
            pythonDoBackend(),
            ['-m', 'uvicorn', 'app.main:app', '--host', '127.0.0.1', '--port', String(porta)],
            {
                cwd: PASTA_BACKEND,
                env: {
                    ...process.env,
                    TELEMETRIA_FONTE: fonte,
                    SIMULADOR_LABIRINTO: labirinto,
                    SIMULADOR_CENARIO: cenario,
                    SIMULADOR_REPETIR: repetir ? '1' : '0',
                    SIMULADOR_VELOCIDADE: String(velocidade),
                    DATABASE_URL: bancoPadrao,
                    ...ambiente,
                },
                stdio: ['ignore', 'pipe', 'pipe'],
            }
        );
        processo.stdout.on('data', (d) => { log += d; });
        processo.stderr.on('data', (d) => { log += d; });
        try {
            await esperarSaude(porta, processo);
        } catch (erro) {
            // O backend ainda não entrou na lista do teste: o log vai junto do erro.
            processo.kill();
            throw new Error(`${erro.message}\nlog do backend:\n${log.slice(-3000)}`);
        }
    }

    async function desligar() {
        if (!processo || processo.exitCode !== null) return;
        const encerrado = new Promise((r) => processo.once('exit', r));
        processo.kill();
        await encerrado;
    }

    await ligar();
    return { porta, ligar, desligar, log: () => log };
}

export const test = base.extend({
    // Uso: const backend = await iniciarBackend({ labirinto: '8x4' });
    // O painel da página do teste passa a usar a porta desse backend.
    iniciarBackend: async ({ page }, use, testInfo) => {
        const backends = [];
        await use(async (opcoes = {}) => {
            const backend = await criarBackend(opcoes);
            backends.push(backend);
            // O painel usa <host>:8000 no WebSocket e na API REST; aqui só o
            // navegador do teste troca a porta pela do backend deste teste.
            await page.addInitScript((porta) => {
                const trocarPorta = (url) => String(url).replace(':8000/', `:${porta}/`);
                const Original = window.WebSocket;
                window.WebSocket = class extends Original {
                    constructor(url, protocolos) {
                        super(trocarPorta(url), protocolos);
                    }
                };
                const fetchOriginal = window.fetch.bind(window);
                window.fetch = (recurso, opcoes) => fetchOriginal(
                    typeof recurso === 'string' || recurso instanceof URL ? trocarPorta(recurso) : recurso,
                    opcoes
                );
            }, backend.porta);
            return backend;
        });
        for (const backend of backends) {
            if (testInfo.status !== testInfo.expectedStatus) {
                await testInfo.attach('log do backend', { body: backend.log(), contentType: 'text/plain' });
            }
            await backend.desligar();
        }
    },

    // Print anexado ao relatório e, com E2E_EVIDENCIAS=1, salvo nas evidências.
    evidencia: async ({ page }, use, testInfo) => {
        await use(async (nome) => {
            const corpo = await page.screenshot({ fullPage: true });
            await testInfo.attach(nome, { body: corpo, contentType: 'image/png' });
            if (process.env.E2E_EVIDENCIAS) {
                mkdirSync(PASTA_EVIDENCIAS, { recursive: true });
                await page.screenshot({ path: path.join(PASTA_EVIDENCIAS, `${nome}.png`), fullPage: true });
            }
        });
    },
});

export { expect };
