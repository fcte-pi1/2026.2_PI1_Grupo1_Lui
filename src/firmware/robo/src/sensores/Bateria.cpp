#include "sensores/Bateria.h"
#include <Arduino.h>
#include <math.h>

void Bateria::iniciar() {
    pinMode(PINO_ADC, INPUT);
    // Atenuação de 11 dB: o ADC mede até cerca de 3,1 V no pino; com a bateria cheia
    // o divisor de 5:1 entrega 1,68 V.
    analogSetPinAttenuation(PINO_ADC, ADC_11db);
}

uint16_t Bateria::lerMilivolts() {
    // analogReadMilliVolts já aplica a calibração de fábrica do ADC do ESP32.
    uint32_t soma = 0;
    for (uint8_t i = 0; i < AMOSTRAS; i++) {
        soma += analogReadMilliVolts(PINO_ADC);
    }
    const uint32_t mvPino = (soma + AMOSTRAS / 2) / AMOSTRAS;
    const uint32_t mvAjustado = (uint32_t)lroundf((float)mvPino * BATERIA_FATOR_AJUSTE);
    return bateriaMvDoAdc(mvAjustado);
}
