#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float voltage_volts;     // Tensão da bateria em Volts (ex: 7.82 V)
    uint16_t voltage_mv;     // Tensão da bateria em Milivolts (ex: 7820 mV)
    uint8_t percentage;      // Carga da bateria em porcentagem (0 a 100%)
    bool is_critical;        // Flag de bateria baixa (< 6.8 V)
} BatteryStatus_t;

/**
 * @brief Configura o conversor analógico-digital (ADC1_CH6 no GPIO 34).
 */
void battery_monitor_init(void);

/**
 * @brief Efetua amostragem com filtro de média móvel para leitura estável.
 * @return Estrutura com tensão, porcentagem e alarme de bateria crítica.
 */
BatteryStatus_t battery_monitor_read(void);

/**
 * @brief Converte tensão em Volts para porcentagem de carga de bateria LiPo 2S (6.6V a 8.4V).
 */
uint8_t battery_voltage_to_percentage(float volts);

#ifdef __cplusplus
}
#endif
