#include <stdint.h>
#include <unity.h>
#include "../../src/controle/ConversorVelocidadePwm.h"
#include "../../src/controle/ControleCurva.h"
#include "../../src/controle/ControleReta.h"

// Testes da conversão de velocidade em PWM (achado B6 de docs/firmware/integracao-loop.md).

void setUp(void) {}
void tearDown(void) {}

static ParametrosConversorPwm parametros(uint8_t minimo, float porMmS, uint8_t maximo) {
    ParametrosConversorPwm p = ConversorVelocidadePwm::parametrosPadrao();
    p.pwmMinimo = minimo;
    p.pwmPorMmS = porMmS;
    p.pwmMaximo = maximo;
    return p;
}

void test_padrao_usa_as_constantes_de_pinos(void) {
    const ParametrosConversorPwm p = ConversorVelocidadePwm::parametrosPadrao();
    TEST_ASSERT_EQUAL_UINT8(PWM_MINIMO, p.pwmMinimo);
    TEST_ASSERT_EQUAL_FLOAT(PWM_POR_MM_S, p.pwmPorMmS);
    TEST_ASSERT_EQUAL_UINT8(PWM_MAXIMO, p.pwmMaximo);
}

void test_velocidade_zero_e_pwm_zero(void) {
    ConversorVelocidadePwm c;
    TEST_ASSERT_EQUAL_INT16(0, c.converter(0.0f));
    TEST_ASSERT_EQUAL_INT16(0, c.converter(0.4f));
    TEST_ASSERT_EQUAL_INT16(0, c.converter(-0.4f));
}

void test_mapa_linear_a_partir_da_zona_morta(void) {
    ConversorVelocidadePwm c(parametros(40, 0.6f, 255));
    TEST_ASSERT_EQUAL_INT16(40 + 60, c.converter(100.0f));
    TEST_ASSERT_EQUAL_INT16(40 + 90, c.converter(150.0f));
    TEST_ASSERT_EQUAL_INT16(46, c.converter(10.0f));
}

void test_sinal_e_preservado(void) {
    ConversorVelocidadePwm c(parametros(40, 0.6f, 255));
    TEST_ASSERT_EQUAL_INT16(-100, c.converter(-100.0f));
    TEST_ASSERT_EQUAL_INT16(-c.converter(37.0f), c.converter(-37.0f));
}

void test_nunca_emite_pwm_dentro_da_zona_morta(void) {
    // Varre de -300 a 300 mm/s em passos de 0,25 mm/s.
    ConversorVelocidadePwm c;
    for (int i = -1200; i <= 1200; i++) {
        const int16_t pwm = c.converter(i * 0.25f);
        const int16_t modulo = pwm < 0 ? (int16_t)-pwm : pwm;
        TEST_ASSERT_TRUE(modulo == 0 || modulo >= PWM_MINIMO);
        TEST_ASSERT_TRUE(modulo <= PWM_MAXIMO);
    }
}

void test_satura_no_maximo(void) {
    ConversorVelocidadePwm c(parametros(40, 0.6f, 255));
    TEST_ASSERT_EQUAL_INT16(255, c.converter(1000.0f));
    TEST_ASSERT_EQUAL_INT16(-255, c.converter(-1000.0f));
    ConversorVelocidadePwm limitado(parametros(40, 0.6f, 120));
    TEST_ASSERT_EQUAL_INT16(120, limitado.converter(150.0f));
}

void test_monotono_na_velocidade(void) {
    ConversorVelocidadePwm c;
    int16_t anterior = 0;
    for (int v = 1; v <= 400; v++) {
        const int16_t pwm = c.converter((float)v);
        TEST_ASSERT_TRUE(pwm >= anterior);
        anterior = pwm;
    }
}

void test_velocidade_minima_da_curva_vence_a_zona_morta(void) {
    // O piso de 12 mm/s do ControleCurva (R1): com o conversor, a roda recebe ao menos
    // PWM_MINIMO e não fica parada com tração ativa.
    ConversorVelocidadePwm c;
    const float minimo = ControleCurva::parametrosPadrao().velocidadeMinMmS;
    TEST_ASSERT_TRUE(c.converter(minimo) >= PWM_MINIMO);
    TEST_ASSERT_TRUE(c.converter(-minimo) <= -(int16_t)PWM_MINIMO);
}

void test_faixa_do_controle_de_reta_cabe_no_pwm(void) {
    // Velocidade base mais a correção máxima ainda fica abaixo da saturação.
    ConversorVelocidadePwm c;
    const ParametrosControleReta r = ControleReta::parametrosPadrao();
    TEST_ASSERT_TRUE(c.converter(r.velocidadeBaseMmS + r.limiteGiroMmS) < PWM_MAXIMO);
    TEST_ASSERT_TRUE(c.converter(r.velocidadeBaseMmS - r.limiteGiroMmS) > PWM_MINIMO);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_padrao_usa_as_constantes_de_pinos);
    RUN_TEST(test_velocidade_zero_e_pwm_zero);
    RUN_TEST(test_mapa_linear_a_partir_da_zona_morta);
    RUN_TEST(test_sinal_e_preservado);
    RUN_TEST(test_nunca_emite_pwm_dentro_da_zona_morta);
    RUN_TEST(test_satura_no_maximo);
    RUN_TEST(test_monotono_na_velocidade);
    RUN_TEST(test_velocidade_minima_da_curva_vence_a_zona_morta);
    RUN_TEST(test_faixa_do_controle_de_reta_cabe_no_pwm);
    return UNITY_END();
}
