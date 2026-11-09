import { test, expect } from './fixtures';

// HU-09 (#44): "Conexão RF perdida" após 3 s sem pacote válido do robô, com o
// backend no ar. No cenário perda_conexao o simulador perde alguns pacotes no meio
// do percurso e fica 4 s em silêncio (em tempo real, mesmo com o fator de
// velocidade); o robô virtual segue navegando e a corrida termina concluída.

// Registra, dentro da página, a chegada de cada mensagem do WebSocket e quando o
// aviso aparece ou some, sem a granularidade das esperas do Playwright.
async function registrarRadio(page) {
    await page.addInitScript(() => {
        window.__rf = { quadros: [], aparicoes: [], sumicos: [] };
        const Base = window.WebSocket;
        window.WebSocket = class extends Base {
            constructor(...argumentos) {
                super(...argumentos);
                this.addEventListener('message', () => window.__rf.quadros.push(performance.now()));
            }
        };
        let visivel = false;
        new MutationObserver(() => {
            const agora = Boolean(document.querySelector('[data-testid="aviso-rf"]'));
            if (agora === visivel) return;
            visivel = agora;
            (agora ? window.__rf.aparicoes : window.__rf.sumicos).push(performance.now());
        }).observe(document, { childList: true, subtree: true });
    });
}

test('HU-09: aviso de conexão RF perdida após 3 s sem pacotes e retorno ao normal (perda_conexao)', async ({ page, iniciarBackend, evidencia }) => {
    await iniciarBackend({ labirinto: '4x4', cenario: 'perda_conexao' });
    await registrarRadio(page);
    await page.goto('/');

    const aviso = page.getByTestId('aviso-rf');
    const conexao = page.getByTestId('status-conexao');
    const status = page.getByTestId('valor-status');

    await expect(status).toHaveText('Explorando');
    await expect(aviso).toHaveCount(0);

    // Critério 1: depois de 3 s sem pacotes, o aviso aparece com o backend no ar.
    await expect(aviso).toBeVisible({ timeout: 15_000 });
    await expect(aviso).toContainText('Conexão RF perdida');
    await expect(aviso).toHaveAttribute('role', 'alert');
    await expect(conexao).toHaveText(/CONECTADO/);
    await expect(conexao).not.toHaveText(/DESCONECTADO/);
    // O status da corrida não muda: o robô não depende do rádio.
    await expect(status).toHaveText('Explorando');
    await evidencia('hu09-rf-perdida');

    // Critério 2: com a volta dos pacotes, o painel volta ao normal.
    await expect(aviso).toHaveCount(0, { timeout: 10_000 });
    await evidencia('hu09-rf-retomada');
    // O robô seguiu navegando durante a perda e conclui a corrida.
    await expect(status).toHaveText('Concluído', { timeout: 15_000 });

    const { quadros, aparicoes, sumicos } = await page.evaluate(() => window.__rf);
    expect(aparicoes).toHaveLength(1);
    expect(sumicos).toHaveLength(1);
    const ultimoAntes = quadros.filter((t) => t < aparicoes[0]).at(-1);
    const primeiroDepois = quadros.find((t) => t > aparicoes[0]);
    // A verificação roda a cada 250 ms: o aviso sai entre 3,0 s e pouco mais de 3,25 s.
    expect(aparicoes[0] - ultimoAntes).toBeGreaterThanOrEqual(3000);
    expect(aparicoes[0] - ultimoAntes).toBeLessThan(3700);
    // E some com o primeiro pacote depois da perda.
    expect(sumicos[0]).toBeGreaterThanOrEqual(primeiroDepois);
    expect(sumicos[0] - primeiroDepois).toBeLessThan(500);
});

test('HU-09: sem aviso falso depois da chegada, pois o robô parado continua transmitindo (completa)', async ({ page, iniciarBackend }) => {
    await iniciarBackend({ labirinto: '4x4', cenario: 'completa' });
    await registrarRadio(page);
    await page.goto('/');

    await expect(page.getByTestId('valor-status')).toHaveText('Concluído', { timeout: 30_000 });
    // Mais que o limite de 3 s depois de parar: o painel continua recebendo pacotes.
    await page.waitForTimeout(4500);
    await expect(page.getByTestId('aviso-rf')).toHaveCount(0);
    await expect(page.getByTestId('valor-status')).toHaveText('Concluído');

    const { quadros, aparicoes } = await page.evaluate(() => window.__rf);
    expect(aparicoes).toEqual([]);
    const ultimos = quadros.slice(-5);
    expect(ultimos.at(-1) - ultimos[0]).toBeLessThan(2000); // 1 pacote a cada 200 ms
});
