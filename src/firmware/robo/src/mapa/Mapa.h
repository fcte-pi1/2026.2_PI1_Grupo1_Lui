#ifndef MAPA_H
#define MAPA_H

#include <stdint.h>
#include "TelemetriaPacket.h"

class Mapa {
private:
    GeometriaLabirinto geometria;
    uint8_t max_linhas;
    uint8_t max_colunas;
    
    // Matriz de paredes (usando tamanho máximo 12x4 para alocação estática)
    // Cada byte pode representar as paredes de uma célula
    uint8_t paredes[12][4];
    bool visitadas[12][4];

public:
    Mapa() : geometria(LAB_4X4), max_linhas(4), max_colunas(4) {
        limpar();
    }

    void setGeometria(GeometriaLabirinto geo) {
        geometria = geo;
        max_colunas = 4; // Colunas A a D (sempre 4 nas geometrias oficiais)
        switch(geo) {
            case LAB_4X4: max_linhas = 4; break;
            case LAB_8X4: max_linhas = 8; break;
            case LAB_12X4: max_linhas = 12; break;
        }
        limpar();
    }

    GeometriaLabirinto getGeometria() const {
        return geometria;
    }

    uint8_t getMaxLinhas() const { return max_linhas; }
    uint8_t getMaxColunas() const { return max_colunas; }

    // Compatibilidade temporária se necessário
    uint8_t getMaxX() const { return max_linhas; }
    uint8_t getMaxY() const { return max_colunas; }

    void limpar() {
        for(int l = 0; l < 12; l++) {
            for(int c = 0; c < 4; c++) {
                paredes[l][c] = 0;
                visitadas[l][c] = false;
            }
        }
    }
};

#endif

