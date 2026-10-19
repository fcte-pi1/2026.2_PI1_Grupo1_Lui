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

// ---------------------------------------------------------------------------
// congelar() (HU-07): o tempo da prova para ao chegar no objetivo
// ---------------------------------------------------------------------------

void test_congelar_fixa_o_tempo_decorrido(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(1000);
    cronometro.congelar(43500);

    TEST_ASSERT_TRUE(cronometro.congelado());
    TEST_ASSERT_FALSE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(42500, cronometro.decorridoMs(43500));
    TEST_ASSERT_EQUAL_UINT32(42500, cronometro.decorridoMs(90000));
    TEST_ASSERT_EQUAL_UINT32(42500, cronometro.decorridoMs(5000000));
}

void test_congelar_sem_largada_nao_faz_nada(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.congelar(5000);

    TEST_ASSERT_FALSE(cronometro.congelado());
    TEST_ASSERT_EQUAL_UINT32(0, cronometro.decorridoMs(9000));

    // A largada depois disso conta normalmente.
    cronometro.iniciar(9000);
    TEST_ASSERT_TRUE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(1000, cronometro.decorridoMs(10000));
}

void test_congelar_de_novo_mantem_o_primeiro_valor(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(0);
    cronometro.congelar(20000);
    cronometro.congelar(35000);

    TEST_ASSERT_EQUAL_UINT32(20000, cronometro.decorridoMs(60000));
}

void test_iniciar_desfaz_o_congelamento(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(0);
    cronometro.congelar(20000);

    cronometro.iniciar(100000);  // nova prova

    TEST_ASSERT_FALSE(cronometro.congelado());
    TEST_ASSERT_TRUE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(500, cronometro.decorridoMs(100500));
    cronometro.congelar(103000);
    TEST_ASSERT_EQUAL_UINT32(3000, cronometro.decorridoMs(200000));
}

void test_zerar_desfaz_o_congelamento(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(0);
    cronometro.congelar(20000);

    cronometro.zerar();

    TEST_ASSERT_FALSE(cronometro.congelado());
    TEST_ASSERT_FALSE(cronometro.emAndamento());
    TEST_ASSERT_EQUAL_UINT32(0, cronometro.decorridoMs(50000));
}

void test_congelar_perto_do_limite_de_10_minutos(void) {
    Cronometro antes(TELEMETRIA_TIMESTAMP_MAX_MS);
    antes.iniciar(1000);
    antes.congelar(1000 + 599999);
    TEST_ASSERT_EQUAL_UINT32(599999, antes.decorridoMs(1000 + 700000));

    Cronometro noLimite(TELEMETRIA_TIMESTAMP_MAX_MS);
    noLimite.iniciar(1000);
    noLimite.congelar(1000 + 600000);
    TEST_ASSERT_EQUAL_UINT32(600000, noLimite.decorridoMs(1000 + 700000));

    // Depois do limite o valor congelado também fica no limite do contrato.
    Cronometro depois(TELEMETRIA_TIMESTAMP_MAX_MS);
    depois.iniciar(1000);
    depois.congelar(1000 + 650000);
    TEST_ASSERT_EQUAL_UINT32(600000, depois.decorridoMs(1000 + 650000));
    TEST_ASSERT_EQUAL_UINT32(600000, depois.decorridoMs(1000 + 9000000));
}

void test_congelar_com_millis_voltando_a_zero(void) {
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    cronometro.iniciar(0xFFFFF000u);   // 4096 ms antes de o millis() voltar a zero
    cronometro.congelar(0x00001000u);  // 4096 ms depois

    TEST_ASSERT_EQUAL_UINT32(8192, cronometro.decorridoMs(0x00001000u));
    TEST_ASSERT_EQUAL_UINT32(8192, cronometro.decorridoMs(0x7FFFFFFFu));
    TEST_ASSERT_EQUAL_UINT32(8192, cronometro.decorridoMs(0xFFFFF000u));
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
    RUN_TEST(test_congelar_fixa_o_tempo_decorrido);
    RUN_TEST(test_congelar_sem_largada_nao_faz_nada);
    RUN_TEST(test_congelar_de_novo_mantem_o_primeiro_valor);
    RUN_TEST(test_iniciar_desfaz_o_congelamento);
    RUN_TEST(test_zerar_desfaz_o_congelamento);
    RUN_TEST(test_congelar_perto_do_limite_de_10_minutos);
    RUN_TEST(test_congelar_com_millis_voltando_a_zero);
    return UNITY_END();
}
