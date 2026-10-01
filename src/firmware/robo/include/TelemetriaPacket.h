#ifndef TELEMETRIA_PACKET_H
#define TELEMETRIA_PACKET_H

#include <stdint.h>

// Definindo os estados possíveis (conforme doc 4.4 seção 6.2)
enum EstadoRobo {
    AGUARDANDO = 0,
    MAPEANDO   = 1,
    SPEED_RUN  = 2,
    COLISAO    = 3,
    CONCLUIDO  = 4,
    TIMEOUT    = 5,
    TRAVADO    = 6
};

// Geometrias possíveis
enum GeometriaLabirinto {
    LAB_4X4  = 1,
    LAB_8X4  = 2,
    LAB_12X4 = 3
};

// Estrutura do pacote de telemetria (18 bytes)
struct __attribute__((packed)) TelemetriaPacket {
    uint32_t timestamp_ms;     // Tempo decorrido de corrida (ms)
    uint8_t  labirinto_id;     // 1: 4x4, 2: 8x4, 3: 12x4
    uint8_t  status_id;        // 0: AGUARDANDO, 1: MAPEANDO, 2: SPEED_RUN, 3: COLISAO, 4: CONCLUIDO, 5: TIMEOUT, 6: TRAVADO
    uint8_t  desafio_cumprido; // 0: Nao, 1: Sim
    uint8_t  bateria_pct;      // 0 a 100 %
    uint16_t bateria_mv;       // Tensao em milivolts (ex: 7420 mV = 7.42 V)
    int16_t  velocidade_atual; // Velocidade instantanea em mm/s
    int16_t  velocidade_media; // Velocidade media acumulada em mm/s
    uint8_t  coluna;           // Indice interno 0 a 3 (exibido como A a D)
    uint8_t  linha;            // Indice interno 0 a 11 (exibido como 1 a 12)
    uint8_t  orientacao;       // 0: Norte, 1: Sul, 2: Leste, 3: Oeste
    uint8_t  paredes;          // Bitmask: bit0=N, bit1=S, bit2=L, bit3=O
};

#endif

