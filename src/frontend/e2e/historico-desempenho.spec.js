import { execFileSync } from 'node:child_process';
import { mkdirSync, mkdtempSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { test, expect, PASTA_BACKEND, pythonDoBackend } from './fixtures';

// RNF-SFT04 (#64), critério da #142: com 500 corridas no banco, filtrar o histórico
// no painel responde em menos de 2,0 s. O banco é populado por
// src/backend/scripts/popular_historico.py: um SQLite temporário por padrão ou o
// banco de E2E_DATABASE_URL (ex.: um MySQL de teste com o armazena.sql aplicado).
// Com E2E_EVIDENCIAS=1, os prints e as medições vão para docs/evidencias/5.1.2.1.

const TOTAL_CORRIDAS = 500;
const LIMITE_MS = 2000;
const PASTA_EVIDENCIAS = path.resolve(PASTA_BACKEND, '..', '..', 'docs', 'evidencias', '5.1.2.1');

const ehListagem = (url) => new URL(url).pathname.endsWith('/api/corridas');

function popularBanco(url) {
    execFileSync(
        pythonDoBackend(),
        ['-m', 'scripts.popular_historico', '--url', url, '--corridas', String(TOTAL_CORRIDAS)],
        { cwd: PASTA_BACKEND, stdio: 'pipe' }
    );
}

function relatorioHtml(contexto, medicoes) {
    const linhas = medicoes.map((m) => `
        <tr>
            <td>${m.filtro}</td>
            <td><code>${m.requisicao}</code></td>
            <td>${m.status}</td>
            <td>${m.registros}</td>
            <td>${m.ttfb_ms.toFixed(1)}</td>
            <td>${m.resposta_ms.toFixed(1)}</td>
            <td>${m.tela_ms}</td>
        </tr>`).join('');
    return `<!doctype html><html lang="pt-BR"><head><meta charset="utf-8"><style>
        body { font-family: Segoe UI, Arial, sans-serif; margin: 24px; color: #1a202c; background: #fff; }
        h1 { font-size: 20px; margin: 0 0 4px; } p { margin: 2px 0; font-size: 13px; color: #4a5568; }
        table { border-collapse: collapse; margin-top: 16px; font-size: 13px; }
        th, td { border: 1px solid #cbd5e0; padding: 6px 10px; text-align: left; }
        th { background: #edf2f7; } td:nth-child(n+3) { text-align: right; }
        code { font-size: 12px; } .ok { color: #276749; font-weight: 600; }
    </style></head><body>
        <h1>RNF-SFT04: tempo de resposta do histórico com ${contexto.corridas} corridas</h1>
        <p>Medido em ${contexto.medido_em} com Playwright ${contexto.navegador} headless, painel no Vite (modo dev) e backend FastAPI com banco ${contexto.banco}.</p>
        <p>TTFB e resposta completa: tempos da requisição à API registrados pelo navegador (equivalentes à aba Network do DevTools). Tela atualizada: da ação no filtro até a última atualização da tabela com os dados da API, medida na própria página.</p>
        <table>
            <thead><tr><th>Filtro</th><th>Requisição</th><th>HTTP</th><th>Corridas</th><th>TTFB (ms)</th><th>Resposta completa (ms)</th><th>Tela atualizada (ms)</th></tr></thead>
            <tbody>${linhas}</tbody>
        </table>
        <p class="ok" style="margin-top:12px">Limite do RNF-SFT04: ${LIMITE_MS} ms. Maior tempo medido: ${contexto.maior_ms} ms.</p>
    </body></html>`;
}

test('RNF-SFT04: histórico com 500 corridas filtrado em menos de 2 s', async ({ page, browser, iniciarBackend }, testInfo) => {
    test.setTimeout(180_000);
    const url = process.env.E2E_DATABASE_URL
        || `sqlite:///${path.join(mkdtempSync(path.join(tmpdir(), 'e2e-historico-')), 'historico.db').replaceAll('\\', '/')}`;
    popularBanco(url);
    await iniciarBackend({ fonte: 'nenhuma', ambiente: { DATABASE_URL: url } });

    const tabela = page.getByTestId('tabela-historico');
    const linhas = tabela.locator('tbody tr.linha-corrida');

    await page.goto('/');
    await expect(linhas).toHaveCount(TOTAL_CORRIDAS);

    const medicoes = [];
    // A tela é medida dentro da página: do instante da ação até a última mudança
    // no corpo da tabela depois de a resposta da API chegar (MutationObserver e
    // Resource Timing), sem a granularidade das esperas do Playwright.
    async function medir(filtro, acao, geometria) {
        await page.evaluate(() => {
            const corpo = document.querySelector('[data-testid="tabela-historico"] tbody');
            window.__observador?.disconnect();
            window.__medicao = { inicio: performance.now(), mutacoes: [] };
            window.__observador = new MutationObserver(() => window.__medicao.mutacoes.push(performance.now()));
            window.__observador.observe(corpo, { childList: true, subtree: true, characterData: true });
        });
        const aguardando = page.waitForResponse((r) => ehListagem(r.url()) && r.request().method() === 'GET');
        await acao();
        const resposta = await aguardando;
        await resposta.finished();
        const dados = await resposta.json();
        await expect(linhas).toHaveCount(dados.length);
        if (geometria) {
            await expect(tabela.locator('tbody td.col-geometria').filter({ hasNotText: geometria })).toHaveCount(0);
        }
        const tela = await page.evaluate((endereco) => {
            const { inicio, mutacoes } = window.__medicao;
            const entrada = performance.getEntriesByType('resource')
                .filter((e) => e.name === endereco && e.startTime >= inicio).at(-1);
            const fimResposta = entrada ? entrada.responseEnd : inicio;
            return Math.round(Math.max(fimResposta, ...mutacoes.filter((t) => t >= fimResposta)) - inicio);
        }, resposta.url());
        const tempo = resposta.request().timing();
        const endereco = new URL(resposta.url());
        medicoes.push({
            filtro,
            requisicao: `GET ${endereco.pathname}${endereco.search}`,
            status: resposta.status(),
            registros: dados.length,
            ttfb_ms: tempo.responseStart - tempo.requestStart,
            resposta_ms: tempo.responseEnd,
            tela_ms: tela,
        });
        return dados;
    }

    const so8x4 = await medir('Geometria 8x4', () => page.getByTestId('filtro-geometria').selectOption('8x4'), '8x4');
    expect(so8x4).toHaveLength(167);
    await medir('8x4 a partir de 01/10/2026', () => page.getByTestId('filtro-data-inicio').fill('2026-10-01'), '8x4');
    const periodo = await medir('8x4 de 01/10 a 15/10/2026', () => page.getByTestId('filtro-data-fim').fill('2026-10-15'), '8x4');
    expect(periodo.length).toBeGreaterThan(0);
    expect(periodo.every((c) => c.iniciada_em >= '2026-10-01' && c.iniciada_em < '2026-10-16')).toBe(true);
    // Recorte do histórico filtrado: filtros e primeiras linhas da tabela.
    // Coordenadas da página inteira (o boundingBox é relativo à área visível).
    const caixa = await page.getByTestId('historico-geral').evaluate((el) => {
        const r = el.getBoundingClientRect();
        return { x: r.x + window.scrollX, y: r.y + window.scrollY, width: r.width, height: r.height };
    });
    const printPainel = await page.screenshot({
        fullPage: true,
        clip: { x: caixa.x, y: caixa.y, width: caixa.width, height: Math.min(caixa.height, 760) },
    });
    const todas = await medir('Sem filtro (Limpar)', () => page.getByTestId('botao-limpar-filtros').click());
    expect(todas).toHaveLength(TOTAL_CORRIDAS);

    const maior = Math.max(...medicoes.flatMap((m) => [m.resposta_ms, m.tela_ms]));
    const contexto = {
        medido_em: new Date().toLocaleString('pt-BR', { timeZone: 'America/Sao_Paulo' }),
        navegador: `Chromium ${browser.version()}`,
        banco: url.split(':')[0].split('+')[0] === 'mysql' ? 'MySQL (armazena.sql)' : 'SQLite',
        corridas: TOTAL_CORRIDAS,
        maior_ms: Math.round(maior),
    };
    const paginaRelatorio = await page.context().newPage();
    await paginaRelatorio.setViewportSize({ width: 1100, height: 400 });
    await paginaRelatorio.setContent(relatorioHtml(contexto, medicoes));
    const printMedicao = await paginaRelatorio.screenshot({ fullPage: true });

    await testInfo.attach('painel-historico-filtrado', { body: printPainel, contentType: 'image/png' });
    await testInfo.attach('medicao-tempo-resposta', { body: printMedicao, contentType: 'image/png' });
    await testInfo.attach('medicoes.json', { body: JSON.stringify({ ...contexto, medicoes }, null, 2), contentType: 'application/json' });
    if (process.env.E2E_EVIDENCIAS) {
        mkdirSync(PASTA_EVIDENCIAS, { recursive: true });
        writeFileSync(path.join(PASTA_EVIDENCIAS, 'painel-historico-filtrado.png'), printPainel);
        writeFileSync(path.join(PASTA_EVIDENCIAS, 'medicao-tempo-resposta.png'), printMedicao);
        writeFileSync(path.join(PASTA_EVIDENCIAS, 'medicoes.json'), `${JSON.stringify({ ...contexto, medicoes }, null, 2)}\n`);
    }

    for (const m of medicoes) {
        expect(m.status).toBe(200);
        expect(m.resposta_ms, `${m.filtro}: resposta da API`).toBeLessThan(LIMITE_MS);
        expect(m.tela_ms, `${m.filtro}: tabela atualizada`).toBeLessThan(LIMITE_MS);
    }
});
