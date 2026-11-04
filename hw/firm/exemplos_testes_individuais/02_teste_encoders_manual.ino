/**
 * ==============================================================================
 * TESTE INDIVIDUAL 02: LEITURA MANUAL DE ENCODERS (GIRO NA MÃO)
 * ==============================================================================
 * Conexões:
 *   - Motor 1 (Esquerdo): Canal A -> GPIO 36 (VP) | Canal B -> GPIO 39 (VN)
 *   - Motor 2 (Direito) : Canal A -> GPIO 35      | Canal B -> GPIO 2 (Pull-up)
 *   - VCC: 3,3 V | GND: Terra comum
 * ==============================================================================
 */

#include <Arduino.h>

#define PIN_ENC1_A 36
#define PIN_ENC1_B 39
#define PIN_ENC2_A 35
#define PIN_ENC2_B  2

volatile int32_t ticks_m1 = 0;
volatile int32_t ticks_m2 = 0;

void IRAM_ATTR isr_m1() {
    ticks_m1 += digitalRead(PIN_ENC1_B) ? -1 : 1;
}

void IRAM_ATTR isr_m2() {
    ticks_m2 += digitalRead(PIN_ENC2_B) ? 1 : -1;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n--- [TESTE 02] LEITURA DE ENCODERS ---");
    Serial.println("Gire as rodas do robô devagar na mão:");
    Serial.println("  - 1 volta completa na roda deve marcar cerca de 360 ticks.");
    Serial.println("-------------------------------------------------------");

    pinMode(PIN_ENC1_A, INPUT);
    pinMode(PIN_ENC1_B, INPUT);
    pinMode(PIN_ENC2_A, INPUT);
    pinMode(PIN_ENC2_B, INPUT_PULLUP); // GPIO 2 com pull-up interno

    attachInterrupt(digitalPinToInterrupt(PIN_ENC1_A), isr_m1, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC2_A), isr_m2, RISING);
}

void loop() {
    float dist_m1_mm = (ticks_m1 / 360.0f) * 100.53f;
    float dist_m2_mm = (ticks_m2 / 360.0f) * 100.53f;

    Serial.printf("[ENCODER] Motor 1 (Esq): %5d ticks (%5.1f mm) | Motor 2 (Dir): %5d ticks (%5.1f mm)\n",
                  ticks_m1, dist_m1_mm, ticks_m2, dist_m2_mm);
    delay(150);
}
