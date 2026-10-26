#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Requisitos Eletroeletronicos Formais do Projeto
typedef struct {
    bool rf_ele01_sensors_ok;     // Sensoriamento de paredes (ToF Frontal/Laterais)
    bool rf_ele02_encoders_ok;    // Odometria em quadratura sem perda de passos
    bool rf_ele03_motors_ok;      // Ponte H TB6612 com PWM linear reversivel
    bool rf_ele04_telemetry_ok;   // Envio de telemetria sem fio a 1 Hz
    bool rf_ele05_visual_ok;      // LED RGB para os 5 estados operacionais
    bool rf_ele06_buzzer_ok;      // Buzzer >= 60 dB para bipes de eventos

    bool rnf_ele01_rate_ok;       // Taxa de amostragem sensorial >= 20 Hz
    bool rnf_ele02_range_ok;      // Faixa util 30-180 mm com erro <= 10 mm
    bool rnf_ele03_wireless_ok;   // Enlace sem fio estavel a 8 m com perda < 2%
    bool rnf_ele04_pcb_ok;        // Montagem modular com Shield e soquetes
    bool rnf_ele05_noise_ok;      // Imunidade a ruido indutivo de motores (zero resets)
} HardwareComplianceReport_t;

/**
 * @brief Avalia o laudo geral de conformidade de hardware.
 */
static inline bool hardware_is_fully_compliant(const HardwareComplianceReport_t* rep) {
    if (!rep) return false;
    return (rep->rf_ele01_sensors_ok &&
            rep->rf_ele02_encoders_ok &&
            rep->rf_ele03_motors_ok &&
            rep->rf_ele04_telemetry_ok &&
            rep->rf_ele05_visual_ok &&
            rep->rf_ele06_buzzer_ok &&
            rep->rnf_ele01_rate_ok &&
            rep->rnf_ele02_range_ok &&
            rep->rnf_ele03_wireless_ok &&
            rep->rnf_ele04_pcb_ok &&
            rep->rnf_ele05_noise_ok);
}

#ifdef __cplusplus
}
#endif
