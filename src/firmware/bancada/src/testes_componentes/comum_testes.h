#pragma once

/**
 * ==============================================================================
 * TESTES POR COMPONENTE - FUNÇÕES COMUNS
 * ==============================================================================
 * Cada teste em src/testes_componentes/ é um firmware completo (setup/loop)
 * compilado em um ambiente próprio do platformio.ini (env:teste_*). A pinagem
 * vem exclusivamente de include/pinout.h, o mesmo arquivo do firmware principal.
 *
 * Antes de testar um componente, os demais são postos em estado seguro:
 *   - ponte H em standby (motores não giram por pino flutuante);
 *   - todos os XSHUT dos VL53L0X em LOW (nenhum sensor ToF no barramento I2C).
 * Assim cada teste exercita só o componente dele, mesmo com tudo ligado.
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include "pinout.h"

#define TESTE_SERIAL_BAUD 115200

// Ponte H em standby, direções em LOW e PWM em LOW.
static inline void teste_motores_em_seguranca(void) {
    const uint8_t pinos[] = { PIN_STBY, PIN_AIN1, PIN_AIN2, PIN_PWMA,
                              PIN_BIN1, PIN_BIN2, PIN_PWMB };
    for (uint8_t i = 0; i < sizeof(pinos); i++) {
        pinMode(pinos[i], OUTPUT);
        digitalWrite(pinos[i], LOW);
    }
}

// Desliga (reset por XSHUT) os 3 VL53L0X do pinout.h.
static inline void teste_tof_todos_em_reset(void) {
    const uint8_t pinos[] = { PIN_XSHUT_1, PIN_XSHUT_2, PIN_XSHUT_3 };
    for (uint8_t i = 0; i < sizeof(pinos); i++) {
        pinMode(pinos[i], OUTPUT);
        digitalWrite(pinos[i], LOW);
    }
}

static inline void teste_cabecalho(const char* nome, const char* instrucoes) {
    Serial.begin(TESTE_SERIAL_BAUD);
    delay(800);
    Serial.println();
    Serial.println("=======================================================");
    Serial.printf ("  TESTE DE COMPONENTE: %s\n", nome);
    Serial.println("=======================================================");
    Serial.println(instrucoes);
    Serial.println("-------------------------------------------------------");
}

// Varre o barramento I2C e imprime os endereços que responderam.
static inline uint8_t teste_varrer_i2c(void) {
    uint8_t achados = 0;
    Serial.print("  Varredura I2C:");
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf(" 0x%02X", addr);
            achados++;
        }
    }
    if (achados == 0) Serial.print(" nenhum dispositivo");
    Serial.println();
    return achados;
}

// PWM do LEDC compatível com o core Arduino-ESP32 2.x e 3.x.
static inline void teste_pwm_configurar(uint8_t pino, uint8_t canal) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    (void)canal;
    ledcAttach(pino, PWM_FREQ_HZ, PWM_RESOLUTION);
#else
    ledcSetup(canal, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(pino, canal);
#endif
}

static inline void teste_pwm_escrever(uint8_t pino, uint8_t canal, uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    (void)canal;
    ledcWrite(pino, duty);
#else
    (void)pino;
    ledcWrite(canal, duty);
#endif
}
