/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #132 [EAP 4.7.1]
 * Teste de Carga, Autonomia e Protecao por Sobrecorrente
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t PIN_LOAD = 34;
const uint8_t LED_FUSE = 2;

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_LOAD, INPUT);
    pinMode(LED_FUSE, OUTPUT);
    digitalWrite(LED_FUSE, HIGH);

    Wire.begin(21, 22);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    Serial.println("\n=======================================================");
    Serial.println("  WOKWI SIMULATOR - ISSUE #132: CARGA E AUTONOMIA");
    Serial.println("=======================================================");
    Serial.println("Ajuste o potenciometro de carga para simular correntes de 100 mA a 1500 mA.");
}

void loop() {
    int raw = analogRead(PIN_LOAD);
    // Simula corrente de carga entre 100 mA e 1500 mA
    float current_ma = 100.0f + ((float)raw / 4095.0f) * 1400.0f;
    float useful_mah = 850.0f * 0.85f; // 722.5 mAh
    float autonomy_min = (useful_mah / current_ma) * 60.0f;

    bool is_overcurrent = (current_ma >= 1200.0f);
    digitalWrite(LED_FUSE, is_overcurrent ? LOW : HIGH); // Desarma fusivel se sobrecorrente

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("BATTERY AUTONOMY TEST");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 16);
    display.printf("Bateria:   850 mAh LiPo\n");
    display.printf("Corrente:  %5.0f mA\n", current_ma);
    display.printf("Autonomia: %5.1f min\n", autonomy_min);
    display.printf("Req Min:   15.0 min\n");

    display.setCursor(0, 52);
    if (is_overcurrent) {
        display.print("ALERTA: SOBRECORRENTE!");
    } else if (autonomy_min >= 15.0f) {
        display.print("STATUS: RNF-ENE01 OK");
    } else {
        display.print("ALERTA: AUTONOMIA <15m");
    }

    display.display();
    delay(200);
}
