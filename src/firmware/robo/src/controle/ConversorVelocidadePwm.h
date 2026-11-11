#ifndef CONVERSOR_VELOCIDADE_PWM_H
#define CONVERSOR_VELOCIDADE_PWM_H

#include <stdint.h>
#include "../hardware/Pinos.h"

// Conversão da velocidade desejada de uma roda (mm/s, saída do ControleReta e do
// ControleCurva) em PWM da ponte H (-255 a 255), sem dependência do Arduino
// (docs/firmware/integracao-loop.md, achado B6).
//
// Modelo em malha aberta, linear com zona morta:
//   |PWM| = pwmMinimo + |v| x pwmPorMmS, saturado em pwmMaximo, com o sinal de v.
// Abaixo de velocidadeNulaMmS a saída é 0 (roda parada). Por construção, o conversor nunca
// emite PWM entre 1 e pwmMinimo - 1: um comando que não vence a zona morta deixaria a roda
// parada com tração ativa, e o monitor de travamento cortaria os motores em 1 s (achado R1).
//
// Os valores padrão (hardware/Pinos.h) são provisórios e saem da rampa de PWM da bancada.
// Se sobrar tempo, um PI de velocidade por roda sobre a Odometria pode entrar entre o
// controle e este conversor sem mudar esta interface.

struct ParametrosConversorPwm {
    uint8_t pwmMinimo;        // PWM que vence a zona morta
    float pwmPorMmS;          // inclinação: PWM a mais por mm/s
    uint8_t pwmMaximo;        // saturação
    float velocidadeNulaMmS;  // |v| abaixo disso é tratado como 0
};

class ConversorVelocidadePwm {
public:
    static ParametrosConversorPwm parametrosPadrao() {
        ParametrosConversorPwm p;
        p.pwmMinimo = PWM_MINIMO;      // hardware/Pinos.h, a calibrar
        p.pwmPorMmS = PWM_POR_MM_S;    // hardware/Pinos.h, a calibrar
        p.pwmMaximo = PWM_MAXIMO;
        p.velocidadeNulaMmS = 0.5f;
        return p;
    }

    explicit ConversorVelocidadePwm(const ParametrosConversorPwm& p = parametrosPadrao())
        : p_(p) {}

    int16_t converter(float velocidadeMmS) const {
        const bool negativa = velocidadeMmS < 0.0f;
        const float modulo = negativa ? -velocidadeMmS : velocidadeMmS;
        if (modulo < p_.velocidadeNulaMmS) return 0;

        float pwm = (float)p_.pwmMinimo + modulo * p_.pwmPorMmS;
        if (pwm > (float)p_.pwmMaximo) pwm = (float)p_.pwmMaximo;
        int16_t inteiro = (int16_t)(pwm + 0.5f);
        if (inteiro < p_.pwmMinimo) inteiro = p_.pwmMinimo;  // nunca dentro da zona morta
        if (inteiro > p_.pwmMaximo) inteiro = p_.pwmMaximo;
        return negativa ? (int16_t)-inteiro : inteiro;
    }

    const ParametrosConversorPwm& parametros() const { return p_; }

private:
    ParametrosConversorPwm p_;
};

#endif
