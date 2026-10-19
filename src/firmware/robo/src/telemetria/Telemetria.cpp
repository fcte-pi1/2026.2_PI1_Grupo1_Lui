// src/telemetria/Telemetria.cpp
#include "telemetria/Telemetria.h"
#include <WiFi.h>
#include <esp_now.h>

static uint8_t peer_mac[6];

bool Telemetria::iniciar(const uint8_t macGateway[6]) {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return false;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macGateway, 6);
    memcpy(peer_mac, macGateway, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

bool Telemetria::enviar(const TelemetriaPacket& p) {
    return esp_now_send(peer_mac, (const uint8_t*)&p, sizeof(p)) == ESP_OK;
}