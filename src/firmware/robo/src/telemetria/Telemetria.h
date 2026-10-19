// include/telemetria/Pacote.h
#ifndef PACOTE_H
#define PACOTE_H

#include <stdint.h>

// Declaração antecipada (forward declaration).
struct DadosProva;

struct __attribute__((packed)) TelemetriaPacket {
    uint32_t timestamp_ms;      // 4 bytes
    uint8_t labirinto_id;       // 1 byte
    uint8_t status_id;          // 1 byte
    uint8_t desafio_cumprido;   // 1 byte
    uint8_t bateria_pct;        // 1 byte
    uint16_t bateria_mv;        // 2 bytes
    int16_t velocidade_atual;   // 2 bytes
    int16_t velocidade_media;   // 2 bytes
    uint8_t coluna;             // 1 byte
    uint8_t linha;              // 1 byte
    uint8_t orientacao;         // 1 byte
    uint8_t paredes;            // 1 byte
}; // Total: 18 bytes

TelemetriaPacket montarPacote(const DadosProva& d, int16_t velAtualMmS, int16_t velMediaMmS, uint16_t bateriaMv, uint8_t bateriaPct);

#endif // PACOTE_H