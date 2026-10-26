#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define ESPNOW_CHANNEL 1

typedef struct struct_message {
    uint32_t msg_id;
    char payload[200];
} struct_message;

struct_message incomingData;

volatile uint32_t expected_id = 1;
volatile uint32_t current_id = 0;
volatile uint32_t packetsReceived = 0;
volatile uint32_t packetsLost = 0;
volatile bool firstPacket = true;
unsigned long lastReportTime = 0;
unsigned long lastPacketTime = 0;

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingBuffer, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingBuffer, int len) {
#endif
    if (len == sizeof(incomingData)) {
        memcpy(&incomingData, incomingBuffer, sizeof(incomingData));
        
        current_id = incomingData.msg_id;
        lastPacketTime = millis();

        if (firstPacket) {
            expected_id = current_id;
            firstPacket = false;
        }

        if (current_id < expected_id) {
            Serial.println("\n--- Transmissor Reiniciado / Contador Resetado ---");
            packetsReceived = 0;
            packetsLost = 0;
            expected_id = current_id;
        }

        if (current_id > expected_id) {
            packetsLost += (current_id - expected_id);
        }

        packetsReceived++;
        expected_id = current_id + 1;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Aguarda estabilizar a serial

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Trava o rádio no Canal 1
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    Serial.println("\n========================================");
    Serial.printf("ESP32 RECEPTOR INICIADO COM SUCESSO!\n");
    Serial.printf("MAC Real do Receptor: %s\n", WiFi.macAddress().c_str());
    Serial.printf("Canal Wi-Fi: %d\n", WiFi.channel());
    Serial.println("========================================\n");

    if (esp_now_init() != ESP_OK) {
        Serial.println("ERRO CRITICO: Falha ao inicializar ESP-NOW!");
        return;
    }

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    esp_now_register_recv_cb(OnDataRecv);
#else
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
#endif
}

void loop() {
    if (millis() - lastReportTime >= 2000) {
        lastReportTime = millis();
        
        if (firstPacket) {
            Serial.printf("[Aguardando] Canal %d | MAC: %s | Nenhum pacote recebido ainda...\n", 
                          WiFi.channel(), WiFi.macAddress().c_str());
        } else {
            uint32_t totalExpected = packetsReceived + packetsLost;
            float lossPercentage = (totalExpected > 0) ? (((float)packetsLost / totalExpected) * 100.0) : 0.0;
            bool semSinal = (millis() - lastPacketTime > 2000);

            Serial.printf("\n================ METRICAS ESP-NOW ================\n");
            if (semSinal) {
                Serial.printf("STATUS             : ALERTA - SEM RECEBER PACOTES RECENTES!\n");
            } else {
                Serial.printf("STATUS             : OK - RECEBENDO DADOS!\n");
            }
            Serial.printf("Ultimo ID Recebido : %u\n", current_id);
            Serial.printf("Pacotes Recebidos  : %u\n", packetsReceived);
            Serial.printf("Pacotes Perdidos   : %u\n", packetsLost);
            Serial.printf("Taxa de Perda      : %.2f%%\n", lossPercentage);
            Serial.printf("==================================================\n");
        }
    }
}
