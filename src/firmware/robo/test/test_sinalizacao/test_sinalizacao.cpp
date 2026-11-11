#include <stdint.h>
#include <unity.h>
#include "../../src/sinalizacao/PadraoLed.h"

// Testes dos padrões do LED de estado (achado R5 de docs/firmware/integracao-loop.md).

void setUp(void) {}
void tearDown(void) {}

static const EstadoRobo TODOS[] = {AGUARDANDO, MAPEANDO, SPEED_RUN, COLISAO, CONCLUIDO, TIMEOUT, TRAVADO};
static const uint8_t QTD = sizeof(TODOS) / sizeof(TODOS[0]);

static void assertCor(uint8_t r, uint8_t g, uint8_t b, CorLed c) {
    TEST_ASSERT_EQUAL_UINT8(r, c.r);
    TEST_ASSERT_EQUAL_UINT8(g, c.g);
    TEST_ASSERT_EQUAL_UINT8(b, c.b);
}

// Quantos ms o LED comum fica aceso num ciclo de 1 s (amostrado a cada 10 ms).
static uint16_t acesoNoCiclo(EstadoRobo e, bool armada, uint32_t inicio = 0) {
    uint16_t aceso = 0;
    for (uint32_t t = inicio; t < inicio + 1000; t += 10) {
        if (ledComumAceso(e, armada, t)) aceso += 10;
    }
    return aceso;
}

// Número de vezes que o LED comum acende num ciclo de 1 s.
static uint8_t piscadasNoCiclo(EstadoRobo e, bool armada) {
    uint8_t n = 0;
    bool antes = ledComumAceso(e, armada, 990);
    for (uint32_t t = 1000; t < 2000; t += 10) {
        const bool agora = ledComumAceso(e, armada, t);
        if (agora && !antes) n++;
        antes = agora;
    }
    return n;
}

// ---------------------------------------------------------------------------
// WS2812B: as cores de hoje do main.cpp
// ---------------------------------------------------------------------------

void test_cores_de_cada_estado(void) {
    assertCor(0, 255, 0, corDoEstado(AGUARDANDO, false, 0));
    assertCor(255, 255, 255, corDoEstado(AGUARDANDO, true, 0));
    assertCor(0, 255, 255, corDoEstado(SPEED_RUN, false, 0));
    assertCor(128, 0, 128, corDoEstado(CONCLUIDO, false, 0));
    assertCor(255, 0, 0, corDoEstado(TRAVADO, false, 0));
    assertCor(255, 0, 0, corDoEstado(COLISAO, false, 0));
    assertCor(255, 0, 0, corDoEstado(TIMEOUT, false, 0));
}

void test_mapeando_pisca_amarelo_a_cada_500_ms(void) {
    assertCor(0, 0, 0, corDoEstado(MAPEANDO, false, 0));
    assertCor(0, 0, 0, corDoEstado(MAPEANDO, false, 499));
    assertCor(255, 255, 0, corDoEstado(MAPEANDO, false, 500));
    assertCor(255, 255, 0, corDoEstado(MAPEANDO, false, 999));
    assertCor(0, 0, 0, corDoEstado(MAPEANDO, false, 1000));
}

// ---------------------------------------------------------------------------
// LED comum: ritmos distintos por estado
// ---------------------------------------------------------------------------

void test_aguardando_fica_aceso(void) {
    TEST_ASSERT_EQUAL_UINT16(1000, acesoNoCiclo(AGUARDANDO, false));
}

void test_largada_armada_pisca_rapido(void) {
    TEST_ASSERT_EQUAL_UINT8(5, piscadasNoCiclo(AGUARDANDO, true));
}

void test_mapeando_pisca_meio_a_meio(void) {
    TEST_ASSERT_EQUAL_UINT16(500, acesoNoCiclo(MAPEANDO, false));
    TEST_ASSERT_EQUAL_UINT8(1, piscadasNoCiclo(MAPEANDO, false));
    TEST_ASSERT_EQUAL_UINT16(500, acesoNoCiclo(SPEED_RUN, false));
}

