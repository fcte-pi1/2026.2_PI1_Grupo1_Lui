/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #98 [EAP 3.1.4]
 * Interface de Usuario e Sinalizacao (IHM): LED RGB, Buzzer e Botao de Largada
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define PIN_RGB 2
#define PIN_BTN 15
#define PIN_BUZZER 4

Adafruit_NeoPixel strip(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

int current_mode = 0; // 0=Calib, 1=Pronto, 2=Navegando, 3=Objetivo, 4=Falha
uint32_t last_btn_time = 0;
bool last_btn_val = HIGH;

void play_beep(int freq, int dur) {
    tone(PIN_BUZZER, freq, dur);
    delay(dur);
    noTone(PIN_BUZZER);
}

void apply_mode(int mode) {
    current_mode = mode;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("IHM STATUS MONITOR");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 16);
    display.setTextSize(1);

    switch (mode) {
        case 0:
            strip.setPixelColor(0, strip.Color(0, 0, 255)); // Azul
            display.println("MODO: 1. CALIBRACAO");
            display.println("LED:  AZUL");
            display.println("SOM:  2 Bipes de Boot");
            Serial.println("[IHM] Modo 1: CALIBRACAO (LED Azul)");
            play_beep(1000, 80); delay(40); play_beep(1500, 80);
            break;
        case 1:
            strip.setPixelColor(0, strip.Color(0, 255, 0)); // Verde
            display.println("MODO: 2. PRONTO (ARM)");
            display.println("LED:  VERDE");
            display.println("SOM:  Bipe de Confirmacao");
            Serial.println("[IHM] Modo 2: PRONTO (LED Verde)");
            play_beep(1800, 150);
            break;
        case 2:
            strip.setPixelColor(0, strip.Color(255, 200, 0)); // Amarelo
            display.println("MODO: 3. NAVEGANDO");
            display.println("LED:  AMARELO");
            display.println("SOM:  Bipe de Largada");
            Serial.println("[IHM] Modo 3: NAVEGANDO (LED Amarelo)");
            play_beep(2200, 300);
            break;
        case 3:
            strip.setPixelColor(0, strip.Color(180, 0, 255)); // Roxo
            display.println("MODO: 4. GOL ALCANCADO!");
            display.println("LED:  ROXO");
            display.println("SOM:  Fanfarra Vitoria");
            Serial.println("[IHM] Modo 4: OBJETIVO ALCANCADO (LED Roxo)");
            play_beep(1200, 100); delay(30); play_beep(1600, 100); delay(30); play_beep(2200, 250);
            break;
        case 4:
            strip.setPixelColor(0, strip.Color(255, 0, 0)); // Vermelho
            display.println("MODO: 5. FALHA/BATERIA");
            display.println("LED:  VERMELHO");
            display.println("SOM:  Alarme Continuo");
            Serial.println("[IHM] Modo 5: FALHA / BATERIA FRACA (LED Vermelho)");
            play_beep(3000, 80); delay(50); play_beep(3000, 80);
            break;
    }
    strip.show();
    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);

    strip.begin();
    strip.show();

    Wire.begin(21, 22);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    Serial.println("\n=======================================================");
    Serial.println("  WOKWI SIMULATOR - ISSUE #98: SINALIZACAO E IHM");
    Serial.println("=======================================================");
    Serial.println("Pressione o botao verde (ou tecla '1') para alternar entre os 5 modos!");

    apply_mode(0);
}

void loop() {
    bool btn = digitalRead(PIN_BTN);
    if (btn != last_btn_val) {
        last_btn_time = millis();
    }
    if ((millis() - last_btn_time) >= 50) {
        if (btn == LOW && last_btn_val == HIGH) {
            int next = (current_mode + 1) % 5;
            apply_mode(next);
        }
        last_btn_val = btn;
    }

    if (Serial.available()) {
        char c = Serial.read();
        if (c >= '1' && c <= '5') {
            apply_mode(c - '1');
        } else if (c == 'n' || c == 'N') {
            apply_mode((current_mode + 1) % 5);
        }
    }

    // Pisca no estado de falha
    if (current_mode == 4) {
        static bool blink = false;
        blink = !blink;
        if (blink) strip.setPixelColor(0, strip.Color(255, 0, 0));
        else strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
    }

    delay(100);
}
