import { test, expect, pastaEvidencias } from './fixtures';
import { COMPONENTES, procurarDistorcoes } from './layout';

// RNF-SFT05 (#65): o painel funciona sem distorções no Chrome e no Firefox em
// 1366x768 e 1920x1080. Roda nos projetos chromium-* e firefox-* do
// playwright.config.js, um por navegador e resolução. Com E2E_EVIDENCIAS=1, o print
// de cada combinação vai para docs/evidencias/5.1.1.3.

const PASTA = pastaEvidencias('5.1.1.3');

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
