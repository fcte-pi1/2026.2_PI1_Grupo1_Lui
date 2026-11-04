# Firmware Final Micromouse - ESP32-WROOM-32D (30 Pinos)
## Grupo 1 - PI1 2026.2 (Prof. Lui - FCTE/UnB)

Este diretório contém o firmware completo, homologado e modular do robô Micromouse, adaptado e configurado para a **pinagem unificada e sem conflitos** da placa ESP32 DevKit V1 (30 pinos), integrando os **2 Motores N20 com Encoders em Quadratura**, a **Ponte H TB6612FNG**, os **3 Sensores de Distância VL53L0X (ToF I2C)** e o **Monitor de Bateria 2S (ADC)**.

Além da operação normal, o firmware inclui um **Menu de Testes Graduais e Comissionamento Interativo** pelo Monitor Serial (115200 bps), permitindo testar cada subsistema isoladamente (sensores, motores, encoders na mão, avanço de 1 célula, curvas de 90° e freio anti-colisão) antes de rodar o robô no labirinto.

---

## 1. Mapeamento Unificado de Portas e Pinos

| GPIO ESP32 | Função Atribuída | Periférico Conectado | Observações de Hardware |
|:---:|:---|:---|:---|
| **GPIO 2** | Encoder Motor 2 - Canal B | Motor Direito (N20) | Entrada com Pull-up interno ativado no ESP32. |
| **GPIO 4** | XSHUT - Sensor 1 | Sensor VL53L0X #1 (Esquerdo) | Habilitação e reset digital individual (I2C: `0x30`). |
| **GPIO 13** | STBY (Standby) | Ponte H (TB6612FNG) | Mantido em nível `HIGH` para habilitar os motores. |
| **GPIO 14** | Status LED / 4º Sensor | LED Onboard / Reserva | **Pino Livre** (Ideal caso use um 4º sensor VL53L0X). |
| **GPIO 15** | Botão de Largada / IHM | Botão Físico Pushbutton | Entrada com Pull-up interno ativado. |
| **GPIO 16** | XSHUT - Sensor 2 | Sensor VL53L0X #2 (Frontal) | Habilitação e reset digital individual (I2C: `0x31`). |
| **GPIO 17** | XSHUT - Sensor 3 | Sensor VL53L0X #3 (Direito) | Habilitação e reset digital individual (I2C: `0x32`). |
| **GPIO 18** | PWMB (PWM Motor 2) | Ponte H (TB6612FNG) | Frequência de modulação de 20 kHz (Canal LEDC 1). |
| **GPIO 21** | SDA (I2C) | Sensores ToF (1, 2 e 3) | Linha de dados I2C compartilhada (Fast-mode 100/400 kHz). |
| **GPIO 22** | SCL (I2C) | Sensores ToF (1, 2 e 3) | Linha de clock I2C compartilhada. |
| **GPIO 25** | PWMA (PWM Motor 1) | TB6612FNG | Frequência de modulação de 20 kHz (Canal LEDC 0). |
| **GPIO 26** | AIN1 (Direção Motor 1) | TB6612FNG | Sentido de rotação do Motor Esquerdo. |
| **GPIO 27** | AIN2 (Direção Motor 1) | TB6612FNG | Sentido de rotação do Motor Esquerdo. |
| **GPIO 32** | BIN2 (Direção Motor 2) | TB6612FNG | **Sentido do Motor Direito**. *(Não utilizar como sensor)*. |
| **GPIO 33** | BIN1 (Direção Motor 2) | TB6612FNG | Sentido de rotação do Motor Direito. |
| **GPIO 34** | Leitura Analógica (ADC) | Divisor de Tensão Bateria | Entrada pura ADC1_CH6 (Amostragem com filtro de média). |
| **GPIO 35** | Encoder Motor 2 - Canal A | Motor Direito (N20) | Entrada pura com interrupção externa (`RISING`). |
| **GPIO 36 (VP)**| Encoder Motor 1 - Canal A | Motor Esquerdo (N20) | Entrada pura com interrupção externa (`RISING`). |
| **GPIO 39 (VN)**| Encoder Motor 1 - Canal B | Motor Esquerdo (N20) | Entrada pura para leitura de quadratura e sentido. |

