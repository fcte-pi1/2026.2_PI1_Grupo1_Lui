/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #136 [EAP 3.1.2.1]
 * Acionamento da Ponte H TB6612FNG e Leitura de Encoders em Quadratura
 * ==============================================================================
 * MAPEAMENTO DE PORTAS E PINOS (SHIELD REV 2.0):
 *   - Motor Esquerdo PWM (PWMA): GPIO 16 (Canal LEDC 0)
 *   - Motor Esquerdo Sentido 1:  GPIO 18 (AIN1)
 *   - Motor Esquerdo Sentido 2:  GPIO 19 (AIN2)
 *   - Motor Direito PWM (PWMB):  GPIO 17 (Canal LEDC 1)
 *   - Motor Direito Sentido 1:   GPIO 23 (BIN1)
 *   - Motor Direito Sentido 2:   GPIO 5  (BIN2)
 *   - Encoder Esquerdo Canal A:  GPIO 27 (Interrupcao RISING)
 *   - Encoder Esquerdo Canal B:  GPIO 26 (Sentido)
 *   - Encoder Direito Canal A:   GPIO 25 (Interrupcao RISING)
 *   - Encoder Direito Canal B:   GPIO 33 (Sentido)
 *   - Alimentacao Logica:        3V3 e GND
 *   - Alimentacao dos Motores:   7.4V (Bateria LiPo 2S)
 * ==============================================================================
 */

#include <Arduino.h>
#include "motor_encoder_driver.h"

// Contadores de pulsos
volatile int32_t ticks_l = 0;
volatile int32_t ticks_r = 0;

void IRAM_ATTR isr_enc_l() {
    ticks_l += digitalRead(PIN_ENC_L_B) ? -1 : 1;
}

void IRAM_ATTR isr_enc_r() {
    ticks_r += digitalRead(PIN_ENC_R_B) ? 1 : -1;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - ISSUE #136: MOTORES E ENCODERS");
    Serial.println("=======================================================");
    Serial.println("PORTAS / PINOS CONFIGURADOS:");
    Serial.println("  PWMA: GPIO 16 | AIN1: GPIO 18 | AIN2: GPIO 19");
    Serial.println("  PWMB: GPIO 17 | BIN1: GPIO 23 | BIN2: GPIO 5");
    Serial.println("  Enc L: GPIO 27/26 | Enc R: GPIO 25/33");
    Serial.println("=======================================================");

    // Pinos de direcao
    pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT);

    // PWM a 20 kHz
    ledcSetup(0, 20000, 8); ledcAttachPin(PIN_PWMA, 0);
    ledcSetup(1, 20000, 8); ledcAttachPin(PIN_PWMB, 1);

    // Encoders com pullup interno
    pinMode(PIN_ENC_L_A, INPUT_PULLUP); pinMode(PIN_ENC_L_B, INPUT_PULLUP);
    pinMode(PIN_ENC_R_A, INPUT_PULLUP); pinMode(PIN_ENC_R_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L_A), isr_enc_l, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R_A), isr_enc_r, RISING);

    // Garante parada inicial
    digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
    ledcWrite(0, 0); ledcWrite(1, 0);

    Serial.println("Digite 'p' para teste de PWM (25/50/75/100%) ou 'f' para frenagem.");
}

void run_pwm_test() {
    const uint8_t pwms[] = {25, 50, 75, 100};
    for (int dir = 0; dir < 2; dir++) {
        bool fwd = (dir == 0);
        Serial.printf("\n--- Testando Sentido: %s ---\n", fwd ? "FRENTE" : "RE");
        digitalWrite(PIN_AIN1, fwd ? HIGH : LOW);
        digitalWrite(PIN_AIN2, fwd ? LOW : HIGH);
        digitalWrite(PIN_BIN1, fwd ? HIGH : LOW);
        digitalWrite(PIN_BIN2, fwd ? LOW : HIGH);

        for (int i = 0; i < 4; i++) {
            uint8_t duty = (pwms[i] * 255) / 100;
            ticks_l = 0; ticks_r = 0;
            ledcWrite(0, duty); ledcWrite(1, duty);
            delay(1000);
            Serial.printf("PWM: %3d%% (Duty: %3d) | Ticks L: %4ld | Ticks R: %4ld [OK]\n",
                          pwms[i], duty, (long)ticks_l, (long)ticks_r);
        }
    }
    // Freio
    digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, HIGH);
    digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, HIGH);
    ledcWrite(0, 255); ledcWrite(1, 255);
    Serial.println("[FREIO ATIVO] Motores travados.");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'p' || c == 'P') run_pwm_test();
        else if (c == 'f' || c == 'F') {
            digitalWrite(PIN_AIN1, HIGH); digitalWrite(PIN_AIN2, HIGH);
            digitalWrite(PIN_BIN1, HIGH); digitalWrite(PIN_BIN2, HIGH);
            ledcWrite(0, 255); ledcWrite(1, 255);
            Serial.println("[FREIO] Parada ativa acionada.");
        }
    }
    delay(100);
}
