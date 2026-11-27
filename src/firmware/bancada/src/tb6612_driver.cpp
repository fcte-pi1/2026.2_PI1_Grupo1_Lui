#include "tb6612_driver.h"
#include "pinout.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_arduino_version.h>

static bool s_invert_left = false;
static bool s_invert_right = false;

static inline void apply_pwm_channel(uint8_t pin, uint8_t ledc_ch, uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    (void)ledc_ch;
    ledcWrite(pin, duty);
#else
    (void)pin;
    ledcWrite(ledc_ch, duty);
#endif
}

static void apply_motor_channel(uint8_t in1, uint8_t in2, uint8_t pwm_pin, uint8_t ledc_ch,
                                int16_t speed, bool invert) {
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;
    if (invert) speed = -speed;

    if (speed > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        apply_pwm_channel(pwm_pin, ledc_ch, (uint8_t)speed);
    } else if (speed < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        apply_pwm_channel(pwm_pin, ledc_ch, (uint8_t)(-speed));
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        apply_pwm_channel(pwm_pin, ledc_ch, 0);
    }
}

void tb6612_init(void) {
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);
    pinMode(PIN_STBY, OUTPUT);

    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    digitalWrite(PIN_STBY, HIGH);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(PIN_PWMA, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttach(PIN_PWMB, PWM_FREQ_HZ, PWM_RESOLUTION);
#else
    ledcSetup(LEDC_CH_PWMA, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWMA, LEDC_CH_PWMA);
    ledcSetup(LEDC_CH_PWMB, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWMB, LEDC_CH_PWMB);
#endif
    apply_pwm_channel(PIN_PWMA, LEDC_CH_PWMA, 0);
    apply_pwm_channel(PIN_PWMB, LEDC_CH_PWMB, 0);
}

void tb6612_set_motor_left(int16_t speed) {
    apply_motor_channel(PIN_AIN1, PIN_AIN2, PIN_PWMA, LEDC_CH_PWMA, speed, s_invert_left);
}

void tb6612_set_motor_right(int16_t speed) {
    apply_motor_channel(PIN_BIN1, PIN_BIN2, PIN_PWMB, LEDC_CH_PWMB, speed, s_invert_right);
}

void tb6612_set_motors(int16_t speed_left, int16_t speed_right) {
    tb6612_set_motor_left(speed_left);
    tb6612_set_motor_right(speed_right);
}

void tb6612_brake(void) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    apply_pwm_channel(PIN_PWMA, LEDC_CH_PWMA, 255);

    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    apply_pwm_channel(PIN_PWMB, LEDC_CH_PWMB, 255);
}

void tb6612_stop(void) {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    apply_pwm_channel(PIN_PWMA, LEDC_CH_PWMA, 0);

    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    apply_pwm_channel(PIN_PWMB, LEDC_CH_PWMB, 0);
}

void tb6612_set_standby(bool standby_active) {
    digitalWrite(PIN_STBY, standby_active ? LOW : HIGH);
}

void tb6612_set_inversion(bool invert_left, bool invert_right) {
    s_invert_left = invert_left;
    s_invert_right = invert_right;
}

#else
static int16_t s_speed_l = 0;
static int16_t s_speed_r = 0;
static bool s_standby = false;
static bool s_invert_l = false;
static bool s_invert_r = false;

void tb6612_init(void) { s_speed_l = 0; s_speed_r = 0; s_standby = false; }
void tb6612_set_motor_left(int16_t speed) { s_speed_l = s_invert_l ? -speed : speed; }
void tb6612_set_motor_right(int16_t speed) { s_speed_r = s_invert_r ? -speed : speed; }
void tb6612_set_motors(int16_t l, int16_t r) { tb6612_set_motor_left(l); tb6612_set_motor_right(r); }
void tb6612_brake(void) { s_speed_l = 0; s_speed_r = 0; }
void tb6612_stop(void) { s_speed_l = 0; s_speed_r = 0; }
void tb6612_set_standby(bool standby_active) { s_standby = standby_active; }
void tb6612_set_inversion(bool invert_left, bool invert_right) { s_invert_l = invert_left; s_invert_r = invert_right; }
#endif
