#pragma once

#include <stdint.h>
#include "CalibracaoRobo.h"

/**
 * ==============================================================================
 * MAPEAMENTO FINAL DE PORTAS E PINOS - MICROMOUSE ESP32-WROOM-32D (30 PINOS)
 * Grupo 1 - PI1 2026.2 (Prof. Lui - FCTE/UnB)
 * ==============================================================================
 * 
 * RESOLUÇÃO DE CONFLITOS DE HARDWARE:
 *  1. Conflito no GPIO 4 resolvido:
 *     - O GPIO 4 agora é dedicado EXCLUSIVAMENTE ao XSHUT do Sensor 1 (VL53L0X).
 *     - O Canal B do Encoder do Motor 2 foi transferido para o GPIO 2 (que possui
 *       pull-up interno configurável no ESP32).
 *  2. Pino 32 vs 4º Sensor:
 *     - O GPIO 32 está ATRIBUÍDO como BIN2 da Ponte H (Sentido do Motor 2).
 *     - Por ser saída de motor, NÃO PODE ser compartilhado com sensor ToF.
 *     - Caso o robô utilize um 4º sensor de distância, use o pino LIVRE GPIO 14!
 *  3. Barramento I2C limpo:
 *     - SDA no GPIO 21 e SCL no GPIO 22 compartilhados pelos 3 sensores.
 *     - Endereçamento dinâmico via XSHUT (0x30, 0x31, 0x32).
 * ==============================================================================
 */

// ==============================================================================
// 1. PONTE H TB6612FNG (MOTORES DC N20)
// ==============================================================================
#define PIN_PWMA         25  // Sinal PWM para velocidade do Motor 1 (Esquerdo)
#define PIN_AIN1         26  // Sentido de rotação A do Motor 1
#define PIN_AIN2         27  // Sentido de rotação B do Motor 1
#define PIN_STBY         13  // Standby (Habilita a ponte H em HIGH)
#define PIN_BIN1         33  // Sentido de rotação A do Motor 2
#define PIN_BIN2         32  // Sentido de rotação B do Motor 2 (Alocado para Motor 2!)
#define PIN_PWMB         18  // Sinal PWM para velocidade do Motor 2 (Direito)

// Configuração dos Canais PWM (LEDC do ESP32)
#define PWM_FREQ_HZ      20000 // 20 kHz (frequência inaudível, silenciosa e eficiente)
#define PWM_RESOLUTION   8     // Resolução de 8 bits (valores de 0 a 255)
#define LEDC_CH_PWMA     0     // Canal LEDC para o Motor 1 (Esquerdo)
#define LEDC_CH_PWMB     1     // Canal LEDC para o Motor 2 (Direito)

// ==============================================================================
// 2. ENCODERS EM QUADRATURA E CINEMÁTICA DO MICROMOUSE
// ==============================================================================
// Motor 1 (Esquerdo)
#define PIN_ENC1_A       34  // Canal A (Amarelo) - Entrada pura, sem pull-up (trocado com bateria)
#define PIN_ENC1_B       39  // Canal B (Verde)   - Pino VN (Entrada pura, sem pull-up)

// Motor 2 (Direito)
#define PIN_ENC2_A       35  // Canal A (Amarelo) - Entrada pura, sem pull-up
#define PIN_ENC2_B        2  // Canal B (Verde)   - GPIO 2 (com pull-up interno)

// Parâmetros Físicos e de Calibração (fonte única: src/firmware/lib/calibracao/src/CalibracaoRobo.h)
#define ENCODER_TICKS_PER_REV  CAL_ENCODER_PULSOS_POR_VOLTA
#define WHEEL_DIAMETER_MM      CAL_RODA_DIAMETRO_MM
#define WHEEL_CIRCUMFERENCE_MM (3.14159265f * WHEEL_DIAMETER_MM)
#define TRACK_WIDTH_MM         CAL_BITOLA_MM
#define ODOM_ESCALA_RETO       CAL_ODOM_ESCALA_RETO
#define CELL_SIZE_MM           ((float)CAL_CELULA_MM)
#define PWM_BASE_MOVIMENTO     ((int16_t)CAL_PWM_BASE_MOVIMENTO)
#define CURVA_ANTECIPACAO_FREIO_GRAUS CAL_CURVA_ANTECIPACAO_FREIO_GRAUS

