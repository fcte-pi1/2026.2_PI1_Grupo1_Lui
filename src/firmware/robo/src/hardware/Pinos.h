#ifndef PINOS_H
#define PINOS_H

#include <stdint.h>
#include "PinosRobo.h"
#include "CalibracaoRobo.h"

// Configuracao unica do hardware do robo: reexporta pinos (PinosRobo.h) e parametros
// fisicos (CalibracaoRobo.h), acrescentando constantes derivadas e de controle.
// Sem dependencia do Arduino, para os testes nativos poderem usar os mesmos valores.

// --- Constantes fisicas (fonte unica em CalibracaoRobo.h) ---
constexpr float RODA_DIAMETRO_MM = CAL_RODA_DIAMETRO_MM;
// Pulsos por volta da RODA, contando so a borda de subida do canal A (como a ISR de
// drivers/Encoders.cpp).
const uint16_t ENCODER_PULSOS_POR_VOLTA = CAL_ENCODER_PULSOS_POR_VOLTA;
// Bitola real (centro-a-centro dos O-rings): rodas montadas POR FORA do chassi.
constexpr float BITOLA_MM = CAL_BITOLA_MM;
// Deslocamento de uma roda por pulso: pi x RODA_DIAMETRO_MM / ENCODER_PULSOS_POR_VOLTA.
constexpr float RODA_MM_POR_PULSO = 3.14159265f * RODA_DIAMETRO_MM / ENCODER_PULSOS_POR_VOLTA;
// Fator empirico da odometria em linha reta (real / medido em 4-5 celulas no piso).
constexpr float ODOM_ESCALA_RETO = CAL_ODOM_ESCALA_RETO;
// Compensacao longitudinal na PRIMEIRA reta da prova (mm).
constexpr float COMPENSACAO_LARGADA_MM = CAL_COMPENSACAO_LARGADA_MM;

// Sentidos de montagem (achado R7). Valores configurados em CalibracaoRobo.h.
const bool MOTOR_ESQ_INVERTIDO = CAL_MOTOR_ESQ_INVERTIDO;
const bool MOTOR_DIR_INVERTIDO = CAL_MOTOR_DIR_INVERTIDO;
const bool ENCODER_ESQ_INVERTIDO = CAL_ENCODER_ESQ_INVERTIDO;
const bool ENCODER_DIR_INVERTIDO = CAL_ENCODER_DIR_INVERTIDO;

// --- Ponte H: PWM do ESP32 (LEDC) ---
const uint32_t PONTE_PWM_FREQ_HZ = 20000;  // acima da faixa audivel
const uint8_t PONTE_PWM_BITS = 8;          // PWM de 0 a 255
const uint8_t PWM_MAXIMO = 255;

// Conversao de velocidade em PWM (achado B6, controle/ConversorVelocidadePwm.h):
//   |PWM| = PWM_MINIMO + |v| x PWM_POR_MM_S, saturado em PWM_MAXIMO_MOVIMENTO.
const uint8_t PWM_MINIMO = CAL_PWM_MINIMO;
const uint8_t PWM_BASE_MOVIMENTO = CAL_PWM_BASE_MOVIMENTO;
const uint8_t PWM_MAXIMO_MOVIMENTO = CAL_PWM_MAXIMO_MOVIMENTO;
constexpr float PWM_POR_MM_S = (float)(PWM_BASE_MOVIMENTO - PWM_MINIMO) / 150.0f;

// Rampas de aceleracao/desaceleracao e curva sem IMU (calibracao de pista).
const uint32_t TEMPO_RAMPA_PARTIDA_MS = 120;
constexpr float DIST_DESACELERACAO_MM = 35.0f;
constexpr float VEL_MIN_APROXIMACAO_MM_S = 65.0f;
constexpr float CURVA_ANTECIPACAO_FREIO_GRAUS = CAL_CURVA_ANTECIPACAO_FREIO_GRAUS;

// --- Labirinto (regulamento) ---
constexpr uint16_t CELULA_MM = CAL_CELULA_MM;
constexpr uint16_t VAO_CORREDOR_MM = CAL_VAO_CORREDOR_MM;
constexpr float DIST_IDEAL_PAREDE_LATERAL_MM = CAL_DIST_IDEAL_PAREDE_LATERAL_MM;

// --- Sensores ToF VL53L0X ---
// Offset individual de calibracao (mm que o sensor le ACIMA da distancia real).
const int16_t TOF_OFFSET_ESQ_MM = CAL_TOF_OFFSET_ESQ_MM;
const int16_t TOF_OFFSET_FRONTAL_MM = CAL_TOF_OFFSET_FRONTAL_MM;
const int16_t TOF_OFFSET_DIR_MM = CAL_TOF_OFFSET_DIR_MM;
const uint16_t TOF_DIST_MIN_VALIDA_MM = 10;   // permite leituras validas perto do encosto frontal (15-24 mm)
const uint16_t TOF_DIST_MAX_VALIDA_MM = 2000; // acima disso: sem alvo ao alcance
const uint32_t TOF_I2C_HZ = 100000;           // 100 kHz (estabilidade em fiacao com jumpers)
const uint32_t TOF_ORCAMENTO_US = 33000;      // orcamento de medicao: cerca de 30 Hz por sensor
// A leitura nao espera o sensor (achado B10): se um sensor ficar sem amostra nova por mais
// que TOF_VALIDADE_MS, a leitura passa a ser invalida.
const uint16_t TOF_VALIDADE_MS = 100;
const uint16_t TOF_TIMEOUT_BIBLIOTECA_MS = 20;
const uint8_t TOF_PERIODO_CONSULTA_MS = 5;
const uint8_t TOF_I2C_TIMEOUT_MS = 15;
const uint8_t TOF_ESPERA_APOS_ERRO_MS = 100;
const uint16_t TOF_LEITURA_INVALIDA_MM = 8190;  // valor que o VL53L0X devolve sem alvo

// Distancias frontais do avanco de uma celula:
const uint16_t DIST_MINIMA_SEGURA_MM = CAL_DIST_MINIMA_SEGURA_MM;
const uint16_t DIST_ENCOSTO_FRONTAL_MM = CAL_DIST_ENCOSTO_FRONTAL_MM;

// Classificacao de parede (achado R3, sensores/ClassificadorParedes.h).
const uint16_t LIMIAR_PAREDE_MM = CAL_LIMIAR_PAREDE_MM;
const uint16_t HISTERESE_PAREDE_MM = CAL_HISTERESE_PAREDE_MM;

// --- LED de estado ---
enum TipoLedEstado : uint8_t {
    LED_WS2812B,
    LED_COMUM
};
const TipoLedEstado TIPO_LED_ESTADO = LED_COMUM;

// Divisor de tensao da bateria (modulo sensor de tensao 30 kOhm / 7,5 kOhm, 5:1).
const uint32_t BATERIA_R_SERIE_OHM = CAL_BATERIA_R_SERIE_OHM;
const uint32_t BATERIA_R_GND_OHM = CAL_BATERIA_R_GND_OHM;
constexpr float BATERIA_FATOR_AJUSTE = CAL_BATERIA_FATOR_AJUSTE;

// Tensao minima para aceitar a largada (achado R4): 3,4 V por celula.
const uint16_t BATERIA_MINIMA_LARGADA_MV = CAL_BATERIA_MINIMA_LARGADA_MV;

#endif
