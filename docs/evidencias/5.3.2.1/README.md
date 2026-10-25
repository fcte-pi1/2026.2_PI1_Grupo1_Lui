# Evidências: EAP 5.3.2.1 (Testes E2E com Playwright)

Prints e vídeo gerados pelo próprio Playwright com `npm run test:e2e:evidencias` (em `src/frontend`), com o backend rodando o simulador no lugar do robô e do gateway. Gerados em 25/10/2026; os 6 testes passaram.

| Evidência | Arquivo | O que mostra |
|:---|:---|:---|
| CT-09 em 4x4 | [ct09-4x4-concluido.png](ct09-4x4-concluido.png) | Corrida concluída, robô em D4, paredes e rastro desenhados |
| CT-09 em 8x4 | [ct09-8x4-concluido.png](ct09-8x4-concluido.png) | Corrida concluída, robô em D8 |
| CT-09 em 12x4, durante a exploração | [ct09-12x4-explorando.png](ct09-12x4-explorando.png) | Mapa desenhado aos poucos, status "Explorando" |
| CT-09 em 12x4, ao final | [ct09-12x4-concluido.png](ct09-12x4-concluido.png) | Corrida concluída, robô em D12 |
| CT-09 em 12x4, vídeo | [ct09-12x4.webm](ct09-12x4.webm) | A corrida inteira, da espera na largada até a chegada |
| CT-15 | [ct15-travado.png](ct15-travado.png) | Status "Travado" em destaque, com o aviso de corte de tração |
| CT-08 (parcial), backend derrubado | [ct08-desconectado.png](ct08-desconectado.png) | Indicador DESCONECTADO, último estado mantido na tela |
| CT-08 (parcial), backend de volta | [ct08-reconectado.png](ct08-reconectado.png) | Indicador CONECTADO; o backend novo começa outra corrida (Tentativa 2) |
| Recomeço do mapa | [recomeco-tentativa-2.png](recomeco-tentativa-2.png) | Na Tentativa 2, mapa limpo e robô em A1; a Tentativa 1 fica no histórico |

O relatório HTML do Playwright de cada execução no CI fica no artefato `relatorio-e2e` do workflow **Frontend CI**.
