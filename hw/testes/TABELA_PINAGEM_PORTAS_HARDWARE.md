# Tabela Mestra de Mapeamento de Portas e Pinos (Pinout Matrix)
## Projeto Micromouse - PI1 2026.2 (Prof. Lui - Grupo 1)
### ESP32 DevKit V1 (30 Pinos) — Configuração Oficial (3x VL53L0X)

> **Fonte única no código:** [`src/firmware/lib/calibracao/src/PinosRobo.h`](../../src/firmware/lib/calibracao/src/PinosRobo.h).

---

## 1. Mapeamento Oficial de Pinos em Uso

| GPIO ESP32 | Direção | Conexão no Micromouse | Função / Descrição Técnica |
|:---:|:---|:---|:---|
| **GPIO 2** | Entrada | Encoder Direito Canal B | Quadratura do Motor 2 (com pull-up interno) |
| **GPIO 4** | Saída | ToF XSHUT Esquerdo | Habilitação do VL53L0X Esquerdo (`0x30`) |
| **GPIO 13** | Saída | Ponte H STBY | Habilita a TB6612FNG em nível `HIGH` |
| **GPIO 14** | Saída | LED de Estado | Sinalização visual de status do robô |
| **GPIO 15** | Entrada | Botão de Largada / IHM | Pull-up interno (`LOW` = pressionado) |
| **GPIO 16** | Saída | ToF XSHUT Frontal | Habilitação do VL53L0X Frontal (`0x31`) |
| **GPIO 17** | Saída | ToF XSHUT Direito | Habilitação do VL53L0X Direito (`0x32`) |
| **GPIO 18** | Saída | Ponte H PWMB | PWM 20 kHz do Motor Direito |
| **GPIO 21** | Bidirecional | I2C SDA | Barramento I2C dos 3 VL53L0X (100 kHz) |
| **GPIO 22** | Saída | I2C SCL | Clock I2C dos 3 VL53L0X (100 kHz) |
| **GPIO 25** | Saída | Ponte H PWMA | PWM 20 kHz do Motor Esquerdo |
| **GPIO 26** | Saída | Ponte H AIN1 | Sentido A do Motor Esquerdo |
| **GPIO 27** | Saída | Ponte H AIN2 | Sentido B do Motor Esquerdo |
| **GPIO 32** | Saída | Ponte H BIN2 | Sentido B do Motor Direito |
| **GPIO 33** | Saída | Ponte H BIN1 | Sentido A do Motor Direito |
| **GPIO 34** | Entrada | Encoder Esquerdo Canal A | Interrupção externa (entrada pura, sem pull-up) |
| **GPIO 35** | Entrada | Encoder Direito Canal A | Interrupção externa (entrada pura, sem pull-up) |
| **GPIO 36 (VP)** | Entrada | Monitor da Bateria 2S | Leitura ADC1_CH0 do divisor 5:1 (`30k / 7,5k`) |
| **GPIO 39 (VN)** | Entrada | Encoder Esquerdo Canal B | Quadratura do Motor 1 (entrada pura) |

---

## 2. Resumo por Subsistema

### A. Sensores ToF VL53L0X (3 unidades)
* **SDA / SCL:** GPIO 21 / GPIO 22
* **XSHUT Esquerdo (`0x30`):** GPIO 4
* **XSHUT Frontal (`0x31`):** GPIO 16
* **XSHUT Direito (`0x32`):** GPIO 17

### B. Atuação (Ponte H TB6612FNG)
* **Motor Esquerdo (A):** PWMA = GPIO 25, AIN1 = GPIO 26, AIN2 = GPIO 27
* **Motor Direito (B):** PWMB = GPIO 18, BIN1 = GPIO 33, BIN2 = GPIO 32
* **Standby:** STBY = GPIO 13

### C. Encoders em Quadratura (Motores N20)
* **Esquerdo:** Canal A = GPIO 34, Canal B = GPIO 39
* **Direito:** Canal A = GPIO 35, Canal B = GPIO 2

### D. IHM e Bateria
* **Botão de Largada:** GPIO 15
* **LED de Estado:** GPIO 14
* **Bateria LiPo 2S (ADC):** GPIO 36

### E. Comunicação Sem Fio e Gateway
* **Robô:** ESP32 DevKit V1 (ESP-NOW 1 Hz, Canal Wi-Fi 1, pacote binário de 18 bytes).
* **Gateway:** ESP8266 (NodeMCU / D1 mini) ou ESP32 de bancada (`src/firmware/gateway/`), escrevendo JSON na serial a 115200 bps.
