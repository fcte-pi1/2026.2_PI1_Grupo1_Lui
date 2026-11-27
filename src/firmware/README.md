# Firmwares do Micromouse

Todos os firmwares compiláveis e bibliotecas compartilhadas do projeto estão centralizados neste diretório (PlatformIO).

## Projetos e Bibliotecas

| Pasta | Papel | Alvo |
|:---|:---|:---|
| [`robo/`](robo) | Firmware oficial de navegação autônoma e prova (3x VL53L0X) | ESP32 (`esp32dev`) + `native` |
| [`gateway/`](gateway) | Receptor ESP-NOW -> JSON na serial USB | ESP8266 (`esp8266`), ESP32 (`esp32dev`) + `native` |
| [`bancada/`](bancada) | Console serial de comissionamento e testes isolados por componente (`env:teste_*`) | ESP32 (`esp32dev`, `teste_*`) + `test/run_tests.sh` |
| [`diagnostico_tof/`](diagnostico_tof) | Utilitário isolado para testar e localizar 1 sensor VL53L0X na protoboard | ESP32 (`esp32dev`) |
| [`lib/calibracao/`](lib/calibracao) | **Fonte única** de pinagem ([`PinosRobo.h`](lib/calibracao/src/PinosRobo.h)) e calibração ([`CalibracaoRobo.h`](lib/calibracao/src/CalibracaoRobo.h)) | Compartilhada (`robo`, `bancada`) |
| [`lib/telemetria/`](lib/telemetria) | Contrato de telemetria de 18 bytes (ESP-NOW -> JSON) | Compartilhada (`robo`, `gateway`, `bancada`) |

## Hardware Oficial

- **ESP32 DevKit V1 (30 pinos)**
- **3x Sensores ToF VL53L0X** em I2C (`SDA=21`, `SCL=22`): Esquerdo (`GPIO 4`, `0x30`), Frontal (`GPIO 16`, `0x31`) e Direito (`GPIO 17`, `0x32`).
- Para alterar qualquer pino ou constante física calibrada, edite apenas [`lib/calibracao/src/PinosRobo.h`](lib/calibracao/src/PinosRobo.h) e [`lib/calibracao/src/CalibracaoRobo.h`](lib/calibracao/src/CalibracaoRobo.h).

## Compilação e Testes

```bash
# Testes nativos (host)
pio test -d src/firmware/robo -e native
pio test -d src/firmware/gateway -e native
./src/firmware/bancada/test/run_tests.sh

# Build ESP32 / ESP8266 (somente compilar)
pio run -d src/firmware/robo -e esp32dev
pio run -d src/firmware/gateway -e esp8266
pio run -d src/firmware/gateway -e esp32dev
pio run -d src/firmware/bancada -e esp32dev
pio run -d src/firmware/diagnostico_tof -e esp32dev
```

## Gravação (Upload) nas Placas

> Feche o Monitor Serial e scripts Python que estejam usando a porta USB antes de rodar `-t upload`.

### ESP32 do Robô — Firmware de Bancada
```bash
# Menu interativo completo + suporte aos comandos remotos ESP-NOW
pio run -d src/firmware/bancada -e esp32dev -t upload

# (Opcional) Gravar um teste isolado por componente (ex.: mapa de XSHUT dos ToF)
pio run -d src/firmware/bancada -e teste_tof_mapa_xshut -t upload -t monitor
```

### ESP32 do Robô — Firmware Oficial da Prova
```bash
pio run -d src/firmware/robo -e esp32dev -t upload
```

### ESP8266 do Gateway (PC Server)
```bash
# NodeMCU / D1 mini conectado na USB
pio run -d src/firmware/gateway -e esp8266 -t upload

# (Se estiver usando um segundo ESP32 como gateway de bancada)
pio run -d src/firmware/gateway -e esp32dev -t upload
```

## Controle Remoto e Testes Sem Fio (`scripts/`)

Com o **ESP32 do robô** rodando o firmware `bancada` e o **ESP8266 gateway** conectado via USB (ex.: `/dev/ttyUSB0` ou `COM3`):

```bash
# Controle remoto completo (modos 1..9, avançar célula, curvas 90°/180°, parada 's'/ESPAÇO)
src/backend/.venv/bin/python scripts/controle_remoto.py /dev/ttyUSB0

# Teste rápido de motores tecla-a-tecla (e/E, d/D, +/-, s)
src/backend/.venv/bin/python scripts/teste_motores_remoto.py /dev/ttyUSB0

# Teste rápido dos 3 sensores ToF pela telemetria (livre / PAREDE < 110 mm)
src/backend/.venv/bin/python scripts/teste_sensores_remoto.py /dev/ttyUSB0

# Monitor de telemetria ao vivo
src/backend/.venv/bin/python scripts/monitor_telemetria.py /dev/ttyUSB0
```
*(No Windows PowerShell, troque `src/backend/.venv/bin/python` por `.\src\backend\.venv\Scripts\python.exe` e a porta por `COM3`/`COM5`.)*
