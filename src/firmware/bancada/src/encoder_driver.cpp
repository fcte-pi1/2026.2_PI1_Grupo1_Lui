#include "encoder_driver.h"
#include "pinout.h"

#ifdef ARDUINO
#include <Arduino.h>

static volatile int32_t s_ticks_left = 0;
static volatile int32_t s_ticks_right = 0;

static bool s_invert_l = false;
static bool s_invert_r = false;

// Rotina de Interrupção de Alta Prioridade - Encoder Esquerdo (Canal A no GPIO 36)
void IRAM_ATTR isr_encoder_left(void) {
    // Leitura rápida do Canal B (GPIO 39 - VN)
    int step = digitalRead(PIN_ENC1_B) ? -1 : 1;
    if (s_invert_l) step = -step;
    s_ticks_left += step;
}

// Rotina de Interrupção de Alta Prioridade - Encoder Direito (Canal A no GPIO 35)
void IRAM_ATTR isr_encoder_right(void) {
    // Leitura rápida do Canal B (GPIO 2)
    int step = digitalRead(PIN_ENC2_B) ? 1 : -1;
    if (s_invert_r) step = -step;
    s_ticks_right += step;
}

void encoder_init(void) {
    // GPIO 36 (VP), GPIO 39 (VN) e GPIO 35 são pinos INPUT ONLY puros (sem resistores internos)
    pinMode(PIN_ENC1_A, INPUT);
    pinMode(PIN_ENC1_B, INPUT);
    pinMode(PIN_ENC2_A, INPUT);

    // GPIO 2 possui pull-up interno no silício do ESP32
    pinMode(PIN_ENC2_B, INPUT_PULLUP);

    // Registra interrupções por borda de subida (RISING) no Canal A
    attachInterrupt(digitalPinToInterrupt(PIN_ENC1_A), isr_encoder_left, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC2_A), isr_encoder_right, RISING);

    encoder_reset();
}

int32_t encoder_get_ticks_left(void) {
    noInterrupts();
    int32_t val = s_ticks_left;
    interrupts();
    return val;
}

int32_t encoder_get_ticks_right(void) {
    noInterrupts();
    int32_t val = s_ticks_right;
    interrupts();
    return val;
}

void encoder_reset(void) {
    noInterrupts();
    s_ticks_left = 0;
    s_ticks_right = 0;
    interrupts();
}

float encoder_get_dist_left_mm(void) {
    int32_t ticks = encoder_get_ticks_left();
    return ((float)ticks / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM;
}

float encoder_get_dist_right_mm(void) {
    int32_t ticks = encoder_get_ticks_right();
    return ((float)ticks / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM;
}

void encoder_set_inversion(bool invert_left, bool invert_right) {
    s_invert_l = invert_left;
    s_invert_r = invert_right;
}

#else
// Stubs para compilação e teste unitário nativo
static int32_t s_mock_l = 0;
static int32_t s_mock_r = 0;

void encoder_init(void) { s_mock_l = 0; s_mock_r = 0; }
int32_t encoder_get_ticks_left(void) { return s_mock_l; }
int32_t encoder_get_ticks_right(void) { return s_mock_r; }
void encoder_reset(void) { s_mock_l = 0; s_mock_r = 0; }
float encoder_get_dist_left_mm(void) { return ((float)s_mock_l / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM; }
float encoder_get_dist_right_mm(void) { return ((float)s_mock_r / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM; }
void encoder_set_inversion(bool, bool) {}
#endif
