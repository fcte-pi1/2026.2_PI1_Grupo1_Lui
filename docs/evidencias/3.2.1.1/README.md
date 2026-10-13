# Evidências — EAP 3.2.1.1 (Protocolo de telemetria e gateway ESP-NOW)

## Já registrado (logs gerados em 12/10/2026)

| Evidência | Arquivo | Resultado |
|:---|:---|:---|
| Testes nativos do robô (codec, JSON, temporizador 1 Hz, mapa) | [pio_test_robo.log](pio_test_robo.log) | 25/25 aprovados |
| Testes nativos do gateway (ponte, `seq`, descarte) | [pio_test_gateway.log](pio_test_gateway.log) | 6/6 aprovados |
| Build do firmware do gateway (ESP32) | [pio_build_gateway.log](pio_build_gateway.log) | SUCCESS (Flash 55,9 %, RAM 13,8 %) |
| Build do firmware do robô (ESP32) | [pio_build_robo.log](pio_build_robo.log) | SUCCESS (Flash 56,6 %, RAM 13,4 %) |
| Testes no CI | Aba *Actions* → workflow **Firmware CI** (artefato `test-report-*`) | Disponível após o push/PR |

## Pendente — exige hardware (fotografar ou gravar)

Salvar nesta pasta com os nomes sugeridos:

- [ ] `foto_bancada.jpg` — robô e gateway ligados lado a lado.
- [ ] `video_gateway_serial.mp4` — monitor serial mostrando uma linha JSON por segundo, com `seq` incrementando e `rssi`.
- [ ] `captura_json.png` — print de uma linha JSON válida (pode ser validada em um validador JSON).
- [ ] `gateway_serial.log` — captura da serial por ≥ 60 s (CT-07 do doc 4.4).
- [ ] Teste de enlace a 8 m (EAP 3.1.5.1): fotos/vídeos em `docs/evidencias/3.1.5.1/`.

Observação: os logs de teste e de compilação acima foram gerados automaticamente; as fotos e gravações do hardware só podem ser feitas por quem está com as placas.
