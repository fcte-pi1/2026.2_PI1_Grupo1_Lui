#ifndef BATERIA_H
#define BATERIA_H

#include <stdint.h>
#include "BateriaConversao.h"

// Driver da leitura da bateria pelo divisor de tensão no GPIO 34 (ADC1, doc 4.3).
// Só lê o ADC; as contas ficam em BateriaConversao.h.
class Bateria {
public:
    static const uint8_t PINO_ADC = 34;
    // Leituras por medição: a média suaviza o ruído do ADC do ESP32.
    static const uint8_t AMOSTRAS = 16;

    void iniciar();

    // Tensão da bateria em milivolts (média de AMOSTRAS leituras).
    uint16_t lerMilivolts();
};

#endif // BATERIA_H
