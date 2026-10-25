#include "unity_compat.h"
#include "../firmware/include/telemetry_gateway_protocol.h"

void test_18_bytes_size_and_packing(void) {
    TEST_ASSERT_EQUAL(18, sizeof(TelemetryPacket18_t));
}

void test_encoding_and_crc_validation(void) {
    TelemetryPacket18_t pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.seq_number = 105;
    pkt.timestamp_s = 45;
    pkt.pos_x = 3;
    pkt.pos_y = 7;
    pkt.heading_state = (1 << 4) | 2; // Heading 1 (EAST), State 2 (EXPLORING)
    pkt.walls_mask = 0x05; // Paredes Norte e Sul
    pkt.dist_left_cm = 8;
    pkt.dist_front_cm = 25;
    pkt.dist_right_cm = 8;
    pkt.imu_yaw_deci_deg = 900; // 90.0 graus
    pkt.vbat_mv = 7600; // 7.60 V

    telemetry_encode_18b(&pkt);

    TEST_ASSERT_EQUAL(0xA5, pkt.header);
    TEST_ASSERT_TRUE(telemetry_decode_and_validate_18b(&pkt));

    // Corrompe 1 byte e assegura rejeicao
    pkt.dist_front_cm ^= 0xFF;
    TEST_ASSERT_FALSE(telemetry_decode_and_validate_18b(&pkt));
}

void test_json_conversion_contract(void) {
    TelemetryPacket18_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.seq_number = 42;
    pkt.timestamp_s = 10;
    pkt.pos_x = 0;
    pkt.pos_y = 1;
    pkt.heading_state = (0 << 4) | 1; // Heading NORTH, State READY
    pkt.walls_mask = 0x08;
    pkt.dist_left_cm = 12;
    pkt.dist_front_cm = 40;
    pkt.dist_right_cm = 12;
    pkt.imu_yaw_deci_deg = 0;
    pkt.vbat_mv = 8100;

    telemetry_encode_18b(&pkt);

    char json_buffer[256];
    int len = telemetry_to_json(&pkt, -63, json_buffer, sizeof(json_buffer));

    TEST_ASSERT_TRUE(len > 0);
    // Verifica campos obrigatorios no JSON
    TEST_ASSERT_TRUE(strstr(json_buffer, "\"seq\":42") != NULL);
    TEST_ASSERT_TRUE(strstr(json_buffer, "\"rssi\":-63") != NULL);
    TEST_ASSERT_TRUE(strstr(json_buffer, "\"timestamp_ms\":10000") != NULL);
    TEST_ASSERT_TRUE(strstr(json_buffer, "\"battery_v\":8.10") != NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_18_bytes_size_and_packing);
    RUN_TEST(test_encoding_and_crc_validation);
    RUN_TEST(test_json_conversion_contract);
    return UNITY_END();
}
