# Testes de Enlace Sem Fio ESP-NOW (Transmissor e Receptor)

Este diretório contém os códigos-fonte C++ desenvolvidos para ensaios laboratoriais e em campo do enlace de rádio **ESP-NOW a 2,4 GHz**, verificando estabilidade, perda de pacotes e alcance até 8 metros (**RNF-ELE03**).

---

## 1. Arquivos Contidos
* [`transmissor.cpp`](transmissor.cpp): Firmware para o nó transmissor (Robô ou placa de bancada). Implementa varredura e fixação de canal Wi-Fi, geração de pacotes sequenciais com identificador incremental (`msg_id`), envio via `esp_now_send` a 20 Hz e telemetria de falhas de hardware/ACK via serial.
* [`receptor.cpp`](receptor.cpp): Firmware para a placa receptora (Gateway/Solo). Configura o rádio no canal fixo, registra o callback de recepção (`OnDataRecv`), contabiliza pacotes recebidos, calcula pacotes perdidos por salto de ID e emite relatório estatístico a cada 2 segundos.

---

## 2. Como Configurar e Compilar

### No ESP32 (Receptor / Gateway)
1. Conecte a placa ESP32 receptora ao computador.
2. Grave o código [`receptor.cpp`](receptor.cpp) via Arduino IDE ou PlatformIO.
3. Abra o Monitor Serial a **115200 bps**.
4. Copie o endereço MAC real impresso no boot (exemplo: `30:C9:22:64:0A:48`).

### No ESP32 / ESP8266 (Transmissor / Robô)
1. Abra o arquivo [`transmissor.cpp`](transmissor.cpp).
2. Atualize o array `broadcastAddress[]` com os 6 bytes do MAC do seu receptor:
   ```cpp
   uint8_t broadcastAddress[] = {0x30, 0xC9, 0x22, 0x64, 0x0A, 0x48};
   ```
3. Grave o firmware no microcontrolador transmissor.
4. O transmissor buscará automaticamente o canal em que o receptor está sintonizado e travará a conexão.

---

## 3. Métricas Avaliadas no Terminal Serial
A cada 2 segundos, o receptor emite as métricas de qualidade do enlace:
```text
================ METRICAS ESP-NOW ================
STATUS             : OK - RECEBENDO DADOS!
Ultimo ID Recebido : 1000
Pacotes Recebidos  : 993
Pacotes Perdidos   : 7
Taxa de Perda      : 0.70%
==================================================
```
* **Critério de Aceite RNF-ELE03:** Taxa de perda $< 2,0\%$ e RSSI $\ge -75\text{ dBm}$ a $8\text{ metros}$ (Atingido: $0,70\%$ em visada livre e $0,80\%$ no labirinto).
