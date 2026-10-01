#ifndef TELEMETRIA_PACKET_H
#define TELEMETRIA_PACKET_H

#include <stdint.h>

// Definindo os estados possíveis (conforme diagrama de estados)
enum EstadoRobo {
    AGUARDANDO = 0,
    MAPEANDO,
    SPEED_RUN,
    CONCLUIDO,
    COLISAO,
    TIMEOUT,
    TRAVADO
};

// Geometrias possíveis
enum GeometriaLabirinto {
    LAB_4X4 = 0,
    LAB_8X4,
    LAB_12X4
};

// Estrutura do pacote de telemetria
struct __attribute__((packed)) TelemetriaPacket {
    uint8_t status;           // EstadoRobo
    uint8_t labirinto;        // GeometriaLabirinto
    float posicao_x;
    float posicao_y;
    float orientacao;
    float bateria_pct;
    float bateria_volts;
    float velocidade_media;
    float velocidade_atual;
    uint32_t timestamp_ms;
    bool desafio_cumprido;
};

#endif
