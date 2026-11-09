import { test, expect, pastaEvidencias } from './fixtures';

// RNF-SFT05 (#65): o painel funciona sem distorções no Chrome e no Firefox em
// 1366x768 e 1920x1080. Roda nos projetos chromium-* e firefox-* do
// playwright.config.js, um por navegador e resolução. Com E2E_EVIDENCIAS=1, o print
// de cada combinação vai para docs/evidencias/5.1.1.3.

const PASTA = pastaEvidencias('5.1.1.3');
const MAPA = '.dashboard-layout > [data-testid="labirinto-container"]';

// Componentes que o operador precisa ver, pelo seletor de cada um.
const COMPONENTES = {
    'mapa ao vivo': MAPA,
    status: '[data-testid="indicador-status"]',
    tempo: '[data-testid="indicador-tempo"]',
    velocidade: '[data-testid="indicador-velocidade"]',
    bateria: '[data-testid="indicador-bateria"]',
    'histórico da sessão': '[data-testid="historico-corridas"]',
    'histórico geral': '[data-testid="historico-geral"]',
    'replay da corrida': '[data-testid="replay-corrida"]',
};

// Procura, dentro da página, o que passa dos limites: rolagem horizontal da página,
// blocos do layout fora do seu container, qualquer elemento fora do seu cartão e
// áreas com rolagem própria que precisam rolar na horizontal.
function procurarDistorcoes() {
    const TOLERANCIA = 1;
    const problemas = [];
    const caixa = (el) => el.getBoundingClientRect();
    const nome = (el) => {
        if (el.dataset?.testid) return `[data-testid="${el.dataset.testid}"]`;
        const classes = typeof el.className === 'string' ? el.className.trim().split(/\s+/).filter(Boolean) : [];
        return el.tagName.toLowerCase() + classes.map((c) => `.${c}`).join('');
    };
    const rolaSozinho = (el) => ['auto', 'scroll'].includes(getComputedStyle(el).overflowX);
    const verificar = (filho, pai) => {
        const f = caixa(filho);
        const p = caixa(pai);
        if (f.width === 0 && f.height === 0) return;
        const fora = [
            f.left < p.left - TOLERANCIA && `${Math.round(p.left - f.left)} px à esquerda`,
            f.right > p.right + TOLERANCIA && `${Math.round(f.right - p.right)} px à direita`,
            f.top < p.top - TOLERANCIA && `${Math.round(p.top - f.top)} px acima`,
            f.bottom > p.bottom + TOLERANCIA && `${Math.round(f.bottom - p.bottom)} px abaixo`,
        ].filter(Boolean);
        if (fora.length) problemas.push(`${nome(filho)} passa de ${nome(pai)} (${fora.join(', ')})`);
    };

    const raiz = document.documentElement;
    if (raiz.scrollWidth > raiz.clientWidth) {
        problemas.push(`rolagem horizontal na página (${raiz.scrollWidth} px de conteúdo em ${raiz.clientWidth} px)`);
    }

    // Blocos do layout dentro dos seus containers.
    for (const seletor of ['.painel-container', '.dashboard-layout', '.indicadores-coluna', '.secao-historico-geral']) {
        const container = document.querySelector(seletor);
        if (container) for (const filho of container.children) verificar(filho, container);
    }

    // Todo elemento de um cartão dentro do cartão, salvo o que fica numa área com
    // rolagem própria (essa área é conferida abaixo).
    const cartoes = document.querySelectorAll(
        '.indicador, [data-testid="labirinto-container"], .historico-geral-container, .replay-card'
    );
    for (const cartao of cartoes) {
        for (const elemento of cartao.querySelectorAll('*')) {
            let ancestral = elemento.parentElement;
            let dentroDeRolagem = false;
            while (ancestral && ancestral !== cartao) {
                if (rolaSozinho(ancestral)) dentroDeRolagem = true;
                ancestral = ancestral.parentElement;
            }
            if (!dentroDeRolagem) verificar(elemento, cartao);
        }
    }

    for (const elemento of document.querySelectorAll('.painel-container *')) {
        if (rolaSozinho(elemento) && elemento.scrollWidth > elemento.clientWidth + TOLERANCIA) {
            problemas.push(`${nome(elemento)} precisa de rolagem horizontal (${elemento.scrollWidth} px em ${elemento.clientWidth} px)`);
        }
    }
    return [...new Set(problemas)];
}

test('RNF-SFT05: painel sem distorções com uma corrida 12x4 concluída', async ({ page, iniciarBackend, evidencia }, testInfo) => {
    // O 12x4 é o maior mapa; com a corrida concluída, o histórico e o replay também
    // estão na tela.
    await iniciarBackend({ labirinto: '12x4' });
    await page.goto('/');

    await expect(page.getByTestId('valor-status')).toHaveText('Concluído', { timeout: 45_000 });
    await expect(page.locator('[data-testid^="linha-corrida-"]').first()).toBeVisible();
    await expect(page.getByTestId('replay-corrida')).toBeVisible();
    await page.evaluate(() => document.fonts.ready);

    await evidencia(testInfo.project.name, PASTA);

    for (const [componente, seletor] of Object.entries(COMPONENTES)) {
        await expect(page.locator(seletor), `${componente} visível`).toBeVisible();
    }
    expect(await page.evaluate(procurarDistorcoes)).toEqual([]);
});
