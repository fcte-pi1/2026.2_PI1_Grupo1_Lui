# Código-fonte

| Pasta | Conteúdo | Tecnologias |
|:---|:---|:---|
| [`firmware/robo`](firmware/robo) | Firmware do robô | C++, Arduino-ESP32, PlatformIO |
| [`firmware/gateway`](firmware/gateway) | Firmware do ESP32 de bancada: recebe o ESP-NOW e escreve JSON na serial | C++, ArduinoJson, PlatformIO |
| [`firmware/lib/telemetria`](firmware/lib/telemetria) | Contrato de dados em C++, compartilhado por robô e gateway | C++ (somente cabeçalhos) |
| [`backend`](backend) | Ingestão da telemetria, WebSocket e API REST | Python 3.11, FastAPI |
| [`frontend`](frontend) | Painel web | React, Vite, Vitest |

A arquitetura completa está no [Projeto conceitual de software](../docs/4.4%20-%20Projeto%20conceitual%20de%20software.md) (doc 4.4).

---

## Contrato de dados

> [!IMPORTANT]
> **Contrato congelado (issue #139).** O formato descrito nesta seção é a interface entre firmware, backend e frontend, e as três partes são desenvolvidas em paralelo contando com ele. Qualquer mudança exige um PR revisado por ao menos uma pessoa de cada dupla de Software: Davi e João Pedro, Gustavo e Júnior, Pedro e Alexandre. Veja [Como mudar o contrato](#como-mudar-o-contrato).

```text
Robô ── ESP-NOW (18 bytes, 1 Hz) ──> Gateway ── serial USB (JSON, 115200 bps) ──> Backend ── WebSocket (JSON, 1 Hz) ──> Painel
```

Esta seção resume e detalha a seção 6 do doc 4.4. A fonte da verdade é o código listado em [Arquivos-fonte](#arquivos-fonte-do-contrato); em caso de divergência, o código prevalece e este documento deve ser corrigido.

### Convenções comuns

- **Posição:** colunas identificadas por letra (`A` a `D`) e linhas por número (`1` a `12`). A largada é a célula `A1`, no canto superior esquerdo da grade. Internamente o firmware usa índices a partir de zero, e a largada é `(0, 0)`.
- **Direções absolutas:** orientação e paredes são referenciadas ao labirinto, não à frente do robô. O norte corresponde ao topo da tela.
- **Paredes:** sempre as da célula em que o robô está no momento do pacote.

### 1. Pacote binário ESP-NOW (robô para gateway)

Definido em [`TelemetriaPacket.h`](firmware/lib/telemetria/src/TelemetriaPacket.h) e serializado por [`TelemetriaCodec.h`](firmware/lib/telemetria/src/TelemetriaCodec.h).

- Exatamente **18 bytes**, em **little-endian**. A serialização é feita campo a campo por `telemetriaCodificar`, sem depender do alinhamento da struct na memória.
- Enviado a cada **1000 ms** em broadcast no canal Wi-Fi 1 (`TELEMETRIA_PERIODO_MS` e `TELEMETRIA_CANAL_WIFI` em [`TelemetriaConfig.h`](firmware/lib/telemetria/src/TelemetriaConfig.h)).

| Offset | Campo | Tipo | Unidade | Valores válidos |
|---:|:---|:---|:---|:---|
| 0 | `timestamp_ms` | `uint32_t` | ms | 0 a 600000 (limite de 10 minutos de prova) |
| 4 | `labirinto_id` | `uint8_t` | - | 1 a 3 ([geometrias](#geometrias)) |
| 5 | `status_id` | `uint8_t` | - | 0 a 6 ([status](#status)) |
| 6 | `desafio_cumprido` | `uint8_t` | - | 0 (não) ou 1 (sim) |
| 7 | `bateria_pct` | `uint8_t` | % | 0 a 100 |
| 8 | `bateria_mv` | `uint16_t` | mV | 0 a 65535 (ex.: 7420 = 7,42 V) |
| 10 | `velocidade_atual` | `int16_t` | mm/s | -32768 a 32767 |
| 12 | `velocidade_media` | `int16_t` | mm/s | -32768 a 32767 |
| 14 | `coluna` | `uint8_t` | índice | 0 a 3 |
| 15 | `linha` | `uint8_t` | índice | 0 até o número de linhas da geometria menos 1 |
| 16 | `orientacao` | `uint8_t` | - | 0 a 3 ([orientações](#orientações)) |
| 17 | `paredes` | `uint8_t` | bitmask | 0x00 a 0x0F ([paredes](#paredes)) |

**Validação e descarte.** O gateway decodifica com `telemetriaDecodificar` e valida com `telemetriaValidar`. Um quadro com tamanho diferente de 18 bytes, ou com qualquer campo fora dos valores válidos (inclusive `linha` além da geometria e bits 4 a 7 de `paredes` ligados), é descartado: nada é escrito na serial e o `seq` não avança.

#### Status

| `status_id` | `status` (JSON) | Rótulo no painel | Significado |
|---:|:---|:---|:---|
| 0 | `AGUARDANDO` | Em espera | Robô ligado, aguardando a largada |
| 1 | `MAPEANDO` | Explorando | Exploração em andamento |
| 2 | `SPEED_RUN` | Speed Run | Percurso otimizado após a exploração |
| 3 | `COLISAO` | Colisão | Prova encerrada por colisão |
| 4 | `CONCLUIDO` | Concluído | Robô alcançou a célula objetivo |
| 5 | `TIMEOUT` | Tempo esgotado | Limite de 10 minutos atingido |
| 6 | `TRAVADO` | Travado | Tração cortada por travamento das rodas |

"Conexão RF perdida" não é um status do protocolo: é um estado do painel quando nenhum pacote chega por mais de 3 segundos (doc 4.4, seção 6.7).

#### Geometrias

| `labirinto_id` | `labirinto` (JSON) | Colunas | Linhas | `posicao.linha` válida |
|---:|:---|:---|:---|:---|
| 1 | `4x4` | A a D | 4 | 1 a 4 |
| 2 | `8x4` | A a D | 8 | 1 a 8 |
| 3 | `12x4` | A a D | 12 | 1 a 12 |

#### Orientações

| `orientacao` (pacote) | `orientacao` (JSON) |
|---:|:---|
| 0 | `NORTE` |
| 1 | `SUL` |
| 2 | `LESTE` |
| 3 | `OESTE` |

#### Paredes

| Bit | Máscara | Direção | Campo no JSON |
|---:|:---|:---|:---|
| 0 | `0x01` | Norte | `paredes.norte` |
| 1 | `0x02` | Sul | `paredes.sul` |
| 2 | `0x04` | Leste | `paredes.leste` |
| 3 | `0x08` | Oeste | `paredes.oeste` |
| 4 a 7 | - | Reservados | Devem ser zero |

Exemplo: `0x06` indica parede ao sul e a leste.

### 2. Linha JSON na serial (gateway para backend)

Gerada por [`TelemetriaJson.h`](firmware/lib/telemetria/src/TelemetriaJson.h) e [`GatewayBridge.h`](firmware/lib/telemetria/src/GatewayBridge.h).

- **115200 bps**, 8N1 (`TELEMETRIA_BAUD_SERIAL`).
- **Um objeto JSON por pacote válido**, em uma única linha terminada por `\n`.
- Com a opção de compilação `-DGATEWAY_DEBUG`, o gateway também escreve linhas de diagnóstico que começam com `#` e **não são JSON**. Essa opção não deve ser usada com o backend; ainda assim, o backend descarta qualquer linha que não passe na validação.

```json
{"seq":42,"rssi":-58,"timestamp_ms":142000,"labirinto":"8x4","status":"MAPEANDO","desafio_cumprido":false,"bateria_pct":78,"bateria_volts":7.42,"velocidade_atual":35,"velocidade_media":28.5,"posicao":{"coluna":"B","linha":4},"orientacao":"LESTE","paredes":{"norte":false,"sul":true,"leste":true,"oeste":false}}
```

**Conversões feitas pelo gateway**

| Campo no pacote | Campo no JSON | Conversão |
|:---|:---|:---|
| `timestamp_ms` | `timestamp_ms` | Nenhuma |
| `labirinto_id` | `labirinto` | 1, 2, 3 para `"4x4"`, `"8x4"`, `"12x4"` |
| `status_id` | `status` | Número para o nome ([status](#status)) |
| `desafio_cumprido` | `desafio_cumprido` | 0 e 1 para `false` e `true` |
| `bateria_pct` | `bateria_pct` | Nenhuma |
| `bateria_mv` | `bateria_volts` | Divide por 1000 (mV para V) |
| `velocidade_atual` | `velocidade_atual` | Divide por 10 (mm/s para cm/s) |
| `velocidade_media` | `velocidade_media` | Divide por 10 (mm/s para cm/s) |
| `coluna` | `posicao.coluna` | Índice 0 a 3 para a letra `"A"` a `"D"` |
| `linha` | `posicao.linha` | Índice mais 1 (0 a 11 para 1 a 12) |
| `orientacao` | `orientacao` | Número para o nome ([orientações](#orientações)) |
| `paredes` | `paredes.norte`, `.sul`, `.leste`, `.oeste` | Um booleano por bit ([paredes](#paredes)) |

**Campos acrescentados pelo gateway** (não existem no pacote do robô nem no exemplo da seção 6.4 do doc 4.4):

- `seq`: número de sequência dos pacotes válidos entregues pelo gateway. Começa em 1 e reinicia quando o gateway é religado. Como é contado no gateway, um pacote perdido no rádio não deixa lacuna no `seq`; a perda no enlace é estimada pela taxa fixa de 1 Hz.
- `rssi`: potência do quadro recebido, em dBm (ex.: `-58`; quanto mais perto de 0, mais forte o sinal).

**Números decimais.** `bateria_volts`, `velocidade_atual` e `velocidade_media` são números JSON e podem vir sem casas decimais: o ArduinoJson escreve `35.0` como `35`. Quem consome o JSON deve aceitar inteiro ou decimal nesses campos.

### 3. Mensagem do WebSocket (backend para painel)

- **Endpoint:** `ws://<host>:<porta>/ws/telemetria` (WebSocket, RFC 6455).
- **Frequência:** 1 Hz, uma mensagem por pacote recebido.
- **Formato:** quadro de texto com um objeto JSON em UTF-8.
- O backend valida cada linha da serial com [`TelemetriaMensagem`](backend/app/modelos/telemetria.py) e difunde o mesmo objeto a todos os clientes conectados. Linhas inválidas são descartadas.
- Os campos são os da linha serial. `seq` e `rssi` são opcionais: só aparecem quando vierem do gateway. Campos desconhecidos são ignorados na validação, para que um campo novo no gateway não derrube a ingestão durante a prova, e não são repassados ao painel.
- O canal é só de ida: o servidor ignora o que o cliente enviar, porque a aplicação web nunca envia comandos ao robô.

Exemplo da seção 6.4 do doc 4.4 (mensagem sem `seq` e `rssi`):

```json
{
  "timestamp_ms": 142000,
  "labirinto": "8x4",
  "status": "MAPEANDO",
  "desafio_cumprido": false,
  "bateria_pct": 78,
  "bateria_volts": 7.42,
  "velocidade_atual": 35.0,
  "velocidade_media": 28.5,
  "posicao": {
    "coluna": "B",
    "linha": 4
  },
  "orientacao": "LESTE",
  "paredes": {
    "norte": false,
    "sul": true,
    "leste": true,
    "oeste": false
  }
}
```

### Dicionário de dados

Vale para a linha serial e para a mensagem do WebSocket.

| Campo | Tipo JSON | Unidade | Obrigatório | Restrições |
|:---|:---|:---|:---|:---|
| `timestamp_ms` | inteiro | ms | sim | Tempo decorrido de prova, de 0 a 600000 |
| `labirinto` | string | - | sim | `"4x4"`, `"8x4"` ou `"12x4"` |
| `status` | string | - | sim | `"AGUARDANDO"`, `"MAPEANDO"`, `"SPEED_RUN"`, `"COLISAO"`, `"CONCLUIDO"`, `"TIMEOUT"` ou `"TRAVADO"` |
| `desafio_cumprido` | booleano | - | sim | `true` se o robô alcançou a célula objetivo |
| `bateria_pct` | inteiro | % | sim | 0 a 100 |
| `bateria_volts` | número | V | sim | 0 a 65.535 (limite do `uint16_t` em mV) |
| `velocidade_atual` | número | cm/s | sim | -3276.8 a 3276.7 (limite do `int16_t` em mm/s) |
| `velocidade_media` | número | cm/s | sim | -3276.8 a 3276.7 |
| `posicao.coluna` | string | - | sim | `"A"`, `"B"`, `"C"` ou `"D"` |
| `posicao.linha` | inteiro | - | sim | 1 até o número de linhas da geometria (4, 8 ou 12) |
| `orientacao` | string | - | sim | `"NORTE"`, `"SUL"`, `"LESTE"` ou `"OESTE"` |
| `paredes.norte` | booleano | - | sim | Parede detectada ao norte da célula atual |
| `paredes.sul` | booleano | - | sim | Parede detectada ao sul da célula atual |
| `paredes.leste` | booleano | - | sim | Parede detectada a leste da célula atual |
| `paredes.oeste` | booleano | - | sim | Parede detectada a oeste da célula atual |
| `seq` | inteiro | - | não | 1 a 4294967295, acrescentado pelo gateway |
| `rssi` | inteiro | dBm | não | -128 a 127, acrescentado pelo gateway |

Os tipos são estritos na validação do backend: inteiros não aceitam texto nem número com casas decimais (`"78"` e `78.5` são rejeitados), e booleanos só aceitam `true` e `false`. Campos do tipo número aceitam inteiros.

### Arquivos-fonte do contrato

| Arquivo | Papel |
|:---|:---|
| [`firmware/lib/telemetria/src/TelemetriaPacket.h`](firmware/lib/telemetria/src/TelemetriaPacket.h) | Struct de 18 bytes e enums de status e geometria |
| [`firmware/lib/telemetria/src/TelemetriaCodec.h`](firmware/lib/telemetria/src/TelemetriaCodec.h) | Codificação, decodificação, validação e nomes textuais |
| [`firmware/lib/telemetria/src/TelemetriaJson.h`](firmware/lib/telemetria/src/TelemetriaJson.h) | Conversão do pacote para o JSON |
| [`firmware/lib/telemetria/src/GatewayBridge.h`](firmware/lib/telemetria/src/GatewayBridge.h) | Lógica do gateway: validação, `seq` e descarte |
| [`firmware/lib/telemetria/src/TelemetriaConfig.h`](firmware/lib/telemetria/src/TelemetriaConfig.h) | Canal Wi-Fi, período de 1 Hz e baud rate da serial |
| [`backend/app/modelos/telemetria.py`](backend/app/modelos/telemetria.py) | Validação da mensagem no backend |

Testes que verificam o contrato: [`firmware/robo/test/test_telemetria`](firmware/robo/test/test_telemetria/test_telemetria.cpp), [`firmware/gateway/test/test_bridge`](firmware/gateway/test/test_bridge/test_bridge.cpp) e [`backend/tests/test_modelos_telemetria.py`](backend/tests/test_modelos_telemetria.py).

### Como mudar o contrato

1. Abra uma issue descrevendo a mudança e o motivo.
2. No mesmo PR, atualize todas as partes afetadas: os cabeçalhos de `firmware/lib/telemetria`, o schema `backend/app/modelos/telemetria.py`, os consumidores no frontend, os testes das três partes, este README e a seção 6 do doc 4.4.
3. Peça revisão de ao menos uma pessoa de cada dupla de Software: Davi e João Pedro, Gustavo e Júnior, Pedro e Alexandre. O PR só é mesclado com as três aprovações.
4. Grave robô e gateway com a mesma versão: o gateway descarta pacotes com tamanho diferente de 18 bytes.

**Limitação conhecida:** o pacote não carrega número de sequência gerado pelo robô, então a perda de pacotes no rádio é estimada pela taxa fixa de 1 Hz (ver o teste de enlace no [README do gateway](firmware/gateway/README.md)). Incluir esse campo é uma mudança de contrato.

---

## Testes e integração contínua

Cada parte tem um workflow no GitHub Actions que roda em todo PR que altere a sua pasta e em todo push na `main`.

| Parte | Workflow | O que roda | Localmente |
|:---|:---|:---|:---|
| Firmware | [`firmware-ci.yml`](../.github/workflows/firmware-ci.yml) | Testes nativos (Unity) com cobertura (gcovr) e build para o ESP32, para `robo` e `gateway` | `pio test -e native` em `firmware/robo` e em `firmware/gateway` |
| Backend | [`backend-ci.yml`](../.github/workflows/backend-ci.yml) | pytest com cobertura (pytest-cov) | `pytest --cov=app` em `backend` |
| Frontend | [`frontend-ci.yml`](../.github/workflows/frontend-ci.yml) | ESLint, Vitest com cobertura (v8) e build | `npm run lint`, `npm run test:coverage` e `npm run build` em `frontend` |

**Onde ver a cobertura:** na aba *Checks* do PR, abra a execução do workflow e role até o *Summary*, que mostra a tabela de cobertura por arquivo. O relatório HTML completo fica nos artefatos da execução (`cobertura-*`). Por enquanto não há percentual mínimo: a cobertura é informativa e não reprova o PR.

**Cobertura do firmware localmente.** O env `native` já compila com `--coverage`. Depois de `pio test -e native`, rode em `src/firmware` (troque `robo` por `gateway` se for o caso):

```bash
gcovr --root . --filter lib/ --filter robo/src/ robo/.pio/build/native
```

Como a biblioteca de telemetria é feita só de cabeçalhos, funções `inline` que nenhum teste chama não são geradas pelo compilador e, por isso, não aparecem como linhas descobertas.
