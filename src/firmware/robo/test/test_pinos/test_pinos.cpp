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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nenhum_pino_aparece_duas_vezes);
    RUN_TEST(test_todos_os_pinos_estao_na_lista_em_uso);
    RUN_TEST(test_pinos_da_pinagem_da_eletronica);
    RUN_TEST(test_constantes_fisicas);
    return UNITY_END();
}
