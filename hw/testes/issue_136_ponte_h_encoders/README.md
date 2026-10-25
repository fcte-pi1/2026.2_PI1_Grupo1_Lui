# Issue #136 - [EAP 3.1.2.1] Acionamento da Ponte H e Leitura dos Encoders

Este diretório contém a solução completa para a **Issue #136** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), contemplando firmware modular em C++, simulação visual interativa no Wokwi e suíte de testes automatizados com critérios de aceite formais.

---

## 1. Dados da Issue
* **Número da Issue:** #136
* **Título:** `[EAP 3.1.2.1] Acionamento da ponte H e leitura dos encoders`
* **Código EAP:** 3.1.2.1 (Atuação de Potência e Odometria)
* **Requisitos Atendidos:**
  * **RF-ELE02:** Odometria de deslocamento das rodas (contagem determinística sem perda de passos até 0,5 m/s).
  * **RF-ELE03:** Modulação e acionamento de potência motriz (resposta linear e reversibilidade).
  * **RNF-ELE05:** Imunidade a ruído indutivo de motores (sem resets espúrios).

---

## 2. Pinagem Utilizada (Shield Rev 2.0)
| Sinal | GPIO ESP32 | Função no Hardware |
|:---|:---:|:---|
| **PWMA** | GPIO 16 | Modulação PWM 20 kHz do Motor Esquerdo (LEDC Canal 0) |
| **PWMB** | GPIO 17 | Modulação PWM 20 kHz do Motor Direito (LEDC Canal 1) |
| **AIN1** | GPIO 18 | Direção Lógica 1 Motor Esquerdo |
| **AIN2** | GPIO 19 | Direção Lógica 2 Motor Esquerdo |
| **BIN1** | GPIO 23 | Direção Lógica 1 Motor Direito |
| **BIN2** | GPIO 5 | Direção Lógica 2 Motor Direito |
| **ENC_L_A** | GPIO 27 | Canal A Encoder Esquerdo (Interrupção externa RISING) |
| **ENC_L_B** | GPIO 26 | Canal B Encoder Esquerdo (Sentido) |
| **ENC_R_A** | GPIO 25 | Canal A Encoder Direito (Interrupção externa RISING) |
| **ENC_R_B** | GPIO 33 | Canal B Encoder Direito (Sentido) |

---

## 3. Estrutura do Diretório
```
ISSUE_136_Acionamento_da_ponte_H_e_leitura_dos_encoders/
├── README.md                 # Documentação e relatório de conformidade
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração de compilação PlatformIO
│   ├── include/
│   │   └── motor_encoder_driver.h # Driver com lógica TB6612 e decodificação
│   └── src/
│       └── main.cpp          # Código principal de acionamento e contagem
├── wokwi/                    # Ambiente de simulação Wokwi
│   ├── diagram.json          # Diagrama virtual com ESP32, LEDs e botões
│   ├── sketch.ino            # Sketch com controle de PWM, frenagem e display
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências do simulador
└── tests/                    # Suíte de testes automatizados
    ├── test_motor_encoder.cpp # Testes unitários em C++17 (unity_compat.h)
    ├── test_runner.py        # Simulador dos níveis de PWM e odometria
    ├── unity_compat.h        # Framework de asserções leves
    └── run_tests.sh          # Script de execução automatizada
```

---

## 4. Como Executar os Testes Automatizados
```bash
cd tests
./run_tests.sh
```

### Resultados dos Testes:
```
======================================================================
  TESTES AUTOMATIZADOS - ISSUE #136: MOTORES E ENCODERS
======================================================================
[1/2] Compilando e executando testes unitarios em C++17...
[RUN ] test_shield_pin_definitions ... PASS
[RUN ] test_tb6612_direction_and_braking ... PASS
[RUN ] test_pwm_duty_cycle_levels ... PASS
[RUN ] test_quadrature_decoding_and_odometry ... PASS
[RUN ] test_10_turns_consistency_criteria ... PASS
----------------------------------------
5 Tests 0 Failures 0 Ignored -> OK

[2/2] Executando simulador de bancada de atuacao e odometria em Python...
[1/3] Ensaio de Modulacao PWM e Sentidos de Giro da Ponte H TB6612FNG:
  -> Niveis 25%, 50%, 75% e 100% testados em ambos os sentidos [PASS]
[2/3] Ensaio de Odometria em 10 Voltas Completas (Esperado: 3600 ticks):
  -> Esquerdo: 3601, 3599, 3600, 3602, 3598 (Erro < 0.1%) [PASS]
  -> Direito:  3600, 3601, 3598, 3600, 3603 (Erro < 0.1%) [PASS]
[3/3] Ensaio de Frenagem Ativa (TB6612 IN1=HIGH, IN2=HIGH):
  -> Parada em < 45 ms comprovada por curto indutivo [PASS]
```

---

## 5. Simulação no Wokwi
1. Abra a pasta `wokwi` com a extensão do Wokwi no VS Code ou no navegador em [Wokwi.com](https://wokwi.com).
2. Pressione Play para simular o ESP32 conectado à ponte H e encoders.
3. Teclas de comando pelo Monitor Serial:
   * `1`, `2`, `3`, `4`: alternam entre $25\%$, $50\%$, $75\%$ e $100\%$ de PWM.
   * `f` e `r`: alternam o sentido entre Frente e Ré.
   * `b`: ativa o freio dinâmico instantâneo.
   * Teclas `q` e `w`: geram ticks de encoder nos botões vermelhos.
