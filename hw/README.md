# _Hardware_

Esta pasta armazena todos os artefatos de engenharia de hardware, esquemáticos, placa de circuito impresso (PCB Shield), simulações e suítes de testes laboratoriais e automatizados do projeto **Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB)**.

---

## 1. Conteúdo do Diretório

* [`micromouse_shield/`](micromouse_shield/): Projeto completo de CAD da placa Shield Rev 2.0 (KiCad 8), contemplando esquemático elétrico, layout de roteamento, arquivos Gerber, PDF do circuito e visualização gráfica.
* [`testes/`](testes/): Suíte completa de testes de hardware, ensaios de bancada, scripts automatizados e simulações interativas no Wokwi:
  * **Tabela e Pinout:** [`PINOUT_CONFIG.h`](testes/PINOUT_CONFIG.h) e [`TABELA_PINAGEM_PORTAS_HARDWARE.md`](testes/TABELA_PINAGEM_PORTAS_HARDWARE.md);
  * **Automação:** [`run_all_issue_tests.sh`](testes/run_all_issue_tests.sh) (executa 100% dos testes unitários e de integração);
  * **Simulações Virtuais:** [`simulacoes_wokwi/`](testes/simulacoes_wokwi/) (Sensores, Protótipo Real e Labirinto Total);
  * **Testes por Subsistema:** IHM ([`issue_98`](testes/issue_98_ihm/)), Autodiagnóstico ([`issue_99`](testes/issue_99_hardware_selftest/)), Alimentação ([`issue_131`](testes/issue_131_alimentacao_tensao/)), Carga ([`issue_132`](testes/issue_132_carga_autonomia_protecao/)), VL53L0X ([`issue_135`](testes/issue_135_vl53l0x_calibracao/)), Ponte H e Encoders ([`issue_136`](testes/issue_136_ponte_h_encoders/)), ESP-NOW 8m ([`issue_137`](testes/issue_137_espnow_enlace_8m/)) e Telemetria Gateway ([`issue_140`](testes/issue_140_telemetria_gateway/));
  * **Bancada Real:** Enlace de rádio ([`esp_now/`](testes/esp_now/)) e validação de sensores ToF ([`vl53l0x_individual/`](testes/vl53l0x_individual/) e [`vl53l0x_multiplo/`](testes/vl53l0x_multiplo/)).

> [!NOTE]
> Os firmwares definitivos de produção embarcados no robô e no gateway de telemetria encontram-se estruturados em [`src/firmware/`](../src/firmware/).
