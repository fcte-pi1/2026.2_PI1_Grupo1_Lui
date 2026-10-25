import path from 'node:path';
import { test, expect, PASTA_EVIDENCIAS } from './fixtures';

// Casos do roteiro de testes funcionais (doc 4.4, seção 7) que rodam com o
// simulador do backend no lugar do robô e do gateway.

const LINHAS = { '4x4': 4, '8x4': 8, '12x4': 12 };
const chegada = (labirinto) => `D${String(LINHAS[labirinto]).padStart(2, '0')}`;

// O ícone do robô está dentro da célula indicada (ex.: 'A01', 'D12')?
async function roboNaCelula(page, id) {
    const robo = await page.getByTestId('robo-icon').boundingBox();
    const celula = await page.getByTestId(`celula-${id}`).boundingBox();
    if (!robo || !celula) return false;
    const x = robo.x + robo.width / 2;
    const y = robo.y + robo.height / 2;
    return x >= celula.x && x <= celula.x + celula.width && y >= celula.y && y <= celula.y + celula.height;
}

// Posição do robô na tela, para saber se ele andou.
async function posicaoRobo(page) {
    return page.getByTestId('robo-icon').evaluate((el) => `${el.style.left},${el.style.top}`).catch(() => null);
}

// Quantidade de pontos do rastro desenhado.
async function pontosDoRastro(page) {
    const linha = page.locator('.trajeto-overlay polyline');
    if ((await linha.count()) === 0) return 0;
    const pontos = await linha.getAttribute('points');
    return pontos.trim().split(/\s+/).length;
}

for (const labirinto of ['4x4', '8x4', '12x4']) {
    test(`CT-09 (com CT-04 e CT-07 parciais): painel acompanha a corrida completa em ${labirinto}`, async ({ page, iniciarBackend, evidencia }) => {
        await iniciarBackend({ labirinto });
        await page.goto('/');

        await expect(page.getByTestId('status-conexao')).toHaveText(/CONECTADO/);
        await expect(page.getByTestId('status-conexao')).not.toHaveText(/DESCONECTADO/);

        // CT-04 (parcial): grade com as dimensões da geometria.
        const linhas = LINHAS[labirinto];
        await expect(page.getByText(`GRID ${linhas}x4`, { exact: false }).first()).toBeVisible();
        await expect(page.locator('[data-testid^="celula-"]')).toHaveCount(4 * linhas);

        // O mapa é desenhado aos poucos: o robô passa por várias posições e o
        // rastro cresce até a chegada.
        const posicoes = new Set();
        let maiorRastro = 0;
        let printExplorando = false;
        const valorStatus = page.getByTestId('valor-status');
        await expect(async () => {
            const posicao = await posicaoRobo(page);
            if (posicao) posicoes.add(posicao);
            maiorRastro = Math.max(maiorRastro, await pontosDoRastro(page));
            if (!printExplorando && posicoes.size >= 4 && labirinto === '12x4') {
                printExplorando = true;
                await evidencia('ct09-12x4-explorando');
            }
            await expect(valorStatus).toHaveText('Concluído', { timeout: 100 });
        }).toPass({ timeout: 45_000, intervals: [100] });

        expect(posicoes.size).toBeGreaterThanOrEqual(4);
        expect(maiorRastro).toBeGreaterThanOrEqual(4);
        expect(await roboNaCelula(page, chegada(labirinto))).toBe(true);

        // CT-07 (parcial): os parâmetros do edital aparecem no painel.
        await expect(page.getByTestId('indicador-tempo')).not.toContainText('00:00 /');
        await expect(page.getByTestId('indicador-bateria')).toContainText(/\d+%/);
        await expect(page.getByTestId('indicador-bateria')).toContainText(/\d\.\d{2} V/);
        await expect(page.getByTestId('indicador-velocidade')).toContainText('cm/s');

        await evidencia(`ct09-${labirinto}-concluido`);

        if (labirinto === '12x4' && process.env.E2E_EVIDENCIAS) {
            const video = page.video();
            await page.close();
            await video?.saveAs(path.join(PASTA_EVIDENCIAS, 'ct09-12x4.webm'));
        }
    });
}

test('CT-15: travamento aparece como "Travado" em destaque', async ({ page, iniciarBackend, evidencia }) => {
    await iniciarBackend({ cenario: 'travado', labirinto: '8x4' });
    await page.goto('/');

    await expect(page.getByTestId('valor-status')).toHaveText('Explorando');
    await expect(page.getByTestId('valor-status')).toContainText('Travado', { timeout: 30_000 });
    await expect(page.getByTestId('indicador-status')).toHaveClass(/estilo-alerta/);
    await expect(page.getByTestId('icone-alerta-status')).toBeVisible();
    await expect(page.getByTestId('aviso-travamento')).toHaveText('Corte de tração por travamento das rodas');

    await evidencia('ct15-travado');
});

test('CT-08 (parcial): indicador vai para DESCONECTADO quando o backend cai e volta ao subir', async ({ page, iniciarBackend, evidencia }) => {
    // Mais devagar, para a queda acontecer com a corrida em andamento.
    const backend = await iniciarBackend({ labirinto: '12x4', velocidade: 2 });
    await page.goto('/');

    const conexao = page.getByTestId('status-conexao');
    await expect(conexao).toHaveText(/^\s*CONECTADO/);
    await expect(page.getByTestId('valor-status')).toHaveText('Explorando');

    await backend.desligar();
    await expect(conexao).toHaveText(/DESCONECTADO/);
    await expect(conexao).toHaveClass(/desconectado/);
    // O último estado recebido continua na tela durante a queda.
    await expect(page.getByTestId('robo-icon')).toBeVisible();
    await evidencia('ct08-desconectado');

    await backend.ligar();
    await expect(conexao).toHaveText(/^\s*CONECTADO/, { timeout: 15_000 });
    await expect(conexao).toHaveClass(/ conectado/);
    await evidencia('ct08-reconectado');
});

test('recomeço limpo do mapa quando o simulador repete a corrida', async ({ page, iniciarBackend, evidencia }) => {
    await iniciarBackend({ labirinto: '4x4', repetir: true, velocidade: 2 });
    await page.goto('/');

    const tentativa = page.getByTestId('status-tentativa');
    await expect(tentativa).toHaveText('Tentativa 1');
    await expect(page.getByTestId('valor-status')).toHaveText('Concluído', { timeout: 30_000 });
    const rastroPrimeira = await pontosDoRastro(page);
    expect(rastroPrimeira).toBeGreaterThanOrEqual(4);

    // Na largada da segunda corrida, o mapa antigo some: robô em A1 e rastro vazio.
    await expect(tentativa).toHaveText('Tentativa 2', { timeout: 30_000 });
    expect(await pontosDoRastro(page)).toBeLessThanOrEqual(2);
    expect(await roboNaCelula(page, 'A01')).toBe(true);
    await evidencia('recomeco-tentativa-2');

    // A segunda corrida termina igual à primeira.
    await expect(page.getByTestId('valor-status')).toHaveText('Concluído', { timeout: 30_000 });
    expect(await pontosDoRastro(page)).toBe(rastroPrimeira);
    expect(await roboNaCelula(page, 'D04')).toBe(true);
});
