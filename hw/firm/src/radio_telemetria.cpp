#include "radio_telemetria.h"
#include "TelemetriaCodec.h"
#include "TelemetriaConfig.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const uint8_t ENDERECO_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_radio_pronto = false;

bool radio_telemetria_init(void) {
    // Coloca Wi-Fi em modo Station e desconecta de qualquer AP
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Fixa o canal Wi-Fi no Canal 1 (compartilhado com o ESP8266 Gateway)
    if (esp_wifi_set_channel(TELEMETRIA_CANAL_WIFI, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        s_radio_pronto = false;
        return false;
    }

    // Inicializa o protocolo ESP-NOW
    if (esp_now_init() != ESP_OK) {
        s_radio_pronto = false;
        return false;
    }

    // Registra o peer de broadcast para alcançar o Gateway ESP8266 sem necessidade de pareamento MAC
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, ENDERECO_BROADCAST, 6);
    peer.channel = TELEMETRIA_CANAL_WIFI;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        s_radio_pronto = false;
        return false;
    }

    s_radio_pronto = true;
    return true;
}

bool radio_telemetria_enviar(const TelemetriaPacket* pacote) {
    if (!s_radio_pronto || pacote == nullptr) return false;

    uint8_t buffer[TELEMETRIA_TAMANHO_BYTES];
    const size_t bytes_codificados = telemetriaCodificar(*pacote, buffer, sizeof(buffer));

    if (bytes_codificados != TELEMETRIA_TAMANHO_BYTES) {
        return false;
    }

    esp_err_t res = esp_now_send(ENDERECO_BROADCAST, buffer, bytes_codificados);
    return (res == ESP_OK);
}

bool radio_telemetria_is_ready(void) {
    return s_radio_pronto;
}

#else
// Stubs para compilação nativa em testes
static bool s_mock_ready = true;
bool radio_telemetria_init(void) { return true; }
bool radio_telemetria_enviar(const TelemetriaPacket* p) { return p != nullptr; }
bool radio_telemetria_is_ready(void) { return s_mock_ready; }
#endif
