#include <stdint.h>
#include <unity.h>
#include "../../src/sensores/ClassificadorParedes.h"
#include "../../src/sensores/LeituraToF.h"

// Testes da leitura não bloqueante dos ToF (achado B10) e do classificador de paredes
// (achado R3) de docs/firmware/integracao-loop.md.

void setUp(void) {}
void tearDown(void) {}

static AmostraParedes amostra(uint16_t esq, uint16_t frente, uint16_t dir) {
    AmostraParedes a;
    a.esqMm = esq;
    a.frenteMm = frente;
    a.dirMm = dir;
    a.esqValida = a.frenteValida = a.dirValida = true;
    return a;
}

// ---------------------------------------------------------------------------
// LeituraToF: validade por idade da amostra
// ---------------------------------------------------------------------------

void test_sem_amostra_a_leitura_e_invalida(void) {
    LeituraToF l;
    TEST_ASSERT_FALSE(l.valida(0));
    TEST_ASSERT_FALSE(l.respondendo(0));
    TEST_ASSERT_FALSE(l.validaParaParede(0));
    TEST_ASSERT_FALSE(l.nova());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, l.distanciaParaControle(0));
}

void test_amostra_vale_por_100_ms(void) {
    LeituraToF l;
    l.registrar(42, 1000);
    TEST_ASSERT_TRUE(l.valida(1000));
    TEST_ASSERT_TRUE(l.valida(1000 + TOF_VALIDADE_MS));
    TEST_ASSERT_FALSE(l.valida(1000 + TOF_VALIDADE_MS + 1));  // sensor parou de responder
    TEST_ASSERT_FALSE(l.validaParaParede(1000 + TOF_VALIDADE_MS + 1));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 42.0f, l.distanciaParaControle(1050));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, l.distanciaParaControle(1200));
}

void test_amostra_nova_renova_a_validade(void) {
    LeituraToF l;
    l.registrar(42, 1000);
    l.registrar(43, 1090);
    TEST_ASSERT_TRUE(l.valida(1150));
    TEST_ASSERT_EQUAL_UINT16(43, l.distanciaMm());
}

void test_sem_alvo_ao_alcance_serve_para_parede_mas_nao_para_o_controle(void) {
    LeituraToF l;
    l.registrar(TOF_LEITURA_INVALIDA_MM, 1000);
    TEST_ASSERT_TRUE(l.respondendo(1010));
    TEST_ASSERT_TRUE(l.validaParaParede(1010));
    TEST_ASSERT_FALSE(l.valida(1010));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, l.distanciaParaControle(1010));
}

void test_leitura_zero_e_erro_do_sensor(void) {
    LeituraToF l;
    l.registrar(0, 1000);
    TEST_ASSERT_FALSE(l.valida(1000));
    TEST_ASSERT_FALSE(l.validaParaParede(1000));
}

void test_amostra_nova_e_consumida_uma_vez(void) {
    LeituraToF l;
    l.registrar(42, 1000);
    TEST_ASSERT_TRUE(l.nova());
    l.consumir();
    TEST_ASSERT_FALSE(l.nova());
    TEST_ASSERT_TRUE(l.valida(1010));  // consumir não invalida a distância
    l.registrar(44, 1033);
    TEST_ASSERT_TRUE(l.nova());
}

void test_validade_na_volta_do_millis(void) {
    LeituraToF l;
    l.registrar(42, 0xFFFFFFC0u);
    TEST_ASSERT_TRUE(l.valida(0x10u));   // 80 ms depois, atravessando o zero
    TEST_ASSERT_FALSE(l.valida(0x30u));  // 112 ms depois
}

// ---------------------------------------------------------------------------
// ClassificadorParedes: limiar, histerese e inválidos
// ---------------------------------------------------------------------------

void test_centrado_no_corredor_ve_as_paredes_laterais(void) {
    ClassificadorParedes c;
    c.atualizar(amostra(40, 400, 40));
    bool f, e, d;
    TEST_ASSERT_TRUE(c.lerParedes(f, e, d));
    TEST_ASSERT_TRUE(e);
    TEST_ASSERT_TRUE(d);
    TEST_ASSERT_FALSE(f);
}

void test_primeira_leitura_decide_pelo_limiar(void) {
    ClassificadorParedes c;
    c.atualizar(amostra(LIMIAR_PAREDE_MM, LIMIAR_PAREDE_MM + 1, 200));
    TEST_ASSERT_TRUE(c.paredeEsquerda());
    TEST_ASSERT_FALSE(c.paredeFrente());
    TEST_ASSERT_FALSE(c.paredeDireita());
}

void test_histerese_segura_a_decisao_perto_do_limiar(void) {
    ClassificadorParedes c(110, 5);
    c.atualizar(amostra(100, 200, 200));
    TEST_ASSERT_TRUE(c.paredeEsquerda());
    c.atualizar(amostra(115, 200, 200));  // dentro da faixa: continua parede
    TEST_ASSERT_TRUE(c.paredeEsquerda());
    c.atualizar(amostra(116, 200, 200));  // passou de 110 + 5: sem parede
    TEST_ASSERT_FALSE(c.paredeEsquerda());
    c.atualizar(amostra(106, 200, 200));  // dentro da faixa: continua sem parede
    TEST_ASSERT_FALSE(c.paredeEsquerda());
    c.atualizar(amostra(104, 200, 200));  // abaixo de 110 - 5: parede
    TEST_ASSERT_TRUE(c.paredeEsquerda());
}

