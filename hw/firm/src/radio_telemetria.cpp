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

// Buffer circular simples para comandos recebidos via ESP-NOW
#define CMD_QUEUE_SIZE 16
static volatile char s_cmd_queue[CMD_QUEUE_SIZE];
static volatile uint8_t s_cmd_head = 0;
static volatile uint8_t s_cmd_tail = 0;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void aoReceberComandoEspNow(const esp_now_recv_info_t* info, const uint8_t* dados, int tamanho) {
    (void)info;
    if (dados == nullptr || tamanho <= 0) return;
    for (int i = 0; i < tamanho; i++) {
        char c = (char)dados[i];
        if (c == '\r' || c == '\n') continue;
        uint8_t next = (s_cmd_head + 1) % CMD_QUEUE_SIZE;
        if (next != s_cmd_tail) {
            s_cmd_queue[s_cmd_head] = c;
            s_cmd_head = next;
        }
    }
}
#else
static void aoReceberComandoEspNow(const uint8_t* mac, const uint8_t* dados, int tamanho) {
    (void)mac;
    if (dados == nullptr || tamanho <= 0) return;
    for (int i = 0; i < tamanho; i++) {
        char c = (char)dados[i];
        if (c == '\r' || c == '\n') continue;
        uint8_t next = (s_cmd_head + 1) % CMD_QUEUE_SIZE;
        if (next != s_cmd_tail) {
            s_cmd_queue[s_cmd_head] = c;
            s_cmd_head = next;
        }
    }
}
#endif

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

    // Registra callback de recepção de comandos remotos (bancada)
    esp_now_register_recv_cb(aoReceberComandoEspNow);

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

bool radio_telemetria_ler_comando(char* cmd) {
    if (s_cmd_head == s_cmd_tail || cmd == nullptr) return false;
    *cmd = s_cmd_queue[s_cmd_tail];
    s_cmd_tail = (s_cmd_tail + 1) % CMD_QUEUE_SIZE;
    return true;
}

#else
// Stubs para compilação nativa em testes
static bool s_mock_ready = true;
bool radio_telemetria_init(void) { return true; }
bool radio_telemetria_enviar(const TelemetriaPacket* p) { return p != nullptr; }
bool radio_telemetria_is_ready(void) { return s_mock_ready; }
bool radio_telemetria_ler_comando(char* cmd) { (void)cmd; return false; }
#endif
