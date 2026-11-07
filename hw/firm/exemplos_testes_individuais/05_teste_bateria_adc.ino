/**
 * ==============================================================================
 * TESTE INDIVIDUAL 05: MEDIÇÃO E CALIBRAÇÃO DA BATERIA (ADC GPIO 34)
 * ==============================================================================
 * Conexões:
 *   - Sinal do Divisor: GPIO 34 (ADC1_CH6)
 *   - GND: Terra comum
 * ==============================================================================
 */

#include <Arduino.h>

#define PIN_BATTERY_ADC 34
#define FATOR_DIVISOR   5.0f // Fator do módulo sensor de tensão comercial (5:1)

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n--- [TESTE 05] MONITOR DE BATERIA ADC ---");
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
}

void loop() {
    uint32_t soma = 0;
    for (int i = 0; i < 32; i++) {
        soma += analogRead(PIN_BATTERY_ADC);
        delay(2);
    }
    float adc_medio = (float)soma / 32.0f;
    float v_pino = (adc_medio / 4095.0f) * 3.3f;
    float v_bateria = v_pino * FATOR_DIVISOR;

    Serial.printf("[BATERIA] ADC Bruto: %4.0f | Tensão Pino GPIO 34: %1.3f V | Tensão Bateria: %2.2f V\n",
                  adc_medio, v_pino, v_bateria);
    delay(500);
}
