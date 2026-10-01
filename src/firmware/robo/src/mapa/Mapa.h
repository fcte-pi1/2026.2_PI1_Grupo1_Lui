#ifndef MAPA_H
#define MAPA_H

#include <stdint.h>
#include "../include/TelemetriaPacket.h"

class Mapa {
private:
    GeometriaLabirinto geometria;
    uint8_t max_x;
    uint8_t max_y;
    
    // Matriz de paredes (usando tamanho máximo 12x4 para alocação estática)
    // Cada byte pode representar as paredes de uma célula
    uint8_t paredes[12][4];
    bool visitadas[12][4];

public:
    Mapa() : geometria(LAB_4X4), max_x(4), max_y(4) {
        limpar();
    }

    void setGeometria(GeometriaLabirinto geo) {
        geometria = geo;
        max_y = 4; // y é sempre 4 nas geometrias oficiais (4x4, 8x4, 12x4)
        switch(geo) {
            case LAB_4X4: max_x = 4; break;
            case LAB_8X4: max_x = 8; break;
            case LAB_12X4: max_x = 12; break;
        }
        limpar();
    }

    GeometriaLabirinto getGeometria() const {
        return geometria;
    }

    uint8_t getMaxX() const { return max_x; }
    uint8_t getMaxY() const { return max_y; }

    void limpar() {
        for(int x = 0; x < 12; x++) {
            for(int y = 0; y < 4; y++) {
                paredes[x][y] = 0;
                visitadas[x][y] = false;
            }
        }
    }
};

#endif
