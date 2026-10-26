#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_CAPACITY_MAH     850    // Capacidade nominal da LiPo 2S
#define BATTERY_USEFUL_FACTOR    0.85f  // Margem de seguranca de descarga profunda (85%)
#define MIN_REQUIRED_AUTONOMY_M  15.0f  // Requisito RNF-ENE01: >= 15 minutos
#define FUSE_MAX_TRIP_TIME_MS    500    // Requisito RNF-ENE03: < 500 ms sob curto-circuito

// Consumos Correntes Tipicos Medidos
#define CURRENT_IDLE_MA          130    // ESP32 ativo + sensores I2C
#define CURRENT_NOMINAL_RUN_MA   420    // Motores a 50% PWM + ToF + ESP-NOW
#define CURRENT_STALL_MA        1400    // Rodas travadas (condicao de pico de corrente)

/**
 * @brief Calcula a autonomia em minutos com base na capacidade da bateria e corrente media.
 */
static inline float power_calc_autonomy_minutes(uint16_t capacity_mah, float avg_current_ma) {
    if (avg_current_ma <= 0.0f) return 0.0f;
    float useful_mah = (float)capacity_mah * BATTERY_USEFUL_FACTOR;
    return (useful_mah / avg_current_ma) * 60.0f;
}

/**
 * @brief Verifica o cumprimento do requisito RNF-ENE01 (autonomia >= 15 minutos).
 */
static inline bool power_is_autonomy_sufficient(float calculated_minutes) {
    return (calculated_minutes >= MIN_REQUIRED_AUTONOMY_M);
}

/**
 * @brief Valida se o tempo de resposta do fusivel atende ao requisito RNF-ENE03 (< 500 ms).
 */
static inline bool power_is_protection_trip_valid(uint32_t trip_time_ms) {
    return (trip_time_ms < FUSE_MAX_TRIP_TIME_MS);
}

#ifdef __cplusplus
}
#endif
