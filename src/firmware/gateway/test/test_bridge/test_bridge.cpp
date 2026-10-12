#include <unity.h>
#include <string.h>
#include <ArduinoJson.h>
#include "GatewayBridge.h"

void setUp(void) {}
void tearDown(void) {}

static void pacoteValido(uint8_t* buf) {
    TelemetriaPacket p;
    memset(&p, 0, sizeof(p));
    p.timestamp_ms = 142000;
    p.labirinto_id = LAB_8X4;
    p.status_id = MAPEANDO;
    p.bateria_pct = 78;
    p.bateria_mv = 7420;
    p.velocidade_atual = 350;
    p.velocidade_media = 285;
    p.coluna = 1;
    p.linha = 3;
    p.orientacao = 2;
    p.paredes = 0x06;
    telemetriaCodificar(p, buf, TELEMETRIA_TAMANHO_BYTES);
}

void test_quadro_valido_gera_uma_linha_json(void) {
    GatewayBridge g;
    uint8_t buf[18];
    char out[512];
    pacoteValido(buf);
    size_t n = g.processar(buf, 18, -61, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_EQUAL('\n', out[n - 1]);
    TEST_ASSERT_EQUAL(strlen(out), n);
    // Exatamente uma linha (delimitador de quadro do doc 4.4, seção 6.3)
    TEST_ASSERT_EQUAL_PTR(out + n - 1, strchr(out, '\n'));

    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, out) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_UINT32(1, doc["seq"].as<uint32_t>());
    TEST_ASSERT_EQUAL_INT(-61, doc["rssi"].as<int>());
    TEST_ASSERT_EQUAL_STRING("MAPEANDO", doc["status"].as<const char*>());
}

void test_sequencia_incrementa_a_cada_pacote_valido(void) {
    GatewayBridge g;
    uint8_t buf[18];
    char out[512];
    pacoteValido(buf);
    for (uint32_t i = 1; i <= 5; i++) {
        TEST_ASSERT_GREATER_THAN(0, g.processar(buf, 18, -50, out, sizeof(out)));
        JsonDocument doc;
        deserializeJson(doc, out);
        TEST_ASSERT_EQUAL_UINT32(i, doc["seq"].as<uint32_t>());
    }
    TEST_ASSERT_EQUAL_UINT32(5, g.sequencia());
    TEST_ASSERT_EQUAL_UINT32(0, g.descartados());
}

void test_quadro_com_tamanho_errado_e_descartado(void) {
    GatewayBridge g;
    uint8_t buf[32] = {0};
    char out[512];
    TEST_ASSERT_EQUAL(0, g.processar(buf, 17, -50, out, sizeof(out)));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_TAMANHO, g.ultimoErro());
    TEST_ASSERT_EQUAL(0, g.processar(buf, 32, -50, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT32(2, g.descartados());
    TEST_ASSERT_EQUAL_UINT32(0, g.sequencia());
}

void test_quadro_com_campo_invalido_e_descartado_sem_consumir_seq(void) {
    GatewayBridge g;
    uint8_t buf[18];
    char out[512];
    pacoteValido(buf);
    buf[5] = 9;  // status inexistente
    TEST_ASSERT_EQUAL(0, g.processar(buf, 18, -50, out, sizeof(out)));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_STATUS, g.ultimoErro());
    pacoteValido(buf);
    TEST_ASSERT_GREATER_THAN(0, g.processar(buf, 18, -50, out, sizeof(out)));
    JsonDocument doc;
    deserializeJson(doc, out);
    TEST_ASSERT_EQUAL_UINT32(1, doc["seq"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(1, g.descartados());
}

void test_buffer_de_saida_insuficiente_descarta(void) {
    GatewayBridge g;
    uint8_t buf[18];
    char out[32];
    pacoteValido(buf);
    TEST_ASSERT_EQUAL(0, g.processar(buf, 18, -50, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT32(0, g.sequencia());
    TEST_ASSERT_EQUAL_UINT32(1, g.descartados());
}

void test_ponteiro_nulo_e_descartado(void) {
    GatewayBridge g;
    char out[512];
    TEST_ASSERT_EQUAL(0, g.processar(nullptr, 18, -50, out, sizeof(out)));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_PONTEIRO, g.ultimoErro());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_quadro_valido_gera_uma_linha_json);
    RUN_TEST(test_sequencia_incrementa_a_cada_pacote_valido);
    RUN_TEST(test_quadro_com_tamanho_errado_e_descartado);
    RUN_TEST(test_quadro_com_campo_invalido_e_descartado_sem_consumir_seq);
    RUN_TEST(test_buffer_de_saida_insuficiente_descarta);
    RUN_TEST(test_ponteiro_nulo_e_descartado);
    return UNITY_END();
}
