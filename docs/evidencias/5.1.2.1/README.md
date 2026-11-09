# Evidências: EAP 5.1.2.1 (API do histórico e replay)

Medição do tempo de resposta do histórico exigida pelo RNF-SFT04 (#64) e pelo critério de aceite da #142: com 500 corridas no banco, as consultas do histórico devem responder em menos de 2,0 s.

Geradas em 09/11/2026 pelo Playwright com `e2e/historico-desempenho.spec.js` (em `src/frontend`). O teste passou:

- O banco foi um MySQL 8.0 com o `migrations/armazena.sql` aplicado, populado com 500 corridas (167 de 4x4, 167 de 8x4, 166 de 12x4, de 01/09 a 02/11/2026) e 5.000 leituras por `src/backend/scripts/popular_historico.py`.
- O painel rodou no Vite em modo de desenvolvimento, no Chromium headless.

| Filtro aplicado no painel | Requisição | Corridas | TTFB | Resposta completa | Tabela atualizada |
|:---|:---|---:|---:|---:|---:|
| Geometria 8x4 | `GET /api/corridas?geometria=8x4` | 167 | 38 ms | 41 ms | 474 ms |
| 8x4 a partir de 01/10/2026 | `...&data_inicio=2026-10-01` | 88 | 29 ms | 30 ms | 116 ms |
| 8x4 de 01/10 a 15/10/2026 | `...&data_inicio=2026-10-01&data_fim=2026-10-15` | 40 | 116 ms | 118 ms | 194 ms |
| Sem filtro (Limpar) | `GET /api/corridas` | 500 | 106 ms | 109 ms | 444 ms |

Maior tempo medido: 474 ms, abaixo do limite de 2.000 ms.

- **TTFB e resposta completa:** tempos da requisição à API registrados pelo próprio navegador, os mesmos da aba Network do DevTools.
- **Tabela atualizada:** vai da ação no filtro até a última mudança da tabela com os dados da API, medida dentro da página com MutationObserver e Resource Timing.

| Evidência | Arquivo | O que mostra |
|:---|:---|:---|
| Medição | [medicao-tempo-resposta.png](medicao-tempo-resposta.png) | Tabela com os tempos de cada filtro, gerada pelo teste no mesmo navegador |
| Painel filtrado | [painel-historico-filtrado.png](painel-historico-filtrado.png) | Histórico com 8x4 de 01/10 a 15/10/2026 |
| Dados brutos | [medicoes.json](medicoes.json) | Tempos de cada requisição, navegador e banco usados |

## Como reproduzir

Com um MySQL de teste (por exemplo, o contêiner descrito no [README do backend](../../../src/backend/README.md#persistência-contra-o-mysql)), em `src/frontend`:

```powershell
$env:E2E_EVIDENCIAS = "1"
$env:E2E_DATABASE_URL = "mysql+pymysql://root:teste_pi1@127.0.0.1:3307/armazena"
npx playwright test historico-desempenho
```

O script apaga as corridas do banco indicado antes de popular, então use um banco só de teste. Sem `E2E_DATABASE_URL`, o teste usa um SQLite temporário. É assim que ele roda no job de E2E do Frontend CI, que também verifica o limite de 2 s a cada PR.
