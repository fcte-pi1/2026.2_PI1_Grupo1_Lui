/**
 * TESTE DO MONITOR DE BATERIA 2S (divisor resistivo no ADC), ISOLADO.
 *
 * Ambiente: env:teste_bateria
 *
 * Lê PIN_BATTERY_ADC do pinout.h com a mesma conta do battery_monitor.cpp
 * (média de 16 amostras de analogReadMilliVolts x razão do divisor x fator).
 * Compare a tensão mostrada com um multímetro nos polos da bateria.
 */

#include "comum_testes.h"

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho("MONITOR DE BATERIA 2S (ADC)",
        "  Meça a bateria com um multimetro e compare com 'bateria' abaixo.\n"
        "  Fator de ajuste fino = V_multimetro / V_lida (CalibracaoRobo.h).\n"
        "  'pino' perto de 0 mV com a bateria ligada indica divisor solto.");

    pinMode(PIN_BATTERY_ADC, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);
    Serial.printf("  ADC=GPIO%u | divisor x%.3f | fator de ajuste %.3f\n",
                  PIN_BATTERY_ADC, BATTERY_DIVIDER_RATIO, BATTERY_CALIBRATION_FACTOR);
}

void loop() {
    static uint32_t ultimo_ms = 0;
    if (millis() - ultimo_ms < 500) return;
    ultimo_ms = millis();

    const uint8_t AMOSTRAS = 16;
    uint32_t soma_mv = 0;
    uint32_t soma_bruto = 0;
    for (uint8_t i = 0; i < AMOSTRAS; i++) {
        soma_mv += analogReadMilliVolts(PIN_BATTERY_ADC);
        soma_bruto += analogRead(PIN_BATTERY_ADC);
        delayMicroseconds(50);
    }
    float pino_mv = (float)soma_mv / AMOSTRAS;
    float bateria_v = pino_mv * BATTERY_DIVIDER_RATIO * BATTERY_CALIBRATION_FACTOR / 1000.0f;
    const char* estado = bateria_v < BATTERY_MIN_SAFE_V ? "DESCARREGADA / sem bateria"
                       : (bateria_v > BATTERY_MAX_FULL_V + 0.2f ? "ACIMA do maximo: confira o divisor"
                       : "na faixa");
    Serial.printf("bruto=%4lu  pino=%6.0f mV  bateria=%5.2f V  (%s)\n",
                  (unsigned long)(soma_bruto / AMOSTRAS), pino_mv, bateria_v, estado);
}
