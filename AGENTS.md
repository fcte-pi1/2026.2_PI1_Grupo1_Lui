# Guia do Repositório para Agentes de IA

Projeto **Micromouse — Grupo 1 (PI1 2026.2, FCTE/UnB)**.

## Mapa do Código

- `src/firmware/` — **Todos os firmwares ativos** (PlatformIO):
  - `src/firmware/robo/` — Firmware oficial de navegação e prova do robô (ESP32, `esp32dev` + testes `native` Unity).
  - `src/firmware/geral/` — Firmware unificado (ESP32 `esp32dev`) que combina a prova autônoma de `robo/` (incluindo comando remoto `k`) com os testes/menu de bancada usando os mesmos drivers e `ExecutorMovimentoRobo`.
  - `src/firmware/gateway/` — Gateway receptor ESP-NOW -> Serial JSON (ESP8266/ESP32 + testes `native` Unity).
  - `src/firmware/bancada/` — Firmware interativo de comissionamento e testes de bancada via Serial (ESP32 `esp32dev`, ambientes isolados por componente `env:teste_*` em `src/testes_componentes/` + testes em `test/run_tests.sh`).
  - `src/firmware/diagnostico_tof/` — Firmware mínimo para diagnóstico e auto-scan I2C/XSHUT de 1 sensor VL53L0X avulso.
  - `src/firmware/lib/calibracao/` — **Fonte única de verdade** de hardware e calibração:
    - `src/PinosRobo.h` — pinagem oficial do ESP32 DevKit V1 (30 pinos, 3x VL53L0X).
    - `src/CalibracaoRobo.h` — parâmetros mecânicos, PWM, offsets ToF e bateria.
  - `src/firmware/lib/telemetria/` — Contrato binário congelado de 18 bytes (ESP-NOW) e conversão JSON (issue #139).
- `src/backend/` — API REST, WebSocket e ingestão serial/simulador (Python 3.11, FastAPI, pytest).
- `src/frontend/` — Painel web de telemetria e histórico (React, Vite, Vitest, Playwright).
- `scripts/` — Utilitários de bancada via gateway ESP-NOW (`controle_remoto.py`, `teste_motores_remoto.py`, `teste_sensores_remoto.py`, `monitor_telemetria.py`).
- `hw/` — **Apenas artefatos de hardware e referência** (KiCad PCB em `micromouse_shield/`, `CHASI.md`, `ROTEIRO_TESTES_BANCADA.md`, tabela de pinagem e simulador Python `testes/mms/`). Não há firmwares ativos em `hw/`.
- `docs/` — Documentação acadêmica e relatórios do projeto (não alterar relatórios fechados sem pedido explícito).

## Hardware Oficial

- **MCU do robô:** ESP32 DevKit V1 (30 pinos).
- **Sensores de distância:** exclusivamente **3x VL53L0X (ToF I2C)** — Esquerdo (`GPIO 4`, `0x30`), Frontal (`GPIO 16`, `0x31`) e Direito (`GPIO 17`, `0x32`). Não existe suporte a modo de 2 sensores ou sensor ultrassônico.
- **Atuação e odometria:** Ponte H TB6612FNG + 2 motores N20 com encoders em quadratura.
- **Pinagem:** definida exclusivamente em `src/firmware/lib/calibracao/src/PinosRobo.h` (reexportada por `robo/src/hardware/Pinos.h` e `bancada/include/pinout.h`).

## Comandos Principais

```bash
# Testes nativos de firmware (use `pio` ou `src/backend/.venv/bin/pio`)
src/backend/.venv/bin/pio test -d src/firmware/robo -e native
src/backend/.venv/bin/pio test -d src/firmware/gateway -e native
./src/firmware/bancada/test/run_tests.sh

# Build ESP32 dos firmwares
src/backend/.venv/bin/pio run -d src/firmware/robo -e esp32dev
src/backend/.venv/bin/pio run -d src/firmware/geral -e esp32dev
src/backend/.venv/bin/pio run -d src/firmware/gateway -e esp32dev
src/backend/.venv/bin/pio run -d src/firmware/bancada -e esp32dev
src/backend/.venv/bin/pio run -d src/firmware/diagnostico_tof -e esp32dev

# Backend e Frontend
src/backend/.venv/bin/pytest src/backend
npm --prefix src/frontend run test
```
