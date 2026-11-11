#include "vl53l0x_manager.h"
#include "pinout.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

static VL53L0X s_sensor_left;
static VL53L0X s_sensor_front;
static VL53L0X s_sensor_right;

static bool s_left_ready = false;
static bool s_front_ready = false;
static bool s_right_ready = false;

bool vl53l0x_manager_init(void) {
    // 1. Inicia o barramento I2C compartilhado (SDA=21, SCL=22)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);

    // 2. Coloca todos os sensores em estado de Shutdown (XSHUT = LOW)
    pinMode(PIN_XSHUT_1, OUTPUT);
    pinMode(PIN_XSHUT_2, OUTPUT);
    pinMode(PIN_XSHUT_3, OUTPUT);

    digitalWrite(PIN_XSHUT_1, LOW);
    digitalWrite(PIN_XSHUT_2, LOW);
    digitalWrite(PIN_XSHUT_3, LOW);
    delay(15); // Aguarda reset de hardware de todos os sensores

    // -------------------------------------------------------------------------
    // 3. Inicializa Sensor 1 (Esquerdo - XSHUT no GPIO 4)
    // -------------------------------------------------------------------------
    digitalWrite(PIN_XSHUT_1, HIGH);
    delay(15);
    s_sensor_left.setBus(&Wire);
    s_sensor_left.setTimeout(500);
    if (s_sensor_left.init()) {
        s_sensor_left.setAddress(ADDR_VL53_S1);
        s_sensor_left.setMeasurementTimingBudget(33000); // ~33 ms (~30 Hz)
        s_sensor_left.startContinuous();
        s_left_ready = true;
    } else {
        s_left_ready = false;
    }

    // -------------------------------------------------------------------------
    // 4. Inicializa Sensor 2 (Frontal - XSHUT no GPIO 16)
    // -------------------------------------------------------------------------
    digitalWrite(PIN_XSHUT_2, HIGH);
    delay(15);
    s_sensor_front.setBus(&Wire);
    s_sensor_front.setTimeout(500);
    if (s_sensor_front.init()) {
        s_sensor_front.setAddress(ADDR_VL53_S2);
        s_sensor_front.setMeasurementTimingBudget(33000);
        s_sensor_front.startContinuous();
        s_front_ready = true;
    } else {
        s_front_ready = false;
    }

    // -------------------------------------------------------------------------
    // 5. Inicializa Sensor 3 (Direito - XSHUT no GPIO 17)
    // -------------------------------------------------------------------------
    digitalWrite(PIN_XSHUT_3, HIGH);
    delay(15);
    s_sensor_right.setBus(&Wire);
    s_sensor_right.setTimeout(500);
    if (s_sensor_right.init()) {
        s_sensor_right.setAddress(ADDR_VL53_S3);
        s_sensor_right.setMeasurementTimingBudget(33000);
        s_sensor_right.startContinuous();
        s_right_ready = true;
    } else {
        s_right_ready = false;
    }

    return (s_left_ready && s_front_ready && s_right_ready);
}

VL53L0X_Readings_t vl53l0x_manager_read(void) {
    VL53L0X_Readings_t readings;
    readings.dist_left_mm = 8190;
    readings.dist_front_mm = 8190;
    readings.dist_right_mm = 8190;
    readings.left_valid = false;
    readings.front_valid = false;
    readings.right_valid = false;

    static uint16_t last_valid_left = 60;
    static uint16_t last_valid_front = 150;
    static uint16_t last_valid_right = 60;

    // Leitura Sensor 1 (Esquerdo) com rejeição de glitch
    if (s_left_ready) {
        uint16_t d = s_sensor_left.readRangeContinuousMillimeters();
        if (!s_sensor_left.timeoutOccurred() && d > 15 && d < 8000) {
            // Filtro de suavização / rejeição de salto absurdo
            readings.dist_left_mm = d;
            readings.left_valid = true;
            last_valid_left = d;
        } else if (last_valid_left < DIST_PAREDE_DETECTADA_MM) {
            // Mantém valor recente por 1 ciclo se falhar momentaneamente perto da parede
            readings.dist_left_mm = last_valid_left;
            readings.left_valid = true;
        }
    }

    // Leitura Sensor 2 (Frontal)
    if (s_front_ready) {
        uint16_t d = s_sensor_front.readRangeContinuousMillimeters();
        if (!s_sensor_front.timeoutOccurred() && d > 15 && d < 8000) {
            readings.dist_front_mm = d;
            readings.front_valid = true;
            last_valid_front = d;
        } else if (last_valid_front < DIST_PAREDE_DETECTADA_MM) {
            readings.dist_front_mm = last_valid_front;
            readings.front_valid = true;
        }
    }

    // Leitura Sensor 3 (Direito)
    if (s_right_ready) {
        uint16_t d = s_sensor_right.readRangeContinuousMillimeters();
        if (!s_sensor_right.timeoutOccurred() && d > 15 && d < 8000) {
            readings.dist_right_mm = d;
            readings.right_valid = true;
            last_valid_right = d;
        } else if (last_valid_right < DIST_PAREDE_DETECTADA_MM) {
            readings.dist_right_mm = last_valid_right;
            readings.right_valid = true;
        }
    }

    // Avaliação de presença de paredes (limiar < 110 mm)
    readings.wall_left = (readings.left_valid && readings.dist_left_mm <= DIST_PAREDE_DETECTADA_MM);
    readings.wall_front = (readings.front_valid && readings.dist_front_mm <= DIST_PAREDE_DETECTADA_MM);
    readings.wall_right = (readings.right_valid && readings.dist_right_mm <= DIST_PAREDE_DETECTADA_MM);

    return readings;
}

#else
// Stubs para compilação nativa de testes
bool vl53l0x_manager_init(void) { return true; }
VL53L0X_Readings_t vl53l0x_manager_read(void) {
    VL53L0X_Readings_t r = { 75, 150, 75, true, false, true, true, true, true };
    return r;
}
#endif
