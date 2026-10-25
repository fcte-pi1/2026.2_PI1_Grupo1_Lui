# Tabela Mestra de Mapeamento de Portas e Pinos (Pinout Matrix)
## Projeto Micromouse - PI1 2026.2 (Prof. Lui - Grupo 1)
### Hardware Shield Rev 2.0 (ESP32 DevKit V1 de 30 Pinos)

Esta tabela consolida todas as portas e pinos de conexões físicas entre o microcontrolador ESP32 e todos os módulos de hardware, atuadores, sensores e periféricos do projeto.

---

## 1. Mapeamento Completo de Pinos do ESP32 (30 Pinos)

| Pino Físico | Identificação | Direção | Nível | Conexão no Micromouse | Função / Descrição Técnica |
|:---:|:---:|:---:|:---:|:---|:---|
| **P1** | **3V3** | Saída | 3,3 V | Alimentação de Sensores | Linha regulada lógica para VL53L0X e Encoders |
| **P2** | **GND** | Referência | 0 V | Terra Geral | Plano de terra comum lógica/potência |
| **P3** | **GPIO 15** | Entrada | 3,3 V | Botão de Largada / IHM | Entrada digital com pull-up interno e debounce |
| **P4** | **GPIO 2** | Saída | 3,3 V | LED RGB / Status | Sinalização visual de modos operacionais |
| **P5** | **GPIO 4** | Saída | 3,3 V | Buzzer Ativo | Alarme sonoro piezoelétrico ($\ge 60\text{ dB}$) |
| **P6** | **GPIO 16** | Saída | 3,3 V | Ponte H PWMA | Modulação PWM 20 kHz do Motor Esquerdo |
| **P7** | **GPIO 17** | Saída | 3,3 V | Ponte H PWMB | Modulação PWM 20 kHz do Motor Direito |
| **P8** | **GPIO 5** | Saída | 3,3 V | Ponte H BIN2 | Direção lógica 2 do Motor Direito |
| **P9** | **GPIO 18** | Saída | 3,3 V | Ponte H AIN1 | Direção lógica 1 do Motor Esquerdo |
| **P10** | **GPIO 19** | Saída | 3,3 V | Ponte H AIN2 | Direção lógica 2 do Motor Esquerdo |
| **P11** | **GPIO 21** | Bidirecional | 3,3 V | I2C SDA | Linha de dados I2C (VL53L0X, OLED, IMU) |
| **P12** | **GPIO 3** | Entrada | 3,3 V | RX Serial USB | Recepção serial UART0 (Bancada) |
| **P13** | **GPIO 1** | Saída | 3,3 V | TX Serial USB | Transmissão serial UART0 (Bancada/Gateway) |
| **P14** | **GPIO 22** | Saída | 3,3 V | I2C SCL | Linha de relógio I2C (Fast-mode 400 kHz) |
| **P15** | **GPIO 23** | Saída | 3,3 V | Ponte H BIN1 | Direção lógica 1 do Motor Direito |
| **P16** | **VIN / 5V** | Entrada | 5,0 V | Barramento 5V | Entrada do regulador step-down da bateria |
| **P17** | **GND** | Referência | 0 V | Terra de Potência | Retorno de corrente dos motores e drivers |
| **P18** | **GPIO 13** | Saída | 3,3 V | ToF XSHUT Esquerdo | Habilitação do sensor esquerdo (90°) |
| **P19** | **GPIO 12** | Isolado | - | **NÃO CONECTADO** | Isolado na Shield para prevenir travamento de boot |
| **P20** | **GPIO 14** | Saída | 3,3 V | ToF XSHUT Direito | Habilitação do sensor direito (90°) |
| **P21** | **GPIO 27** | Entrada | 3,3 V | Encoder Motor Esq A | Interrupção externa por borda de subida |
| **P22** | **GPIO 26** | Entrada | 3,3 V | Encoder Motor Esq B | Leitura de sentido de giro |
| **P23** | **GPIO 25** | Entrada | 3,3 V | Encoder Motor Dir A | Interrupção externa por borda de subida |
| **P24** | **GPIO 33** | Entrada | 3,3 V | Encoder Motor Dir B | Leitura de sentido de giro |
| **P25** | **GPIO 32** | Saída | 3,3 V | ToF XSHUT Frontal | Habilitação do sensor frontal (0°) |
| **P26** | **GPIO 35** | Entrada | 3,3 V | Reserva Analógica | Entrada livre ADC1 |
| **P27** | **GPIO 34** | Entrada | 3,3 V | Monitor da Bateria | Divisor resistivo 100k/47k (ADC1_CH6) |
| **P28** | **GPIO 39** | Entrada | 3,3 V | Reserva VN | Entrada livre ADC1 |
| **P29** | **GPIO 36** | Entrada | 3,3 V | Reserva VP | Entrada livre ADC1 |
| **P30** | **EN / RST**| Entrada | 3,3 V | Reset do ESP32 | Circuito RC integrado da DevKit |

---

## 2. Resumo Rápido por Subsistema

### A. Sensores ToF (Distância às Paredes)
* **SDA:** GPIO 21
* **SCL:** GPIO 22
* **XSHUT Frontal (Sensor Principal):** GPIO 32
* **XSHUT Esquerdo:** GPIO 13
* **XSHUT Direito:** GPIO 14
* **Alimentação:** 3V3 e GND

### B. Atuação (Ponte H TB6612FNG)
* **PWMA (Motor Esquerdo):** GPIO 16 (PWM 20 kHz)
* **AIN1 / AIN2 (Motor Esquerdo):** GPIO 18 / GPIO 19
* **PWMB (Motor Direito):** GPIO 17 (PWM 20 kHz)
* **BIN1 / BIN2 (Motor Direito):** GPIO 23 / GPIO 5
* **Alimentação Lógica:** 3V3 e GND
* **Alimentação Motores (VM):** 7,4V (Bateria)

### C. Odometria (Encoders em Quadratura dos Motores N20)
* **Encoder Esquerdo Canal A (INT):** GPIO 27
* **Encoder Esquerdo Canal B:** GPIO 26
* **Encoder Direito Canal A (INT):** GPIO 25
* **Encoder Direito Canal B:** GPIO 33
* **Alimentação Sensores Hall:** 3V3 e GND

### D. Interface de Usuário e Sinalização (IHM)
* **Botão Físico de Largada:** GPIO 15 (Pull-Up interno)
* **LED RGB / Indicador:** GPIO 2
* **Buzzer Ativo:** GPIO 4
* **Display OLED I2C (Opcional na bancada):** SDA=21, SCL=22

### E. Monitoramento Energético
* **Tensão da Bateria (ADC):** GPIO 34 (Divisor resistivo R1=100k, R2=47k)
* **Faixa Segura:** 6,6 V a 8,4 V
