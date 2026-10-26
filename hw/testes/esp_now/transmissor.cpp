#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <espnow.h>
  extern "C" {
    #include <user_interface.h>
  }
#elif defined(ESP32)
  #include <WiFi.h>
  #include <esp_now.h>
  #include <esp_wifi.h>
#endif

// MAC ADDRESS DO RECEPTOR (ESP32)
uint8_t broadcastAddress[] = {0x30, 0xC9, 0x22, 0x64, 0x0A, 0x48};

typedef struct struct_message {
    uint32_t msg_id;
    char payload[200];
} struct_message;

struct_message testData;

#if defined(ESP32)
esp_now_peer_info_t peerInfo;
#endif

volatile uint32_t packetsSent = 0;
volatile uint32_t hardwareFailures = 0;
unsigned long lastReportTime = 0;

bool channelLocked = false;
uint8_t currentChannel = 1;
uint8_t channelAttempts = 0;

void setWifiChannel(uint8_t ch) {
#if defined(ESP8266)
    wifi_promiscuous_enable(true);
    wifi_set_channel(ch);
    wifi_promiscuous_enable(false);
    esp_now_del_peer(broadcastAddress);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, ch, NULL, 0);
#elif defined(ESP32)
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    esp_now_del_peer(broadcastAddress);
    peerInfo.channel = ch;
    esp_now_add_peer(&peerInfo);
#endif
}

#if defined(ESP8266)
void OnDataSent(uint8_t *mac_addr, uint8_t status) {
    packetsSent++;
    if (status != 0) {
        hardwareFailures++;
    }
}
#elif defined(ESP32)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    packetsSent++;
    if (status != ESP_NOW_SEND_SUCCESS) {
        hardwareFailures++;
    }
}
#endif

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != 0) {
        Serial.println("Erro ao inicializar o ESP-NOW");
        return;
    }

#if defined(ESP8266)
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_send_cb(OnDataSent);
#elif defined(ESP32)
    esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.encrypt = false;
#endif

    setWifiChannel(currentChannel);

    memset(testData.payload, 'X', sizeof(testData.payload) - 1);
    testData.payload[sizeof(testData.payload) - 1] = '\0';
    testData.msg_id = 0;
}

void loop() {
    testData.msg_id++;
    esp_now_send(broadcastAddress, (uint8_t *) &testData, sizeof(testData));
    delay(50); // 20 pacotes por segundo

    // Varredura automática: procura em qual canal Wi-Fi a ESP32 receptora está ouvindo
    if (!channelLocked) {
        channelAttempts++;
        if (packetsSent > hardwareFailures) {
            channelLocked = true;
            Serial.printf("\n>>> SUCESSO! Receptor ESP32 detectado no Canal %d! Canal fixado. <<<\n\n", currentChannel);
            packetsSent = 0;
            hardwareFailures = 0;
        } else if (channelAttempts >= 15) { // Se enviar 15 pacotes sem nenhum ACK, tenta o próximo canal
            channelAttempts = 0;
            currentChannel = (currentChannel % 13) + 1;
            Serial.printf("Buscando receptor no Canal %d...\n", currentChannel);
            setWifiChannel(currentChannel);
            packetsSent = 0;
            hardwareFailures = 0;
        }
    }

    // Imprime telemetria no PC a cada 2 segundos
    if (millis() - lastReportTime >= 2000 && packetsSent > 0) {
        lastReportTime = millis();
        float hardwareLossRate = ((float)hardwareFailures / packetsSent) * 100.0;
        Serial.printf("[Canal %d%s] Enviados: %u | Falhas ACK: %u | Perda: %.2f%%\n", 
                      currentChannel, channelLocked ? " (Conectado)" : " (Buscando)",
                      packetsSent, hardwareFailures, hardwareLossRate);
    }
}
