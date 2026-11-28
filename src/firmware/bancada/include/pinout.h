#pragma once

#include <stdint.h>
#include "PinosRobo.h"
#include "CalibracaoRobo.h"

// Adaptador dos nomes usados pelos drivers de bancada para as fontes unicas
// em src/firmware/lib/calibracao/src/ (PinosRobo.h e CalibracaoRobo.h).

// ==============================================================================
// 1. PONTE H TB6612FNG (MOTORES DC N20)
// ==============================================================================
#define PIN_PWMA         PINO_PONTE_PWMA
#define PIN_AIN1         PINO_PONTE_AIN1
#define PIN_AIN2         PINO_PONTE_AIN2
#define PIN_STBY         PINO_PONTE_STBY
#define PIN_BIN1         PINO_PONTE_BIN1
#define PIN_BIN2         PINO_PONTE_BIN2
#define PIN_PWMB         PINO_PONTE_PWMB

#define PWM_FREQ_HZ      20000
#define PWM_RESOLUTION   8
#define LEDC_CH_PWMA     0
#define LEDC_CH_PWMB     1

// ==============================================================================
// 2. ENCODERS EM QUADRATURA E CINEMATICA DO MICROMOUSE
// ==============================================================================
#define PIN_ENC1_A       PINO_ENCODER_ESQ_A
#define PIN_ENC1_B       PINO_ENCODER_ESQ_B
#define PIN_ENC2_A       PINO_ENCODER_DIR_A
#define PIN_ENC2_B       PINO_ENCODER_DIR_B

#define ENCODER_TICKS_PER_REV         CAL_ENCODER_PULSOS_POR_VOLTA
#define WHEEL_DIAMETER_MM             CAL_RODA_DIAMETRO_MM
#define WHEEL_CIRCUMFERENCE_MM        (3.14159265f * WHEEL_DIAMETER_MM)
#define TRACK_WIDTH_MM                CAL_BITOLA_MM
#define ODOM_ESCALA_RETO              CAL_ODOM_ESCALA_RETO
#define CELL_SIZE_MM                  ((float)CAL_CELULA_MM)
#define PWM_BASE_MOVIMENTO            ((int16_t)CAL_PWM_BASE_MOVIMENTO)
#define CURVA_ANTECIPACAO_FREIO_GRAUS CAL_CURVA_ANTECIPACAO_FREIO_GRAUS

#define MOTOR_ESQ_INVERTIDO           CAL_MOTOR_ESQ_INVERTIDO
#define MOTOR_DIR_INVERTIDO           CAL_MOTOR_DIR_INVERTIDO
#define ENCODER_ESQ_INVERTIDO         CAL_ENCODER_ESQ_INVERTIDO
#define ENCODER_DIR_INVERTIDO         CAL_ENCODER_DIR_INVERTIDO

// ==============================================================================
// 3. SENSORES DE DISTANCIA (3x VL53L0X: Esq, Frontal, Dir)
// ==============================================================================
#define PIN_I2C_SDA      PINO_I2C_SDA
#define PIN_I2C_SCL      PINO_I2C_SCL
#define I2C_CLOCK_SPEED  100000

#define PIN_XSHUT_1      PINO_TOF_XSHUT_ESQ
#define ADDR_VL53_S1     TOF_ENDERECO_ESQ
#define PIN_XSHUT_2      PINO_TOF_XSHUT_FRONTAL
#define ADDR_VL53_S2     TOF_ENDERECO_FRONTAL
#define PIN_XSHUT_3      PINO_TOF_XSHUT_DIR
#define ADDR_VL53_S3     TOF_ENDERECO_DIR

#define DIST_PAREDE_DETECTADA_MM     CAL_LIMIAR_PAREDE_MM
#define DIST_MINIMA_SEGURA_MM        CAL_DIST_MINIMA_SEGURA_MM
#define DIST_ENCOSTO_FRONTAL_MM      CAL_DIST_ENCOSTO_FRONTAL_MM
#define DIST_IDEAL_PAREDE_LATERAL_MM CAL_DIST_IDEAL_PAREDE_LATERAL_MM
#define DIST_MAXIMA_UTIL_MM          CAL_CELULA_MM
#define TOF_OFFSET_ESQ_MM            CAL_TOF_OFFSET_ESQ_MM
#define TOF_OFFSET_FRONTAL_MM        CAL_TOF_OFFSET_FRONTAL_MM
#define TOF_OFFSET_DIR_MM            CAL_TOF_OFFSET_DIR_MM

// ==============================================================================
// 4. MONITORAMENTO DE BATERIA 2S (DIVISOR RESISTIVO)
// ==============================================================================
#define PIN_BATTERY_ADC  PINO_BATERIA_ADC
#define BATTERY_DIVIDER_RATIO ((float)(CAL_BATERIA_R_SERIE_OHM + CAL_BATERIA_R_GND_OHM) / (float)CAL_BATERIA_R_GND_OHM)
#define BATTERY_CALIBRATION_FACTOR CAL_BATERIA_FATOR_AJUSTE
#define BATTERY_ADC_VREF      3.3f
#define BATTERY_MIN_SAFE_V    6.6f
#define BATTERY_MAX_FULL_V    8.4f

// ==============================================================================
// 5. INTERFACE DO USUARIO
// ==============================================================================
// Sem botao de largada nem LED de estado ligados a GPIOs: a chave fisica e
// apenas liga/desliga geral (largada via software/ESP-NOW) e o LED de estado
// esta ligado diretamente em 3.3 V. GPIOs 14 e 15 ficam livres.
