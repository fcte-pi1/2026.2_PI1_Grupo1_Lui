#include <unity.h>
#include <string.h>
#include <ArduinoJson.h>
#include "TelemetriaCodec.h"
#include "TelemetriaJson.h"
#include "../../src/mapa/Direcao.h"

static const Direcao DIRECOES[] = {DIR_NORTE, DIR_SUL, DIR_LESTE, DIR_OESTE};
// Nome de cada direção nos campos 'paredes' do JSON, na ordem dos valores de Direcao.
static const char* const NOMES_PAREDES[] = {"norte", "sul", "leste", "oeste"};

void setUp(void) {}
void tearDown(void) {}

void test_valores_de_direcao_seguem_o_contrato(void) {
    TEST_ASSERT_EQUAL_STRING("NORTE", telemetriaNomeOrientacao(DIR_NORTE));
    TEST_ASSERT_EQUAL_STRING("SUL", telemetriaNomeOrientacao(DIR_SUL));
    TEST_ASSERT_EQUAL_STRING("LESTE", telemetriaNomeOrientacao(DIR_LESTE));
    TEST_ASSERT_EQUAL_STRING("OESTE", telemetriaNomeOrientacao(DIR_OESTE));
}

void test_mascaras_de_parede_seguem_o_contrato(void) {
    TEST_ASSERT_EQUAL_HEX8(0x01, PAREDE_NORTE);
    TEST_ASSERT_EQUAL_HEX8(0x02, PAREDE_SUL);
    TEST_ASSERT_EQUAL_HEX8(0x04, PAREDE_LESTE);
    TEST_ASSERT_EQUAL_HEX8(0x08, PAREDE_OESTE);
    TEST_ASSERT_EQUAL_HEX8(0x0F, PAREDE_TODAS);
    for (uint8_t i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_HEX8(1u << i, mascaraParede(DIRECOES[i]));
    }
}

// Passa cada máscara pelo conversor do gateway e confere que só o booleano da
// direção correspondente fica verdadeiro no JSON.
void test_mascara_liga_o_booleano_da_mesma_direcao_no_json(void) {
    for (uint8_t i = 0; i < 4; i++) {
        TelemetriaPacket p = {0, LAB_4X4, AGUARDANDO, 0, 100, 7400, 0, 0, 0, 0,
                              (uint8_t)DIRECOES[i], mascaraParede(DIRECOES[i])};
        char json[512];
        TEST_ASSERT_GREATER_THAN(0, telemetriaParaJson(p, 1, -50, json, sizeof(json)));

        JsonDocument doc;
        TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
        TEST_ASSERT_EQUAL_STRING(telemetriaNomeOrientacao(DIRECOES[i]), doc["orientacao"].as<const char*>());
        for (uint8_t j = 0; j < 4; j++) {
            TEST_ASSERT_EQUAL_MESSAGE(i == j, doc["paredes"][NOMES_PAREDES[j]].as<bool>(), NOMES_PAREDES[j]);
        }
    }
}

void test_direcao_oposta(void) {
    TEST_ASSERT_EQUAL(DIR_SUL, direcaoOposta(DIR_NORTE));
    TEST_ASSERT_EQUAL(DIR_NORTE, direcaoOposta(DIR_SUL));
    TEST_ASSERT_EQUAL(DIR_OESTE, direcaoOposta(DIR_LESTE));
    TEST_ASSERT_EQUAL(DIR_LESTE, direcaoOposta(DIR_OESTE));
}

void test_girar_direita_segue_o_sentido_horario(void) {
    TEST_ASSERT_EQUAL(DIR_LESTE, girarDireita(DIR_NORTE));
    TEST_ASSERT_EQUAL(DIR_SUL, girarDireita(DIR_LESTE));
    TEST_ASSERT_EQUAL(DIR_OESTE, girarDireita(DIR_SUL));
    TEST_ASSERT_EQUAL(DIR_NORTE, girarDireita(DIR_OESTE));
}

void test_girar_esquerda_desfaz_girar_direita(void) {
    for (uint8_t i = 0; i < 4; i++) {
        const Direcao d = DIRECOES[i];
        TEST_ASSERT_EQUAL(d, girarEsquerda(girarDireita(d)));
        TEST_ASSERT_EQUAL(direcaoOposta(d), girarEsquerda(girarEsquerda(d)));
        TEST_ASSERT_EQUAL(d, girarDireita(girarDireita(girarDireita(girarDireita(d)))));
    }
}

void test_lados_do_robo_em_direcoes_absolutas_nas_quatro_orientacoes(void) {
    // {orientação, frente, esquerda, direita}
    const Direcao casos[4][4] = {
        {DIR_NORTE, DIR_NORTE, DIR_OESTE, DIR_LESTE},
        {DIR_SUL, DIR_SUL, DIR_LESTE, DIR_OESTE},
        {DIR_LESTE, DIR_LESTE, DIR_NORTE, DIR_SUL},
        {DIR_OESTE, DIR_OESTE, DIR_SUL, DIR_NORTE},
    };
    for (uint8_t i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL(casos[i][1], direcaoAbsoluta(casos[i][0], LADO_FRENTE));
        TEST_ASSERT_EQUAL(casos[i][2], direcaoAbsoluta(casos[i][0], LADO_ESQUERDA));
        TEST_ASSERT_EQUAL(casos[i][3], direcaoAbsoluta(casos[i][0], LADO_DIREITA));
    }
}

void test_deslocamento_na_grade(void) {
    // Sul aumenta a linha e leste aumenta a coluna (doc 4.4, seção 5.1).
    TEST_ASSERT_EQUAL_INT8(-1, deltaLinha(DIR_NORTE));
    TEST_ASSERT_EQUAL_INT8(0, deltaColuna(DIR_NORTE));
    TEST_ASSERT_EQUAL_INT8(1, deltaLinha(DIR_SUL));
    TEST_ASSERT_EQUAL_INT8(0, deltaColuna(DIR_SUL));
    TEST_ASSERT_EQUAL_INT8(0, deltaLinha(DIR_LESTE));
    TEST_ASSERT_EQUAL_INT8(1, deltaColuna(DIR_LESTE));
    TEST_ASSERT_EQUAL_INT8(0, deltaLinha(DIR_OESTE));
    TEST_ASSERT_EQUAL_INT8(-1, deltaColuna(DIR_OESTE));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_valores_de_direcao_seguem_o_contrato);
    RUN_TEST(test_mascaras_de_parede_seguem_o_contrato);
    RUN_TEST(test_mascara_liga_o_booleano_da_mesma_direcao_no_json);
    RUN_TEST(test_direcao_oposta);
    RUN_TEST(test_girar_direita_segue_o_sentido_horario);
    RUN_TEST(test_girar_esquerda_desfaz_girar_direita);
    RUN_TEST(test_lados_do_robo_em_direcoes_absolutas_nas_quatro_orientacoes);
    RUN_TEST(test_deslocamento_na_grade);
    return UNITY_END();
}
