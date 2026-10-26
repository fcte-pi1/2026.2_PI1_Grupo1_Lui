#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// MAPEAMENTO DE PINOS - SHIELD REV 2.0 (TB6612FNG + ENCODERS QUADRATURA)
// ==============================================================================
#define PIN_PWMA       16  // Modulacao PWM Motor Esquerdo (LEDC canal 0)
#define PIN_PWMB       17  // Modulacao PWM Motor Direito  (LEDC canal 1)
#define PIN_AIN1       18  // Sentido A Motor Esquerdo
#define PIN_AIN2       19  // Sentido B Motor Esquerdo
#define PIN_BIN1       23  // Sentido A Motor Direito
#define PIN_BIN2        5  // Sentido B Motor Direito

#define PIN_ENC_L_A    27  // Canal A Encoder Esquerdo (Interrupcao)
#define PIN_ENC_L_B    26  // Canal B Encoder Esquerdo
#define PIN_ENC_R_A    25  // Canal A Encoder Direito  (Interrupcao)
#define PIN_ENC_R_B    33  // Canal B Encoder Direito

// ==============================================================================
// PARAMETROS MECANICOS DO MOTOR N20 COM ENCODER MAGNETICO
// ==============================================================================
#define ENCODER_TICKS_PER_REV  360    // 12 CPR * 30:1 de reducao = 360 ticks/volta
#define WHEEL_DIAMETER_MM      32.0f  // Diametro da roda em mm
#define WHEEL_CIRCUMFERENCE_MM (WHEEL_DIAMETER_MM * 3.14159265f) // ~100.53 mm

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD,
    MOTOR_BRAKE
} MotorState_t;

typedef struct {
    int32_t ticks;
    int8_t last_channel_a;
    int8_t last_channel_b;
    float current_rpm;
} EncoderData_t;

typedef struct {
    MotorState_t state;
    uint8_t duty_cycle_percent; // 0 a 100%
    uint16_t raw_pwm;           // 0 a 255 (ou 1023 em 10 bits)
} MotorControl_t;

// ==============================================================================
// FUNCOES DO CONTROLADOR DE MOTORES E ENCODERS
// ==============================================================================

/**
 * @brief Converte porcentagem de PWM (0-100%) em valor de registrador (0-255).
 */
static inline uint16_t motor_percent_to_pwm(uint8_t percent) {
    if (percent > 100) percent = 100;
    return (uint16_t)((percent * 255) / 100);
}

/**
 * @brief Obtem os niveis logicos para os pinos IN1 e IN2 da ponte H TB6612FNG.
 * - FORWARD:  IN1=HIGH, IN2=LOW
 * - BACKWARD: IN1=LOW,  IN2=HIGH
 * - BRAKE:    IN1=HIGH, IN2=HIGH (Frenagem ativa por curto-circuito dinamico)
 * - STOP:     IN1=LOW,  IN2=LOW  (Roda livre / Alta impedancia)
 */
static inline void tb6612_get_logic_levels(MotorState_t state, bool* in1, bool* in2) {
    switch (state) {
        case MOTOR_FORWARD:
            *in1 = true;
            *in2 = false;
            break;
        case MOTOR_BACKWARD:
            *in1 = false;
            *in2 = true;
            break;
        case MOTOR_BRAKE:
            *in1 = true;
            *in2 = true;
            break;
        case MOTOR_STOP:
        default:
            *in1 = false;
            *in2 = false;
            break;
    }
}

/**
 * @brief Decodificador de quadratura acionado pela borda do Canal A.
 * Se Canal B estiver LOW no momento da borda de subida do Canal A -> Horario (+1).
 * Se Canal B estiver HIGH -> Anti-horario (-1).
 */
static inline int32_t encoder_decode_step(bool channel_a, bool channel_b) {
    (void)channel_a;
    return channel_b ? -1 : 1;
}

/**
 * @brief Calcula o deslocamento linear em milimetros a partir dos ticks contados.
 */
static inline float encoder_ticks_to_distance_mm(int32_t ticks) {
    return ((float)ticks / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM;
}

/**
 * @brief Calcula a consistencia de ticks para N voltas completas esperadas.
 */
static inline bool encoder_verify_10_turns(int32_t measured_ticks, float tolerance_percent) {
    int32_t expected = 10 * ENCODER_TICKS_PER_REV;
    int32_t diff = labs(measured_ticks - expected);
    float max_error = (float)expected * (tolerance_percent / 100.0f);
    return ((float)diff <= max_error);
}

#ifdef __cplusplus
}
#endif
