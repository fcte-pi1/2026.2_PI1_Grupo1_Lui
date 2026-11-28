/**
 * TESTE DE PINOS EM NIVEL LOGICO ALTO CONSTANTE (3.3V DC)
 * Mantem os 7 pinos da ponte H (TB6612FNG) em HIGH continuamente para medicao
 * com multimetro / verificacao de continuidade e GPIOs da ESP32.
 */

#include <Arduino.h>
#include "PinosRobo.h"

static constexpr uint8_t PINOS_PONTE[] = {
    PINO_PONTE_PWMA,  // 25
    PINO_PONTE_AIN1,  // 26
    PINO_PONTE_AIN2,  // 27
    PINO_PONTE_STBY,  // 13
    PINO_PONTE_BIN1,  // 18
    PINO_PONTE_BIN2,  // 19
    PINO_PONTE_PWMB,  // 23
};

static const char* const NOMES_PINOS[] = {
    "PWMA", "AIN1", "AIN2", "STBY", "BIN1", "BIN2", "PWMB",
};

static constexpr size_t QTD_PINOS = sizeof(PINOS_PONTE) / sizeof(PINOS_PONTE[0]);

void setup() {
    Serial.begin(115200);
    delay(300);

    for (size_t i = 0; i < QTD_PINOS; i++) {
        pinMode(PINOS_PONTE[i], OUTPUT);
        digitalWrite(PINOS_PONTE[i], HIGH);
    }

    Serial.println("\n=======================================================");
    Serial.println("  TODOS OS 7 PINOS DA PONTE H EM NIVEL ALTO (HIGH)");
    Serial.println("=======================================================");
    for (size_t i = 0; i < QTD_PINOS; i++) {
        Serial.printf("  %-4s (GPIO %2u) = HIGH\n", NOMES_PINOS[i], (unsigned)PINOS_PONTE[i]);
    }
    Serial.println("=======================================================");
}

void loop() {
    static uint32_t ultimoMs = 0;
    if (millis() - ultimoMs >= 2000) {
        ultimoMs = millis();
        Serial.print("[HIGH CONSTANTE]");
        for (size_t i = 0; i < QTD_PINOS; i++) {
            digitalWrite(PINOS_PONTE[i], HIGH);
            Serial.printf(" %s(%u)=%d", NOMES_PINOS[i], (unsigned)PINOS_PONTE[i],
                          digitalRead(PINOS_PONTE[i]));
        }
        Serial.println();
    }
}
