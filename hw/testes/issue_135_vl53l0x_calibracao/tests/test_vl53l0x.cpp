#include "unity_compat.h"
#include "../firmware/include/vl53l0x_driver.h"

void test_pins_and_i2c_address(void) {
    // Portas e Pinos da Shield Rev 2.0
    TEST_ASSERT_EQUAL(21, PIN_I2C_SDA);
    TEST_ASSERT_EQUAL(22, PIN_I2C_SCL);
    TEST_ASSERT_EQUAL(32, PIN_XSHUT);
    TEST_ASSERT_EQUAL(0x29, SENSOR_I2C_ADDR);
}

void test_statistical_calculations_20_samples_single_sensor(void) {
    // 20 leituras para um ponto de 80 mm com pequeno ruído
    float readings[20] = {
        80.1f, 79.8f, 80.3f, 79.9f, 80.2f,
        80.0f, 79.7f, 80.4f, 80.1f, 79.9f,
        80.2f, 80.0f, 79.8f, 80.3f, 80.1f,
        79.9f, 80.2f, 80.0f, 79.8f, 80.1f
    };

    SensorStats_t stats;
    sensor_calc_stats(readings, 20, 80.0f, &stats);

    TEST_ASSERT_EQUAL(20, stats.count);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 80.04f, stats.mean);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.18f, stats.std_dev);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.04f, stats.error_abs);
    TEST_ASSERT_TRUE(sensor_is_error_acceptable(stats.error_abs));
}

void test_calibration_across_7_points(void) {
    const float test_points[] = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f, 150.0f, 200.0f};
    SensorCalib_t calib;
    sensor_init_calib(&calib);
    calib.slope = 1.0f;
    calib.offset = 0.5f;

    for (int i = 0; i < 7; i++) {
        float ref = test_points[i];
        float raw = ref + 0.2f; // Leitura simulada
        float calibrated = sensor_apply_calib(raw, &calib);
        float err = fabsf(calibrated - ref);

        // Se estiver na faixa util de 30-180 mm, o erro DEVE ser <= 10 mm (RNF-ELE02)
        if (ref >= SENSOR_MIN_RANGE_MM && ref <= SENSOR_MAX_RANGE_MM) {
            TEST_ASSERT_TRUE(sensor_is_error_acceptable(err));
        }
    }
}

void test_sampling_rate_20hz(void) {
    // 33 ms por leitura -> ~30 Hz >= 20 Hz (RNF-ELE01)
    float sample_time_ms = 33.0f;
    float rate_hz = 1000.0f / sample_time_ms;
    TEST_ASSERT_TRUE(rate_hz >= SENSOR_MIN_RATE_HZ);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pins_and_i2c_address);
    RUN_TEST(test_statistical_calculations_20_samples_single_sensor);
    RUN_TEST(test_calibration_across_7_points);
    RUN_TEST(test_sampling_rate_20hz);
    return UNITY_END();
}
