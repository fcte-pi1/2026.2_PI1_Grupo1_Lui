/**
 * ==============================================================================
 * TESTE INDIVIDUAL 03: 3 SENSORES VL53L0X NO MESMO BARRAMENTO I2C
 * ==============================================================================
 * Conexões:
 *   - I2C compartilhado: SDA -> GPIO 21 | SCL -> GPIO 22
 *   - XSHUT 1 (Esquerdo): GPIO 4  (Reendereçado para 0x30)
 *   - XSHUT 2 (Frontal) : GPIO 16 (Reendereçado para 0x31)
 *   - XSHUT 3 (Direito) : GPIO 17 (Reendereçado para 0x32)
 *   - VCC: 3,3 V | GND: Terra comum
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

#define PIN_SDA      21
#define PIN_SCL      22
#define PIN_XSHUT_1   4
#define PIN_XSHUT_2  16
#define PIN_XSHUT_3  17

#define ADDR_S1 0x30
#define ADDR_S2 0x31
#define ADDR_S3 0x32

VL53L0X sensorL;
VL53L0X sensorF;
VL53L0X sensorR;

bool okL = false, okF = false, okR = false;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n--- [TESTE 03] 3 SENSORES ToF VL53L0X ---");

    Wire.begin(PIN_SDA, PIN_SCL, 100000);

    // 1. Coloca todos em reset
    pinMode(PIN_XSHUT_1, OUTPUT); digitalWrite(PIN_XSHUT_1, LOW);
    pinMode(PIN_XSHUT_2, OUTPUT); digitalWrite(PIN_XSHUT_2, LOW);
    pinMode(PIN_XSHUT_3, OUTPUT); digitalWrite(PIN_XSHUT_3, LOW);
    delay(20);

    // 2. Sensor Esquerdo (GPIO 4)
    digitalWrite(PIN_XSHUT_1, HIGH);
    delay(20);
    sensorL.setBus(&Wire);
    if (sensorL.init()) {
        sensorL.setAddress(ADDR_S1);
        sensorL.startContinuous(33);
        okL = true;
    }

    // 3. Sensor Frontal (GPIO 16)
    digitalWrite(PIN_XSHUT_2, HIGH);
    delay(20);
    sensorF.setBus(&Wire);
    if (sensorF.init()) {
        sensorF.setAddress(ADDR_S2);
        sensorF.startContinuous(33);
        okF = true;
    }

    // 4. Sensor Direito (GPIO 17)
    digitalWrite(PIN_XSHUT_3, HIGH);
    delay(20);
    sensorR.setBus(&Wire);
    if (sensorR.init()) {
        sensorR.setAddress(ADDR_S3);
        sensorR.startContinuous(33);
        okR = true;
    }

    Serial.printf("Status Inicialização: Esq=%s | Fre=%s | Dir=%s\n",
                  okL ? "OK" : "FALHA", okF ? "OK" : "FALHA", okR ? "OK" : "FALHA");
}

void loop() {
    uint16_t dL = okL ? sensorL.readRangeContinuousMillimeters() : 9999;
    uint16_t dF = okF ? sensorF.readRangeContinuousMillimeters() : 9999;
    uint16_t dR = okR ? sensorR.readRangeContinuousMillimeters() : 9999;

    Serial.printf("[DISTÂNCIA] Esq: %4d mm | Fre: %4d mm | Dir: %4d mm\n", dL, dF, dR);
    delay(100);
}
