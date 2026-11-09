import { defineConfig, devices } from '@playwright/test';

// Testes E2E do painel (EAP 5.3.2.1): o Vite sobe uma vez para todos os testes e
// cada teste sobe o seu próprio backend com o simulador (e2e/fixtures.js).
const PORTA_PAINEL = 4173;
// npm run test:e2e:evidencias também salva os prints em docs/evidencias/5.3.2.1.
if (process.env.npm_lifecycle_event === 'test:e2e:evidencias') process.env.E2E_EVIDENCIAS = '1';
const evidencias = Boolean(process.env.E2E_EVIDENCIAS);

// Compatibilidade desktop (RNF-SFT05): navegadores e resoluções do critério de aceite.
const LAYOUT_DESKTOP = /layout-desktop\.spec\.js/;
const NAVEGADORES_DESKTOP = [['chromium', 'Desktop Chrome'], ['firefox', 'Desktop Firefox']];
const RESOLUCOES_DESKTOP = [{ width: 1366, height: 768 }, { width: 1920, height: 1080 }];

export default defineConfig({
  testDir: 'e2e',
  timeout: 60_000,
  expect: { timeout: 15_000 },
  fullyParallel: true,
  forbidOnly: Boolean(process.env.CI),
  retries: process.env.CI ? 1 : 0,
  workers: process.env.CI ? 2 : undefined,
  reporter: [['list'], ['html', { open: 'never' }]],
  use: {
    baseURL: `http://localhost:${PORTA_PAINEL}`,
    viewport: { width: 1280, height: 900 },
    trace: 'retain-on-failure',
    video: evidencias ? 'on' : 'retain-on-failure',
  },
  projects: [
    {
      name: 'chromium',
      testIgnore: LAYOUT_DESKTOP,
      use: { ...devices['Desktop Chrome'], viewport: { width: 1280, height: 900 } },
    },
    // RNF-SFT05 (#65): o teste de layout roda em cada navegador e resolução do
    // critério; os demais testes ficam só no projeto chromium acima.
    ...NAVEGADORES_DESKTOP.flatMap(([navegador, dispositivo]) =>
      RESOLUCOES_DESKTOP.map(({ width, height }) => ({
        name: `${navegador}-${width}x${height}`,
        testMatch: LAYOUT_DESKTOP,
        use: { ...devices[dispositivo], viewport: { width, height } },
      }))
    ),
  ],
  webServer: {
    command: `npm run dev -- --port ${PORTA_PAINEL} --strictPort`,
    url: `http://localhost:${PORTA_PAINEL}`,
    reuseExistingServer: !process.env.CI,
    timeout: 60_000,
  },
});
