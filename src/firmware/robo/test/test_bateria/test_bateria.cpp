#include <unity.h>
#include "TelemetriaCodec.h"
#include "../../src/sensores/BateriaConversao.h"

void setUp(void) {}
void tearDown(void) {}

void test_divisor_converte_a_tensao_do_pino_para_a_da_bateria(void) {
    // 8,4 V x 33k / 133k = 2084 mV no pino; de volta: 2084 x 133/33 = 8399,15.
    TEST_ASSERT_EQUAL_UINT16(8399, bateriaMvDoAdc(2084));
    TEST_ASSERT_EQUAL_UINT16(7420, bateriaMvDoAdc(1841));  // 7419,79: cerca de 7,42 V
    TEST_ASSERT_EQUAL_UINT16(133, bateriaMvDoAdc(33));      // fator exato 133/33
    TEST_ASSERT_EQUAL_UINT16(0, bateriaMvDoAdc(0));
}

void test_divisor_arredonda_ao_milivolt_mais_proximo(void) {
    // 1 mV no pino = 4,03 mV na bateria; 2 mV = 8,06 mV.
    TEST_ASSERT_EQUAL_UINT16(4, bateriaMvDoAdc(1));
    TEST_ASSERT_EQUAL_UINT16(8, bateriaMvDoAdc(2));
}

void test_divisor_fica_no_limite_do_campo_do_pacote(void) {
    TEST_ASSERT_EQUAL_UINT16(65535, bateriaMvDoAdc(20000));
    TEST_ASSERT_EQUAL_UINT16(65535, bateriaMvDoAdc(0xFFFFFFFFu));
}

void test_porcentagem_nos_extremos_da_faixa(void) {
    TEST_ASSERT_EQUAL_UINT8(0, porcentagemBateria(BATERIA_VAZIA_MV));
    TEST_ASSERT_EQUAL_UINT8(100, porcentagemBateria(BATERIA_CHEIA_MV));
}

void test_porcentagem_satura_fora_da_faixa(void) {
    TEST_ASSERT_EQUAL_UINT8(0, porcentagemBateria(6500));
    TEST_ASSERT_EQUAL_UINT8(0, porcentagemBateria(0));
    TEST_ASSERT_EQUAL_UINT8(100, porcentagemBateria(8600));
    TEST_ASSERT_EQUAL_UINT8(100, porcentagemBateria(65535));
}

void test_porcentagem_no_meio_da_faixa_e_arredondamento(void) {
    TEST_ASSERT_EQUAL_UINT8(50, porcentagemBateria(7700));
    TEST_ASSERT_EQUAL_UINT8(1, porcentagemBateria(7007));   // 0,5% arredonda para cima
    TEST_ASSERT_EQUAL_UINT8(0, porcentagemBateria(7006));   // 0,43%
    TEST_ASSERT_EQUAL_UINT8(99, porcentagemBateria(8392));  // 99,43%
}

// Pares (mV, %) gerados pelo simulador do backend (src/backend/app/simulador).
void test_mesma_porcentagem_do_simulador_do_backend(void) {
    TEST_ASSERT_EQUAL_UINT8(86, porcentagemBateria(8200));
    TEST_ASSERT_EQUAL_UINT8(85, porcentagemBateria(8196));
    TEST_ASSERT_EQUAL_UINT8(84, porcentagemBateria(8180));
    TEST_ASSERT_EQUAL_UINT8(84, porcentagemBateria(8172));
    TEST_ASSERT_EQUAL_UINT8(83, porcentagemBateria(8164));
}

void test_valores_da_bateria_passam_na_validacao_do_contrato(void) {
    const uint16_t mv = bateriaMvDoAdc(1960);
    TelemetriaPacket p = {0, LAB_4X4, AGUARDANDO, 0, porcentagemBateria(mv), mv, 0, 0, 0, 0, 2, 0x0B};
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaValidar(p));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_divisor_converte_a_tensao_do_pino_para_a_da_bateria);
    RUN_TEST(test_divisor_arredonda_ao_milivolt_mais_proximo);
    RUN_TEST(test_divisor_fica_no_limite_do_campo_do_pacote);
    RUN_TEST(test_porcentagem_nos_extremos_da_faixa);
    RUN_TEST(test_porcentagem_satura_fora_da_faixa);
    RUN_TEST(test_porcentagem_no_meio_da_faixa_e_arredondamento);
    RUN_TEST(test_mesma_porcentagem_do_simulador_do_backend);
    RUN_TEST(test_valores_da_bateria_passam_na_validacao_do_contrato);
    return UNITY_END();
}
