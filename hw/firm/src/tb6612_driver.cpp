#include "tb6612_driver.h"
#include "pinout.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_arduino_version.h>

void tb6612_init(void) {
    // Configura os pinos de sentido como saída
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);
    pinMode(PIN_STBY, OUTPUT);

    // Inicializa os pinos de sentido em nível baixo (motores desligados)
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);

    // Habilita a ponte H no pino STBY (HIGH = Ativo)
    digitalWrite(PIN_STBY, HIGH);

    // Configuração dos canais PWM do ESP32 (LEDC)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(PIN_PWMA, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttach(PIN_PWMB, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcWrite(PIN_PWMA, 0);
    ledcWrite(PIN_PWMB, 0);
#else
    ledcSetup(LEDC_CH_PWMA, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWMA, LEDC_CH_PWMA);
    ledcSetup(LEDC_CH_PWMB, PWM_FREQ_HZ, PWM_RESOLUTION);
    ledcAttachPin(PIN_PWMB, LEDC_CH_PWMB);
    ledcWrite(LEDC_CH_PWMA, 0);
    ledcWrite(LEDC_CH_PWMB, 0);
#endif
}

static inline void apply_pwm_left(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(PIN_PWMA, duty);
#else
    ledcWrite(LEDC_CH_PWMA, duty);
#endif
}

static inline void apply_pwm_right(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(PIN_PWMB, duty);
#else
    ledcWrite(LEDC_CH_PWMB, duty);
#endif
}

void tb6612_set_motor_left(int16_t speed) {
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    if (speed > 0) {
        // Sentido Frente
        digitalWrite(PIN_AIN1, HIGH);
        digitalWrite(PIN_AIN2, LOW);
        apply_pwm_left((uint8_t)speed);
    } else if (speed < 0) {
        // Sentido Ré
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, HIGH);
        apply_pwm_left((uint8_t)(-speed));
    } else {
        // Roda Livre
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, LOW);
        apply_pwm_left(0);
    }
}

void tb6612_set_motor_right(int16_t speed) {
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    if (speed > 0) {
        // Sentido Frente
        digitalWrite(PIN_BIN1, HIGH);
        digitalWrite(PIN_BIN2, LOW);
        apply_pwm_right((uint8_t)speed);
    } else if (speed < 0) {
        // Sentido Ré
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, HIGH);
        apply_pwm_right((uint8_t)(-speed));
    } else {
        // Roda Livre
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, LOW);
        apply_pwm_right(0);
    }
}

void tb6612_set_motors(int16_t speed_left, int16_t speed_right) {
    tb6612_set_motor_left(speed_left);
    tb6612_set_motor_right(speed_right);
}

void tb6612_brake(void) {
    // Frenagem ativa no TB6612FNG: IN1=HIGH e IN2=HIGH com PWM máximo
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    apply_pwm_left(255);

    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    apply_pwm_right(255);
}

void tb6612_stop(void) {
    // Roda livre: IN1=LOW e IN2=LOW com PWM zero
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    apply_pwm_left(0);

    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    apply_pwm_right(0);
}

void tb6612_set_standby(bool standby_active) {
    // LOW = Ponte H em Standby (desligada); HIGH = Ponte H em Operação Normal
    digitalWrite(PIN_STBY, standby_active ? LOW : HIGH);
}

#else
// Stubs para compilação nativa em testes
static int16_t s_speed_l = 0;
static int16_t s_speed_r = 0;
static bool s_standby = false;

void tb6612_init(void) { s_speed_l = 0; s_speed_r = 0; s_standby = false; }
void tb6612_set_motor_left(int16_t speed) { s_speed_l = speed; }
void tb6612_set_motor_right(int16_t speed) { s_speed_r = speed; }
void tb6612_set_motors(int16_t l, int16_t r) { s_speed_l = l; s_speed_r = r; }
void tb6612_brake(void) { s_speed_l = 0; s_speed_r = 0; }
void tb6612_stop(void) { s_speed_l = 0; s_speed_r = 0; }
void tb6612_set_standby(bool standby_active) { s_standby = standby_active; }
#endif
