/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #137 [EAP 3.1.5.1]
 * Ensaio de Perda de Pacotes ESP-NOW a 8 metros (RNF-ELE03)
 * ==============================================================================
 * PORTAS / CANAL / INTERFACES:
 *   - Canal RF: 2.4 GHz WiFi Canal 1 (ESP-NOW sem roteador)
 *   - TX Serial USB: GPIO 1 (115200 bps)
 *   - RX Serial USB: GPIO 3
 *   - LED Status:    GPIO 2 (Indica recepcao de pacotes)
 * ==============================================================================
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "espnow_link_driver.h"

uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint32_t rcv_count = 0;
uint32_t sent_count = 0;

void on_receive(const uint8_t *mac, const uint8_t *data, int len) {
    (void)mac;
    if (len == sizeof(EspNowPacket_t)) {
        EspNowPacket_t* pkt = (EspNowPacket_t*)data;
        rcv_count++;
        if (pkt->sequence_number % 100 == 0 || pkt->sequence_number == 1000) {
            Serial.printf("[RX] Pacote %u recebido. Total: %u / 1000\n",
                          pkt->sequence_number, rcv_count);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERRO] Falha ao inicializar ESP-NOW!");
        return;
    }

    esp_now_register_recv_cb(on_receive);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcast_mac, 6);
    peer.channel = 1;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - TESTE DE ENLACE ESP-NOW (ISSUE #137)");
    Serial.println("=======================================================");
    Serial.println("PORTAS E INTERFACES:");
    Serial.println("  - Protocolo: ESP-NOW (2.4 GHz Canal 1)");
    Serial.println("  - Serial: GPIO 1 (TX) / GPIO 3 (RX) a 115200 bps");
    Serial.println("  - Meta RNF-ELE03: Perda < 2.0% a 8 metros");
    Serial.println("=======================================================");
    Serial.println("Digite 't' para transmitir 1000 pacotes ou 'r' para zerar contador receptor.");
}

void transmit_1000_packets() {
    Serial.println("\n[TX] Enviando 1000 pacotes sequenciais a 20 Hz...");
    sent_count = 0;
    for (uint32_t i = 1; i <= 1000; i++) {
        EspNowPacket_t pkt;
        pkt.sequence_number = i;
        pkt.timestamp_ms = millis();
        pkt.tx_power = 20;
        pkt.checksum = espnow_calc_crc((const uint8_t*)&pkt, sizeof(pkt) - 2);

        esp_now_send(broadcast_mac, (uint8_t*)&pkt, sizeof(pkt));
        sent_count++;
        if (i % 200 == 0) Serial.printf("  Enviados: %u / 1000...\n", i);
        delay(50); // 20 Hz
    }
    Serial.println("[TX] Concluido 1000 pacotes enviados.");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 't' || c == 'T') transmit_1000_packets();
        else if (c == 'r' || c == 'R') {
            rcv_count = 0;
            Serial.println("[RX] Contador reiniciado. Pronto para escuta.");
        }
    }
    delay(50);
}
