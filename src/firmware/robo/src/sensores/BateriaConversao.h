#ifndef BATERIA_CONVERSAO_H
#define BATERIA_CONVERSAO_H

#include <stdint.h>

// Conversões da leitura da bateria, sem dependência do Arduino (testáveis nativamente).
//
// Divisor de tensão do doc 4.3 (RF_ENE02): 100 kΩ em série e 33 kΩ para o GND, com o
// ponto médio no GPIO 34. A tensão da bateria é a do pino vezes (100k + 33k) / 33k;
// com a LiPo 2S cheia (8,4 V) o pino fica em cerca de 2,08 V.
//
// A porcentagem usa a mesma faixa linear do simulador do backend
// (src/backend/app/simulador/gerador.py): 0% em 7,0 V e 100% em 8,4 V.

const uint32_t BATERIA_R_SERIE_OHM = 100000;
const uint32_t BATERIA_R_GND_OHM = 33000;
const uint16_t BATERIA_VAZIA_MV = 7000;
const uint16_t BATERIA_CHEIA_MV = 8400;

// Milivolts da bateria a partir dos milivolts medidos no pino do ADC, arredondado.
// Limitado a 65535, o máximo do campo bateria_mv do pacote.
inline uint16_t bateriaMvDoAdc(uint32_t mvAdc) {
    const uint32_t total = BATERIA_R_SERIE_OHM + BATERIA_R_GND_OHM;
    const uint64_t mv = ((uint64_t)mvAdc * total + BATERIA_R_GND_OHM / 2) / BATERIA_R_GND_OHM;
    return mv > 65535 ? 65535 : (uint16_t)mv;
}

// Carga estimada de 0 a 100%, saturando fora da faixa e arredondando ao inteiro mais
// próximo (o meio vai para cima).
inline uint8_t porcentagemBateria(uint16_t mvBateria) {
    if (mvBateria <= BATERIA_VAZIA_MV) return 0;
    if (mvBateria >= BATERIA_CHEIA_MV) return 100;
    const uint32_t faixa = BATERIA_CHEIA_MV - BATERIA_VAZIA_MV;
    return (uint8_t)(((uint32_t)(mvBateria - BATERIA_VAZIA_MV) * 100 + faixa / 2) / faixa);
}

#endif
