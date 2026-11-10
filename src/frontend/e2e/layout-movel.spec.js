import { test, expect, pastaEvidencias } from './fixtures';
import { COMPONENTES, procurarDistorcoes } from './layout';

// RNF-SFT06 (#66): em telas verticais de celular, de 360x640 a 412x915, o painel
// mostra status, cronômetro e telemetria legíveis, sem rolagem horizontal
// desnecessária. Roda nos projetos chromium-360x640 e chromium-412x915 do
// playwright.config.js. Com E2E_EVIDENCIAS=1, o print de cada viewport e geometria
// vai para docs/evidencias/5.1.1.4.

const PASTA = pastaEvidencias('5.1.1.4');

// Componentes essenciais do critério: status, cronômetro e telemetria.
const ESSENCIAIS = ['status', 'tempo', 'velocidade', 'bateria'];

// Textos que o operador lê de relance: o status e os valores dos indicadores.
const VALORES = '[data-testid="valor-status"], .indicador .valor, .indicador .valor-porcentagem';
const FONTE_MINIMA_PX = 14;

// A tabela do histórico geral tem 9 colunas e não cabe em 360 px. Ela rola dentro do
// próprio cartão, que é uma rolagem necessária; a página continua sem rolar.
const ROLAGEM_PERMITIDA = ['.tabela-wrapper'];

// Mede, no navegador, a fonte de cada valor e se ele aparece inteiro: dentro da
// largura da tela e sem texto cortado no próprio elemento.
function medirValores(seletor) {
    const largura = document.documentElement.clientWidth;
    return [...document.querySelectorAll(seletor)].map((el) => {
        const caixa = el.getBoundingClientRect();
        return {
            texto: el.textContent.trim(),
            fontePx: parseFloat(getComputedStyle(el).fontSize),
            dentroDaTela: caixa.left >= 0 && caixa.right <= largura + 1,
            cortado: el.clientWidth > 0 && el.scrollWidth > el.clientWidth + 1,
        };
    });
}

// O 12x4 é o mapa mais alto e o 4x4 o mais largo (células de 100 px no desktop):
// as três geometrias precisam caber na largura do celular.
for (const labirinto of ['4x4', '8x4', '12x4']) {
    test(`RNF-SFT06: painel legível no celular com uma corrida ${labirinto} concluída`, async ({ page, iniciarBackend, evidencia }, testInfo) => {
        await iniciarBackend({ labirinto });
        await page.goto('/');

        await expect(page.getByTestId('valor-status')).toHaveText('Concluído', { timeout: 45_000 });
        await expect(page.locator('[data-testid^="linha-corrida-"]').first()).toBeVisible();
        await expect(page.getByTestId('replay-corrida')).toBeVisible();
        await page.evaluate(() => document.fonts.ready);

        await evidencia(`${testInfo.project.name}-${labirinto}`, PASTA);

        for (const [componente, seletor] of Object.entries(COMPONENTES)) {
            await expect(page.locator(seletor), `${componente} visível`).toBeVisible();
        }

        // As verificações abaixo são soft: uma falha não interrompe o teste, que lista
        // todos os problemas de uma vez.

        // Status, cronômetro e telemetria inteiros dentro da largura da tela.
        const largura = page.viewportSize().width;
        for (const componente of ESSENCIAIS) {
            const caixa = await page.locator(COMPONENTES[componente]).boundingBox();
            expect.soft(caixa.x, `${componente} começa dentro da tela`).toBeGreaterThanOrEqual(0);
            expect.soft(caixa.x + caixa.width, `${componente} termina dentro da tela`).toBeLessThanOrEqual(largura + 1);
        }

        // Legibilidade: fonte de pelo menos 14 px e nenhum valor cortado.
        const valores = await page.evaluate(medirValores, VALORES);
        expect(valores.length).toBeGreaterThanOrEqual(5);
        for (const valor of valores) {
            expect.soft(valor.fontePx, `fonte de "${valor.texto}"`).toBeGreaterThanOrEqual(FONTE_MINIMA_PX);
            expect.soft(valor.dentroDaTela, `"${valor.texto}" dentro da tela`).toBe(true);
            expect.soft(valor.cortado, `"${valor.texto}" sem corte`).toBe(false);
        }

        expect(await page.evaluate(procurarDistorcoes, { rolagemPermitida: ROLAGEM_PERMITIDA })).toEqual([]);
    });
}
