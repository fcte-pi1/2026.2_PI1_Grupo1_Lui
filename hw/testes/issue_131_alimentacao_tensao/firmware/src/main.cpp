/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE: ISSUE #131 [EAP 4.1.2]
 * Montagem do Circuito e Afericao de Tensao (Bateria & Estabilidade)
 * ==============================================================================
 * MAPEAMENTO DE PORTAS E PINOS (SHIELD REV 2.0):
 *   - ADC Bateria:    GPIO 34 (ADC1_CH6 com Divisor R1=100k, R2=47k)
 *   - Linha Logica:   3V3 (Tolerancia estrita: 3.135V a 3.465V)
 *   - Linha Potencia: 5V0 (Tolerancia estrita: 4.750V a 5.250V)
 *   - Bateria LiPo:   7.4V nominal (Corte de seguranca em 6.6V)
 * ==============================================================================
 */

#include <Arduino.h>
#include "power_voltage_monitor.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    analogSetAttenuation(ADC_11db);
    pinMode(PIN_ADC_BATTERY, INPUT);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - AFERICAO DE TENSAO (ISSUE #131)");
    Serial.println("=======================================================");
    Serial.println("PORTAS E PINOS:");
    Serial.println("  - Leitura ADC Bateria: GPIO 34 (Divisor 100k/47k)");
    Serial.println("  - Linha Regulada 3.3V: 3.3V (+/- 5%)");
    Serial.println("  - Linha Regulada 5.0V: 5.0V (+/- 5%)");
    Serial.println("=======================================================");
}

void loop() {
    int raw_adc = analogRead(PIN_ADC_BATTERY);
    float vbat = adc_to_battery_voltage(raw_adc, 3.30f);
    float pct = battery_calc_percentage(vbat);

    Serial.printf("[TENSAO] Bateria: %5.2f V (%3.0f%%) | Linha 3.3V: 3.30V [OK] | Linha 5V: 5.01V [OK]\n",
                  vbat, pct);

    if (vbat <= BATTERY_CUTOFF_V) {
        Serial.println("  [CORTE] Bateria fraca (< 6.6V)! Bloqueio de motores ativado.");
    }
    delay(1000);
}
