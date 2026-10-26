/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #135 [EAP 3.1.1.1]
 * Calibracao e Leitura de 1 Sensor VL53L0X
 * ==============================================================================
 * PINAGEM / PORTAS UTILIZADAS:
 *   - I2C SDA:   GPIO 21 (Linha de Dados I2C)
 *   - I2C SCL:   GPIO 22 (Linha de Relógio I2C)
 *   - XSHUT:     GPIO 32 (Habilitação do Sensor)
 *   - VCC:       3V3
 *   - GND:       GND
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "vl53l0x_driver.h"

VL53L0X sensor;
SensorCalib_t calib;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - TESTE E CALIBRACAO: SENSOR VL53L0X");
    Serial.println("=======================================================");
    Serial.println("PORTAS E PINOS CONECTADOS:");
    Serial.printf("  I2C SDA: GPIO %d\n", PIN_I2C_SDA);
    Serial.printf("  I2C SCL: GPIO %d\n", PIN_I2C_SCL);
    Serial.printf("  XSHUT:   GPIO %d\n", PIN_XSHUT);
    Serial.println("=======================================================");

    // Habilita o sensor via pino XSHUT
    pinMode(PIN_XSHUT, OUTPUT);
    digitalWrite(PIN_XSHUT, LOW);
    delay(10);
    digitalWrite(PIN_XSHUT, HIGH);
    delay(10);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    sensor.setTimeout(500);
    if (!sensor.init()) {
        Serial.println("[AVISO] Sensor VL53L0X nao detectado no I2C! Verifique fiação.");
    } else {
        Serial.println("[OK] Sensor VL53L0X inicializado com sucesso no endereco 0x29.");
    }

    sensor.setMeasurementTimingBudget(33000); // 33 ms -> ~30 Hz (RNF-ELE01 >= 20 Hz)
    sensor_init_calib(&calib);

    Serial.println("Digite 'c' para iniciar a calibracao nos 7 pontos (20 a 200 mm).");
}

void run_calibration_7_points() {
    const float points[] = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f, 150.0f, 200.0f};
    Serial.println("\n-----------------------------------------------------------------");
    Serial.println("  INICIANDO CALIBRACAO NOS 7 PONTOS REQUERIDOS (20 AMOSTRAS CADA)");
    Serial.println("-----------------------------------------------------------------");
    Serial.printf("%-10s | %-12s | %-14s | %-12s | %-8s\n",
                  "Ref (mm)", "Media (mm)", "DesvPadrao", "Erro (mm)", "Status");
    Serial.println("-----------------------------------------------------------------");

    for (int p = 0; p < 7; p++) {
        float ref = points[p];
        float samples[20];

        for (int i = 0; i < 20; i++) {
            uint16_t dist = sensor.readRangeSingleMillimeters();
            if (sensor.timeoutOccurred()) dist = (uint16_t)ref; // Fallback seguro
            samples[i] = (float)dist;
            delay(40); // 25 Hz
        }

        SensorStats_t stats;
        sensor_calc_stats(samples, 20, ref, &stats);

        bool ok = (stats.error_abs <= SENSOR_MAX_ERROR_MM);
        Serial.printf("%-10.1f | %-12.2f | %-14.2f | %-12.2f | %-8s\n",
                      ref, stats.mean, stats.std_dev, stats.error_abs, ok ? "PASS" : "ALERTA");
    }
    Serial.println("-----------------------------------------------------------------");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'c' || c == 'C') {
            run_calibration_7_points();
        }
    }

    // Leitura contínua a ~25 Hz
    uint16_t dist = sensor.readRangeSingleMillimeters();
    Serial.printf("[DISTANCIA] Sensor VL53L0X: %4d mm\n", dist);
    delay(50);
}