void test_concluido_uma_piscada_curta(void) {
    TEST_ASSERT_EQUAL_UINT16(100, acesoNoCiclo(CONCLUIDO, false));
    TEST_ASSERT_EQUAL_UINT8(1, piscadasNoCiclo(CONCLUIDO, false));
}

void test_falhas_duas_piscadas_curtas(void) {
    TEST_ASSERT_EQUAL_UINT8(2, piscadasNoCiclo(TRAVADO, false));
    TEST_ASSERT_EQUAL_UINT8(2, piscadasNoCiclo(COLISAO, false));
    TEST_ASSERT_EQUAL_UINT8(2, piscadasNoCiclo(TIMEOUT, false));
    TEST_ASSERT_EQUAL_UINT16(200, acesoNoCiclo(TRAVADO, false));
}

void test_estados_de_familias_diferentes_tem_ritmos_diferentes(void) {
    // Aguardando, prova em andamento, concluído e falha precisam ser distinguíveis a olho.
    const uint16_t aguardando = acesoNoCiclo(AGUARDANDO, false);
    const uint16_t mapeando = acesoNoCiclo(MAPEANDO, false);
    const uint16_t concluido = acesoNoCiclo(CONCLUIDO, false);
    const uint16_t falha = acesoNoCiclo(TRAVADO, false);
    TEST_ASSERT_TRUE(aguardando != mapeando && aguardando != concluido && aguardando != falha);
    TEST_ASSERT_TRUE(mapeando != concluido && mapeando != falha);
    TEST_ASSERT_TRUE(concluido != falha);
}

void test_aviso_pisca_as_vezes_pedidas_e_termina(void) {
    // 3 piscadas de 150 ms aceso e 150 ms apagado: 900 ms no total.
    uint8_t acendimentos = 0;
    bool antes = false;
    for (uint32_t t = 0; t < 1200; t += 10) {
        const bool agora = avisoAceso(3, t);
        if (agora && !antes) acendimentos++;
        antes = agora;
    }
    TEST_ASSERT_EQUAL_UINT8(3, acendimentos);
    TEST_ASSERT_TRUE(avisoEmAndamento(3, 899));
    TEST_ASSERT_FALSE(avisoEmAndamento(3, 900));
    TEST_ASSERT_TRUE(avisoAceso(3, 0));
    TEST_ASSERT_FALSE(avisoAceso(3, 150));
    TEST_ASSERT_FALSE(avisoAceso(3, 900));
    TEST_ASSERT_FALSE(avisoEmAndamento(0, 0));
}

void test_todo_estado_acende_em_algum_momento(void) {
    for (uint8_t i = 0; i < QTD; i++) {
        TEST_ASSERT_TRUE(acesoNoCiclo(TODOS[i], false) > 0);
        const CorLed c = corDoEstado(TODOS[i], false, 500);  // no meio do ciclo do amarelo
        TEST_ASSERT_TRUE(c.r > 0 || c.g > 0 || c.b > 0);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cores_de_cada_estado);
    RUN_TEST(test_mapeando_pisca_amarelo_a_cada_500_ms);
    RUN_TEST(test_aguardando_fica_aceso);
    RUN_TEST(test_largada_armada_pisca_rapido);
    RUN_TEST(test_mapeando_pisca_meio_a_meio);
    RUN_TEST(test_concluido_uma_piscada_curta);
    RUN_TEST(test_falhas_duas_piscadas_curtas);
    RUN_TEST(test_estados_de_familias_diferentes_tem_ritmos_diferentes);
    RUN_TEST(test_todo_estado_acende_em_algum_momento);
    RUN_TEST(test_aviso_pisca_as_vezes_pedidas_e_termina);
    return UNITY_END();
}
