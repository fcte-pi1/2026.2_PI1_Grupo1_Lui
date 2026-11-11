#include "drivers/PonteHTb6612.h"
#include <Arduino.h>
#include <esp_arduino_version.h>
#include "hardware/Pinos.h"

// Canais do LEDC usados na API anterior à 3.0 do Arduino-ESP32 (a 3.0 usa o pino).
static const uint8_t CANAL_PWM_ESQ = 0;
static const uint8_t CANAL_PWM_DIR = 1;

static void escreverPwm(uint8_t pino, uint8_t canal, uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    (void)canal;
    ledcWrite(pino, duty);
#else
    (void)pino;
    ledcWrite(canal, duty);
#endif
}

// Uma roda: sentido pelos pinos IN1/IN2 e módulo no PWM.
static void aplicarRoda(uint8_t in1, uint8_t in2, uint8_t pinoPwm, uint8_t canal, int16_t pwm) {
    if (pwm > (int16_t)PWM_MAXIMO) pwm = PWM_MAXIMO;
    if (pwm < -(int16_t)PWM_MAXIMO) pwm = -(int16_t)PWM_MAXIMO;
    if (pwm > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        escreverPwm(pinoPwm, canal, (uint8_t)pwm);
    } else if (pwm < 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        escreverPwm(pinoPwm, canal, (uint8_t)(-pwm));
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        escreverPwm(pinoPwm, canal, 0);
    }
}

void PonteHTb6612::iniciar() {
    pinMode(PINO_PONTE_AIN1, OUTPUT);
    pinMode(PINO_PONTE_AIN2, OUTPUT);
    pinMode(PINO_PONTE_BIN1, OUTPUT);
    pinMode(PINO_PONTE_BIN2, OUTPUT);
    pinMode(PINO_PONTE_STBY, OUTPUT);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(PINO_PONTE_PWMA, PONTE_PWM_FREQ_HZ, PONTE_PWM_BITS);
    ledcAttach(PINO_PONTE_PWMB, PONTE_PWM_FREQ_HZ, PONTE_PWM_BITS);
#else
    ledcSetup(CANAL_PWM_ESQ, PONTE_PWM_FREQ_HZ, PONTE_PWM_BITS);
    ledcAttachPin(PINO_PONTE_PWMA, CANAL_PWM_ESQ);
    ledcSetup(CANAL_PWM_DIR, PONTE_PWM_FREQ_HZ, PONTE_PWM_BITS);
    ledcAttachPin(PINO_PONTE_PWMB, CANAL_PWM_DIR);
#endif

    soltar();
    standby(false);
}

void PonteHTb6612::aplicar(int16_t pwmEsq, int16_t pwmDir) {
    aplicarRoda(PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_PWMA, CANAL_PWM_ESQ, pwmEsq);
    aplicarRoda(PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB, CANAL_PWM_DIR, pwmDir);
}

void PonteHTb6612::frear() {
    digitalWrite(PINO_PONTE_AIN1, HIGH);
    digitalWrite(PINO_PONTE_AIN2, HIGH);
    escreverPwm(PINO_PONTE_PWMA, CANAL_PWM_ESQ, PWM_MAXIMO);
    digitalWrite(PINO_PONTE_BIN1, HIGH);
    digitalWrite(PINO_PONTE_BIN2, HIGH);
    escreverPwm(PINO_PONTE_PWMB, CANAL_PWM_DIR, PWM_MAXIMO);
}

void PonteHTb6612::soltar() {
    aplicar(0, 0);
}

void PonteHTb6612::standby(bool ativo) {
    digitalWrite(PINO_PONTE_STBY, ativo ? LOW : HIGH);
}
