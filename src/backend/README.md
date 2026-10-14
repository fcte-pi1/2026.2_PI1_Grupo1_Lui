# Backend

Servidor da telemetria em Python 3.11 com FastAPI. Recebe as linhas JSON do gateway pela serial, difunde cada pacote pelo WebSocket e persiste as corridas no MySQL (doc 4.4, seção 4).

O formato das mensagens é o [contrato de dados](../README.md#contrato-de-dados), que está congelado.

## Estado atual

| Parte | Situação |
|:---|:---|
| `GET /health` | Pronto |
| WebSocket `/ws/telemetria` com difusão para todos os clientes | Pronto |
| Validação da mensagem (`TelemetriaMensagem`) | Pronta |
| Fontes de telemetria intercambiáveis: gateway pela serial ou simulador (`app/ingestao`) | Pronto |
| Simulador de corridas (`app/simulador`) | Pronto |
| Watchdog de enlace | A implementar (HU-09, #44) |
| Ciclo de vida da corrida e persistência (`app/servicos`, `app/repositorios`) | A implementar (HU-12, #49) |
| API REST do histórico (`/api/corridas`) | A implementar (#142) |

## Estrutura

```text
backend/
├── requirements.txt       # Dependências de execução e de teste
├── pyproject.toml         # Configuração do pytest e da cobertura
├── app/
│   ├── main.py            # criar_app() e a instância usada pelo Uvicorn
│   ├── config.py          # Variáveis de ambiente (fonte da telemetria e simulador)
│   ├── api/               # GET /health e WebSocket /ws/telemetria
│   ├── ingestao/          # Fontes de telemetria (serial e simulador) e difusão
│   ├── simulador/         # Pistas, robô virtual com flood fill e gerador de telemetria
│   ├── servicos/          # Ciclo de vida da corrida
│   ├── repositorios/      # Acesso ao banco
│   └── modelos/           # Esquemas de validação (TelemetriaMensagem)
└── tests/
```

## Como executar

```bash
cd src/backend
python -m venv .venv
.venv\Scripts\activate          # Windows (PowerShell)
source .venv/bin/activate       # Linux e macOS
pip install -r requirements.txt
uvicorn app.main:app --reload
```

- Saúde: <http://127.0.0.1:8000/health>
- Documentação interativa da API: <http://127.0.0.1:8000/docs>
- Telemetria em tempo real: `ws://127.0.0.1:8000/ws/telemetria`

## Fonte da telemetria

A fonte é escolhida pela variável `TELEMETRIA_FONTE`. Cada mensagem válida da fonte é difundida a todos os clientes do `/ws/telemetria`.

| Variável | Valores | Padrão |
|:---|:---|:---|
| `TELEMETRIA_FONTE` | `nenhuma`, `simulador` ou `serial` | `nenhuma` |
| `SERIAL_PORTA` | Porta do gateway: `COM3`, `/dev/ttyUSB0` ou uma URL do pyserial, como `socket://localhost:7777` | - (obrigatória com `serial`) |
| `SERIAL_BAUD` | Velocidade da serial | `115200` |
| `SIMULADOR_LABIRINTO` | `4x4`, `8x4` ou `12x4` | `4x4` |
| `SIMULADOR_CENARIO` | `completa`, `travado` ou `perda_conexao` | `completa` |
| `SIMULADOR_VELOCIDADE` | Fator de velocidade: `1` é uma mensagem por segundo, `5` é cinco vezes mais rápido | `1` |
| `SIMULADOR_REPETIR` | `1` para recomeçar a corrida ao terminar | `0` |
| `SIMULADOR_SEMENTE` | Semente dos valores aleatórios (bateria, velocidade, RSSI) | `141` |

O padrão é nenhuma fonte, para que um esquecimento no dia da prova não exiba uma corrida simulada como se fosse real; nesse caso o backend avisa no log. Valores inválidos impedem o backend de iniciar, com a mensagem do erro.

### Com o gateway (prova real)

```powershell
# PowerShell
$env:TELEMETRIA_FONTE = "serial"
$env:SERIAL_PORTA = "COM3"
uvicorn app.main:app
```

```bash
# Linux e macOS
TELEMETRIA_FONTE=serial SERIAL_PORTA=/dev/ttyUSB0 uvicorn app.main:app
```

Linhas vazias, linhas de diagnóstico do gateway (começam com `#`) e mensagens fora do contrato são descartadas. Se a porta não existir ou o cabo for desconectado, o backend registra o problema no log e tenta reabrir a porta a cada 2 segundos.

### Com o simulador

```powershell
# PowerShell
$env:TELEMETRIA_FONTE = "simulador"
$env:SIMULADOR_LABIRINTO = "8x4"
$env:SIMULADOR_CENARIO = "completa"
$env:SIMULADOR_REPETIR = "1"
uvicorn app.main:app
```

```bash
# Linux e macOS
TELEMETRIA_FONTE=simulador SIMULADOR_LABIRINTO=8x4 SIMULADOR_REPETIR=1 uvicorn app.main:app
```

Para voltar ao padrão no PowerShell, remova as variáveis com `Remove-Item Env:TELEMETRIA_FONTE` (e as demais) ou abra um novo terminal.

## Simulador de corridas

O simulador emula o robô explorando uma pista e gera, a cada célula percorrida, a mesma linha JSON que o gateway escreve na serial: mesma ordem de campos, `seq` e `rssi`, e as conversões de `TelemetriaJson.h`. Toda mensagem passa no `TelemetriaMensagem`.

- **Pistas:** as três pistas de referência são as dos slides do tema do PI1, desenhadas em [`app/simulador/pistas.txt`](app/simulador/pistas.txt). O formato está no cabeçalho do arquivo. Ao carregar, o simulador confere o perímetro fechado, a largada em A1 num beco aberto só para o leste e a chegada (D4, D8 ou D12) alcançável. As pistas de 8x4 e 12x4 têm regiões fechadas, fiéis aos slides.
- **Robô virtual:** só detecta as paredes da célula onde está (frente, esquerda e direita); o lado por onde entrou é passagem. Ele decide o caminho com flood fill sobre o próprio mapa, tratando paredes ainda não vistas como abertas. Esse flood fill serve só ao simulador; a navegação do firmware é a HU-06.
- **Mensagens:** duas em `AGUARDANDO` com `timestamp_ms` 0, depois uma por célula, com o tempo avançando 1000 ms. Velocidade em torno de 18 cm/s (menor em curvas e meias-voltas), bateria LiPo 2S descarregando devagar a partir de cerca de 8,2 V e RSSI entre -72 e -48 dBm.
- **Cenários:**

| Cenário | Como termina |
|:---|:---|
| `completa` | `CONCLUIDO` na chegada, com `desafio_cumprido` verdadeiro |
| `travado` | `TRAVADO` no meio do percurso, com velocidade atual 0 |
| `perda_conexao` | Para de enviar no meio do percurso, ainda em `MAPEANDO` |

Em `CONCLUIDO` e `TRAVADO` a última mensagem se repete duas vezes com o cronômetro congelado, como o robô real continua transmitindo depois de parar.

O simulador também roda sozinho, escrevendo as linhas na saída padrão:

```bash
python -m app.simulador --labirinto 12x4 --cenario travado
python -m app.simulador --labirinto 8x4 --tempo-real --velocidade 2
```

**Testar a fonte serial sem o gateway:** `SERIAL_PORTA` aceita `socket://host:porta`. Um programa que sirva as linhas do simulador por TCP faz o papel do ESP32, e o backend as lê pelo pyserial exatamente como leria a porta COM.

## Testes e cobertura

```bash
pytest --cov=app --cov-report=term-missing
```

O workflow [`backend-ci.yml`](../../.github/workflows/backend-ci.yml) roda esse comando em todo PR que altere `src/backend` e publica a tabela de cobertura no resumo da execução. Os testes não abrem portas reais: a serial é testada com uma porta falsa e com um gateway emulado por socket local.

Não versione `.env`, `.venv/`, `__pycache__/` nem os relatórios de cobertura; o `.gitignore` desta pasta já os ignora.
