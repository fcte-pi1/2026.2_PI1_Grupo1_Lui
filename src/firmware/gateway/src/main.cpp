// Firmware do ESP32 gateway (bancada) - EAP 3.2.1.1
//
// Recebe pacotes de telemetria de 18 bytes via ESP-NOW, valida, e emite uma linha de
// JSON por pacote na serial USB (115200 bps, delimitador '\n'), conforme o contrato de
// dados (doc 4.4, seção 6). Cada JSON inclui "seq" (contador de pacotes válidos) e
// "rssi" (dBm do quadro recebido).
//
// A saída serial contém SOMENTE linhas JSON (exceto o ruído do bootloader do ESP32 na
// inicialização). Diagnósticos só são impressos com -DGATEWAY_DEBUG.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "GatewayBridge.h"
#include "TelemetriaConfig.h"

// Fila SPSC (callback do Wi-Fi -> loop). O callback roda na task do Wi-Fi, então
// a serial é escrita apenas no loop().
struct QuadroRx {
    uint8_t dados[64];
    uint16_t tamanho;  // tamanho real recebido (pode exceder 64; será rejeitado)
    int8_t rssi;
};

static const uint8_t TAM_FILA = 16;
static QuadroRx fila[TAM_FILA];
static volatile uint8_t filaEscrita = 0;
static volatile uint8_t filaLeitura = 0;
static volatile uint32_t filaPerdidos = 0;  // quadros perdidos por fila cheia

static void enfileirar(const uint8_t* dados, int tamanho, int rssi) {
    const uint8_t prox = (filaEscrita + 1) % TAM_FILA;
    if (prox == filaLeitura) {
        filaPerdidos++;
        return;
    }
    QuadroRx& q = fila[filaEscrita];
    q.tamanho = (uint16_t)tamanho;
    q.rssi = (int8_t)rssi;
    memcpy(q.dados, dados, tamanho < (int)sizeof(q.dados) ? tamanho : sizeof(q.dados));
    filaEscrita = prox;
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
// Arduino-ESP32 3.x: o RSSI vem junto com o callback do ESP-NOW.
static void aoReceber(const esp_now_recv_info_t* info, const uint8_t* dados, int tamanho) {
    enfileirar(dados, tamanho, info->rx_ctrl->rssi);
}
#else
// Arduino-ESP32 2.x: o callback do ESP-NOW não informa RSSI. Capturamos o RSSI do
// último quadro de ação (ESP-NOW trafega em action frames, byte 0xD0) pelo modo promíscuo.
static volatile int8_t ultimoRssi = 0;

static void aoReceberPromiscuo(void* buf, wifi_promiscuous_pkt_type_t tipo) {
    if (tipo != WIFI_PKT_MGMT) return;
    const wifi_promiscuous_pkt_t* pkt = (const wifi_promiscuous_pkt_t*)buf;
    if (pkt->payload[0] == 0xD0) ultimoRssi = pkt->rx_ctrl.rssi;
}

static void aoReceber(const uint8_t* mac, const uint8_t* dados, int tamanho) {
    (void)mac;
    enfileirar(dados, tamanho, ultimoRssi);
}
#endif

static GatewayBridge ponte;

void setup() {
    Serial.begin(TELEMETRIA_BAUD_SERIAL);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(TELEMETRIA_CANAL_WIFI, WIFI_SECOND_CHAN_NONE);

#if ESP_ARDUINO_VERSION_MAJOR < 3
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&aoReceberPromiscuo);
#endif

    if (esp_now_init() != ESP_OK) {
#ifdef GATEWAY_DEBUG
        Serial.println("# erro: esp_now_init falhou; reiniciando");
#endif
        delay(1000);
        ESP.restart();
    }
    esp_now_register_recv_cb(aoReceber);

#ifdef GATEWAY_DEBUG
    Serial.print("# gateway pronto. MAC: ");
    Serial.print(WiFi.macAddress());
    Serial.print(" canal: ");
    Serial.println(TELEMETRIA_CANAL_WIFI);
#endif
}

void loop() {
    static char saida[512];
    while (filaLeitura != filaEscrita) {
        const QuadroRx q = fila[filaLeitura];
        filaLeitura = (filaLeitura + 1) % TAM_FILA;

        const size_t n = ponte.processar(q.dados, q.tamanho, q.rssi, saida, sizeof(saida));
        if (n > 0) {
            Serial.write((const uint8_t*)saida, n);
        }
#ifdef GATEWAY_DEBUG
        else {
            Serial.print("# descartado: ");
            Serial.print(telemetriaNomeResultado(ponte.ultimoErro()));
            Serial.print(" (total ");
            Serial.print(ponte.descartados());
            Serial.println(")");
        }
#endif
    }
    delay(1);
}
