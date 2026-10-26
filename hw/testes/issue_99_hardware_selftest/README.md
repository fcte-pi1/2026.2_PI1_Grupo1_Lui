# Issue #99 - [EAP 3.1.5] Testes de Hardware

Este diretório contém a suíte completa de **Testes de Hardware e Autodiagnóstico Integrado** referente à **Issue #99** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), consolidando a verificação laboratorial de todos os requisitos eletroeletrônicos para emissão do relatório técnico 7.3.

---

## 1. Dados da Issue
* **Número da Issue:** #99
* **Título:** `[EAP 3.1.5] Testes de Hardware`
* **Código EAP:** 3.1.5 (Testes de Hardware)
* **Objetivo:** Validação funcional em bancada do microcontrolador ESP32, calibração de sensores ópticos e inerciais, resposta dos atuadores de tração, sinalização visual/acústica e enlace de rádio.

---

## 2. Matriz de Homologação dos Requisitos
| ID Requisito | Descrição Funcional | Issue Específica | Status |
|:---|:---|:---:|:---:|
| **RF-ELE01** | Sensoriamento ToF (Frontal, Esquerdo, Direito) | Issue #135 | **CONFORME** |
| **RF-ELE02** | Odometria de Rodas por Encoders em Quadratura | Issue #136 | **CONFORME** |
| **RF-ELE03** | Acionamento de Potência TB6612FNG com PWM 20 kHz | Issue #136 | **CONFORME** |
| **RF-ELE04** | Transmissão Sem Fio ESP-NOW a 1 Hz | Issue #140 | **CONFORME** |
| **RF-ELE05** | Sinalização Visual RGB para os 5 Estados Operacionais | Issue #98 | **CONFORME** |
| **RF-ELE06** | Sinalização Sonora com Buzzer ($\ge 60\text{ dB}$) | Issue #98 | **CONFORME** |
| **RNF-ELE01** | Taxa de Amostragem Sensorial $\ge 20\text{ Hz}$ | Issue #135 | **CONFORME** |
| **RNF-ELE02** | Faixa Útil $30\text{ a }180\text{ mm}$ com Erro $\le 10\text{ mm}$ | Issue #135 | **CONFORME** |
| **RNF-ELE03** | Enlace Sem Fio Estável a $8\text{ m}$ com Perda $< 2,0\%$ | Issue #137 | **CONFORME** |
| **RNF-ELE04** | Construção Modular em Shield PCB sem Protoboard | Doc 4.3 | **CONFORME** |
| **RNF-ELE05** | Imunidade a Ruído Indutivo de Motores (Zero Resets) | Issue #131 | **CONFORME** |

---

## 3. Estrutura do Diretório
```
ISSUE_99_Testes_de_Hardware/
├── README.md                 # Documentação e matriz de homologação
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração de compilação PlatformIO
│   ├── include/
│   │   └── hardware_selftest_suite.h # Definições da suíte integrada
│   └── src/
│       └── main.cpp          # Código de diagnóstico e varredura I2C
├── wokwi/                    # Ambiente de simulação Wokwi
│   ├── diagram.json          # Diagrama virtual da bancada completa
│   ├── sketch.ino            # Simulação do auto-teste de hardware
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências do simulador
└── tests/                    # Suíte de testes automatizados
    ├── test_hardware_integration.cpp # Testes unitários em C++17
    ├── test_runner.py        # Auditor da matriz de requisitos em Python
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
  TESTES AUTOMATIZADOS - ISSUE #99: TESTES DE HARDWARE
======================================================================
[1/2] Compilando e executando testes unitarios em C++17...
[RUN ] test_full_hardware_compliance_matrix ... PASS
----------------------------------------
1 Tests 0 Failures 0 Ignored -> OK

[2/2] Executando auditoria da matriz de requisitos em Python...
LAUDO FINAL DE HOMOLOGACAO DE HARDWARE: 100% APROVADO
  Todos os requisitos RF-ELE01 a RF-ELE06 e RNF-ELE01 a RNF-ELE05 estao cumpridos.
```

---

## 5. Simulação no Wokwi
1. Abra a pasta `wokwi` com a extensão do Wokwi no VS Code ou no [Wokwi.com](https://wokwi.com).
2. Pressione Play para simular a bancada de testes de hardware.
3. Acompanhe a inicialização dos módulos no display OLED SSD1306 e no monitor serial a 115200 bps.