---

## 2. Resoluções Críticas de Engenharia

### A. O Conflito do Pino GPIO 4 e Uso do GPIO 2
* **Problema anterior:** O GPIO 4 estava disputado entre o controle de interrupção/canal de encoder e o controle de habilitação do sensor ToF.
* **Solução implementada:** O GPIO 4 ficou dedicado **exclusivamente ao pino XSHUT do Sensor 1**. O Canal B do Encoder do Motor 2 foi alocado para o **GPIO 2**, que possui resistor de pull-up interno e opera com alta estabilidade para leitura em quadratura.

### B. Esclarecimento Sobre o Pino GPIO 32 e 4º Sensor ToF
* **Atenção:** No mapeamento homologado, o **GPIO 32 está alocado para o sinal BIN2 da Ponte H (Direção do Motor 2)**.
* Como o GPIO 32 atua como saída digital rápida de controle dos motores, **ele não pode ser conectado a um sensor simultaneamente**.
* Caso a equipe deseje adicionar um **4º Sensor VL53L0X** (ex: sensor traseiro ou diagonal), utilize o **GPIO 14** (ou GPIO 19 / 23 / 5), que estão totalmente livres na placa. No código [`include/pinout.h`](include/pinout.h), já deixamos `#define PIN_XSHUT_4_EXTRA 14` pronto para uso.

### C. Arquitetura de Alimentação e Regulador Buck MP1584
1. **Ajuste prévio do Buck MP1584:** Antes de conectar à ESP32, alimente o MP1584 com a bateria LiPo 2S e regule o trimpot multivoltas com um multímetro até obter **exatamente 5,0 V** nos pinos de saída `OUT+` e `OUT-`.
2. **Conexão na ESP32:**
   * `OUT+` do MP1584 $\to$ Pino **VIN** da ESP32 (canto superior direito). O LDO interno da ESP32 reduzirá os 5V para 3,3V limpos.
   * `OUT-` do MP1584 $\to$ Pino **GND** da ESP32.
3. **Alimentação dos Motores (Pino VM da TB6612FNG):**
   * **NUNCA** ligue os motores através do Buck MP1584! Os motores N20 consomem picos de 1,5 A a 2 A na partida. Passá-los pelo Buck queima o regulador e reinicia o ESP32 por queda de tensão.
   * Ligue a linha **VM da TB6612FNG diretamente no polo positivo da Bateria LiPo 2S (7,4 V a 8,4 V)**.
4. **Posicionamento do Diodo de Proteção:**
   * Coloque o diodo (1N4007 ou Schottky SS14/1N5819) **apenas na entrada do Buck MP1584**:
     $$\text{Bateria (+)} \longrightarrow \text{Anodo} \to \text{Catodo} \longrightarrow \text{IN+ do Buck}$$
   * Não coloque diodo na linha de alta corrente dos motores.

### D. Módulo Sensor de Tensão vs Módulo Relé
* **Módulo Sensor de Tensão (30kΩ / 7,5kΩ):** **ÚTIL**. Ele forma um divisor resistivo de $5:1$. Conecte o pino `S` ao **GPIO 34**, o pino `-` ao **GND** e deixe o `+` desconectado.
* **Módulo Relé (MT3F-5VDC):** **DESNECESSÁRIO**. É grande, pesado e a ponte H TB6612 já faz todo o chaveamento PWM bidirecional dos motores.

---

## 3. Estrutura Modular do Código (`hw/firm`)

