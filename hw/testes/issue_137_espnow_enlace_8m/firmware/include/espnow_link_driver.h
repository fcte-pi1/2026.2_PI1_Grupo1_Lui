#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESPNOW_TOTAL_PACKETS_TEST   1000
#define ESPNOW_MAX_ALLOWED_LOSS     0.02f   // RNF-ELE03: Perda < 2.0%
#define ESPNOW_MIN_ALLOWED_RSSI    -75.0f   // RNF-ELE03: RSSI >= -75 dBm
#define ESPNOW_CHANNEL              1

#pragma pack(push, 1)
typedef struct {
    uint32_t sequence_number; // Numero sequencial de 1 a 1000
    uint32_t timestamp_ms;    // Timestamp do transmissor
    int8_t   tx_power;        // Potencia configurada em dBm
    uint8_t  dummy[10];       // Carga util para teste de estabilidade
    uint16_t checksum;        // CRC16 de integridade
} EspNowPacket_t;
#pragma pack(pop)

typedef struct {
    uint32_t total_sent;
    uint32_t total_received;
    uint32_t lost_packets;
    uint32_t out_of_order;
    float    loss_percent;
    float    avg_rssi;
} LinkStatistics_t;

/**
 * @brief Calcula o CRC16 simples (Polinomio 0x1021).
 */
static inline uint16_t espnow_calc_crc(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/**
 * @brief Calcula estatisticas de perda e conformidade do enlace.
 */
static inline void espnow_calc_stats(uint32_t sent, uint32_t received, float sum_rssi, LinkStatistics_t* stats) {
    if (!stats) return;
    stats->total_sent = sent;
    stats->total_received = received;
    stats->lost_packets = (sent >= received) ? (sent - received) : 0;
    stats->loss_percent = (sent > 0) ? ((float)stats->lost_packets / (float)sent) * 100.0f : 0.0f;
    stats->avg_rssi = (received > 0) ? (sum_rssi / (float)received) : -99.0f;
}

/**
 * @brief Verifica o criterio de aceite RNF-ELE03 (perda < 2.0% e RSSI >= -75 dBm).
 */
static inline bool espnow_is_link_compliant(float loss_percent, float avg_rssi) {
    return (loss_percent < 2.0f && avg_rssi >= ESPNOW_MIN_ALLOWED_RSSI);
}

#ifdef __cplusplus
}
#endif
