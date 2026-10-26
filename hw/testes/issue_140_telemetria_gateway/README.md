# Issue #140 - [EAP 3.2.1.1] Protocolo de Telemetria e Firmware do Gateway ESP-NOW

Este diretório contém a solução completa para a **Issue #140** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), contemplando o protocolo compacto de 18 bytes, o firmware do Gateway ESP-NOW para Serial JSON, ambiente de simulação no Wokwi e suíte de testes automatizados com validação contratual.

---

## 1. Dados da Issue
* **Número da Issue:** #140
* **Título:** `[EAP 3.2.1.1] Protocolo de telemetria e firmware do gateway ESP-NOW`
* **Código EAP:** 3.2.1.1 (Telemetria Sem Fio e Software Embarcado)
* **Requisitos Atendidos:**
  * **RF-ELE04:** Transmissão Sem Fio de Telemetria (despacho contínuo a 1 Hz).
  * **RNF-ELE03:** Enlace Sem Fio com integridade CRC16.
  * **Contrato EAP 5.3.1.1:** Especificação de dados JSON para o Dashboard Web.

---

## 2. Estrutura Binária do Pacote de 18 Bytes
O pacote é estritamente compactado (`#pragma pack(push, 1)`) com soma de verificação CRC-16-CCITT:

| Campo | Tipo | Bytes | Descrição |
|:---|:---:|:---:|:---|
| `header` | `uint8_t` | 1 | Marcador de início fixo `0xA5` |
| `seq_number` | `uint16_t` | 2 | Número de sequência sequencial (1 a 65535) |
| `timestamp_s` | `uint16_t` | 2 | Tempo de corrida em segundos |
| `pos_x` | `uint8_t` | 1 | Coordenada X na arena (0 a 15) |
| `pos_y` | `uint8_t` | 1 | Coordenada Y na arena (0 a 15) |
| `heading_state` | `uint8_t` | 1 | Bits 0-3: Estado da FSM; Bits 4-7: Orientação (0=N, 1=L, 2=S, 3=O) |
| `walls_mask` | `uint8_t` | 1 | Bitmask de paredes na célula atual (N, L, S, O) |
| `dist_left_cm` | `uint8_t` | 1 | Distância medida pelo ToF Esquerdo em cm |
| `dist_front_cm` | `uint8_t` | 1 | Distância medida pelo ToF Frontal em cm |
| `dist_right_cm` | `uint8_t` | 1 | Distância medida pelo ToF Direito em cm |
| `imu_yaw_deci_deg`| `int16_t` | 2 | Ângulo de guinada da IMU em décimos de grau |
| `vbat_mv` | `uint16_t` | 2 | Tensão da bateria em milivolts (ex: 7400 = 7.4 V) |
| `crc16` | `uint16_t` | 2 | CRC-16-CCITT (Polinômio 0x1021) |
| **TOTAL** | | **18** | **Bytes exatos de carga útil** |

---

## 3. Contrato de Saída JSON do Gateway
Ao receber o pacote binário, o firmware do Gateway valida o CRC16, lê o RSSI do enlace e serializa o JSON na Serial (115200 bps):
```json
{
  "seq": 105,
  "timestamp_ms": 45000,
  "rssi": -62,
  "pos": {"x": 3, "y": 7},
  "heading": 1,
  "state": 2,
  "walls": 5,
  "sensors_cm": {"left": 8, "front": 25, "right": 8},
  "imu_yaw": 90.0,
  "battery_v": 7.60
}
```

---

## 4. Estrutura do Diretório
```
ISSUE_140_Protocolo_de_telemetria_e_firmware_do_gateway_ESP-NOW/
├── README.md                 # Documentação e relatório de conformidade
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração de compilação PlatformIO
│   ├── include/
│   │   └── telemetry_gateway_protocol.h # Definição dos 18 bytes e serializador
│   └── src/
│       └── main.cpp          # Código do Gateway e Robô
├── wokwi/                    # Ambiente de simulação Wokwi
│   ├── diagram.json          # Diagrama virtual com ESP32 e display OLED
│   ├── sketch.ino            # Emulador do Gateway gerando JSON a 1 Hz
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências do simulador (ArduinoJson)
└── tests/                    # Suíte de testes automatizados
    ├── test_protocol.cpp     # Testes unitários em C++17 (unity_compat.h)
    ├── test_runner.py        # Validador de parsing e schema JSON em Python
    ├── unity_compat.h        # Framework de asserções leves
    └── run_tests.sh          # Script de execução automatizada
```

---

## 5. Como Executar os Testes Automatizados
```bash
cd tests
./run_tests.sh
```

### Resultados dos Testes:
```
======================================================================
  TESTES AUTOMATIZADOS - ISSUE #140: PROTOCOLO & GATEWAY JSON
======================================================================
[1/2] Compilando e executando testes unitarios em C++17...
[RUN ] test_18_bytes_size_and_packing ... PASS
[RUN ] test_encoding_and_crc_validation ... PASS
[RUN ] test_json_conversion_contract ... PASS
----------------------------------------
3 Tests 0 Failures 0 Ignored -> OK

[2/2] Validando decodificacao de 18 bytes e schema JSON em Python...
Pacote #01 (18 bytes) -> JSON Gateway valido [OK]
Pacote #02 (18 bytes) -> JSON Gateway valido [OK]
Pacote #03 (18 bytes) -> JSON Gateway valido [OK]
Pacote #04 (18 bytes) -> JSON Gateway valido [OK]

CRITERIOS DE ACEITE DA ISSUE #140 ATENDIDOS COM SUCESSO:
  [X] Empacotamento binario validado exatamente em 18 bytes.
  [X] Codificacao, CRC16 e decodificacao 100% integra.
  [X] Gateway converte para JSON com seq e RSSI conforme contrato EAP 5.3.1.1.
```

---

## 6. Simulação no Wokwi
1. Abra a pasta `wokwi` com a extensão do Wokwi no VS Code ou no [Wokwi.com](https://wokwi.com).
2. Pressione Play para simular o ESP32 Gateway.
3. Observe no Monitor Serial as mensagens JSON sendo geradas continuamente a 1 Hz com número de sequência incremental, RSSI e parâmetros operacionais.
4. Pressione `s` para simular o disparo de um pacote individual imediato.
