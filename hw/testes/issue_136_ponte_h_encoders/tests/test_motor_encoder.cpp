#include "unity_compat.h"
#include "../firmware/include/motor_encoder_driver.h"

void test_shield_pin_definitions(void) {
    // Shield Rev 2.0
    TEST_ASSERT_EQUAL(16, PIN_PWMA);
    TEST_ASSERT_EQUAL(17, PIN_PWMB);
    TEST_ASSERT_EQUAL(18, PIN_AIN1);
    TEST_ASSERT_EQUAL(19, PIN_AIN2);
    TEST_ASSERT_EQUAL(23, PIN_BIN1);
    TEST_ASSERT_EQUAL(5, PIN_BIN2);

    TEST_ASSERT_EQUAL(27, PIN_ENC_L_A);
    TEST_ASSERT_EQUAL(26, PIN_ENC_L_B);
    TEST_ASSERT_EQUAL(25, PIN_ENC_R_A);
    TEST_ASSERT_EQUAL(33, PIN_ENC_R_B);
}

void test_tb6612_direction_and_braking(void) {
    bool in1 = false, in2 = false;

    // FORWARD
    tb6612_get_logic_levels(MOTOR_FORWARD, &in1, &in2);
    TEST_ASSERT_TRUE(in1 == true && in2 == false);

    // BACKWARD
    tb6612_get_logic_levels(MOTOR_BACKWARD, &in1, &in2);
    TEST_ASSERT_TRUE(in1 == false && in2 == true);

    // BRAKE (Frenagem ativa: IN1=HIGH, IN2=HIGH)
    tb6612_get_logic_levels(MOTOR_BRAKE, &in1, &in2);
    TEST_ASSERT_TRUE(in1 == true && in2 == true);

    // STOP (Roda livre: IN1=LOW, IN2=LOW)
    tb6612_get_logic_levels(MOTOR_STOP, &in1, &in2);
    TEST_ASSERT_TRUE(in1 == false && in2 == false);
}

void test_pwm_duty_cycle_levels(void) {
    // 25%, 50%, 75%, 100% conforme requerido na Issue #136
    TEST_ASSERT_EQUAL(63, motor_percent_to_pwm(25));
    TEST_ASSERT_EQUAL(127, motor_percent_to_pwm(50));
    TEST_ASSERT_EQUAL(191, motor_percent_to_pwm(75));
    TEST_ASSERT_EQUAL(255, motor_percent_to_pwm(100));
}

void test_quadrature_decoding_and_odometry(void) {
    // Sentido para frente (horario): Canal B=LOW na subida de A
    int32_t step_fwd = encoder_decode_step(true, false);
    TEST_ASSERT_EQUAL(1, step_fwd);

    // Sentido para tras (anti-horario): Canal B=HIGH na subida de A
    int32_t step_rev = encoder_decode_step(true, true);
    TEST_ASSERT_EQUAL(-1, step_rev);

    // 1 volta completa = 360 ticks
    float dist_1_rev = encoder_ticks_to_distance_mm(360);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, WHEEL_CIRCUMFERENCE_MM, dist_1_rev);
}

void test_10_turns_consistency_criteria(void) {
    // 10 voltas esperadas: 10 * 360 = 3600 ticks
    // Caso ideal
    TEST_ASSERT_TRUE(encoder_verify_10_turns(3600, 1.0f));

    // Desvio de 5 ticks (~0.13%) -> aceitavel
    TEST_ASSERT_TRUE(encoder_verify_10_turns(3605, 1.0f));

    // Perda grave de 50 ticks (~1.38%) -> reprovado
    TEST_ASSERT_FALSE(encoder_verify_10_turns(3550, 1.0f));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_shield_pin_definitions);
    RUN_TEST(test_tb6612_direction_and_braking);
    RUN_TEST(test_pwm_duty_cycle_levels);
    RUN_TEST(test_quadrature_decoding_and_odometry);
    RUN_TEST(test_10_turns_consistency_criteria);
    return UNITY_END();
}
