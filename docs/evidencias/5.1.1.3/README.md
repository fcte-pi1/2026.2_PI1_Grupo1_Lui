# Evidências: EAP 5.1.1.3 (RNF-SFT05, compatibilidade e responsividade desktop)

Prints gerados pelo Playwright com `e2e/layout-desktop.spec.js` (em `src/frontend`). O backend rodou com o simulador no lugar do robô e do gateway, numa corrida 12x4 (o maior mapa) já concluída, com o histórico geral e o replay carregados. Gerados em 09/11/2026; os 4 testes passaram.

O critério do RNF-SFT05 (#65) é funcionar sem distorções no Chrome e no Firefox, em 1366x768 e 1920x1080. O teste roda uma vez em cada combinação (projetos `chromium-1366x768`, `chromium-1920x1080`, `firefox-1366x768` e `firefox-1920x1080` do `playwright.config.js`) e verifica três coisas:

- **Rolagem horizontal:** a página não rola na horizontal.
- **Visibilidade:** o mapa ao vivo, o status, o tempo, a velocidade, a bateria, o histórico da sessão, o histórico geral e o replay estão visíveis.
- **Limites dos componentes:** nenhum componente passa dos limites do seu container. Os blocos do layout ficam dentro do painel, da área principal e da coluna de indicadores. Todo elemento de um cartão (indicadores, mapa, histórico, replay) fica dentro da borda do cartão, com tolerância de 1 px. Áreas com rolagem própria, como a tabela do histórico, também não podem precisar de rolagem horizontal.

| Combinação | Depois da correção | Antes da correção |
|:---|:---|:---|
| Chrome (Chromium) 1366x768 | [chromium-1366x768.png](chromium-1366x768.png) | [antes-chromium-1366x768.png](antes-chromium-1366x768.png) |
| Chrome (Chromium) 1920x1080 | [chromium-1920x1080.png](chromium-1920x1080.png) | [antes-chromium-1920x1080.png](antes-chromium-1920x1080.png) |
| Firefox 1366x768 | [firefox-1366x768.png](firefox-1366x768.png) | [antes-firefox-1366x768.png](antes-firefox-1366x768.png) |
| Firefox 1920x1080 | [firefox-1920x1080.png](firefox-1920x1080.png) | [antes-firefox-1920x1080.png](antes-firefox-1920x1080.png) |

Os prints são da página inteira. Em 768 e 1080 px de altura, o painel rola na vertical, porque o mapa 12x4 e o histórico são mais altos que a tela. Isso é esperado e não faz parte do critério.

## O que o teste acusou antes da correção

Os prints "antes" são do mesmo teste rodando no layout anterior, em que o histórico geral (filtros, tabela e replay) ficava na coluna da direita, ao lado dos indicadores. Nas 4 combinações o teste falhou:

| Problema | 1366x768 | 1920x1080 |
|:---|:---|:---|
| Rolagem horizontal da página | Sim: 1406 px de conteúdo em 1366 px | Não |
| Mapa do replay passando da borda do cartão | 113 px de cada lado | 20 px de cada lado |
| Tabela do histórico com rolagem horizontal interna | 972 px de tabela em 341 px | 972 px de tabela em 525 px |
| Elementos do replay fora do cartão | Cabeçalho, legendas e tempo total do replay | Não |

A causa é que o mapa do replay tem largura mínima de 500 px e a tabela tem 9 colunas, mas a coluna da direita só oferece cerca de 340 px úteis em 1366 e 525 px em 1920.

## Correção

O histórico geral passou para uma seção de largura total, abaixo do mapa e dos indicadores. O histórico compacto da sessão continua na coluna da direita. Com isso, a tabela mostra todas as colunas sem rolagem e o replay fica dentro do cartão nas duas resoluções, sem reduzir o mapa ao vivo.

**Observação sobre o Firefox:** os campos de data dos filtros aparecem como `mm/dd/yyyy`, pelo idioma padrão do navegador de teste; no Chromium aparecem como `dd/mm/aaaa`. O formato depende do idioma do navegador, e não é distorção de layout.

## Como reproduzir

Em `src/frontend`, com os navegadores instalados (`npx playwright install chromium firefox`):

```powershell
$env:E2E_EVIDENCIAS = "1"
npx playwright test layout-desktop
```

Os prints "antes" foram gerados com o mesmo comando no `main` anterior a esta correção, depois renomeados com o prefixo `antes-`. No CI, o teste roda a cada PR no job de E2E do **Frontend CI**, e os prints ficam anexados ao relatório HTML do Playwright (artefato `relatorio-e2e`).