```
hw/firm/
├── platformio.ini              # Configuração de build PlatformIO para esp32dev
├── README.md                   # Esta documentação técnica
├── include/
│   ├── pinout.h                # Tabela mestra de pinos, frequências e limiares
│   ├── tb6612_driver.h         # Declaração do driver da ponte H (PWM 20 kHz e freio)
│   ├── encoder_driver.h        # Declaração do decodificador de pulsos por interrupção
│   ├── vl53l0x_manager.h       # Gerenciador dos 3 sensores ToF com I2C dinâmico
│   └── battery_monitor.h       # Monitor de tensão analógica e porcentagem da bateria
├── src/
│   ├── tb6612_driver.cpp       # Implementação do driver TB6612FNG
│   ├── encoder_driver.cpp      # Rotinas ISR e cálculo de odometria linear (mm)
│   ├── vl53l0x_manager.cpp     # Inicialização sequencial via XSHUT e leitura contínua
│   ├── battery_monitor.cpp     # Amostragem com média móvel e cálculo LiPo 2S
│   └── main.cpp                # Console de testes graduais interativos e telemetria
├── test/
│   ├── test_firmware.cpp       # Testes unitários de conformidade (pinos, bateria, odometria)
│   └── run_tests.sh            # Script de compilação e execução nativa dos testes
└── exemplos_testes_individuais/ # Mini-firmwares isolados para bancada
    ├── 01_teste_motores_basico.ino
    ├── 02_teste_encoders_manual.ino
    ├── 03_teste_3_sensores_vl53l0x.ino
    ├── 04_teste_avanco_e_curva.ino
    └── 05_teste_bateria_adc.ino
```

---

## 4. Menu de Testes Graduais (`main.cpp`)

Ao abrir o Monitor Serial a **115200 bps**, o firmware apresenta o console de testes passo a passo. Você pode acionar cada teste digitando o número correspondente:

```text
=======================================================
     MICROMOUSE PI1 2026.2 - MENU DE TESTES GRADUAIS   
=======================================================
[1] - Teste de Sensores VL53L0X (Leitura contínua em mm)
[2] - Teste Manual de Motores (Esquerdo/Direito passo a passo)
[3] - Teste de Encoders Manual (Gire as rodas na mão)
[4] - Teste de Movimento: Avançar 1 Célula (168 mm) e Parar
[5] - Teste de Curva: Giro de 90° Direita no próprio eixo
[6] - Teste de Curva: Giro de 90° Esquerda no próprio eixo
[7] - Teste de Manobra: Meia-Volta de 180° no próprio eixo
[8] - Teste Integrado: Avanço com Freio Anti-Colisão (< 100 mm)
[9] - Calibração de Bateria (Leitura ADC bruta e tensão)
[0] - Modo Telemetria Contínua (Visão geral em tempo real)
[s] - Parada Imediata de Emergência (Freio ativo)
[m] - Exibir este menu novamente
=======================================================
```

### Roteiro Recomendado de Testes na Bancada:
1. **Passo 1 (`tecla 9`):** Verifique se a bateria está na faixa segura ($> 7,2\text{ V}$).
2. **Passo 2 (`tecla 1`):** Aproxime a mão de cada um dos 3 sensores ToF e confirme se as leituras em mm variam proporcionalmente e sem ruído. *(Pressione 'x' para sair)*.
3. **Passo 3 (`tecla 3`):** Com o robô erguido, gire a roda esquerda e a direita 1 volta completa na mão. Cada volta deve acusar $\approx 360\text{ ticks}$. Se girar para frente e o contador diminuir, inverta o sentido em `encoder_driver.cpp`. *(Pressione 'x' para sair)*.
4. **Passo 4 (`tecla 2`):** Teste os motores individualmente no ar (`e` para motor esquerdo, `d` para direito) e verifique se giram no sentido correto.
5. **Passo 5 (`tecla 4`):** Coloque o robô no chão da pista e envie `4` (ou aperte o botão no GPIO 15). Ele avançará exatamente 1 célula oficial ($168\text{ mm}$) e aplicará o freio ativo.
6. **Passo 6 (`tecla 5` e `6`):** Execute as curvas de 90°. Se o giro passar ou faltar alguns graus, basta calibrar a bitola `TRACK_WIDTH_MM` em [`include/pinout.h`](include/pinout.h).
7. **Passo 7 (`tecla 8`):** Coloque um obstáculo à frente. O robô avançará e freará suavemente a $100\text{ mm}$ da parede.
