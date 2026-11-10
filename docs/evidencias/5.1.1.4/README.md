# Evidências: EAP 5.1.1.4 (RNF-SFT06, responsividade para dispositivos móveis)

Prints gerados pelo Playwright com `e2e/layout-movel.spec.js` (em `src/frontend`). O backend rodou com o simulador no lugar do robô e do gateway, com uma corrida já concluída e o histórico geral e o replay carregados. Gerados em 10/11/2026; os 6 testes passaram.

O critério do RNF-SFT06 (#66) é a emulação de viewport móvel de 360x640 a 412x915 px, conferindo a legibilidade e a ausência de rolagem horizontal desnecessária. O teste roda nos projetos `chromium-360x640` e `chromium-412x915` do `playwright.config.js`, com a emulação de celular do Chromium (`isMobile` e `hasTouch`), uma vez para cada geometria (4x4, 8x4 e 12x4). O 4x4 é o mapa mais largo (células de 100 px no desktop) e o 12x4 o mais alto, por isso entram as três. Cada execução verifica:

- **Rolagem horizontal:** a página não rola na horizontal.
- **Visibilidade:** o mapa ao vivo, o status, o tempo, a velocidade, a bateria, o histórico da sessão, o histórico geral e o replay estão visíveis.
- **Componentes essenciais inteiros na tela:** status, cronômetro, velocidade e bateria começam e terminam dentro da largura do viewport.
- **Legibilidade:** o status e os valores dos indicadores (`[data-testid="valor-status"]`, `.indicador .valor` e `.indicador .valor-porcentagem`, que é o valor da bateria) têm fonte de pelo menos 14 px, ficam dentro da tela e nenhum aparece cortado.
- **Limites dos componentes:** a mesma verificação do RNF-SFT05 (`procurarDistorcoes`, em `e2e/layout.js`): nenhum bloco do layout fora do seu container e nenhum elemento fora do seu cartão, com tolerância de 1 px.

| Viewport | Geometria | Depois da correção | Antes da correção |
|:---|:---|:---|:---|
| 360x640 | 4x4 | [chromium-360x640-4x4.png](chromium-360x640-4x4.png) | [antes-chromium-360x640-4x4.png](antes-chromium-360x640-4x4.png) |
| 360x640 | 8x4 | [chromium-360x640-8x4.png](chromium-360x640-8x4.png) | [antes-chromium-360x640-8x4.png](antes-chromium-360x640-8x4.png) |
| 360x640 | 12x4 | [chromium-360x640-12x4.png](chromium-360x640-12x4.png) | [antes-chromium-360x640-12x4.png](antes-chromium-360x640-12x4.png) |
| 412x915 | 4x4 | [chromium-412x915-4x4.png](chromium-412x915-4x4.png) | [antes-chromium-412x915-4x4.png](antes-chromium-412x915-4x4.png) |
| 412x915 | 8x4 | [chromium-412x915-8x4.png](chromium-412x915-8x4.png) | [antes-chromium-412x915-8x4.png](antes-chromium-412x915-8x4.png) |
| 412x915 | 12x4 | [chromium-412x915-12x4.png](chromium-412x915-12x4.png) | [antes-chromium-412x915-12x4.png](antes-chromium-412x915-12x4.png) |

Os prints são da página inteira. Depois da correção, todos têm exatamente a largura do viewport (360 ou 412 px); antes, tinham 966 px. A rolagem vertical é esperada em celular e não faz parte do critério.

## O que o teste acusou antes da correção

Os prints "antes" são do mesmo teste rodando no layout do `main` (depois do #65). Nas 6 execuções o teste falhou:

| Problema | 360x640 | 412x915 |
|:---|:---|:---|
| Rolagem horizontal da página | 966 px de conteúdo em 360 px | 967 px de conteúdo em 412 px |
| Coluna de indicadores fora da área principal | 654 px à direita | 602 px à direita |
| Status, tempo, velocidade e bateria fora da tela | Terminam em 966 px | Terminam em 966 px |
| Mapa do replay fora do cartão "Inspecionar corrida" | 113 px de cada lado | 113 px de cada lado |
| Cabeçalho, legendas, estatísticas e métricas do replay fora do histórico geral | Até 181 px à direita | Até 129 px à direita |

As fontes já passavam: todos os valores têm 14 px ou mais.

A causa é que o layout era só de desktop: o mapa e os indicadores ficam lado a lado, com larguras mínimas maiores que a tela (`.labirinto-container` com 500 px, `.indicadores-coluna` com 320 px e `.replay-corrida-painel` com 340 px), e o tamanho da célula do mapa era fixo por geometria (100, 70 ou 50 px), o que deixa o 4x4 com 400 px de grade.

## Correção

- **Uma coluna abaixo de 768 px** (`@media (max-width: 767px)`): status, cronômetro, velocidade, bateria e o histórico da sessão aparecem primeiro, depois o mapa e o histórico geral. As larguras mínimas maiores que a tela foram removidas nessa faixa, e os recuos dos cartões diminuíram.
- **Mapa que se ajusta à largura:** o `Labirinto` mede a largura disponível e diminui a célula só quando a grade não cabe (`calcularCellSize(linhas, larguraDisponivel)`), com mínimo de 32 px por causa do ícone do robô, que tem 28 px. Em 360 px, o 4x4 e o 8x4 ficam com células de 62 px no mapa ao vivo. No desktop a largura sobra e as células continuam com 100, 70 e 50 px.
- **Controles do replay:** a barra de tempo (`input range`) tem largura mínima própria no Chromium e passava do cartão em 360 px. Ela passou a poder encolher.

**Decisão sobre a tabela do histórico geral:** a tabela tem 9 colunas e não cabe em 360 px. Ela rola na horizontal dentro do próprio cartão (`.tabela-wrapper`), e a página continua sem rolar. Essa é uma rolagem necessária, que o critério aceita, e mantém todas as informações da corrida no celular, sem esconder colunas. Por isso o teste móvel passa `rolagemPermitida: ['.tabela-wrapper']` ao `procurarDistorcoes`; o teste de desktop continua exigindo a tabela sem rolagem.

## Como reproduzir

Em `src/frontend`, com o Chromium instalado (`npx playwright install chromium`):

```powershell
$env:E2E_EVIDENCIAS = "1"
npx playwright test layout-movel
```

Os prints "antes" foram gerados com o mesmo comando antes da correção de CSS, depois renomeados com o prefixo `antes-`. No CI, o teste roda a cada PR no job de E2E do **Frontend CI**, e os prints ficam anexados ao relatório HTML do Playwright (artefato `relatorio-e2e`).
