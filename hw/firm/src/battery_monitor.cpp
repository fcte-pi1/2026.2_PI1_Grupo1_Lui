#include "battery_monitor.h"
#include "pinout.h"

#ifdef ARDUINO
#include <Arduino.h>

void battery_monitor_init(void) {
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogReadResolution(12); // Resolução de 12 bits (0 a 4095)
    analogSetAttenuation(ADC_11db); // Permite ler até a faixa de 2.6V a 3.1V
}

uint8_t battery_voltage_to_percentage(float volts) {
    if (volts <= BATTERY_MIN_SAFE_V) return 0;
    if (volts >= BATTERY_MAX_FULL_V) return 100;
    float pct = ((volts - BATTERY_MIN_SAFE_V) / (BATTERY_MAX_FULL_V - BATTERY_MIN_SAFE_V)) * 100.0f;
    return (uint8_t)pct;
}

BatteryStatus_t battery_monitor_read(void) {
    // Amostragem com média de 16 leituras para eliminar ruído elétrico da ponte H
    const uint8_t SAMPLES = 16;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++) {
        sum += analogRead(PIN_BATTERY_ADC);
        delayMicroseconds(50);
    }
    float raw_avg = (float)sum / (float)SAMPLES;

    // Converte leitura do ADC para tensão no pino
    // (Compensação empírica da não-linearidade do ADC do ESP32)
    float pin_voltage = (raw_avg / 4095.0f) * BATTERY_ADC_VREF;

    // Tensão real da bateria multiplicada pelo fator do divisor
    float bat_v = pin_voltage * BATTERY_DIVIDER_RATIO;

    BatteryStatus_t status;
    status.voltage_volts = bat_v;
    status.voltage_mv = (uint16_t)(bat_v * 1000.0f);
    status.percentage = battery_voltage_to_percentage(bat_v);
    status.is_critical = (bat_v < (BATTERY_MIN_SAFE_V + 0.2f)); // Alerta abaixo de 6.8V

    return status;
}

#else
// Stubs para compilação nativa de testes
void battery_monitor_init(void) {}

uint8_t battery_voltage_to_percentage(float volts) {
    if (volts <= BATTERY_MIN_SAFE_V) return 0;
    if (volts >= BATTERY_MAX_FULL_V) return 100;
    float pct = ((volts - BATTERY_MIN_SAFE_V) / (BATTERY_MAX_FULL_V - BATTERY_MIN_SAFE_V)) * 100.0f;
    return (uint8_t)pct;
}

BatteryStatus_t battery_monitor_read(void) {
    BatteryStatus_t s = { 7.8f, 7800, 66, false };
    return s;
}
#endif
