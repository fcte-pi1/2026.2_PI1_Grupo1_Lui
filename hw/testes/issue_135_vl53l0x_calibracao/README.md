# Issue #135 - [EAP 3.1.1.1] Calibração e Leitura do Sensor VL53L0X (1 Sensor)

Este diretório contém a solução simplificada e direta para a **Issue #135** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), focando na calibração e levantamento de erro de **1 sensor ToF VL53L0X**, com portas/pinos explicitamente mapeados.

---

## 1. Mapeamento de Portas e Pinos de Conexão

| Sinal do Sensor | Porta / Pino no ESP32 | Nível Lógico | Descrição |
|:---|:---:|:---:|:---|
| **I2C SDA** | **GPIO 21** | 3,3 V | Linha de Dados bidirecional do barramento I2C |
| **I2C SCL** | **GPIO 22** | 3,3 V | Linha de Relógio do barramento I2C (400 kHz) |
| **XSHUT** | **GPIO 32** | 3,3 V | Pino digital de controle e habilitação do sensor |
| **VCC** | **3V3** | 3,3 V | Alimentação regulada lógica protegida |
| **GND** | **GND** | 0 V | Referência comum de terra |

---

## 2. Requisitos e Critérios de Aceite
* **RNF-ELE01:** Taxa de amostragem $\ge 20\text{ Hz}$ (tempo entre medições $\le 50\text{ ms}$).
* **RNF-ELE02:** Faixa útil de operação de $30\text{ mm}$ a $180\text{ mm}$ com erro máximo $\le \pm 10\text{ mm}$.
* **Critério de Calibração:** Ensaio nos 7 pontos ($20, 40, 60, 80, 100, 150\text{ e }200\text{ mm}$) com $20$ amostras por ponto, cálculo de média, desvio padrão e erro absoluto.

---

## 3. Estrutura dos Arquivos
```
ISSUE_135_Calibracao_e_leitura_dos_sensores_VL53L0X/
├── README.md                 # Documentação e pinagem
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração PlatformIO
│   ├── include/
│   │   └── vl53l0x_driver.h  # Driver e funções estatísticas
│   └── src/
│       └── main.cpp          # Código principal de calibração
├── wokwi/                    # Simulação Wokwi direta com 1 sensor
│   ├── diagram.json          # ESP32 + 1 sensor + OLED
│   ├── sketch.ino            # Código de bancada interativo
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências (Adafruit SSD1306/GFX)
└── tests/                    # Testes automatizados
    ├── test_vl53l0x.cpp      # Testes unitários em C++17
    ├── test_runner.py        # Validador estatístico dos 7 pontos em Python
    ├── unity_compat.h        # Framework leve de asserções
    └── run_tests.sh          # Script de execução automatizada
```

---

## 4. Como Executar os Testes Automatizados
```bash
cd tests
./run_tests.sh
```

---

## 5. Simulação no Wokwi
1. Abra a pasta `wokwi` no VS Code ou no [Wokwi.com](https://wokwi.com).
2. Pressione Play.
3. Altere a distância do sensor ou digite `t` no Monitor Serial para rodar a rotina automática dos 7 pontos.
