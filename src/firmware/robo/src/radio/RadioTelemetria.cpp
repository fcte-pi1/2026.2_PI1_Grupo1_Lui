#include "RadioTelemetria.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "TelemetriaCodec.h"
#include "TelemetriaConfig.h"

static const uint8_t ENDERECO_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool radioTelemetriaInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_wifi_set_channel(TELEMETRIA_CANAL_WIFI, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;
    if (esp_now_init() != ESP_OK) return false;

    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, ENDERECO_BROADCAST, 6);
    peer.channel = TELEMETRIA_CANAL_WIFI;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
}

bool radioTelemetriaEnviar(const TelemetriaPacket& pacote) {
    uint8_t buf[TELEMETRIA_TAMANHO_BYTES];
    const size_t n = telemetriaCodificar(pacote, buf, sizeof(buf));
    if (n != TELEMETRIA_TAMANHO_BYTES) return false;
    return esp_now_send(ENDERECO_BROADCAST, buf, n) == ESP_OK;
}
