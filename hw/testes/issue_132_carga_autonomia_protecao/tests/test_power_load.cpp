#include "unity_compat.h"
#include "../firmware/include/power_load_driver.h"

void test_autonomy_calculations_and_requirements(void) {
    // 1. Em repouso (130 mA): Autonomia = (850 * 0.85 / 130) * 60 ~ 333 minutos (~5.5 horas)
    float idle_autonomy = power_calc_autonomy_minutes(BATTERY_CAPACITY_MAH, CURRENT_IDLE_MA);
    TEST_ASSERT_TRUE(idle_autonomy > 300.0f);
    TEST_ASSERT_TRUE(power_is_autonomy_sufficient(idle_autonomy));

    // 2. Em navegacao nominal (420 mA): Autonomia = (850 * 0.85 / 420) * 60 ~ 103 minutos (~1.7 horas)
    float run_autonomy = power_calc_autonomy_minutes(BATTERY_CAPACITY_MAH, CURRENT_NOMINAL_RUN_MA);
    TEST_ASSERT_TRUE(run_autonomy >= 90.0f);
    TEST_ASSERT_TRUE(power_is_autonomy_sufficient(run_autonomy));

    // 3. Em regime extremo (800 mA contínuos): Autonomia = ~54 minutos -> Ainda cumpre >= 15 min (RNF-ENE01)
    float heavy_autonomy = power_calc_autonomy_minutes(BATTERY_CAPACITY_MAH, 800.0f);
    TEST_ASSERT_TRUE(heavy_autonomy >= 15.0f);
    TEST_ASSERT_TRUE(power_is_autonomy_sufficient(heavy_autonomy));
}

void test_protection_fuse_trip_time(void) {
    // Curto-circuito simulado com fusivel atuando em 120 ms (< 500 ms)
    TEST_ASSERT_TRUE(power_is_protection_trip_valid(120));
    TEST_ASSERT_TRUE(power_is_protection_trip_valid(350));
    // Tempo acima de 500 ms reprova
    TEST_ASSERT_FALSE(power_is_protection_trip_valid(550));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_autonomy_calculations_and_requirements);
    RUN_TEST(test_protection_fuse_trip_time);
    return UNITY_END();
}