// Sentidos de montagem
#define MOTOR_ESQ_INVERTIDO    CAL_MOTOR_ESQ_INVERTIDO
#define MOTOR_DIR_INVERTIDO    CAL_MOTOR_DIR_INVERTIDO
#define ENCODER_ESQ_INVERTIDO  CAL_ENCODER_ESQ_INVERTIDO
#define ENCODER_DIR_INVERTIDO  CAL_ENCODER_DIR_INVERTIDO

// ==============================================================================
// 3. SENSORES DE DISTÂNCIA (CAL_MODO_SENSORES: 0=3xToF, 1=2xToF+US, 2=2xToF Esq+Fre)
// ==============================================================================
#define PIN_I2C_SDA      21  // Dados I2C compartilhado pelos sensores VL53L0X
#define PIN_I2C_SCL      22  // Relógio I2C compartilhado pelos sensores VL53L0X
#define I2C_CLOCK_SPEED  100000 // 100 kHz (Estabilidade superior em fiação/jumpers)

// Pinos de Habilitação Individual (XSHUT)
#define PIN_XSHUT_1       4  // XSHUT Sensor Esquerdo (0x30)
#define ADDR_VL53_S1     0x30

#if CAL_TEM_TOF_DIREITO
#define PIN_XSHUT_3      17  // XSHUT Sensor Direito  (0x32)
#define ADDR_VL53_S3     0x32
#endif

#if CAL_USA_ULTRASSONICO_FRONTAL
// Sensor Ultrassônico Frontal (HC-SR04 / 3.3V-compatível)
#define PIN_ULTRA_TRIG   16
#define PIN_ULTRA_ECHO   19
#define ULTRA_TIMEOUT_US 25000UL
#define ULTRA_MIN_DIST_MM 20
#define PIN_XSHUT_4_EXTRA 23
#else
#define PIN_XSHUT_2      16  // XSHUT Sensor Frontal (0x31)
#define ADDR_VL53_S2     0x31
#define PIN_XSHUT_4_EXTRA 14
#endif
#define ADDR_VL53_S4     0x33

// Limiares e Offsets dos Sensores ToF (mm) — fonte única em CalibracaoRobo.h
#define DIST_PAREDE_DETECTADA_MM    CAL_LIMIAR_PAREDE_MM
#define DIST_MINIMA_SEGURA_MM       CAL_DIST_MINIMA_SEGURA_MM
#define DIST_ENCOSTO_FRONTAL_MM     CAL_DIST_ENCOSTO_FRONTAL_MM
#define DIST_IDEAL_PAREDE_LATERAL_MM CAL_DIST_IDEAL_PAREDE_LATERAL_MM
#define DIST_MAXIMA_UTIL_MM         CAL_CELULA_MM
#define TOF_OFFSET_ESQ_MM           CAL_TOF_OFFSET_ESQ_MM
#define TOF_OFFSET_FRONTAL_MM       CAL_TOF_OFFSET_FRONTAL_MM
#define TOF_OFFSET_DIR_MM           CAL_TOF_OFFSET_DIR_MM

// ==============================================================================
// 4. MONITORAMENTO DE BATERIA 2S (DIVISOR RESISTIVO)
// ==============================================================================
#define PIN_BATTERY_ADC  36  // Entrada Analógica ADC1_CH0 (VP)
// Se usar o Módulo Sensor de Tensão comercial (VCC < 25V, 30k / 7.5k): Fator = 5.0
// Se usar resistores discretos 100k / 33k: Fator = (100+33)/33 = 4.03
// Se usar resistores discretos 100k / 47k: Fator = (100+47)/47 = 3.127
#define BATTERY_DIVIDER_RATIO ((float)(CAL_BATERIA_R_SERIE_OHM + CAL_BATERIA_R_GND_OHM) / (float)CAL_BATERIA_R_GND_OHM)
#define BATTERY_CALIBRATION_FACTOR CAL_BATERIA_FATOR_AJUSTE
#define BATTERY_ADC_VREF      3.3f   // Tensão de referência máxima do ESP32
#define BATTERY_MIN_SAFE_V    6.6f   // 3.3V por célula (LiPo 2S descarregada)
#define BATTERY_MAX_FULL_V    8.4f   // 4.2V por célula (LiPo 2S 100% carregada)

// ==============================================================================
// 5. INTERFACE DO USUÁRIO E PINOS LIVRES
// ==============================================================================
#define PIN_BUTTON_START 15  // Botão de largada / IHM (Pull-up interno)
#define PIN_STATUS_LED   14  // LED indicador externo opcional (GPIO 14 livre)
