#include "sensores/Bateria.h"
#include <Arduino.h>

void Bateria::iniciar() {
    // Atenuação de 11 dB: o ADC mede até cerca de 3,1 V no pino; com a bateria cheia
    // o divisor entrega cerca de 2,08 V.
    analogSetPinAttenuation(PINO_ADC, ADC_11db);
}

uint16_t Bateria::lerMilivolts() {
    // analogReadMilliVolts já aplica a calibração de fábrica do ADC do ESP32.
    uint32_t soma = 0;
    for (uint8_t i = 0; i < AMOSTRAS; i++) {
        soma += analogReadMilliVolts(PINO_ADC);
    }
    return bateriaMvDoAdc((soma + AMOSTRAS / 2) / AMOSTRAS);
}
