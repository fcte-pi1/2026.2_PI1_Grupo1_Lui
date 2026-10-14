#include <unity.h>
#include "TelemetriaCodec.h"
#include "../../src/cronometro/Cronometro.h"

void setUp(void) {}
void tearDown(void) {}

void test_vale_zero_antes_da_largada(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    TEST_ASSERT_FALSE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(0, cronometro.decorridoMs(0));
    TEST_ASSERT_EQUAL_UINT32(0, cronometro.decorridoMs(123456));
}

void test_conta_a_partir_da_largada(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(5000);
    TEST_ASSERT_TRUE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(0, cronometro.decorridoMs(5000));
    TEST_ASSERT_EQUAL_UINT32(1234, cronometro.decorridoMs(6234));
    TEST_ASSERT_EQUAL_UINT32(142000, cronometro.decorridoMs(147000));
}

void test_fica_no_limite_de_10_minutos_do_contrato(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(1000);
    TEST_ASSERT_EQUAL_UINT32(599999, cronometro.decorridoMs(1000 + 599999));
    TEST_ASSERT_EQUAL_UINT32(600000, cronometro.decorridoMs(1000 + 600000));
    TEST_ASSERT_EQUAL_UINT32(600000, cronometro.decorridoMs(1000 + 900000));
}

// Um pacote com o tempo do cronômetro continua válido para o gateway mesmo depois
// dos 10 minutos (o encerramento por TIMEOUT é de outra issue).
void test_tempo_limitado_e_aceito_pela_validacao_do_contrato(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(0);
    TelemetriaPacket p = {cronometro.decorridoMs(3600000), LAB_4X4, MAPEANDO, 0, 80, 7400, 0, 0, 0, 0, 0, 0x09};
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaValidar(p));
}

void test_zerar_volta_ao_estado_anterior_a_largada(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(1000);
    cronometro.zerar();
    TEST_ASSERT_FALSE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(0, cronometro.decorridoMs(50000));
}

void test_iniciar_de_novo_reinicia_a_contagem(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(1000);
    cronometro.iniciar(8000);
    TEST_ASSERT_EQUAL_UINT32(500, cronometro.decorridoMs(8500));
}

void test_millis_voltando_a_zero(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(0xFFFFFF00u);
    TEST_ASSERT_EQUAL_UINT32(0x200, cronometro.decorridoMs(0x00000100u));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vale_zero_antes_da_largada);
    RUN_TEST(test_conta_a_partir_da_largada);
    RUN_TEST(test_fica_no_limite_de_10_minutos_do_contrato);
    RUN_TEST(test_tempo_limitado_e_aceito_pela_validacao_do_contrato);
    RUN_TEST(test_zerar_volta_ao_estado_anterior_a_largada);
    RUN_TEST(test_iniciar_de_novo_reinicia_a_contagem);
    RUN_TEST(test_millis_voltando_a_zero);
    return UNITY_END();
}
