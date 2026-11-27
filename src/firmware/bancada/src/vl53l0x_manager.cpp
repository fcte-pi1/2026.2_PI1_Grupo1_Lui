#include "vl53l0x_manager.h"
#include "pinout.h"

static int16_t s_offsets_mm[3] = {
    TOF_OFFSET_ESQ_MM,
    TOF_OFFSET_FRONTAL_MM,
    TOF_OFFSET_DIR_MM
};

void vl53l0x_manager_set_offsets(int16_t left_mm, int16_t front_mm, int16_t right_mm) {
    s_offsets_mm[0] = left_mm;
    s_offsets_mm[1] = front_mm;
    s_offsets_mm[2] = right_mm;
}

int16_t vl53l0x_calcular_offset_mm(uint32_t soma_bruta_mm, uint8_t amostras, uint16_t dist_real_mm) {
    if (amostras == 0) return 0;
    int32_t media = (int32_t)((soma_bruta_mm + (uint32_t)(amostras / 2)) / (uint32_t)amostras);
    return (int16_t)(media - (int32_t)dist_real_mm);
}

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

typedef struct {
    VL53L0X sensor;
    uint8_t xshut_pin;
    uint8_t i2c_addr;
    bool ready;
    uint16_t last_valid_mm;
} TofCanal;

static const uint8_t NUM_TOF = 3;
static TofCanal s_canais[NUM_TOF] = {
    { VL53L0X(), PIN_XSHUT_1, ADDR_VL53_S1, false, 8190 },
    { VL53L0X(), PIN_XSHUT_2, ADDR_VL53_S2, false, 8190 },
    { VL53L0X(), PIN_XSHUT_3, ADDR_VL53_S3, false, 8190 },
};

static void recover_i2c_bus(void) {
    pinMode(PIN_I2C_SDA, INPUT_PULLUP);
    pinMode(PIN_I2C_SCL, OUTPUT);
    digitalWrite(PIN_I2C_SCL, HIGH);
    delayMicroseconds(10);
    for (uint8_t i = 0; i < 9 && digitalRead(PIN_I2C_SDA) == LOW; i++) {
        digitalWrite(PIN_I2C_SCL, LOW);
        delayMicroseconds(10);
        digitalWrite(PIN_I2C_SCL, HIGH);
        delayMicroseconds(10);
    }
    pinMode(PIN_I2C_SCL, INPUT_PULLUP);
}

