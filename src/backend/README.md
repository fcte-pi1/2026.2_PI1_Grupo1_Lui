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
| Persistência e ciclo de vida (`app/servicos`, `app/banco.py`) | Implementados (RF-SFT07) |
| API REST do histórico (`/api/corridas`) | Implementada |

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

No Windows (PowerShell):

```powershell
cd src\backend
python -m venv .venv                                         # só na primeira vez
.\.venv\Scripts\python -m pip install -r requirements.txt    # só na primeira vez
.\.venv\Scripts\python -m uvicorn app.main:app --reload
```

Os comandos chamam o Python de dentro do `.venv`, então não é preciso ativar o ambiente virtual. Rodar só `uvicorn` sem o ambiente ativado dá o erro "O termo 'uvicorn' não é reconhecido", e ativá-lo com `.venv\Scripts\Activate.ps1` costuma ser bloqueado pela política de execução de scripts do PowerShell.

No Linux e no macOS:

```bash
cd src/backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload
```

- Saúde: <http://127.0.0.1:8000/health>
- Documentação interativa da API: <http://127.0.0.1:8000/docs>
- Telemetria em tempo real: `ws://127.0.0.1:8000/ws/telemetria`
- Histórico: `http://127.0.0.1:8000/api/corridas`; detalhe e leituras: `/api/corridas/{id}`

## Persistência das corridas (RF-SFT07)

O backend mapeia as tabelas existentes `Corrida` e `Leitura` do MySQL. SQLite continua disponível para desenvolvimento e testes; em produção, a URL deve apontar para o schema `mydb`. Para um banco vazio, use `migrations/001_corridas.sql`. Para atualizar o modelo MySQL original enviado pelo grupo, faça backup e aplique uma única vez `migrations/002_adaptar_schema_existente_rf_sft07.sql` antes de iniciar o backend. O `create_all` do SQLAlchemy cria tabelas ausentes, mas não substitui migrações em tabelas existentes.

Exemplo no PowerShell (use as credenciais locais do seu MySQL):

```powershell
$env:DATABASE_URL = "mysql+pymysql://usuario:senha@localhost:3306/mydb"
```

O primeiro pacote `MAPEANDO`/`SPEED_RUN` após a espera cria a corrida, alinhado à largada do firmware (`src/firmware/robo/src/main.cpp`). Nesse momento, os campos ainda desconhecidos começam com valores neutros; `fim` e `status_final` permanecem nulos. Cada pacote da corrida, inclusive o terminal, é gravado e confirmado no banco antes de ser difundido pelo WebSocket. `CONCLUIDO`, `TRAVADO`, `COLISAO` e `TIMEOUT` encerram normalmente. Silêncio por mais de `TELEMETRIA_TIMEOUT_S` (padrão 5 s), fim/erro inesperado da fonte, nova espera durante uma corrida ou reinício do backend preenche `fim` e `status_final=INTERROMPIDA`. O timeout deve ser maior que o intervalo de envio configurado pelo firmware.

`modo` recebe `EXPLORACAO` ou `SPEED_RUN`; `AGUARDANDO` continua sendo apenas status da telemetria e não é gravado como modo. A tentativa não é enviada pela ESP, então `tentativa` fica nula até que o contrato forneça esse dado. As quatro paredes são guardadas em cada linha de `Leitura` para que o replay reconstrua o mapa ao longo do tempo. Leituras anteriores à migração não têm essas paredes e permanecem com valores nulos (desconhecidos).

O protocolo do gateway não foi alterado: `INTERROMPIDA` é um status calculado pelo backend, não uma mensagem nova do hardware. A tabela `Celula` permanece no schema para compatibilidade; o replay temporal usa as paredes registradas em `Leitura`.

Endpoints:

```text
GET /api/corridas       lista metadados, mais recentes primeiro
GET /api/corridas/{id}  metadados e leituras JSON ordenadas pela chegada
```

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
.\.venv\Scripts\python -m uvicorn app.main:app
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
.\.venv\Scripts\python -m uvicorn app.main:app
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
- **Espera na largada:** com a fonte `simulador` do backend, enquanto nenhum cliente estiver conectado ao `/ws/telemetria`, o simulador repete a mensagem `AGUARDANDO` em A1 a cada intervalo, como o robô real esperando o botão, e só dá a largada quando um painel conecta. Assim o painel não perde o começo da corrida por ter conectado alguns segundos depois do backend. A espera se repete a cada corrida (com `SIMULADOR_REPETIR=1`) se não houver ninguém conectado. A fonte serial não muda: o robô real transmite no ritmo dele.
- **Mensagens:** duas em `AGUARDANDO` com `timestamp_ms` 0, depois uma por célula, com o tempo avançando 1000 ms. Velocidade em torno de 18 cm/s (menor em curvas e meias-voltas), bateria LiPo 2S descarregando devagar a partir de cerca de 8,2 V e RSSI entre -72 e -48 dBm.
- **Cenários:**

| Cenário | Como termina |
|:---|:---|
| `completa` | `CONCLUIDO` na chegada, com `desafio_cumprido` verdadeiro |
| `travado` | `TRAVADO` no meio do percurso, com velocidade atual 0 |
| `perda_conexao` | Para de enviar no meio do percurso, ainda em `MAPEANDO` |

Em `CONCLUIDO` e `TRAVADO` a última mensagem se repete duas vezes com o cronômetro congelado, como o robô real continua transmitindo depois de parar.

O simulador também roda sozinho, escrevendo as linhas na saída padrão:

```powershell
.\.venv\Scripts\python -m app.simulador --labirinto 12x4 --cenario travado
.\.venv\Scripts\python -m app.simulador --labirinto 8x4 --tempo-real --velocidade 2
```

No Linux e no macOS, com o ambiente ativado, use `python -m app.simulador` com os mesmos argumentos.

**Testar a fonte serial sem o gateway:** `SERIAL_PORTA` aceita `socket://host:porta`. Um programa que sirva as linhas do simulador por TCP faz o papel do ESP32, e o backend as lê pelo pyserial exatamente como leria a porta COM.

## Testes e cobertura

```powershell
.\.venv\Scripts\python -m pytest --cov=app --cov-report=term-missing
```

No Linux e no macOS, com o ambiente ativado: `pytest --cov=app --cov-report=term-missing`.

O workflow [`backend-ci.yml`](../../.github/workflows/backend-ci.yml) roda esse comando em todo PR que altere `src/backend` e publica a tabela de cobertura no resumo da execução. Os testes não abrem portas reais: a serial é testada com uma porta falsa e com um gateway emulado por socket local.

Não versione `.env`, `.venv/`, `__pycache__/` nem os relatórios de cobertura; o `.gitignore` desta pasta já os ignora.
