#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TELEMETRY_HEADER 0xA5

/**
 * ==============================================================================
 * PACOTE DE TELEMETRIA COMPACTO DE 18 BYTES (Issue #140 / EAP 3.2.1.1)
 * ==============================================================================
 * 1. header          (1B):  0xA5
 * 2. seq_number      (2B):  uint16_t (0 a 65535)
 * 3. timestamp_s     (2B):  uint16_t (tempo de corrida em segundos)
 * 4. pos_x           (1B):  uint8_t  (0 a 15)
 * 5. pos_y           (1B):  uint8_t  (0 a 15)
 * 6. heading_state   (1B):  4 bits heading (N,L,S,O), 4 bits RobotState
 * 7. walls_mask      (1B):  bit 0=N, 1=L, 2=S, 3=O
 * 8. dist_left_cm    (1B):  uint8_t (0 a 255 cm)
 * 9. dist_front_cm   (1B):  uint8_t (0 a 255 cm)
 * 10. dist_right_cm  (1B):  uint8_t (0 a 255 cm)
 * 11. imu_yaw_deg    (2B):  int16_t (graus * 10, ex: 900 = 90.0°)
 * 12. vbat_mv        (2B):  uint16_t (ex: 7400 = 7.4V)
 * 13. crc16          (2B):  uint16_t (CRC-CCITT dos primeiros 16 bytes)
 * Total: 18 Bytes
 * ==============================================================================
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  header;          // Offset 0  (1B)
    uint16_t seq_number;      // Offset 1  (2B)
    uint16_t timestamp_s;     // Offset 3  (2B)
    uint8_t  pos_x;           // Offset 5  (1B)
    uint8_t  pos_y;           // Offset 6  (1B)
    uint8_t  heading_state;   // Offset 7  (1B) [Bits 0-3: state, Bits 4-7: heading]
    uint8_t  walls_mask;      // Offset 8  (1B)
    uint8_t  dist_left_cm;    // Offset 9  (1B)
    uint8_t  dist_front_cm;   // Offset 10 (1B)
    uint8_t  dist_right_cm;   // Offset 11 (1B)
    int16_t  imu_yaw_deci_deg;// Offset 12 (2B)
    uint16_t vbat_mv;         // Offset 14 (2B)
    uint16_t crc16;           // Offset 16 (2B)
} TelemetryPacket18_t;
#pragma pack(pop)

// Verificacao estatica do tamanho de 18 bytes
typedef char check_telemetry18_size[sizeof(TelemetryPacket18_t) == 18 ? 1 : -1];

/**
 * @brief Calcula CRC-16-CCITT (Polinomio 0x1021).
 */
static inline uint16_t telemetry_crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; ++b) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/**
 * @brief Finaliza a codificacao do pacote de 18 bytes inserindo header e CRC16.
 */
static inline void telemetry_encode_18b(TelemetryPacket18_t* pkt) {
    if (!pkt) return;
    pkt->header = TELEMETRY_HEADER;
    pkt->crc16 = telemetry_crc16((const uint8_t*)pkt, sizeof(TelemetryPacket18_t) - 2);
}

/**
 * @brief Valida integridade do pacote de 18 bytes.
 */
static inline bool telemetry_decode_and_validate_18b(const TelemetryPacket18_t* pkt) {
    if (!pkt || pkt->header != TELEMETRY_HEADER) return false;
    uint16_t expected_crc = telemetry_crc16((const uint8_t*)pkt, sizeof(TelemetryPacket18_t) - 2);
    return (pkt->crc16 == expected_crc);
}

/**
 * @brief Converte o pacote decodificado em JSON compativel com contrato EAP 5.3.1.1.
 */
static inline int telemetry_to_json(const TelemetryPacket18_t* pkt, int8_t rssi, char* buffer, size_t max_len) {
    if (!pkt || !buffer || max_len == 0) return 0;

    uint8_t state = pkt->heading_state & 0x0F;
    uint8_t heading = (pkt->heading_state >> 4) & 0x0F;
    float yaw = (float)pkt->imu_yaw_deci_deg / 10.0f;
    float vbat = (float)pkt->vbat_mv / 1000.0f;

    return snprintf(buffer, max_len,
        "{\"seq\":%u,\"timestamp_ms\":%u,\"rssi\":%d,\"pos\":{\"x\":%u,\"y\":%u},"
        "\"heading\":%u,\"state\":%u,\"walls\":%u,"
        "\"sensors_cm\":{\"left\":%u,\"front\":%u,\"right\":%u},"
        "\"imu_yaw\":%.1f,\"battery_v\":%.2f}",
        pkt->seq_number,
        (uint32_t)pkt->timestamp_s * 1000,
        rssi,
        pkt->pos_x, pkt->pos_y,
        heading, state, pkt->walls_mask,
        pkt->dist_left_cm, pkt->dist_front_cm, pkt->dist_right_cm,
        yaw, vbat
    );
}

#ifdef __cplusplus
}
#endif
