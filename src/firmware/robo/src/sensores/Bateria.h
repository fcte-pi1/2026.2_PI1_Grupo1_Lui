// include/sensores/Bateria.h
#ifndef BATERIA_H
#define BATERIA_H

#include <stdint.h>

class Bateria {
public:
    void iniciar();
    uint16_t lerMilivolts();
    uint8_t calcularPorcentagem(uint16_t mv) const;
};

#endif // BATERIA_H