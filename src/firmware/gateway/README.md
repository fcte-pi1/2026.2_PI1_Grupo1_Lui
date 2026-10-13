# Gateway ESP-NOW → Serial (EAP 3.2.1.1)

Firmware do **ESP32 de bancada**. Recebe a telemetria do robô por ESP-NOW, valida o pacote de 18 bytes e imprime **uma linha JSON por pacote** na porta serial USB, conforme o contrato de dados (`docs/4.4`, seção 6).

- Serial: **115200 bps**, 8N1, delimitador `\n`.
- Cada JSON inclui os campos do contrato **mais** `seq` e `rssi`.
- Pacotes com tamanho diferente de 18 bytes ou com campos fora do domínio são **descartados** (nada é impresso).

## Gravar o firmware

Requer [PlatformIO](https://platformio.org/) (`pip install platformio`).

```bash
cd src/firmware/gateway
pio run -e esp32dev -t upload      # grava o ESP32 gateway (USB)
pio device monitor -b 115200       # visualiza o JSON
```

O robô deve estar gravado com o firmware de `src/firmware/robo` (envia a 1 Hz).
Ambos usam **canal Wi-Fi 1** (`TELEMETRIA_CANAL_WIFI` em `lib/telemetria/src/TelemetriaConfig.h`) e **broadcast**, então **não é preciso parear MACs**.

## Formato da saída

```json
{"seq":42,"rssi":-58,"timestamp_ms":142000,"labirinto":"8x4","status":"MAPEANDO","desafio_cumprido":false,"bateria_pct":78,"bateria_volts":7.42,"velocidade_atual":35,"velocidade_media":28.5,"posicao":{"coluna":"B","linha":4},"orientacao":"LESTE","paredes":{"norte":false,"sul":true,"leste":true,"oeste":false}}
```

| Campo | Origem |
|:---|:---|
| `seq` | Contador de pacotes **válidos** recebidos pelo gateway, começa em 1 (reinicia ao religar o gateway) |
| `rssi` | Potência do quadro recebido, em dBm (ex.: `-58`; mais perto de 0 = sinal mais forte) |
| demais campos | Conversões do doc 4.4 §6.2: índices → `A`–`D` / `1`–`12`, mm/s → cm/s, mV → V, bitmask → booleanos |

O ArduinoJson escreve decimais sem parte fracionária como inteiros (`35.0` sai como `35`). O formato completo, congelado, está no [contrato de dados](../../README.md#contrato-de-dados).

## Diagnóstico opcional

Para ver pacotes descartados e o MAC do gateway, compile com `-DGATEWAY_DEBUG` (descomente `build_flags` no `platformio.ini`). As linhas de diagnóstico começam com `#` e **não são JSON** — não use essa opção com o backend.

## Testes

```bash
pio test -e native      # em src/firmware/gateway e em src/firmware/robo
```

A lógica (decodificação, validação, JSON, `seq`) fica em `src/firmware/lib/telemetria` e é testada nativamente, sem hardware. O CI (`.github/workflows/firmware-ci.yml`) roda esses testes, publica a cobertura (gcovr) no resumo da execução e compila os dois firmwares a cada push/PR.

## Teste de enlace (EAP 3.1.5.1) — @LeonardoAAugusto e @PedroFCR

Objetivo: medir **perda de pacotes a 8 m** (RNF-ELE03).

1. Grave o gateway (este projeto) e o robô (`src/firmware/robo`). Ligue o robô; ele envia 1 pacote/s mesmo em `AGUARDANDO`.
2. Conecte o gateway ao PC e capture a serial em arquivo, por exemplo:
   ```bash
   pio device monitor -b 115200 --filter log2file   # gera platformio-device-monitor-*.log
   ```
   ou, no Python: `python -m serial.tools.miniterm COMx 115200 > enlace_8m.log`.
3. Execute cada distância (ex.: 1 m, 4 m, 8 m) por **≥ 120 s**, anotando a duração exata.
4. Calcule a perda: como o robô envia exatamente 1 Hz, **perda (%) = (1 − pacotes_recebidos / duração_s) × 100**. `pacotes_recebidos` é o maior `seq` do trecho menos o `seq` inicial mais 1 (reinicie o gateway no início de cada distância para `seq` começar em 1).
5. Registre o `rssi` médio/mínimo de cada distância (campo `rssi` do JSON).
6. **Evidências obrigatórias** (fotografar ou gravar): montagem com a distância medida (trena visível), tela do monitor serial com o JSON e o `seq`, e o arquivo de log. Guarde em `docs/evidencias/3.1.5.1/`.

> Limitação conhecida: o pacote de 18 bytes do contrato não carrega número de sequência do robô; por isso a perda é inferida pela taxa fixa de 1 Hz e pelo `seq` do gateway. Se for necessária contagem por pacote, o contrato (EAP 5.3.1.1) precisaria de um campo extra.
