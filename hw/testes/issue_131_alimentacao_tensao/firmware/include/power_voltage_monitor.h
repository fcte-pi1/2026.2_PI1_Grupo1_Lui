#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// MAPEAMENTO E LIMITES DE TENSAO - SISTEMA ELETRICO E ENERGETICO (EAP 4.1.2)
// ==============================================================================
#define PIN_ADC_BATTERY         34     // GPIO 34 (ADC1_CH6 - Divisor de tensao)
#define DIVIDER_R1_OHMS         100000 // Resistor R1 = 100k
#define DIVIDER_R2_OHMS          47000 // Resistor R2 = 47k
#define DIVIDER_RATIO           ((float)(DIVIDER_R1_OHMS + DIVIDER_R2_OHMS) / (float)DIVIDER_R2_OHMS) // ~3.1276

// Tolerancias Estritas RNF-ENE02 (+/- 5%)
#define V_3V3_NOMINAL           3.300f
#define V_3V3_MIN               3.135f // -5%
#define V_3V3_MAX               3.465f // +5%

#define V_5V0_NOMINAL           5.000f
#define V_5V0_MIN               4.750f // -5%
#define V_5V0_MAX               5.250f // +5%

// Limites Bateria LiPo 2S
#define BATTERY_FULL_V          8.40f
#define BATTERY_NOMINAL_V       7.40f
#define BATTERY_CUTOFF_V        6.60f  // 3.3V por celula (Protecao contra descarga profunda)
#define BATTERY_WARN_V          7.00f  // 3.5V por celula (Alerta amarelo)

typedef struct {
    float v_3v3;
    float v_5v0;
    float v_bat;
    bool  is_3v3_stable;
    bool  is_5v0_stable;
    bool  is_low_battery;
    bool  is_critical_cutoff;
} VoltageStatus_t;

/**
 * @brief Converte leitura bruta do ADC (0-4095) em volts reais da bateria.
 */
static inline float adc_to_battery_voltage(uint16_t adc_raw, float vref) {
    float v_adc = ((float)adc_raw / 4095.0f) * vref;
    return v_adc * DIVIDER_RATIO;
}

/**
 * @brief Avalia a estabilidade das linhas de alimentacao e seguranca da bateria.
 */
static inline VoltageStatus_t evaluate_power_rails(float v_3v3, float v_5v0, float v_bat) {
    VoltageStatus_t st;
    st.v_3v3 = v_3v3;
    st.v_5v0 = v_5v0;
    st.v_bat = v_bat;

    st.is_3v3_stable = (v_3v3 >= V_3V3_MIN && v_3v3 <= V_3V3_MAX);
    st.is_5v0_stable = (v_5v0 >= V_5V0_MIN && v_5v0 <= V_5V0_MAX);
    st.is_low_battery = (v_bat <= BATTERY_WARN_V);
    st.is_critical_cutoff = (v_bat <= BATTERY_CUTOFF_V);

    return st;
}

/**
 * @brief Calcula a porcentagem aproximada de carga util da bateria LiPo 2S (6.6V a 8.4V).
 */
static inline float battery_calc_percentage(float v_bat) {
    if (v_bat >= BATTERY_FULL_V) return 100.0f;
    if (v_bat <= BATTERY_CUTOFF_V) return 0.0f;
    return ((v_bat - BATTERY_CUTOFF_V) / (BATTERY_FULL_V - BATTERY_CUTOFF_V)) * 100.0f;
}

#ifdef __cplusplus
}
#endif
