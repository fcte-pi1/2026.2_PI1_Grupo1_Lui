#include "unity_compat.h"
#include "../firmware/include/power_voltage_monitor.h"

void test_pin_and_divider_ratio(void) {
    TEST_ASSERT_EQUAL(34, PIN_ADC_BATTERY);
    // Ratio = (100k + 47k) / 47k = 147 / 47 = 3.127659
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.128f, DIVIDER_RATIO);
}

void test_adc_to_battery_voltage_conversion(void) {
    // Para Vref = 3.3V:
    // Bateria a 7.4V -> V_adc = 7.4 / 3.127659 = 2.3659V -> ADC = (2.3659 / 3.3) * 4095 = 2936
    float vbat = adc_to_battery_voltage(2936, 3.3f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 7.40f, vbat);

    // Bateria plena a 8.4V -> ADC ~ 3333
    vbat = adc_to_battery_voltage(3333, 3.3f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 8.40f, vbat);

    // Bateria descarregada 6.6V -> ADC ~ 2619
    vbat = adc_to_battery_voltage(2619, 3.3f);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 6.60f, vbat);
}

void test_power_rail_stability_tolerance_strict_5_percent(void) {
    // 1. Em repouso: 3.31V e 5.02V -> Aprovado
    VoltageStatus_t s1 = evaluate_power_rails(3.31f, 5.02f, 7.8f);
    TEST_ASSERT_TRUE(s1.is_3v3_stable);
    TEST_ASSERT_TRUE(s1.is_5v0_stable);
    TEST_ASSERT_FALSE(s1.is_low_battery);

    // 2. Partida dos motores: pequeno transitório para 3.20V e 4.88V -> Dentro de +/- 5%
    VoltageStatus_t s2 = evaluate_power_rails(3.20f, 4.88f, 7.6f);
    TEST_ASSERT_TRUE(s2.is_3v3_stable);
    TEST_ASSERT_TRUE(s2.is_5v0_stable);

    // 3. Queda grave de transitório abaixo de -5% (ex: 3.05V) -> Falha de estabilidade
    VoltageStatus_t s3 = evaluate_power_rails(3.05f, 4.60f, 7.4f);
    TEST_ASSERT_FALSE(s3.is_3v3_stable);
    TEST_ASSERT_FALSE(s3.is_5v0_stable);
}

void test_battery_cutoff_and_percentage(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, battery_calc_percentage(8.40f));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, battery_calc_percentage(6.60f));
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 44.4f, battery_calc_percentage(7.40f));

    VoltageStatus_t s_cut = evaluate_power_rails(3.30f, 5.00f, 6.55f);
    TEST_ASSERT_TRUE(s_cut.is_critical_cutoff);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pin_and_divider_ratio);
    RUN_TEST(test_adc_to_battery_voltage_conversion);
    RUN_TEST(test_power_rail_stability_tolerance_strict_5_percent);
    RUN_TEST(test_battery_cutoff_and_percentage);
    return UNITY_END();
}
