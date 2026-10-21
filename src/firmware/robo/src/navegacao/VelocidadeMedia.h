#ifndef VELOCIDADE_MEDIA_H
#define VELOCIDADE_MEDIA_H

#include <stdint.h>

// Velocidade média da prova para a telemetria, sem dependência do Arduino.
// Células de 180 mm (regulamento): distância = células andadas x 180 mm.
// O tempo é o do cronômetro da prova (0 antes da largada, congelado na chegada), e o
// resultado vai para o campo velocidade_media do pacote, em mm/s (int16_t).

const uint16_t TAMANHO_CELULA_MM = 180;
const int16_t VELOCIDADE_MAX_MM_S = 32767;  // limite do campo int16_t

// Média em mm/s, arredondada; 0 antes da largada (tempo 0).
inline int16_t velocidadeMediaMmS(uint16_t celulasAndadas, uint32_t tempoMs) {
    if (tempoMs == 0) return 0;
    const uint64_t distanciaMm = (uint64_t)celulasAndadas * TAMANHO_CELULA_MM;
    const uint64_t mmPorS = (distanciaMm * 1000 + tempoMs / 2) / tempoMs;
    return mmPorS > (uint64_t)VELOCIDADE_MAX_MM_S ? VELOCIDADE_MAX_MM_S : (int16_t)mmPorS;
}

#endif
