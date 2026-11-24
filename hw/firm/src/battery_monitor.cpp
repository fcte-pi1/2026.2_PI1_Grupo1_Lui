#include "battery_monitor.h"
#include "pinout.h"

static float s_calibration_factor = BATTERY_CALIBRATION_FACTOR;

void battery_monitor_set_factor(float factor) {
    if (factor > 0.2f && factor < 5.0f) {
        s_calibration_factor = factor;
    }
}

float battery_monitor_get_factor(void) {
    return s_calibration_factor;
}

uint8_t battery_voltage_to_percentage(float volts) {
    if (volts <= BATTERY_MIN_SAFE_V) return 0;
    if (volts >= BATTERY_MAX_FULL_V) return 100;
    float pct = ((volts - BATTERY_MIN_SAFE_V) / (BATTERY_MAX_FULL_V - BATTERY_MIN_SAFE_V)) * 100.0f;
    return (uint8_t)pct;
}

#ifdef ARDUINO
#include <Arduino.h>

void battery_monitor_init(void) {
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);
}

BatteryStatus_t battery_monitor_read(void) {
    // analogReadMilliVolts aplica a calibracao eFuse de fabrica do ESP32.
    const uint8_t SAMPLES = 16;
    uint32_t mv_sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++) {
        mv_sum += analogReadMilliVolts(PIN_BATTERY_ADC);
        delayMicroseconds(50);
    }
    float pin_mv = (float)mv_sum / (float)SAMPLES;
    float bat_mv = pin_mv * BATTERY_DIVIDER_RATIO * s_calibration_factor;
    float bat_v = bat_mv / 1000.0f;

    BatteryStatus_t status;
    status.voltage_volts = bat_v;
    status.voltage_mv = (uint16_t)(bat_mv + 0.5f);
    status.percentage = battery_voltage_to_percentage(bat_v);
    status.is_critical = (bat_v < (BATTERY_MIN_SAFE_V + 0.2f));
    return status;
}

#else
void battery_monitor_init(void) {}

BatteryStatus_t battery_monitor_read(void) {
    BatteryStatus_t s = { 7.8f, 7800, 66, false };
    return s;
}
#endif
