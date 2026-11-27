#include "RadioTelemetria.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "TelemetriaCodec.h"
#include "TelemetriaConfig.h"

static const uint8_t ENDERECO_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Fila SPSC mínima para comandos recebidos do Gateway via ESP-NOW (callback Wi-Fi -> loop).
static const uint8_t TAM_FILA_CMD = 16;
static volatile char filaCmd[TAM_FILA_CMD];
static volatile uint8_t cmdEscrita = 0;
static volatile uint8_t cmdLeitura = 0;

static void enfileirarComando(const uint8_t* dados, int tamanho) {
    if (tamanho <= 0 || dados == nullptr) return;
    for (int i = 0; i < tamanho; i++) {
        const char c = (char)dados[i];
        if (c == '\r' || c == '\n') continue;
        const uint8_t prox = (uint8_t)((cmdEscrita + 1) % TAM_FILA_CMD);
        if (prox == cmdLeitura) break;
        filaCmd[cmdEscrita] = c;
        cmdEscrita = prox;
    }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void aoReceberComando(const esp_now_recv_info_t* info, const uint8_t* dados, int tamanho) {
    (void)info;
    enfileirarComando(dados, tamanho);
}
#else
static void aoReceberComando(const uint8_t* mac, const uint8_t* dados, int tamanho) {
    (void)mac;
    enfileirarComando(dados, tamanho);
}
#endif

bool radioTelemetriaInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_wifi_set_channel(TELEMETRIA_CANAL_WIFI, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;
    if (esp_now_init() != ESP_OK) return false;
    esp_now_register_recv_cb(aoReceberComando);

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

bool radioTelemetriaLerComando(char& cmd) {
    if (cmdLeitura == cmdEscrita) return false;
    cmd = filaCmd[cmdLeitura];
    cmdLeitura = (uint8_t)((cmdLeitura + 1) % TAM_FILA_CMD);
    return true;
}
