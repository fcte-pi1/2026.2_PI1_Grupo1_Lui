#ifndef PINOS_ROBO_H
#define PINOS_ROBO_H

#include <stdint.h>

// Fonte unica da pinagem oficial do Micromouse (ESP32 DevKit V1, 30 pinos, 3x VL53L0X).
// Compartilhada entre o firmware oficial (src/firmware/robo) e o firmware de bancada
// (src/firmware/bancada). Altere os GPIOs aqui para atualizar ambos os projetos.

// --- Ponte H TB6612FNG (motor 1 = esquerdo, motor 2 = direito) ---
constexpr uint8_t PINO_PONTE_PWMA = 25;  // PWM do motor esquerdo (20 kHz)
constexpr uint8_t PINO_PONTE_AIN1 = 26;  // sentido do motor esquerdo
constexpr uint8_t PINO_PONTE_AIN2 = 27;  // sentido do motor esquerdo
constexpr uint8_t PINO_PONTE_STBY = 13;  // standby: HIGH habilita a ponte H
constexpr uint8_t PINO_PONTE_BIN1 = 33;  // sentido do motor direito
constexpr uint8_t PINO_PONTE_BIN2 = 32;  // sentido do motor direito
constexpr uint8_t PINO_PONTE_PWMB = 18;  // PWM do motor direito (20 kHz)

// --- Encoders em quadratura ---
// Os GPIO 34 a 39 so funcionam como entrada e nao tem pull-up interno.
constexpr uint8_t PINO_ENCODER_ESQ_A = 34;
constexpr uint8_t PINO_ENCODER_ESQ_B = 39;  // VN
constexpr uint8_t PINO_ENCODER_DIR_A = 35;
constexpr uint8_t PINO_ENCODER_DIR_B = 2;   // pino de boot do ESP32, com pull-up interno

// --- Sensores de distancia: 3x ToF VL53L0X (Esq, Frontal, Dir) ---
constexpr uint8_t PINO_I2C_SDA = 21;
constexpr uint8_t PINO_I2C_SCL = 22;
constexpr uint8_t PINO_TOF_XSHUT_ESQ = 4;       // sensor 1, lateral esquerdo (0x30)
constexpr uint8_t PINO_TOF_XSHUT_FRONTAL = 16;  // sensor 2, frontal (0x31)
constexpr uint8_t PINO_TOF_XSHUT_DIR = 17;      // sensor 3, lateral direito (0x32)

constexpr uint8_t TOF_ENDERECO_ESQ = 0x30;
constexpr uint8_t TOF_ENDERECO_FRONTAL = 0x31;
constexpr uint8_t TOF_ENDERECO_DIR = 0x32;

// --- Interface e bateria ---
constexpr uint8_t PINO_BOTAO = 15;        // botao de largada, com pull-up interno (LOW = pressionado)
constexpr uint8_t PINO_LED_ESTADO = 14;   // LED de estado
constexpr uint8_t PINO_BATERIA_ADC = 36;  // ADC1_CH0 (VP), ponto medio do divisor da bateria

// Lista para validacao estatica contra conflitos de GPIO (test_pinos).
constexpr uint8_t PINOS_EM_USO[] = {
    PINO_PONTE_PWMA, PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_STBY,
    PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB,
    PINO_ENCODER_ESQ_A, PINO_ENCODER_ESQ_B, PINO_ENCODER_DIR_A, PINO_ENCODER_DIR_B,
    PINO_I2C_SDA, PINO_I2C_SCL,
    PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR,
    PINO_BOTAO, PINO_LED_ESTADO, PINO_BATERIA_ADC,
};
constexpr uint8_t PINOS_EM_USO_QTD = sizeof(PINOS_EM_USO) / sizeof(PINOS_EM_USO[0]);

#endif