void test_sem_alvo_ao_alcance_e_ausencia_de_parede(void) {
    ClassificadorParedes c;
    c.atualizar(amostra(40, TOF_LEITURA_INVALIDA_MM, 40));
    bool f, e, d;
    TEST_ASSERT_TRUE(c.lerParedes(f, e, d));
    TEST_ASSERT_FALSE(f);
}

void test_sensor_invalido_faz_ler_paredes_falhar(void) {
    ClassificadorParedes c;
    AmostraParedes a = amostra(40, 400, 40);
    a.frenteValida = false;
    c.atualizar(a);
    bool f = true, e = false, d = false;
    TEST_ASSERT_FALSE(c.lerParedes(f, e, d));
    TEST_ASSERT_FALSE(c.frenteValida());
    TEST_ASSERT_TRUE(c.esquerdaValida());
}

void test_leitura_invalida_nao_apaga_a_decisao_anterior(void) {
    ClassificadorParedes c;
    c.atualizar(amostra(40, 400, 40));
    AmostraParedes a = amostra(500, 400, 40);
    a.esqValida = false;  // o valor 500 deve ser ignorado
    c.atualizar(a);
    TEST_ASSERT_TRUE(c.paredeEsquerda());
    c.atualizar(amostra(40, 400, 40));
    bool f, e, d;
    TEST_ASSERT_TRUE(c.lerParedes(f, e, d));
    TEST_ASSERT_TRUE(e);
}

void test_reiniciar_esquece_o_historico(void) {
    ClassificadorParedes c(110, 5);
    c.atualizar(amostra(100, 200, 200));
    c.reiniciar();
    bool f, e, d;
    TEST_ASSERT_FALSE(c.lerParedes(f, e, d));
    c.atualizar(amostra(112, 200, 200));  // sem histórico: decide pelo limiar puro
    TEST_ASSERT_FALSE(c.paredeEsquerda());
}

void test_amostra_montada_a_partir_das_leituras(void) {
    LeituraToF esq, frente, dir;
    esq.registrar(40, 1000);
    frente.registrar(TOF_LEITURA_INVALIDA_MM, 1000);
    dir.registrar(40, 800);  // velha: inválida em 1000
    ClassificadorParedes c;
    c.atualizar(amostraDasLeituras(esq, frente, dir, 1000));
    bool f, e, d;
    TEST_ASSERT_FALSE(c.lerParedes(f, e, d));  // sensor direito parado
    TEST_ASSERT_TRUE(c.esquerdaValida());
    TEST_ASSERT_TRUE(c.frenteValida());
    TEST_ASSERT_FALSE(c.direitaValida());

    dir.registrar(41, 1010);
    c.atualizar(amostraDasLeituras(esq, frente, dir, 1010));
    TEST_ASSERT_TRUE(c.lerParedes(f, e, d));
    TEST_ASSERT_TRUE(e);
    TEST_ASSERT_FALSE(f);
    TEST_ASSERT_TRUE(d);
}

void test_classificador_implementa_sensores_paredes(void) {
    ClassificadorParedes c;
    SensoresParedes& s = c;  // é o que o Navegador recebe
    c.atualizar(amostra(200, 40, 200));
    bool f, e, d;
    TEST_ASSERT_TRUE(s.lerParedes(f, e, d));
    TEST_ASSERT_TRUE(f);
    TEST_ASSERT_FALSE(e);
    TEST_ASSERT_FALSE(d);
}

void test_limiar_fica_entre_parede_lateral_e_corredor_aberto(void) {
    // Centrado, a parede lateral fica a cerca de 40 mm; sem parede, ao menos 200 mm.
    TEST_ASSERT_TRUE(LIMIAR_PAREDE_MM - HISTERESE_PAREDE_MM > 40);
    TEST_ASSERT_TRUE(LIMIAR_PAREDE_MM + HISTERESE_PAREDE_MM < 200);
    TEST_ASSERT_TRUE(LIMIAR_PAREDE_MM < VAO_CORREDOR_MM);
}

int main(void) {
    UNITY_BEGIN();
    // LeituraToF
    RUN_TEST(test_sem_amostra_a_leitura_e_invalida);
    RUN_TEST(test_amostra_vale_por_100_ms);
    RUN_TEST(test_amostra_nova_renova_a_validade);
    RUN_TEST(test_sem_alvo_ao_alcance_serve_para_parede_mas_nao_para_o_controle);
    RUN_TEST(test_leitura_zero_e_erro_do_sensor);
    RUN_TEST(test_amostra_nova_e_consumida_uma_vez);
    RUN_TEST(test_validade_na_volta_do_millis);
    // ClassificadorParedes
    RUN_TEST(test_centrado_no_corredor_ve_as_paredes_laterais);
    RUN_TEST(test_primeira_leitura_decide_pelo_limiar);
    RUN_TEST(test_histerese_segura_a_decisao_perto_do_limiar);
    RUN_TEST(test_sem_alvo_ao_alcance_e_ausencia_de_parede);
    RUN_TEST(test_sensor_invalido_faz_ler_paredes_falhar);
    RUN_TEST(test_leitura_invalida_nao_apaga_a_decisao_anterior);
    RUN_TEST(test_reiniciar_esquece_o_historico);
    RUN_TEST(test_amostra_montada_a_partir_das_leituras);
    RUN_TEST(test_classificador_implementa_sensores_paredes);
    RUN_TEST(test_limiar_fica_entre_parede_lateral_e_corredor_aberto);
    return UNITY_END();
}
