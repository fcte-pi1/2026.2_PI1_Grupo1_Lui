#include <unity.h>
#include "../../src/navegacao/VelocidadeMedia.h"

void setUp(void) {}
void tearDown(void) {}

void test_zero_antes_da_largada(void) {
    TEST_ASSERT_EQUAL_INT16(0, velocidadeMediaMmS(0, 0));
    TEST_ASSERT_EQUAL_INT16(0, velocidadeMediaMmS(5, 0));  // tempo 0: sem divisão por zero
}

void test_zero_sem_celulas_andadas(void) {
    TEST_ASSERT_EQUAL_INT16(0, velocidadeMediaMmS(0, 15000));
}

void test_valores_tipicos(void) {
    TEST_ASSERT_EQUAL_INT16(180, velocidadeMediaMmS(10, 10000));  // 1 célula por segundo
    TEST_ASSERT_EQUAL_INT16(140, velocidadeMediaMmS(7, 9000));
    TEST_ASSERT_EQUAL_INT16(300, velocidadeMediaMmS(50, 30000));  // 0,30 m/s
}

void test_arredonda_ao_mm_por_s_mais_proximo(void) {
    TEST_ASSERT_EQUAL_INT16(167, velocidadeMediaMmS(13, 14000));  // 167,14
    TEST_ASSERT_EQUAL_INT16(90, velocidadeMediaMmS(1, 2000));
    TEST_ASSERT_EQUAL_INT16(60, velocidadeMediaMmS(1, 3000));
}

void test_fica_no_limite_do_campo_do_pacote(void) {
    TEST_ASSERT_EQUAL_INT16(32767, velocidadeMediaMmS(1000, 1000));  // 180 m/s
    TEST_ASSERT_EQUAL_INT16(32767, velocidadeMediaMmS(65535, 1));
}

void test_valores_grandes_sem_estouro(void) {
    // 65535 células x 180 mm x 1000 não cabe em 32 bits.
    TEST_ASSERT_EQUAL_INT16(19661, velocidadeMediaMmS(65535, 600000));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_antes_da_largada);
    RUN_TEST(test_zero_sem_celulas_andadas);
    RUN_TEST(test_valores_tipicos);
    RUN_TEST(test_arredonda_ao_mm_por_s_mais_proximo);
    RUN_TEST(test_fica_no_limite_do_campo_do_pacote);
    RUN_TEST(test_valores_grandes_sem_estouro);
    return UNITY_END();
}
