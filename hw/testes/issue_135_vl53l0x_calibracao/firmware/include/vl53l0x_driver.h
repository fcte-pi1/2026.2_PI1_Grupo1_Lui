#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// MAPEAMENTO DE PORTAS E PINOS - SENSOR VL53L0X (1 SENSOR)
// ==============================================================================
#define PIN_I2C_SDA          21      // Pino de Dados I2C
#define PIN_I2C_SCL          22      // Pino de Relógio I2C
#define PIN_XSHUT            32      // Pino de Habilitação XSHUT (GPIO 32)
#define SENSOR_I2C_ADDR      0x29    // Endereço I2C padrão do VL53L0X

// Requisitos Operacionais (RNF-ELE01 e RNF-ELE02)
#define SENSOR_MIN_RANGE_MM  30.0f   // Faixa útil mínima (30 mm)
#define SENSOR_MAX_RANGE_MM  180.0f  // Faixa útil máxima (180 mm)
#define SENSOR_MAX_ERROR_MM  10.0f   // Erro tolerado (<= 10 mm)
#define SENSOR_MIN_RATE_HZ   20.0f   // Taxa mínima de amostragem (>= 20 Hz)

typedef struct {
    float slope;       // Fator de escala (ideal: 1.0)
    float offset;      // Deslocamento de calibração em mm
} SensorCalib_t;

typedef struct {
    uint16_t count;
    float mean;
    float std_dev;
    float error_abs;
} SensorStats_t;

/**
 * @brief Inicializa a calibração com valores padrão (sem correção inicial).
 */
static inline void sensor_init_calib(SensorCalib_t* calib) {
    if (calib) {
        calib->slope = 1.0f;
        calib->offset = 0.0f;
    }
}

/**
 * @brief Aplica correção linear à leitura bruta.
 */
static inline float sensor_apply_calib(float raw_mm, const SensorCalib_t* calib) {
    if (!calib) return raw_mm;
    float val = (raw_mm * calib->slope) + calib->offset;
    return (val < 0.0f) ? 0.0f : val;
}

/**
 * @brief Calcula média e desvio padrão para N amostras.
 */
static inline void sensor_calc_stats(const float* data, uint16_t n, float ref_mm, SensorStats_t* stats) {
    if (!data || !stats || n == 0) return;
    stats->count = n;
    float sum = 0.0f;
    for (uint16_t i = 0; i < n; i++) sum += data[i];
    stats->mean = sum / (float)n;

    float sum_sq = 0.0f;
    for (uint16_t i = 0; i < n; i++) {
        float diff = data[i] - stats->mean;
        sum_sq += diff * diff;
    }
    stats->std_dev = (n > 1) ? sqrtf(sum_sq / (float)(n - 1)) : 0.0f;
    stats->error_abs = fabsf(stats->mean - ref_mm);
}

/**
 * @brief Verifica se o erro está dentro da tolerância do requisito RNF-ELE02 (<= 10 mm).
 */
static inline bool sensor_is_error_acceptable(float error_abs) {
    return (error_abs <= SENSOR_MAX_ERROR_MM);
}

#ifdef __cplusplus
}
#endif
