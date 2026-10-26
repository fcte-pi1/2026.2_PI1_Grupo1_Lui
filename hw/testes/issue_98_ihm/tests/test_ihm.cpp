#include "unity_compat.h"
#include "../firmware/include/ihm_driver.h"

void test_ihm_pin_definitions(void) {
    TEST_ASSERT_EQUAL(15, PIN_BUTTON_START);
    TEST_ASSERT_EQUAL(2, PIN_RGB_LED);
    TEST_ASSERT_EQUAL(4, PIN_BUZZER);
}

void test_five_operational_modes_visual_patterns(void) {
    // 1. Calibracao -> Azul
    VisualPattern_t p1 = ihm_get_visual_pattern(STATE_OP_CALIBRATING);
    TEST_ASSERT_EQUAL(0, p1.r);
    TEST_ASSERT_EQUAL(0, p1.g);
    TEST_ASSERT_EQUAL(255, p1.b);
    TEST_ASSERT_FALSE(p1.blink);

    // 2. Pronto -> Verde
    VisualPattern_t p2 = ihm_get_visual_pattern(STATE_OP_READY);
    TEST_ASSERT_EQUAL(0, p2.r);
    TEST_ASSERT_EQUAL(255, p2.g);
    TEST_ASSERT_EQUAL(0, p2.b);
    TEST_ASSERT_FALSE(p2.blink);

    // 3. Navegando -> Amarelo
    VisualPattern_t p3 = ihm_get_visual_pattern(STATE_OP_NAVIGATING);
    TEST_ASSERT_TRUE(p3.r > 200 && p3.g > 150 && p3.b == 0);
    TEST_ASSERT_FALSE(p3.blink);

    // 4. Objetivo Alcancado -> Roxo
    VisualPattern_t p4 = ihm_get_visual_pattern(STATE_OP_GOAL_REACHED);
    TEST_ASSERT_TRUE(p4.r > 100 && p4.b > 200);
    TEST_ASSERT_FALSE(p4.blink);

    // 5. Falha / Bateria Fraca -> Vermelho piscante
    VisualPattern_t p5 = ihm_get_visual_pattern(STATE_OP_FAULT_LOW_BAT);
    TEST_ASSERT_EQUAL(255, p5.r);
    TEST_ASSERT_EQUAL(0, p5.g);
    TEST_ASSERT_EQUAL(0, p5.b);
    TEST_ASSERT_TRUE(p5.blink);
}

void test_button_debounce_filter(void) {
    bool current_state = false;
    uint32_t last_toggle_ms = 100;

    // Transiente rapido de ruido de 10 ms (bounce) -> Deve ser rejeitado
    bool confirmed = ihm_debounce_filter(true, 110, &last_toggle_ms, &current_state);
    TEST_ASSERT_FALSE(confirmed);
    TEST_ASSERT_FALSE(current_state);

    // Pressao estavel apos 55 ms (> 50 ms) -> Deve ser confirmado
    confirmed = ihm_debounce_filter(true, 160, &last_toggle_ms, &current_state);
    TEST_ASSERT_TRUE(confirmed);
    TEST_ASSERT_TRUE(current_state);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ihm_pin_definitions);
    RUN_TEST(test_five_operational_modes_visual_patterns);
    RUN_TEST(test_button_debounce_filter);
    return UNITY_END();
}
