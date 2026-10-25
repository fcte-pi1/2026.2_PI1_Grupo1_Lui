/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #98 [EAP 3.1.4]
 * Interface de Usuario e Sinalizacao (LED RGB, Buzzer e Botao)
 * ==============================================================================
 * MAPEAMENTO DE PORTAS E PINOS (SHIELD REV 2.0):
 *   - Botao de Largada: GPIO 15 (Pull-Up interno por software)
 *   - LED RGB Status:   GPIO 2  (WS2812B / 5 Estados Operacionais)
 *   - Buzzer Ativo:     GPIO 4  (Alarme Acustico >= 60 dB)
 * ==============================================================================
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "ihm_driver.h"

Adafruit_NeoPixel strip(1, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);
int mode = 0; // 0=Calib, 1=Pronto, 2=Navegando, 3=Gol, 4=Falha/Bateria
uint32_t last_press = 0;

void set_mode(int new_mode) {
    mode = new_mode % 5;
    VisualPattern_t p = ihm_get_visual_pattern((OperationalState_t)mode);
    strip.setPixelColor(0, strip.Color(p.r, p.g, p.b));
    strip.show();

    // Bipe sonoro de confirmacao no buzzer
    tone(PIN_BUZZER, 1500 + (mode * 200), 100);
    delay(100);
    noTone(PIN_BUZZER);

    const char* names[] = {"1. CALIBRACAO (Azul)", "2. PRONTO (Verde)", "3. NAVEGANDO (Amarelo)", "4. GOL (Roxo)", "5. FALHA (Vermelho)"};
    Serial.printf("[IHM] Modo: %s | Pinos: Botao 15, LED 2, Buzzer 4\n", names[mode]);
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    strip.begin();

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - SINALIZACAO E IHM (ISSUE #98)");
    Serial.println("=======================================================");
    Serial.println("PORTAS E PINOS:");
    Serial.println("  - Botao de Largada: GPIO 15 (Pull-up)");
    Serial.println("  - LED RGB Status:   GPIO 2");
    Serial.println("  - Buzzer Piezo:     GPIO 4");
    Serial.println("=======================================================");
    Serial.println("Pressione o botao (GPIO 15) para alternar entre os 5 estados.");

    set_mode(0);
}

void loop() {
    // Leitura do botao com debounce simples de 50 ms
    if (digitalRead(PIN_BUTTON_START) == LOW) {
        if (millis() - last_press > 250) {
            last_press = millis();
            set_mode(mode + 1);
        }
    }

    if (Serial.available()) {
        Serial.read();
        set_mode(mode + 1);
    }
    delay(20);
}
