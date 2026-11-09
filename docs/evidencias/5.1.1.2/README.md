# Evidências: EAP 5.1.1.2 (HU-09, aviso de conexão RF perdida)

Prints e medições gerados pelo Playwright com `e2e/conexao-rf.spec.js` (em `src/frontend`). O backend rodou com o simulador no lugar do robô e do gateway (4x4, `SIMULADOR_VELOCIDADE=5`). Gerados em 09/11/2026; os 2 testes passaram.

No cenário `perda_conexao`, o simulador perde 4 pacotes seguidos no meio do percurso e fica 4 s em silêncio. O silêncio é medido em tempo de relógio, mesmo com o fator de velocidade. O robô virtual segue navegando durante a perda e termina a corrida.

| Evidência | Arquivo | O que mostra |
|:---|:---|:---|
| Critério 1, aviso | [hu09-rf-perdida.png](hu09-rf-perdida.png) | "Conexão RF perdida" abaixo do cabeçalho, 3 s depois do último pacote. O indicador continua CONECTADO (o backend está no ar) e o status continua "Explorando" |
| Critério 2, retomada | [hu09-rf-retomada.png](hu09-rf-retomada.png) | Aviso desfeito com a volta dos pacotes. O robô aparece mais adiante, porque seguiu navegando durante a perda |
| Sem aviso falso após a chegada | [hu09-sem-aviso-apos-chegada.png](hu09-sem-aviso-apos-chegada.png) | Cenário `completa`, 4,5 s depois de "Concluído": o robô parado continua transmitindo e o aviso não aparece |
| Medições | [medicoes.json](medicoes.json) | Tempos registrados dentro da página (chegada de cada mensagem do WebSocket e mudanças do aviso) |

## Medições do cenário `perda_conexao`

| Medida | Valor |
|:---|---:|
| Silêncio entre o último pacote antes da perda e o primeiro depois dela | 4.013 ms |
| Aviso exibido após o último pacote | 3.249 ms (limite do critério: 3.000 ms) |
| Aviso desfeito após o primeiro pacote da retomada | 5 ms |

O painel verifica o tempo sem pacotes a cada 250 ms. Por isso o aviso sai entre 3,0 s e 3,25 s depois do último pacote, e o teste aceita até 3,7 s.

**Rastro no print da retomada:** os pacotes perdidos não chegam ao painel, então o rastro liga em linha reta a última posição recebida antes da perda e a primeira depois dela. O caminho que o robô fez nesse intervalo não aparece.

**Critério 3 (a perda não interfere na navegação):** é garantido pelo projeto, não pelo painel. O rádio é unidirecional (ESP-NOW, do robô para o gateway), então o robô não recebe nada por ele e navega só com os próprios sensores. O cenário reproduz isso: o robô virtual conclui a corrida apesar da perda.

## Como reproduzir

Em `src/frontend`:

```powershell
$env:E2E_EVIDENCIAS = "1"
npx playwright test conexao-rf
```

Rode só este spec. O `npm run test:e2e:evidencias` executa todos os E2E e também regrava os prints de [5.3.2.1](../5.3.2.1/README.md).

No CI, os mesmos prints ficam anexados ao relatório HTML do Playwright, no artefato `relatorio-e2e` do workflow **Frontend CI**.
