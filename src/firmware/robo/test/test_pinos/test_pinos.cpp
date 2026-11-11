#include <stdint.h>
#include <unity.h>
#include "../../src/hardware/Pinos.h"

void setUp(void) {}
void tearDown(void) {}

// Todos os pinos declarados em Pinos.h. Ao acrescentar um pino no arquivo, acrescente-o
// aqui e em PINOS_EM_USO.
static const uint8_t TODOS_OS_PINOS[] = {
    PINO_PONTE_PWMA, PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_STBY,
    PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB,
    PINO_ENCODER_ESQ_A, PINO_ENCODER_ESQ_B, PINO_ENCODER_DIR_A, PINO_ENCODER_DIR_B,
    PINO_I2C_SDA, PINO_I2C_SCL,
    PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR,
    PINO_BOTAO, PINO_LED_ESTADO, PINO_BATERIA_ADC,
};
static const uint8_t QTD_TODOS = sizeof(TODOS_OS_PINOS) / sizeof(TODOS_OS_PINOS[0]);

static bool emUso(uint8_t pino) {
    for (uint8_t i = 0; i < PINOS_EM_USO_QTD; i++) {
        if (PINOS_EM_USO[i] == pino) return true;
    }
    return false;
}

void test_nenhum_pino_aparece_duas_vezes(void) {
    for (uint8_t i = 0; i < PINOS_EM_USO_QTD; i++) {
        for (uint8_t j = i + 1; j < PINOS_EM_USO_QTD; j++) {
            TEST_ASSERT_NOT_EQUAL_MESSAGE(PINOS_EM_USO[i], PINOS_EM_USO[j], "GPIO usado duas vezes em Pinos.h");
        }
    }
}

void test_todos_os_pinos_estao_na_lista_em_uso(void) {
    TEST_ASSERT_EQUAL_UINT8(QTD_TODOS, PINOS_EM_USO_QTD);
    for (uint8_t i = 0; i < QTD_TODOS; i++) {
        TEST_ASSERT_TRUE_MESSAGE(emUso(TODOS_OS_PINOS[i]), "pino de Pinos.h fora de PINOS_EM_USO");
    }
}

void test_pinos_da_pinagem_da_eletronica(void) {
    // Pinos que mudaram em relação ao firmware anterior (LED no GPIO 2).
    TEST_ASSERT_EQUAL_UINT8(2, PINO_ENCODER_DIR_B);
    TEST_ASSERT_EQUAL_UINT8(14, PINO_LED_ESTADO);
    TEST_ASSERT_EQUAL_UINT8(15, PINO_BOTAO);
    TEST_ASSERT_EQUAL_UINT8(34, PINO_BATERIA_ADC);
}

void test_constantes_fisicas(void) {
    TEST_ASSERT_EQUAL_FLOAT(32.0f, RODA_DIAMETRO_MM);
    TEST_ASSERT_EQUAL_UINT16(360, ENCODER_PULSOS_POR_VOLTA);
    TEST_ASSERT_EQUAL_FLOAT(80.0f, BITOLA_MM);
    // Uma volta da roda (360 pulsos) percorre a circunferência, pi x 32 = 100,53 mm.
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.53f, RODA_MM_POR_PULSO * ENCODER_PULSOS_POR_VOLTA);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.2793f, RODA_MM_POR_PULSO);
}

void test_celula_do_regulamento(void) {
    TEST_ASSERT_EQUAL_UINT16(180, CELULA_MM);
    TEST_ASSERT_EQUAL_UINT16(168, VAO_CORREDOR_MM);
    TEST_ASSERT_EQUAL_UINT16(12, CELULA_MM - VAO_CORREDOR_MM);  // espessura da parede
}

void test_pwm_da_ponte_h(void) {
    TEST_ASSERT_EQUAL_UINT8((1u << PONTE_PWM_BITS) - 1, PWM_MAXIMO);
    TEST_ASSERT_TRUE(PWM_MINIMO > 0);
    TEST_ASSERT_TRUE(PWM_MINIMO < PWM_MAXIMO);
    TEST_ASSERT_TRUE(PWM_POR_MM_S > 0.0f);
    TEST_ASSERT_EQUAL_UINT32(20000, PONTE_PWM_FREQ_HZ);
}

void test_montagem_sem_inversao_ate_a_bancada(void) {
    // Valores iniciais; a conferência é o primeiro dia de bancada (R7).
    TEST_ASSERT_FALSE(MOTOR_ESQ_INVERTIDO);
    TEST_ASSERT_FALSE(MOTOR_DIR_INVERTIDO);
    TEST_ASSERT_FALSE(ENCODER_ESQ_INVERTIDO);
    TEST_ASSERT_FALSE(ENCODER_DIR_INVERTIDO);
}

void test_sensores_tof(void) {
    // Endereços distintos entre si e do endereço de fábrica (0x29), como no hw/firm.
    TEST_ASSERT_EQUAL_HEX8(0x30, TOF_ENDERECO_ESQ);
    TEST_ASSERT_EQUAL_HEX8(0x31, TOF_ENDERECO_FRONTAL);
    TEST_ASSERT_EQUAL_HEX8(0x32, TOF_ENDERECO_DIR);
    // Uma amostra a cada ~33 ms: a validade cobre ao menos duas amostras perdidas.
    TEST_ASSERT_TRUE(TOF_VALIDADE_MS >= 3 * (TOF_ORCAMENTO_US / 1000));
    // O timeout da biblioteca é só a segunda barreira e fica bem abaixo dos 500 ms do hw/firm.
    TEST_ASSERT_TRUE(TOF_TIMEOUT_BIBLIOTECA_MS < TOF_VALIDADE_MS);
    TEST_ASSERT_TRUE(TOF_PERIODO_CONSULTA_MS < TOF_ORCAMENTO_US / 1000);
    // Pior caso com erro de I2C: três sensores esperando o timeout a cada período de espera
    // ainda deixa a maior parte do tempo para o laço.
    TEST_ASSERT_TRUE(3 * TOF_I2C_TIMEOUT_MS < TOF_ESPERA_APOS_ERRO_MS / 2);
}

void test_limiar_de_parede(void) {
    TEST_ASSERT_EQUAL_UINT16(110, LIMIAR_PAREDE_MM);
    TEST_ASSERT_EQUAL_UINT16(5, HISTERESE_PAREDE_MM);
}

void test_led_de_estado(void) {
    TEST_ASSERT_EQUAL(LED_WS2812B, TIPO_LED_ESTADO);  // provisório (R5)
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nenhum_pino_aparece_duas_vezes);
    RUN_TEST(test_todos_os_pinos_estao_na_lista_em_uso);
    RUN_TEST(test_pinos_da_pinagem_da_eletronica);
    RUN_TEST(test_constantes_fisicas);
    RUN_TEST(test_celula_do_regulamento);
    RUN_TEST(test_pwm_da_ponte_h);
    RUN_TEST(test_montagem_sem_inversao_ate_a_bancada);
    RUN_TEST(test_sensores_tof);
    RUN_TEST(test_limiar_de_parede);
    RUN_TEST(test_led_de_estado);
    return UNITY_END();
}
