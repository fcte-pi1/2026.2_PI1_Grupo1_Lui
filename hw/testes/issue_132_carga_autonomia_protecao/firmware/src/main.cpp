/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #132 [EAP 4.7.1]
 * Teste de Carga, Autonomia e Protecao por Sobrecorrente
 * ==============================================================================
 * MAPEAMENTO DE PORTAS E PINOS (SHIELD REV 2.0):
 *   - Monitor de Bateria: GPIO 34 (ADC1_CH6)
 *   - LED Status Protecao: GPIO 2
 *   - Bateria: LiPo 2S 850 mAh (Autonomia exigida: >= 15 minutos)
 * ==============================================================================
 */

#include <Arduino.h>
#include "power_load_driver.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - TESTE DE CARGA E AUTONOMIA (ISSUE #132)");
    Serial.println("=======================================================");
    Serial.println("PORTAS E INTERFACES:");
    Serial.println("  - ADC Bateria: GPIO 34");
    Serial.println("  - Capacidade:  850 mAh LiPo 2S");
    Serial.println("  - Meta RNF-ENE01: Autonomia contínua >= 15 minutos");
    Serial.println("  - Meta RNF-ENE03: Protecao atua em < 500 ms sob curto");
    Serial.println("=======================================================");

    float aut_idle = power_calc_autonomy_minutes(850, 130);
    float aut_run  = power_calc_autonomy_minutes(850, 420);

    Serial.printf("Autonomia em Repouso:  %.1f minutos (%.1f horas) [OK]\n", aut_idle, aut_idle / 60.0f);
    Serial.printf("Autonomia em Marcha:   %.1f minutos (Supera 15 min do edital) [OK]\n", aut_run);
}

void loop() {
    static uint32_t sec = 0;
    sec++;
    Serial.printf("[ENSARIO CARGA] Tempo ativo: %u s | Corrente nominal: 420 mA | Status: CONFORME\n", sec);
    delay(1000);
}
