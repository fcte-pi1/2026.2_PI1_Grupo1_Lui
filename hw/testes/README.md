# Testes de Hardware, Ensaios de Bancada e Simulações Wokwi

Este diretório consolida todos os códigos-fonte de testes unitários e de integração, firmwares de calibração, suítes automatizadas, simulações interativas no Wokwi e simulações de algoritmos no MMS para o hardware do projeto **Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB)**.

---

## 1. Estrutura dos Módulos de Teste

| Subsistema / Módulo | Diretório | Descrição Técnica | Requisitos / EAP |
|:---|:---|:---|:---:|
| **Pinagem Geral da Placa** | [`PINOUT_CONFIG.h`](PINOUT_CONFIG.h) / [`TABELA_PINAGEM`](TABELA_PINAGEM_PORTAS_HARDWARE.md) | Mapeamento completo de portas e pinos do ESP32 (Shield Rev 2.0). | EAP 3.1.3 |
| **Suíte Master Automatizada** | [`run_all_issue_tests.sh`](run_all_issue_tests.sh) | Script bash que executa 100% dos testes unitários em C++17 e Python. | EAP 3.1.5 |
| **IHM e Sinalização** | [`issue_98_ihm/`](issue_98_ihm/) | Controle do LED RGB (5 modos), Buzzer ($\ge 60\text{ dB}$) e Botão com debounce. | RF-ELE05, RF-ELE06, RNF-SFT01 |
| **Autodiagnóstico Integrado** | [`issue_99_hardware_selftest/`](issue_99_hardware_selftest/) | Suíte integrada de auto-teste de hardware e auditoria de requisitos. | EAP 3.1.5 |
| **Alimentação e Tensão** | [`issue_131_alimentacao_tensao/`](issue_131_alimentacao_tensao/) | Medição ADC da bateria, estabilidade sob transitórios e tolerância de 5%. | RF-ENE02, RNF-ENE02, RNF-ELE05 |
| **Carga e Autonomia** | [`issue_132_carga_autonomia_protecao/`](issue_132_carga_autonomia_protecao/) | Ensaio de consumo elétrico, cálculo de autonomia e desarme por sobrecorrente. | RNF-ENE01, RNF-ENE03 |
| **Calibração VL53L0X** | [`issue_135_vl53l0x_calibracao/`](issue_135_vl53l0x_calibracao/) | Ensaio estático nos 7 pontos (20 a 200 mm), erro $< 10\text{ mm}$ e amostragem $\ge 20\text{ Hz}$. | RF-ELE01, RNF-ELE01, RNF-ELE02 |
| **Ponte H e Encoders** | [`issue_136_ponte_h_encoders/`](issue_136_ponte_h_encoders/) | Modulação PWM 20 kHz (TB6612FNG), frenagem ativa e odometria em 10 voltas. | RF-ELE02, RF-ELE03 |
| **Enlace ESP-NOW a 8m** | [`issue_137_espnow_enlace_8m/`](issue_137_espnow_enlace_8m/) | Teste de 1000 pacotes a 8m, taxa de perda $< 2,0\%$ e RSSI $\ge -75\text{ dBm}$. | RNF-ELE03 |
| **Telemetria e Gateway** | [`issue_140_telemetria_gateway/`](issue_140_telemetria_gateway/) | Pacote compacto de 18 bytes com CRC16 a 1 Hz e conversor JSON serial. | RF-ELE04, EAP 5.3.1.1 |
| **Enlace Real ESP-NOW** | [`esp_now/`](esp_now/) | Firmwares de bancada real com receptor e transmissor assíncrono. | RNF-ELE03 |
| **Sensores VL53L0X (1 Un.)** | [`vl53l0x_individual/`](vl53l0x_individual/) | Firmware Arduino para validação de bancada de 1 sensor ToF. | RF-ELE01 |
| **Sensores VL53L0X (3 Un.)** | [`vl53l0x_multiplo/`](vl53l0x_multiplo/) | Firmware de multiplexação com chaveamento sequencial via XSHUT. | RF-ELE01 |
| **Simulações Virtuais Wokwi**| [`simulacoes_wokwi/`](simulacoes_wokwi/) | Ambientes completos no Wokwi: Protótipo Real com HAL e Simulação Total. | EAP 3.1.5 |
| **Simulador de Labirinto MMS**| [`mms/`](mms/) | Algoritmo padrão de exploração (`DEFAULT.py`), interface `API.py` e labirinto oficial (`MAZE01.txt`). | EAP 5.2.2 |

---

## 2. Como Executar Todos os Testes Automatizados

No terminal, a partir deste diretório ou da raiz do repositório:
```bash
cd hw/testes
./run_all_issue_tests.sh
```
Todas as 8 suítes são executadas automaticamente compilando os códigos C++17 e rodando os validadores estatísticos em Python.
