#include "unity_compat.h"
#include "../firmware/include/espnow_link_driver.h"

void test_packet_structure_and_crc(void) {
    EspNowPacket_t packet;
    memset(&packet, 0, sizeof(packet));
    packet.sequence_number = 42;
    packet.timestamp_ms = 123456;
    packet.tx_power = 20;

    uint16_t crc1 = espnow_calc_crc((const uint8_t*)&packet, sizeof(packet) - 2);
    packet.checksum = crc1;

    // Altera dado e verifica violacao de CRC
    packet.sequence_number = 43;
    uint16_t crc2 = espnow_calc_crc((const uint8_t*)&packet, sizeof(packet) - 2);
    TEST_ASSERT_TRUE(crc1 != crc2);
}

void test_loss_ratio_and_compliance(void) {
    LinkStatistics_t stats;

    // Caso 1: 1000 enviados, 995 recebidos (5 perdidos -> 0.5% de perda, RSSI -62 dBm)
    espnow_calc_stats(1000, 995, -61690.0f, &stats);
    TEST_ASSERT_EQUAL(1000, stats.total_sent);
    TEST_ASSERT_EQUAL(995, stats.total_received);
    TEST_ASSERT_EQUAL(5, stats.lost_packets);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, stats.loss_percent);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, -62.0f, stats.avg_rssi);
    TEST_ASSERT_TRUE(espnow_is_link_compliant(stats.loss_percent, stats.avg_rssi));

    // Caso 2: Limite RNF-ELE03 (18 perdidos -> 1.8% de perda, RSSI -72 dBm) -> APROVADO
    espnow_calc_stats(1000, 982, -70704.0f, &stats);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.8f, stats.loss_percent);
    TEST_ASSERT_TRUE(espnow_is_link_compliant(stats.loss_percent, stats.avg_rssi));

    // Caso 3: Reprovado por perda excessiva (25 perdidos -> 2.5% perda) -> REPROVADO
    espnow_calc_stats(1000, 975, -71000.0f, &stats);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.5f, stats.loss_percent);
    TEST_ASSERT_FALSE(espnow_is_link_compliant(stats.loss_percent, stats.avg_rssi));

    // Caso 4: Reprovado por RSSI fraco (-78 dBm) -> REPROVADO
    espnow_calc_stats(1000, 990, -77220.0f, &stats);
    TEST_ASSERT_FALSE(espnow_is_link_compliant(stats.loss_percent, stats.avg_rssi));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_packet_structure_and_crc);
    RUN_TEST(test_loss_ratio_and_compliance);
    return UNITY_END();
}
