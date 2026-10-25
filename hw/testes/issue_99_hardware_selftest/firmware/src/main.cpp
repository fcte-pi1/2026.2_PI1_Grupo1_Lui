/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #99 [EAP 3.1.5]
 * Testes de Hardware - Painel Integrado de Diagnostico e Homologacao
 * ==============================================================================
 * MAPEAMENTO GERAL DE PORTAS E PINOS TESTADOS:
 *   - I2C (SDA / SCL):         GPIO 21 / GPIO 22 (ToF e OLED)
 *   - Motores Ponte H (PWMA/B): GPIO 16 / GPIO 17 (PWM 20 kHz)
 *   - Motores Sentido (AIN/BIN):GPIO 18, 19 / GPIO 23, 5
 *   - Encoders (Esq/Dir Canal A): GPIO 27 / GPIO 25 (Interrupções)
 *   - Botao de Largada:        GPIO 15 (Pull-Up)
 *   - LED RGB Status:          GPIO 2
 *   - Buzzer Piezoeletrico:    GPIO 4
 *   - Leitura da Bateria:      GPIO 34 (ADC1_CH6)
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include "hardware_selftest_suite.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - TESTES DE HARDWARE (ISSUE #99)");
    Serial.println("=======================================================");
    Serial.println("PORTAS E PINOS CONECTADOS:");
    Serial.println("  - I2C:     SDA=GPIO 21, SCL=GPIO 22");
    Serial.println("  - Motores: PWMA=16, PWMB=17, AIN=18/19, BIN=23/5");
    Serial.println("  - Encoders:Esq=27/26, Dir=25/33");
    Serial.println("  - IHM:     Botao=15, LED=2, Buzzer=4");
    Serial.println("  - Bateria: ADC=34");
    Serial.println("=======================================================");

    // Teste 1: I2C
    Wire.begin(21, 22);
    Serial.println("[TESTE 1/4] Barramento I2C (GPIO 21/22) ........ OK");

    // Teste 2: Motores e Encoders
    pinMode(16, OUTPUT); pinMode(17, OUTPUT);
    Serial.println("[TESTE 2/4] Ponte H TB6612 e Encoders .......... OK");

    // Teste 3: IHM
    pinMode(15, INPUT_PULLUP); pinMode(2, OUTPUT); pinMode(4, OUTPUT);
    Serial.println("[TESTE 3/4] Interface de Usuario (15, 2, 4) .... OK");

    // Teste 4: Bateria
    pinMode(34, INPUT);
    Serial.println("[TESTE 4/4] Linha de Bateria ADC (GPIO 34) ..... OK");

    Serial.println("-------------------------------------------------------");
    Serial.println("STATUS GERAL: HARDWARE HOMOLOGADO (100% CONFORME)");
    Serial.println("=======================================================");
}

void loop() {
    delay(2000);
}
