/**
 * ==============================================================================
 * TESTE INDIVIDUAL 01: ACIONAMENTO BÁSICO DA PONTE H TB6612FNG
 * ==============================================================================
 * Conexões:
 *   - PWMA: GPIO 25 | AIN1: GPIO 26 | AIN2: GPIO 27
 *   - STBY: GPIO 13 (Habilita a ponte H em HIGH)
 *   - BIN1: GPIO 33 | BIN2: GPIO 32 | PWMB: GPIO 18
 *   - VM: Alimentação dos Motores (Bateria 7,4 V direta)
 *   - VCC: 3,3 V | GND: Terra comum
 * ==============================================================================
 */

#include <Arduino.h>

#define PIN_PWMA 25
#define PIN_AIN1 26
#define PIN_AIN2 27
#define PIN_STBY 13
#define PIN_BIN1 33
#define PIN_BIN2 32
#define PIN_PWMB 18

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n--- [TESTE 01] MOTORES TB6612FNG ---");

    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);
    pinMode(PIN_STBY, OUTPUT);

    // Habilita STBY
    digitalWrite(PIN_STBY, HIGH);

    // Configura PWM a 20 kHz
    ledcSetup(0, 20000, 8);
    ledcAttachPin(PIN_PWMA, 0);
    ledcSetup(1, 20000, 8);
    ledcAttachPin(PIN_PWMB, 1);
}

void loop() {
    Serial.println("[1] Motor 1 (Esquerdo) FRENTE a 40% PWM...");
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
    ledcWrite(0, 100);
    delay(1500);

    Serial.println("[2] Motor 1 FREIO...");
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    ledcWrite(0, 255);
    delay(1000);

    Serial.println("[3] Motor 2 (Direito) FRENTE a 40% PWM...");
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
    ledcWrite(1, 100);
    delay(1500);

    Serial.println("[4] Motor 2 FREIO...");
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    ledcWrite(1, 255);
    delay(1000);

    // Desliga ambos
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    delay(2000);
}
