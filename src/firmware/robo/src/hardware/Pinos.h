#ifndef PINOS_H
#define PINOS_H

#include <stdint.h>

// Configuração única do hardware do robô: pinos do ESP32 DevKit V1 (30 pinos) e
// constantes físicas. Sem dependência do Arduino, para os testes nativos poderem usar
// os mesmos valores do firmware.
//
// Fonte: firmware da Eletrônica que está gravado e funcionando no robô
// (hw/firm/include/pinout.h e hw/firm/README.md, branch
// feat/firmware-final-comissionamento-telemetria). A tabela de pinagem do doc 4.3 está
// desatualizada em relação a essa pinagem.
//
// Ao acrescentar um pino, inclua-o também em PINOS_EM_USO (o teste test_pinos confere
// que nenhum GPIO é usado duas vezes).

// --- Ponte H TB6612FNG (motor 1 = esquerdo, motor 2 = direito) ---
const uint8_t PINO_PONTE_PWMA = 25;  // PWM do motor esquerdo (20 kHz)
const uint8_t PINO_PONTE_AIN1 = 26;  // sentido do motor esquerdo
const uint8_t PINO_PONTE_AIN2 = 27;  // sentido do motor esquerdo
const uint8_t PINO_PONTE_STBY = 13;  // standby: HIGH habilita a ponte H
const uint8_t PINO_PONTE_BIN1 = 33;  // sentido do motor direito
const uint8_t PINO_PONTE_BIN2 = 32;  // sentido do motor direito
const uint8_t PINO_PONTE_PWMB = 18;  // PWM do motor direito (20 kHz)

// --- Encoders em quadratura ---
// Os GPIO 34 a 39 só funcionam como entrada e não têm pull-up interno.
const uint8_t PINO_ENCODER_ESQ_A = 36;  // VP
const uint8_t PINO_ENCODER_ESQ_B = 39;  // VN
const uint8_t PINO_ENCODER_DIR_A = 35;
const uint8_t PINO_ENCODER_DIR_B = 2;   // pino de boot do ESP32, com pull-up interno

// --- Sensores ToF VL53L0X (I2C compartilhado + XSHUT individual) ---
const uint8_t PINO_I2C_SDA = 21;
const uint8_t PINO_I2C_SCL = 22;
const uint8_t PINO_TOF_XSHUT_ESQ = 4;       // sensor 1, lateral esquerdo (0x30)
const uint8_t PINO_TOF_XSHUT_FRONTAL = 16;  // sensor 2, frontal (0x31)
const uint8_t PINO_TOF_XSHUT_DIR = 17;      // sensor 3, lateral direito (0x32)

// --- Interface e bateria ---
const uint8_t PINO_BOTAO = 15;  // botão de largada, com pull-up interno (nível LOW = pressionado)
// Provisório: depende de confirmação da Eletrônica. Na pinagem nova o GPIO 2 (antigo
// pino do LED) é o canal B do encoder direito; o README da Eletrônica indica o GPIO 14
// como "Status LED / reserva".
const uint8_t PINO_LED_ESTADO = 14;  // LED WS2812B de estado
const uint8_t PINO_BATERIA_ADC = 34;  // ADC1_CH6, ponto médio do divisor da bateria

const uint8_t PINOS_EM_USO[] = {
    PINO_PONTE_PWMA, PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_STBY,
    PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB,
    PINO_ENCODER_ESQ_A, PINO_ENCODER_ESQ_B, PINO_ENCODER_DIR_A, PINO_ENCODER_DIR_B,
    PINO_I2C_SDA, PINO_I2C_SCL,
    PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR,
    PINO_BOTAO, PINO_LED_ESTADO, PINO_BATERIA_ADC,
};
const uint8_t PINOS_EM_USO_QTD = sizeof(PINOS_EM_USO) / sizeof(PINOS_EM_USO[0]);

// --- Constantes físicas (confirmadas pela Eletrônica) ---
constexpr float RODA_DIAMETRO_MM = 32.0f;
const uint16_t ENCODER_PULSOS_POR_VOLTA = 360;  // pulsos por volta da roda: 12 CPR x redução 30:1
constexpr float BITOLA_MM = 80.0f;              // distância entre os centros das rodas
// Deslocamento de uma roda por pulso: pi x 32 / 360, cerca de 0,279 mm.
constexpr float RODA_MM_POR_PULSO = 3.14159265f * RODA_DIAMETRO_MM / ENCODER_PULSOS_POR_VOLTA;

// --- Labirinto (regulamento) ---
// Passo entre centros de células: é o que o robô avança por célula e o que a velocidade
// média usa. O vão livre do corredor é o passo menos a espessura da parede (12 mm) e serve
// de referência para os limiares dos sensores de distância. O hw/firm usa 168 mm como
// "célula"; aqui o avanço é de centro a centro (docs/firmware/integracao-loop.md, R2).
constexpr uint16_t CELULA_MM = 180;
constexpr uint16_t VAO_CORREDOR_MM = 168;

// Divisor de tensão da bateria (resistor em série com a bateria e resistor para o GND,
// com o ponto médio em PINO_BATERIA_ADC). Valores do firmware que roda no robô: módulo
// sensor de tensão comercial de 30 kΩ / 7,5 kΩ, proporção 5:1 (bateria cheia de 8,4 V
// dá 1,68 V no pino). Depende de confirmação da Eletrônica: o doc 4.3 indica
// 100 kΩ / 47 kΩ (proporção de cerca de 3,13:1).
const uint32_t BATERIA_R_SERIE_OHM = 30000;
const uint32_t BATERIA_R_GND_OHM = 7500;

#endif
