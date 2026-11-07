/**
 * ==============================================================================
 * TESTE INDIVIDUAL 04: MOVIMENTAÇÃO BÁSICA - AVANÇAR E CURVA 90°
 * ==============================================================================
 * Integra Ponte H (TB6612) + Encoders para testar avanço de 1 célula e curvas.
 * ==============================================================================
 */

#include <Arduino.h>

// Pinos Ponte H
#define PIN_PWMA 25
#define PIN_AIN1 26
#define PIN_AIN2 27
#define PIN_STBY 13
#define PIN_BIN1 33
#define PIN_BIN2 32
#define PIN_PWMB 18

// Pinos Encoders
#define PIN_ENC1_A 36
#define PIN_ENC1_B 39
#define PIN_ENC2_A 35
#define PIN_ENC2_B  2

#define WHEEL_CIRC_MM 100.53f
#define TRACK_WIDTH_MM 80.0f
#define TICKS_PER_REV  360

volatile int32_t ticks_l = 0;
volatile int32_t ticks_r = 0;

void IRAM_ATTR isr_l() { ticks_l += digitalRead(PIN_ENC1_B) ? -1 : 1; }
void IRAM_ATTR isr_r() { ticks_r += digitalRead(PIN_ENC2_B) ? 1 : -1; }

void frear() {
    digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, HIGH); ledcWrite(0, 255);
    digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, HIGH); ledcWrite(1, 255);
    delay(200);
    ledcWrite(0, 0); ledcWrite(1, 0);
}

void andarDistancia(float mm, int pwm) {
    ticks_l = 0; ticks_r = 0;
    int32_t target = (int32_t)((mm / WHEEL_CIRC_MM) * TICKS_PER_REV);

    digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, LOW); ledcWrite(0, pwm);
    digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, LOW); ledcWrite(1, pwm);

    while (abs((ticks_l + ticks_r) / 2) < target) {
        delay(5);
    }
    frear();
}

void girar90Graus(bool direita, int pwm) {
    ticks_l = 0; ticks_r = 0;
    float arco = (3.14159f * TRACK_WIDTH_MM) / 4.0f; // 90 graus
    int32_t target = (int32_t)((arco / WHEEL_CIRC_MM) * TICKS_PER_REV);

    if (direita) {
        digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, LOW); ledcWrite(0, pwm);
        digitalWrite(PIN_BIN1, LOW);  digitalWrite(PIN_BIN2, HIGH); ledcWrite(1, pwm);
    } else {
        digitalWrite(PIN_AIN1, LOW);  digitalWrite(PIN_AIN2, HIGH); ledcWrite(0, pwm);
        digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, LOW); ledcWrite(1, pwm);
    }

    while (abs((ticks_l + ticks_r) / 2) < target) {
        delay(5);
    }
    frear();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n--- [TESTE 04] AVANÇO E CURVAS 90° ---");

    pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT);
    pinMode(PIN_STBY, OUTPUT); digitalWrite(PIN_STBY, HIGH);

    ledcSetup(0, 20000, 8); ledcAttachPin(PIN_PWMA, 0);
    ledcSetup(1, 20000, 8); ledcAttachPin(PIN_PWMB, 1);

    pinMode(PIN_ENC1_A, INPUT); pinMode(PIN_ENC1_B, INPUT);
    pinMode(PIN_ENC2_A, INPUT); pinMode(PIN_ENC2_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_ENC1_A), isr_l, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC2_A), isr_r, RISING);

    Serial.println("Pressione '1' para Avançar 1 Célula (168 mm)");
    Serial.println("Pressione '2' para Girar 90° Direita");
    Serial.println("Pressione '3' para Girar 90° Esquerda");
}

void loop() {
    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '1') {
            Serial.println("Avançando 168 mm...");
            andarDistancia(168.0f, 90);
            Serial.printf("Concluído! Ticks: L=%d | R=%d\n", ticks_l, ticks_r);
        } else if (c == '2') {
            Serial.println("Girando 90° à Direita...");
            girar90Graus(true, 90);
            Serial.printf("Concluído! Ticks: L=%d | R=%d\n", ticks_l, ticks_r);
        } else if (c == '3') {
            Serial.println("Girando 90° à Esquerda...");
            girar90Graus(false, 90);
            Serial.printf("Concluído! Ticks: L=%d | R=%d\n", ticks_l, ticks_r);
        }
    }
}
