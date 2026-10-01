#include <unity.h>
#include "../src/mapa/Mapa.h"

void setUp(void) {
    // Configuração antes de cada teste
}

void tearDown(void) {
    // Limpeza após cada teste
}

void test_mapa_default_geometry(void) {
    Mapa mapa;
    
    // Testa estado inicial que deve ser 4x4
    TEST_ASSERT_EQUAL(LAB_4X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(4, mapa.getMaxX());
    TEST_ASSERT_EQUAL(4, mapa.getMaxY());
}

void test_mapa_change_to_8x4(void) {
    Mapa mapa;
    
    // Troca para 8x4 e verifica
    mapa.setGeometria(LAB_8X4);
    TEST_ASSERT_EQUAL(LAB_8X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(8, mapa.getMaxX());
    TEST_ASSERT_EQUAL(4, mapa.getMaxY());
}

void test_mapa_change_to_12x4(void) {
    Mapa mapa;
    
    // Troca para 12x4 e verifica
    mapa.setGeometria(LAB_12X4);
    TEST_ASSERT_EQUAL(LAB_12X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(12, mapa.getMaxX());
    TEST_ASSERT_EQUAL(4, mapa.getMaxY());
}

void test_mapa_cycle_geometry(void) {
    Mapa mapa;
    
    // Testa transições
    mapa.setGeometria(LAB_8X4);
    mapa.setGeometria(LAB_12X4);
    mapa.setGeometria(LAB_4X4);
    
    // Ao final deve retornar a 4x4
    TEST_ASSERT_EQUAL(LAB_4X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(4, mapa.getMaxX());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_mapa_default_geometry);
    RUN_TEST(test_mapa_change_to_8x4);
    RUN_TEST(test_mapa_change_to_12x4);
    RUN_TEST(test_mapa_cycle_geometry);
    return UNITY_END();
}
