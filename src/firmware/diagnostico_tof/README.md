# Firmware de Teste Individual de Sensor VL53L0X (1 por vez)

Firmware criado para validar a saúde de **um sensor VL53L0X por vez**, com suporte a **troca de portas ao vivo via Monitor Serial** (sem precisar saber os pinos antes nem recompilar a cada teste).

## Portas Padrão ao Ligar
- `SDA`: GPIO `21`
- `SCL`: GPIO `22`
- `XSHUT`: GPIO `4`
- `VCC`: `3.3V`
- `GND`: `GND`

*(Se quiser alterar os pinos padrão antes de gravar, edite `DEFAULT_PIN_SDA`, `DEFAULT_PIN_SCL` e `DEFAULT_PIN_XSHUT` no topo de `src/main.cpp`.)*

## Comandos no Monitor Serial (`115200` bps)
- `p <sda> <scl>` — Muda SDA e SCL ao vivo (ex: `p 21 22`, `p 19 23`)
- `p <sda> <scl> <xshut>` — Muda SDA, SCL e XSHUT ao vivo (ex: `p 21 22 4`)
- `x <gpio>` — Define apenas o pino XSHUT (`-1` para ignorar)
- `s` — Escaneia o barramento I2C atual (e detecta se SDA/SCL estão invertidos)
- `a` — **Auto-scan seguro**: procura o sensor automaticamente nos GPIOs `4, 5, 14, 16, 17, 19, 21, 22, 23`
- `r` — Reinicializa o sensor
- `c` — Pausa ou retoma a leitura contínua
- `m` — Exibe o menu de ajuda

## Como Compilar e Gravar
```bash
# Apenas compilar
src/backend/.venv/bin/pio run -d src/firmware/diagnostico_tof

# Gravar na ESP32 e abrir o monitor serial (ex: /dev/ttyUSB0)
src/backend/.venv/bin/pio run -d src/firmware/diagnostico_tof -t upload --upload-port /dev/ttyUSB0
src/backend/.venv/bin/python -m serial.tools.miniterm /dev/ttyUSB0 115200
```