static bool i2c_addr_responds(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

static bool configure_vl53_instance(VL53L0X& sensor, uint8_t addr) {
    for (uint8_t attempt = 0; attempt < 2; attempt++) {
        sensor.setBus(&Wire);
        sensor.setTimeout(100);
        if (sensor.init()) {
            sensor.setAddress(addr);
            sensor.setMeasurementTimingBudget(33000);
            sensor.startContinuous();
            return true;
        }
        delay(15);
    }
    return false;
}

static uint16_t apply_offset_mm(uint16_t raw_mm, int16_t offset_mm) {
    int32_t calibrated = (int32_t)raw_mm - (int32_t)offset_mm;
    return (calibrated < 1) ? 1 : (uint16_t)calibrated;
}

static void read_single_sensor(TofCanal& c, int16_t offset_mm,
                               uint16_t* out_dist, uint16_t* out_raw, bool* out_valid) {
    *out_dist = 8190;
    *out_raw = 8190;
    *out_valid = false;
    if (!c.ready) return;

    uint16_t raw = c.sensor.readRangeContinuousMillimeters();
    *out_raw = raw;
    if (c.sensor.timeoutOccurred()) {
        return;
    }

    if (raw >= 8000) {
        // Corredor aberto / sem parede ao alcance: nunca travar na leitura anterior!
        *out_dist = 8190;
        *out_valid = true;
        c.last_valid_mm = 8190;
    } else if (raw <= 15) {
        // Objeto encostado no sensor (min-range): trata como parede colada, nunca aberto.
        *out_dist = DIST_MINIMA_SEGURA_MM;
        *out_valid = true;
        c.last_valid_mm = DIST_MINIMA_SEGURA_MM;
    } else {
        uint16_t cal = apply_offset_mm(raw, offset_mm);
        *out_dist = cal;
        *out_valid = true;
        c.last_valid_mm = cal;
    }
}

bool vl53l0x_manager_init(void) {
    for (uint8_t i = 0; i < NUM_TOF; i++) {
        s_canais[i].ready = false;
        pinMode(s_canais[i].xshut_pin, OUTPUT);
        digitalWrite(s_canais[i].xshut_pin, LOW);
    }
    delay(25);

    recover_i2c_bus();
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);
    Wire.setTimeOut(20);
    delay(5);

    VL53L0X always_on_sensor;
    bool had_always_on = false;
    if (i2c_addr_responds(0x29)) {
        Serial.print("\n  [AVISO] Sensor respondeu em 0x29 com XSHUTs em LOW (XSHUT solto); movendo p/ 0x3F...");
        had_always_on = configure_vl53_instance(always_on_sensor, 0x3F);
    }

    int8_t missing_idx = -1;
    uint8_t missing_count = 0;
    for (uint8_t i = 0; i < NUM_TOF; i++) {
        digitalWrite(s_canais[i].xshut_pin, HIGH);
        delay(20);
        bool ok = i2c_addr_responds(0x29) && configure_vl53_instance(s_canais[i].sensor, s_canais[i].i2c_addr);
        s_canais[i].ready = ok;
        Serial.printf("\n  [ToF %u] XSHUT=GPIO%u addr=0x%02X -> %s",
                      (unsigned)(i + 1),
                      (unsigned)s_canais[i].xshut_pin,
                      (unsigned)s_canais[i].i2c_addr,
                      ok ? "OK" : "FALHA");
        if (!ok) {
            missing_idx = (int8_t)i;
            missing_count++;
        }
    }

    if (had_always_on && missing_count == 1 && missing_idx >= 0) {
        always_on_sensor.setAddress(s_canais[missing_idx].i2c_addr);
        if (i2c_addr_responds(s_canais[missing_idx].i2c_addr)) {
            s_canais[missing_idx].sensor = always_on_sensor;
            s_canais[missing_idx].ready = true;
            Serial.printf("\n  [RECUPERADO] Sensor sem XSHUT adotado como ToF %d (0x%02X)!",
                          (int)(missing_idx + 1), (unsigned)s_canais[missing_idx].i2c_addr);
        }
    }
    Serial.println();
    return s_canais[0].ready && s_canais[1].ready && s_canais[2].ready;
}

VL53L0X_Readings_t vl53l0x_manager_read(void) {
    VL53L0X_Readings_t r;
    read_single_sensor(s_canais[0], s_offsets_mm[0], &r.dist_left_mm, &r.raw_left_mm, &r.left_valid);
    read_single_sensor(s_canais[1], s_offsets_mm[1], &r.dist_front_mm, &r.raw_front_mm, &r.front_valid);
    read_single_sensor(s_canais[2], s_offsets_mm[2], &r.dist_right_mm, &r.raw_right_mm, &r.right_valid);

    r.wall_left = (r.left_valid && r.dist_left_mm <= DIST_PAREDE_DETECTADA_MM);
    r.wall_front = (r.front_valid && r.dist_front_mm <= DIST_PAREDE_DETECTADA_MM);
    r.wall_right = (r.right_valid && r.dist_right_mm <= DIST_PAREDE_DETECTADA_MM);
    return r;
}

#else
bool vl53l0x_manager_init(void) { return true; }
VL53L0X_Readings_t vl53l0x_manager_read(void) {
    VL53L0X_Readings_t r = { 75, 150, 75, 75, 150, 75, true, false, true, true, true, true };
    return r;
}
#endif
