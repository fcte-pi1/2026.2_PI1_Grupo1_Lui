# Issue #98 - [EAP 3.1.4] Interface de Usuário e Sinalização (IHM)

Este diretório contém a solução completa para a **Issue #98** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), contemplando o controle da interface de usuário (LED RGB WS2812B, Buzzer piezoelétrico e Botão com debounce), simulação no Wokwi e suíte de testes automatizados.

---

## 1. Dados da Issue
* **Número da Issue:** #98
* **Título:** `[EAP 3.1.4] Interface de Usuário e Sinalização (IHM)`
* **Código EAP:** 3.1.4 (Sinalização e IHM)
* **Requisitos Atendidos:**
  * **RF-ELE05:** Sinalização Visual de Modos Operacionais (5 estados operacionais).
  * **RF-ELE06:** Sinalização Acústica de Eventos e Alertas ($\text{NPS} \ge 60\text{ dB}$).
  * **RNF-SFT01:** Disparo de largada física e seleção de rotinas via botão tátil.

---

## 2. Mapeamento de Estados Operacionais e Periféricos (Shield Rev 2.0)
| Modo ID | Estado Operacional | Cor do LED RGB (GPIO 2) | Sinal Acústico do Buzzer (GPIO 4) |
|:---:|:---|:---|:---|
| **0** | **Calibração** | Azul (`#0000FF`) | 2 Bipes curtos de inicialização |
| **1** | **Pronto / Armado** | Verde (`#00FF00`) | Bipe simples de confirmação de prontidão |
| **2** | **Navegando** | Amarelo (`#FFC800`) | Bipe longo de largada (silencioso em prova) |
| **3** | **Objetivo Alcançado** | Roxo (`#B400FF`) | Fanfarra sonora de vitória |
| **4** | **Falha / Bateria Fraca** | Vermelho Piscante (`#FF0000`) | Alarme contínuo intermitente ($\ge 60\text{ dB}$) |

* **Botão Físico de Largada:** GPIO 15 com pull-up interno e filtro antirruído por software com janela temporal de $50\text{ ms}$.

---

## 3. Estrutura do Diretório
```
ISSUE_98_Interface_de_Usuario_e_Sinalizacao_IHM/
├── README.md                 # Documentação e relatório de conformidade
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração de compilação PlatformIO
│   ├── include/
│   │   └── ihm_driver.h      # Driver e máquina de estados da IHM
│   └── src/
│       └── main.cpp          # Código principal de eventos, bipes e cores
├── wokwi/                    # Ambiente de simulação Wokwi
│   ├── diagram.json          # Diagrama virtual com ESP32, NeoPixel, Buzzer e Botão
│   ├── sketch.ino            # Sketch interativo com OLED, som e troca de modos
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências do simulador
└── tests/                    # Suíte de testes automatizados
    ├── test_ihm.cpp          # Testes unitários em C++17 (unity_compat.h)
    ├── test_runner.py        # Simulador dos modos operacionais em Python
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
  TESTES AUTOMATIZADOS - ISSUE #98: SINALIZACAO E IHM
======================================================================
[1/2] Compilando e executando testes unitarios em C++17...
[RUN ] test_ihm_pin_definitions ... PASS
[RUN ] test_five_operational_modes_visual_patterns ... PASS
[RUN ] test_button_debounce_filter ... PASS
----------------------------------------
3 Tests 0 Failures 0 Ignored -> OK

[2/2] Validando modos operacionais e temporizacao em Python...
Modo ID  | Nome do Modo             | Cor LED RGB            | Padrao Buzzer
---------------------------------------------------------------------------
#0       | Calibracao               | Azul (0, 0, 255)       | Bipe duplo confirmacao
#1       | Pronto para Largada      | Verde (0, 255, 0)      | Bipe simples de armar
#2       | Navegando                | Amarelo (255, 200, 0)  | Silencioso em marcha
#3       | Objetivo Alcancado       | Roxo (180, 0, 255)     | Fanfarra de vitoria
#4       | Falha / Bateria Fraca    | Vermelho Piscante      | Alarme continuo >= 60 dB

CRITERIOS DE ACEITE DA ISSUE #98 ATENDIDOS COM SUCESSO:
  [X] 5 Modos operacionais com sinalizacao visual especifica (RF-ELE05).
  [X] Dispositivo acustico com bipes sincronizados a eventos (RF-ELE06).
  [X] Botao de comando fisico com debounce testado.
```

---

## 5. Simulação no Wokwi
1. Abra a pasta `wokwi` com a extensão do Wokwi no VS Code ou no [Wokwi.com](https://wokwi.com).
2. Pressione Play para simular a IHM.
3. Clique no botão verde de largada (ou pressione a tecla `1` no teclado) para alternar sequencialmente entre os 5 modos.
4. Verifique as transições do LED RGB, os bipes do buzzer e as informações no display OLED.
