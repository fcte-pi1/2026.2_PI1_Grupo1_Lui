#ifndef CALIBRACAO_ROBO_H
#define CALIBRACAO_ROBO_H

#include <stdint.h>

// Fonte unica de parametros fisicos e de calibracao de bancada do Micromouse.
// Compartilhada entre o firmware oficial (src/firmware/robo) e o firmware de
// comissionamento/bancada (src/firmware/bancada). Altere os valores medidos aqui
// para atualizar os dois projetos de uma vez.
// Configuracao oficial de hardware: 3x sensores ToF VL53L0X (Esq, Frontal, Dir).

// --- Dimensoes mecanicas e odometria (Fase 0 e Fase 4.1) ---
constexpr float CAL_RODA_DIAMETRO_MM = 22.0f;         // Diametro externo das rodas motrizes (mm)
constexpr uint16_t CAL_ENCODER_PULSOS_POR_VOLTA = 360; // 12 CPR x reducao 30:1 (borda de subida canal A)
constexpr float CAL_BITOLA_MM = 110.0f;               // Distancia centro-a-centro dos pneus (mm)
constexpr float CAL_ODOM_ESCALA_RETO = 1.0f;          // Fator empirico real/medido em linha reta
constexpr float CAL_COMPENSACAO_LARGADA_MM = 0.0f;    // Deslocamento extra na largada em A1 (mm)

// --- Sentidos de montagem (Fase 2.1 e Fase 3.1) ---
constexpr bool CAL_MOTOR_ESQ_INVERTIDO = false;
constexpr bool CAL_MOTOR_DIR_INVERTIDO = false;
constexpr bool CAL_ENCODER_ESQ_INVERTIDO = false;
constexpr bool CAL_ENCODER_DIR_INVERTIDO = false;

// --- Motores e curvas sem IMU (Fase 3.2 e Fase 4.3) ---
constexpr uint8_t CAL_PWM_MINIMO = 40;
constexpr uint8_t CAL_PWM_BASE_MOVIMENTO = 90;
constexpr uint8_t CAL_PWM_MAXIMO_MOVIMENTO = 130;
constexpr float CAL_CURVA_ANTECIPACAO_FREIO_GRAUS = 0.5f;

// --- Labirinto e sensores ToF VL53L0X (Fase 0.4, 0.5, 2.2 e 4.2) ---
constexpr uint16_t CAL_CELULA_MM = 180;
constexpr uint16_t CAL_VAO_CORREDOR_MM = 168;
constexpr float CAL_DIST_IDEAL_PAREDE_LATERAL_MM = 43.0f;

// Offsets individuais (mm que o sensor le ACIMA da distancia real).
constexpr int16_t CAL_TOF_OFFSET_ESQ_MM = 0;
constexpr int16_t CAL_TOF_OFFSET_FRONTAL_MM = 0;
constexpr int16_t CAL_TOF_OFFSET_DIR_MM = 0;

constexpr uint16_t CAL_DIST_MINIMA_SEGURA_MM = 15;
constexpr uint16_t CAL_DIST_ENCOSTO_FRONTAL_MM = 24;
constexpr uint16_t CAL_LIMIAR_PAREDE_MM = 110;
constexpr uint16_t CAL_HISTERESE_PAREDE_MM = 5;

// --- Bateria LiPo 2S (Fase 1.1) ---
constexpr uint32_t CAL_BATERIA_R_SERIE_OHM = 30000;
constexpr uint32_t CAL_BATERIA_R_GND_OHM = 7500;
constexpr float CAL_BATERIA_FATOR_AJUSTE = 1.0f;     // Fator fino empirico (V_multimetro / V_lida)
constexpr uint16_t CAL_BATERIA_MINIMA_LARGADA_MV = 6800;

#endif
