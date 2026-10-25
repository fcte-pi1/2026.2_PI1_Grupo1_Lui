#pragma once

/**
 * ==============================================================================
 * MAPEAMENTO GERAL DE PORTAS E PINOS - MICROMOUSE PI1 2026.2 (SHIELD REV 2.0)
 * ==============================================================================
 */

// I2C (Sensores ToF VL53L0X, Display OLED SSD1306, IMU MPU6050)
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22

// Controle XSHUT dos Sensores ToF
#define PIN_XSHUT_FRONT     32  // Sensor Frontal (0 grau) - Principal
#define PIN_XSHUT_LEFT      13  // Sensor Esquerdo (90 graus)
#define PIN_XSHUT_RIGHT     14  // Sensor Direito (90 graus)

// Ponte H TB6612FNG (Motores DC N20)
#define PIN_PWMA            16  // Modulacao PWM Motor Esquerdo (20 kHz)
#define PIN_PWMB            17  // Modulacao PWM Motor Direito (20 kHz)
#define PIN_AIN1            18  // Sentido A Motor Esquerdo
#define PIN_AIN2            19  // Sentido B Motor Esquerdo
#define PIN_BIN1            23  // Sentido A Motor Direito
#define PIN_BIN2             5  // Sentido B Motor Direito

// Encoders em Quadratura (Leitura de pulsos dos motores)
#define PIN_ENC_L_A         27  // Canal A Encoder Esquerdo (Interrupcao)
#define PIN_ENC_L_B         26  // Canal B Encoder Esquerdo
#define PIN_ENC_R_A         25  // Canal A Encoder Direito (Interrupcao)
#define PIN_ENC_R_B         33  // Canal B Encoder Direito

// Interface Homem-Maquina (IHM)
#define PIN_BUTTON_START    15  // Botao fisico de largada (Pull-up interno)
#define PIN_RGB_LED          2  // LED de sinalizacao visual
#define PIN_BUZZER           4  // Buzzer piezoeletrico ativo (>= 60 dB)

// Monitoramento Eletrico / Bateria
#define PIN_ADC_BATTERY     34  // Leitura analogica do divisor de tensao (ADC1_CH6)

// Isolamento
#define PIN_BOOT_ISOLATED   12  // Isolado na Shield para evitar falha de boot
