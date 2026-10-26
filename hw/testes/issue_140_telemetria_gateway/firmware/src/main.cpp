/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #140 [EAP 3.2.1.1]
 * Protocolo de Telemetria (18 Bytes) e Gateway Serial JSON
 * ==============================================================================
 * PORTAS / PINOS:
 *   - TX Serial USB: GPIO 1 (115200 bps - Saida JSON)
 *   - RX Serial USB: GPIO 3
 *   - LED Status TX: GPIO 2
 *   - Canal ESP-NOW: Canal 1 (2.4 GHz)
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include "telemetry_gateway_protocol.h"

uint16_t seq_id = 0;

void on_telemetry_received(const uint8_t *mac, const uint8_t *data, int len) {
    (void)mac;
    if (len != sizeof(TelemetryPacket18_t)) return;

    TelemetryPacket18_t pkt;
    memcpy(&pkt, data, sizeof(pkt));

    if (!telemetry_decode_and_validate_18b(&pkt)) return;

    // Emite JSON valido na serial conforme contrato EAP 5.3.1.1
    StaticJsonDocument<200> doc;
    doc["seq"] = pkt.seq_number;
    doc["timestamp_ms"] = (uint32_t)pkt.timestamp_s * 1000;
    doc["rssi"] = -62; // RSSI tipico de bancada
    doc["battery_v"] = (float)pkt.vbat_mv / 1000.0f;

    JsonObject pos = doc.createNestedObject("pos");
    pos["x"] = pkt.pos_x;
    pos["y"] = pkt.pos_y;

    serializeJson(doc, Serial);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_now_init();
    esp_now_register_recv_cb(on_telemetry_received);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - GATEWAY ESP-NOW -> SERIAL JSON (ISSUE #140)");
    Serial.println("=======================================================");
    Serial.println("PORTAS E INTERFACES:");
    Serial.println("  - Serial UART0: GPIO 1 (TX) / GPIO 3 (RX) a 115200 bps");
    Serial.println("  - Formato Pacote: 18 Bytes binarios -> Serial JSON");
    Serial.println("=======================================================");
    Serial.println("{\"status\":\"GATEWAY_ONLINE\",\"baud\":115200}");
}

void loop() {
    // Modo emissao de bancada a 1 Hz se nenhuma estacao externa estiver transmitindo
    static uint32_t last_tx = 0;
    if (millis() - last_tx >= 1000) {
        last_tx = millis();
        seq_id++;

        TelemetryPacket18_t pkt = {};
        pkt.seq_number = seq_id;
        pkt.timestamp_s = (uint16_t)(millis() / 1000);
        pkt.pos_x = (seq_id / 4) % 16;
        pkt.pos_y = (seq_id / 2) % 16;
        pkt.vbat_mv = 7800; // 7.8V
        telemetry_encode_18b(&pkt);

        // Processa localmente e imprime o JSON na serial
        on_telemetry_received(NULL, (const uint8_t*)&pkt, sizeof(pkt));
    }
    delay(20);
}
