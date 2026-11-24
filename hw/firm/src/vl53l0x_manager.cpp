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

#if CAL_MODO_SENSORES == 2
static const uint8_t NUM_TOF = 2;
static TofCanal s_canais[NUM_TOF] = {
    { VL53L0X(), PIN_XSHUT_1, ADDR_VL53_S1, false, 60 },
    { VL53L0X(), PIN_XSHUT_2, ADDR_VL53_S2, false, 150 },
};
#elif CAL_USA_ULTRASSONICO_FRONTAL
static const uint8_t NUM_TOF = 2;
static TofCanal s_canais[NUM_TOF] = {
    { VL53L0X(), PIN_XSHUT_1, ADDR_VL53_S1, false, 60 },
    { VL53L0X(), PIN_XSHUT_3, ADDR_VL53_S3, false, 60 },
};
#else
static const uint8_t NUM_TOF = 3;
static TofCanal s_canais[NUM_TOF] = {
    { VL53L0X(), PIN_XSHUT_1, ADDR_VL53_S1, false, 60 },
    { VL53L0X(), PIN_XSHUT_2, ADDR_VL53_S2, false, 150 },
    { VL53L0X(), PIN_XSHUT_3, ADDR_VL53_S3, false, 60 },
};
#endif

static bool init_single_sensor(TofCanal& c) {
    digitalWrite(c.xshut_pin, HIGH);
    delay(15);
    c.sensor.setBus(&Wire);
    c.sensor.setTimeout(500);
    if (c.sensor.init()) {
        c.sensor.setAddress(c.i2c_addr);
        c.sensor.setMeasurementTimingBudget(33000);
        c.sensor.startContinuous();
        c.ready = true;
    } else {
        c.ready = false;
    }
    return c.ready;
}

static uint16_t apply_offset_mm(uint16_t raw_mm, int16_t offset_mm) {
    int32_t calibrated = (int32_t)raw_mm - (int32_t)offset_mm;
    return (calibrated < 0) ? 0 : (uint16_t)calibrated;
}

static void read_single_sensor(TofCanal& c, int16_t offset_mm,
                               uint16_t* out_dist, uint16_t* out_raw, bool* out_valid) {
    *out_dist = 8190;
    *out_raw = 8190;
    *out_valid = false;
    if (!c.ready) return;

    uint16_t raw = c.sensor.readRangeContinuousMillimeters();
    *out_raw = raw;
    if (!c.sensor.timeoutOccurred() && raw > 15 && raw < 8000) {
        uint16_t cal = apply_offset_mm(raw, offset_mm);
        *out_dist = cal;
        *out_valid = true;
        c.last_valid_mm = cal;
    } else if (c.last_valid_mm < DIST_PAREDE_DETECTADA_MM) {
        *out_dist = c.last_valid_mm;
        *out_valid = true;
    }
}

bool vl53l0x_manager_init(void) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);

    for (uint8_t i = 0; i < NUM_TOF; i++) {
        pinMode(s_canais[i].xshut_pin, OUTPUT);
        digitalWrite(s_canais[i].xshut_pin, LOW);
    }
    delay(15);

    bool all_ok = true;
    for (uint8_t i = 0; i < NUM_TOF; i++) {
        bool ok = init_single_sensor(s_canais[i]);
        Serial.printf("\n  [ToF %u] XSHUT=GPIO%u addr=0x%02X -> %s",
                      (unsigned)(i + 1),
                      (unsigned)s_canais[i].xshut_pin,
                      (unsigned)s_canais[i].i2c_addr,
                      ok ? "OK" : "FALHA");
        if (!ok) {
            all_ok = false;
        }
    }
    Serial.println();
    return all_ok;
}

VL53L0X_Readings_t vl53l0x_manager_read(void) {
    VL53L0X_Readings_t r;
    read_single_sensor(s_canais[0], s_offsets_mm[0], &r.dist_left_mm, &r.raw_left_mm, &r.left_valid);
#if CAL_MODO_SENSORES == 2
    read_single_sensor(s_canais[1], s_offsets_mm[1], &r.dist_front_mm, &r.raw_front_mm, &r.front_valid);
    r.dist_right_mm = 8190;
    r.raw_right_mm = 8190;
    r.right_valid = false;
#elif CAL_USA_ULTRASSONICO_FRONTAL
    read_single_sensor(s_canais[1], s_offsets_mm[2], &r.dist_right_mm, &r.raw_right_mm, &r.right_valid);
    r.dist_front_mm = 8190;
    r.raw_front_mm = 8190;
    r.front_valid = false;
#else
    read_single_sensor(s_canais[1], s_offsets_mm[1], &r.dist_front_mm, &r.raw_front_mm, &r.front_valid);
    read_single_sensor(s_canais[2], s_offsets_mm[2], &r.dist_right_mm, &r.raw_right_mm, &r.right_valid);
#endif

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
