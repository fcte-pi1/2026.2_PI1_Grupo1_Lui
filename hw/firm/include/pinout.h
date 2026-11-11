#pragma once

#include <stdint.h>

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
#define PIN_ENC1_A       36  // Canal A (Amarelo) - Pino VP (Entrada pura, sem pull-up)
#define PIN_ENC1_B       39  // Canal B (Verde)   - Pino VN (Entrada pura, sem pull-up)

// Motor 2 (Direito)
#define PIN_ENC2_A       35  // Canal A (Amarelo) - Entrada pura, sem pull-up
#define PIN_ENC2_B        2  // Canal B (Verde)   - GPIO 2 (com pull-up interno)

// Parâmetros Físicos dos Motores e Rodas
#define ENCODER_TICKS_PER_REV  360    // 12 CPR * 30:1 de redução = 360 pulsos/volta
#define WHEEL_DIAMETER_MM      22.0f  // Diâmetro externo da roda em mm (conforme doc 4.1)
#define WHEEL_CIRCUMFERENCE_MM 69.1150384f  // pi * 22 mm = ~69.12 mm
#define TRACK_WIDTH_MM         80.0f  // Distância entre as rodas (bitola) em mm
#define CELL_SIZE_MM           168.0f // Comprimento da célula oficial do labirinto (mm)

// ==============================================================================
// 3. SENSORES DE DISTÂNCIA VL53L0X (TIME-OF-FLIGHT I2C + XSHUT)
// ==============================================================================
#define PIN_I2C_SDA      21  // Dados I2C compartilhado pelos 3 sensores
#define PIN_I2C_SCL      22  // Relógio I2C compartilhado pelos 3 sensores
#define I2C_CLOCK_SPEED  100000 // 100 kHz (Estabilidade superior em fiação/jumpers)

// Pinos de Habilitação Individual (XSHUT)
#define PIN_XSHUT_1       4  // XSHUT Sensor 1 (Esquerdo / 90°)
#define PIN_XSHUT_2      16  // XSHUT Sensor 2 (Frontal  / 0°)
#define PIN_XSHUT_3      17  // XSHUT Sensor 3 (Direito  / 90°)

// Endereços I2C Únicos Atribuídos no Boot
#define ADDR_VL53_S1     0x30 // Endereço pós-boot do Sensor 1 (Esquerdo)
#define ADDR_VL53_S2     0x31 // Endereço pós-boot do Sensor 2 (Frontal)
#define ADDR_VL53_S3     0x32 // Endereço pós-boot do Sensor 3 (Direito)

// Pino alternativo LIVRE caso haja um 4º sensor ToF instalado:
// (LEMBRE-SE: NÃO utilize o pino 32 para o 4º sensor, pois ele é o BIN2 da ponte H!)
#define PIN_XSHUT_4_EXTRA 14 // GPIO 14 é livre e ideal para 4º sensor se necessário
#define ADDR_VL53_S4     0x33 // Endereço reservado para 4º sensor

// Limiares de Parede do Labirinto (mm)
#define DIST_PAREDE_DETECTADA_MM 110 // Abaixo de 110 mm indica presença de parede
#define DIST_MINIMA_SEGURA_MM     30 // Limite crítico de colisão
#define DIST_MAXIMA_UTIL_MM      180 // Limite útil dentro da célula do labirinto (168x168 mm)

// ==============================================================================
// 4. MONITORAMENTO DE BATERIA 2S (DIVISOR RESISTIVO)
// ==============================================================================
#define PIN_BATTERY_ADC  34  // Entrada Analógica ADC1_CH6
// Se usar o Módulo Sensor de Tensão comercial (VCC < 25V, 30k / 7.5k): Fator = 5.0
// Se usar resistores discretos 100k / 33k: Fator = (100+33)/33 = 4.03
// Se usar resistores discretos 100k / 47k: Fator = (100+47)/47 = 3.127
#define BATTERY_DIVIDER_RATIO 5.0f   // Fator do módulo sensor de tensão comercial (5:1)
#define BATTERY_ADC_VREF      3.3f   // Tensão de referência máxima do ESP32
#define BATTERY_MIN_SAFE_V    6.6f   // 3.3V por célula (LiPo 2S descarregada)
#define BATTERY_MAX_FULL_V    8.4f   // 4.2V por célula (LiPo 2S 100% carregada)

// ==============================================================================
// 5. INTERFACE DO USUÁRIO E PINOS LIVRES
// ==============================================================================
#define PIN_BUTTON_START 15  // Botão de largada / IHM (Pull-up interno)
#define PIN_STATUS_LED   14  // LED indicador externo opcional (GPIO 14 livre)
