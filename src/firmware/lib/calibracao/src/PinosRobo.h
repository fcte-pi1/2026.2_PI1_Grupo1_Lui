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
constexpr uint8_t PINO_PONTE_BIN1 = 18;  // sentido do motor direito
constexpr uint8_t PINO_PONTE_BIN2 = 19;  // sentido do motor direito
constexpr uint8_t PINO_PONTE_PWMB = 23;  // PWM do motor direito (20 kHz)

// --- Encoders em quadratura ---
// GPIO 34 e 35 so funcionam como entrada e nao tem pull-up interno.
constexpr uint8_t PINO_ENCODER_ESQ_A = 34;  // EEA
constexpr uint8_t PINO_ENCODER_ESQ_B = 35;  // EEB
constexpr uint8_t PINO_ENCODER_DIR_A = 32;  // EDA
constexpr uint8_t PINO_ENCODER_DIR_B = 33;  // EDB

// --- Sensores de distancia: 3x ToF VL53L0X (Esq, Frontal, Dir) ---
constexpr uint8_t PINO_I2C_SDA = 21;
constexpr uint8_t PINO_I2C_SCL = 22;
constexpr uint8_t PINO_TOF_XSHUT_ESQ = 4;       // sensor 1, lateral esquerdo (0x30)
constexpr uint8_t PINO_TOF_XSHUT_FRONTAL = 16;  // sensor 2, frontal (0x31)
constexpr uint8_t PINO_TOF_XSHUT_DIR = 17;      // sensor 3, lateral direito (0x32)

constexpr uint8_t TOF_ENDERECO_ESQ = 0x30;
constexpr uint8_t TOF_ENDERECO_FRONTAL = 0x31;
constexpr uint8_t TOF_ENDERECO_DIR = 0x32;

// --- Bateria ---
// Nao ha botao de largada nem LED de estado controlados por GPIO: a chave fisica
// e apenas liga/desliga geral (largada via software/ESP-NOW) e o LED de estado
// esta ligado diretamente em 3.3 V. GPIOs 14 e 15 permanecem livres.
constexpr uint8_t PINO_BATERIA_ADC = 36;  // ADC1_CH0 (VP), ponto medio do divisor da bateria

// Lista para validacao estatica contra conflitos de GPIO (test_pinos).
constexpr uint8_t PINOS_EM_USO[] = {
    PINO_PONTE_PWMA, PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_STBY,
    PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB,
    PINO_ENCODER_ESQ_A, PINO_ENCODER_ESQ_B, PINO_ENCODER_DIR_A, PINO_ENCODER_DIR_B,
    PINO_I2C_SDA, PINO_I2C_SCL,
    PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR,
    PINO_BATERIA_ADC,
};
constexpr uint8_t PINOS_EM_USO_QTD = sizeof(PINOS_EM_USO) / sizeof(PINOS_EM_USO[0]);

#endif
