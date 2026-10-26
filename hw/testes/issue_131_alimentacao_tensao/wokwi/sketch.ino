/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #131 [EAP 4.1.2]
 * Montagem do Circuito e Afericao de Tensao (Monitoramento de Bateria)
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t PIN_BAT = 34;
const uint8_t LED_WARN = 4;

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_BAT, INPUT);
    pinMode(LED_WARN, OUTPUT);

    Wire.begin(21, 22);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("MICROMOUSE - GRUPO 1");
        display.println("ISSUE #131: ENERGIA");
        display.println("MONITORAMENTO TENSAO");
        display.display();
        delay(1500);
    }

    Serial.println("\n=======================================================");
    Serial.println("  WOKWI SIMULATOR - ISSUE #131: AFERICAO DE TENSAO");
    Serial.println("=======================================================");
    Serial.println("Gire o potenciometro da bateria (GPIO 34) para simular variacao de 6.0V a 8.4V.");
}

void loop() {
    int raw_adc = analogRead(PIN_BAT);
    // Simula tensao da bateria entre 6.0V e 8.4V proporcional ao potenciometro
    float v_bat = 6.0f + ((float)raw_adc / 4095.0f) * 2.4f;
    float pct = ((v_bat - 6.6f) / (8.4f - 6.6f)) * 100.0f;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    bool is_warn = (v_bat <= 7.0f);
    digitalWrite(LED_WARN, is_warn ? HIGH : LOW);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("POWER SUPPLY MONITOR");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 16);
    display.printf("Bateria: %5.2f V\n", v_bat);
    display.printf("Nivel:   %5.1f %%\n", pct);
    display.printf("Linha 5V:  5.01 V [OK]\n");
    display.printf("Linha 3V3: 3.30 V [OK]\n");

    // Barra de bateria
    display.drawRect(4, 52, 120, 8, WHITE);
    int bar_w = (int)((pct / 100.0f) * 118.0f);
    display.fillRect(5, 53, bar_w, 6, WHITE);

    if (v_bat <= 6.6f) {
        display.setCursor(0, 42);
        display.print("CORTE SUBTENSAO!");
    } else if (is_warn) {
        display.setCursor(0, 42);
        display.print("ALERTA: BATERIA BAIXA");
    }

    display.display();

    Serial.printf("[BATERIA] V_Bat: %5.2f V | Carga: %5.1f%% | Status: %s\n",
                  v_bat, pct, (v_bat <= 6.6f ? "CORTE CRITICO" : (is_warn ? "BATERIA FRACA" : "NORMAL")));
    delay(200);
}
