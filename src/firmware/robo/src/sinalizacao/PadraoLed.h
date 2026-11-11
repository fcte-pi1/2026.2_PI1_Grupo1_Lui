#ifndef PADRAO_LED_H
#define PADRAO_LED_H

#include <stdint.h>
#include "TelemetriaPacket.h"  // EstadoRobo

// O que o LED de estado mostra em cada estado, sem dependência do Arduino
// (docs/firmware/integracao-loop.md, achado R5). O driver (drivers/SinalizacaoLed.cpp)
// só aplica o resultado no LED que estiver montado (TIPO_LED_ESTADO, hardware/Pinos.h).
//
// WS2812B: as mesmas cores do main.cpp de hoje (doc 4.3).
//   AGUARDANDO           verde fixo; branco fixo com a largada armada (pode soltar)
//   MAPEANDO             amarelo piscando, 500 ms aceso e 500 ms apagado
//   SPEED_RUN            ciano fixo
//   CONCLUIDO            roxo fixo
//   TRAVADO, COLISAO,
//   TIMEOUT              vermelho fixo
//
// LED comum (uma cor só): os estados se distinguem pelo ritmo, em ciclos de 1 s.
//   AGUARDANDO           aceso fixo; piscada rápida (100/100 ms) com a largada armada
//   MAPEANDO, SPEED_RUN  500 ms aceso e 500 ms apagado
//   CONCLUIDO            uma piscada curta (100 ms) por segundo
//   TRAVADO, COLISAO,
//   TIMEOUT              duas piscadas curtas por segundo

struct CorLed {
    uint8_t r, g, b;
};

inline CorLed corLed(uint8_t r, uint8_t g, uint8_t b) {
    CorLed c;
    c.r = r;
    c.g = g;
    c.b = b;
    return c;
}

inline CorLed corDoEstado(EstadoRobo estado, bool largadaArmada, uint32_t agoraMs) {
    switch (estado) {
        case AGUARDANDO:
            return largadaArmada ? corLed(255, 255, 255) : corLed(0, 255, 0);
        case MAPEANDO:
            return ((agoraMs / 500) % 2) ? corLed(255, 255, 0) : corLed(0, 0, 0);
        case SPEED_RUN:
            return corLed(0, 255, 255);
        case CONCLUIDO:
            return corLed(128, 0, 128);
        case TRAVADO:
        case COLISAO:
        case TIMEOUT:
            return corLed(255, 0, 0);
    }
    return corLed(0, 0, 0);
}

inline bool ledComumAceso(EstadoRobo estado, bool largadaArmada, uint32_t agoraMs) {
    const uint32_t fase = agoraMs % 1000;  // posição dentro do ciclo de 1 s
    switch (estado) {
        case AGUARDANDO:
            return largadaArmada ? ((agoraMs / 100) % 2) == 0 : true;
        case MAPEANDO:
        case SPEED_RUN:
            return fase < 500;
        case CONCLUIDO:
            return fase < 100;
        case TRAVADO:
        case COLISAO:
        case TIMEOUT:
            return fase < 100 || (fase >= 200 && fase < 300);
    }
    return false;
}

// Piscadas de aviso (geometria escolhida, largada recusada), por cima do padrão do
// estado: 'vezes' ciclos de 150 ms aceso e 150 ms apagado, os mesmos do main.cpp anterior,
// mas pelo relógio em vez de delay() (achado R8).
const uint32_t PISCADA_AVISO_MS = 150;

inline bool avisoEmAndamento(uint8_t vezes, uint32_t desdeInicioMs) {
    return desdeInicioMs < (uint32_t)vezes * 2 * PISCADA_AVISO_MS;
}

inline bool avisoAceso(uint8_t vezes, uint32_t desdeInicioMs) {
    if (!avisoEmAndamento(vezes, desdeInicioMs)) return false;
    return ((desdeInicioMs / PISCADA_AVISO_MS) % 2) == 0;
}

#endif
