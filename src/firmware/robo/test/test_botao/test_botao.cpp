#include <unity.h>
#include "../../src/botao/Botao.h"

// Simula o laço principal: a cada passo informa a leitura bruta ao botão e registra
// os eventos devolvidos.
static Botao botao;
static uint32_t agora;
static uint8_t largadas;
static uint8_t sequencias;
static uint8_t toquesDaUltimaSequencia;
static uint32_t instanteUltimoEvento;

void setUp(void) {
    botao.reiniciar();
    agora = 10000;
    largadas = 0;
    sequencias = 0;
    toquesDaUltimaSequencia = 0;
    instanteUltimoEvento = 0;
}

void tearDown(void) {}

static void manter(bool pressionado, uint32_t duracaoMs, uint32_t passoMs = 10) {
    for (uint32_t t = 0; t < duracaoMs; t += passoMs) {
        const EventoBotao e = botao.atualizar(pressionado, agora);
        if (e.tipo == BOTAO_LARGADA) {
            largadas++;
            instanteUltimoEvento = agora;
        } else if (e.tipo == BOTAO_TOQUES) {
            sequencias++;
            toquesDaUltimaSequencia = e.toques;
            instanteUltimoEvento = agora;
        }
        agora += passoMs;
    }
}

static void pressionarPor(uint32_t ms, uint32_t passoMs = 10) { manter(true, ms, passoMs); }
static void soltarPor(uint32_t ms, uint32_t passoMs = 10) { manter(false, ms, passoMs); }

// Alterna a leitura a cada 5 ms, como o repique mecânico da chave.
static void repicar(uint32_t duracaoMs) {
    bool nivel = true;
    for (uint32_t t = 0; t < duracaoMs; t += 5) {
        manter(nivel, 5, 5);
        nivel = !nivel;
    }
}

void test_sem_acionamento_nao_gera_evento(void) {
    soltarPor(5000);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_FALSE(botao.pressionado());
}

void test_pulso_menor_que_o_debounce_e_ignorado(void) {
    pressionarPor(20, 5);
    soltarPor(1000);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
}

void test_estado_so_muda_depois_de_25_ms_estavel(void) {
    pressionarPor(20, 5);
    TEST_ASSERT_FALSE(botao.pressionado());
    pressionarPor(20, 5);
    TEST_ASSERT_TRUE(botao.pressionado());
}

void test_repique_ao_pressionar_e_ao_soltar_conta_um_toque(void) {
    repicar(30);
    pressionarPor(150);
    repicar(30); // ruído ao soltar
    soltarPor(1000);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
    TEST_ASSERT_EQUAL_UINT8(1, toquesDaUltimaSequencia);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
}

void test_um_dois_e_tres_toques_na_mesma_janela(void) {
    for (uint8_t n = 1; n <= 3; n++) {
        setUp();
        for (uint8_t i = 0; i < n; i++) {
            pressionarPor(100);
            soltarPor(150);
        }
        soltarPor(500);
        TEST_ASSERT_EQUAL_UINT8(1, sequencias);
        TEST_ASSERT_EQUAL_UINT8(n, toquesDaUltimaSequencia);
    }
}

void test_toques_so_sao_entregues_apos_400_ms_de_silencio(void) {
    pressionarPor(100);
    soltarPor(350);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    soltarPor(150);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
}

void test_toques_em_janelas_separadas_geram_eventos_separados(void) {
    pressionarPor(100);
    soltarPor(600);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
    TEST_ASSERT_EQUAL_UINT8(1, toquesDaUltimaSequencia);

    pressionarPor(100);
    soltarPor(150);
    pressionarPor(100);
    soltarPor(600);
    TEST_ASSERT_EQUAL_UINT8(2, sequencias);
    TEST_ASSERT_EQUAL_UINT8(2, toquesDaUltimaSequencia);
}

void test_pressao_curta_conta_como_toque(void) {
    pressionarPor(900);
    soltarPor(600);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
    TEST_ASSERT_EQUAL_UINT8(1, toquesDaUltimaSequencia);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
}

void test_pressao_intermediaria_e_ignorada(void) {
    pressionarPor(1500);
    soltarPor(1000);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
}

void test_largada_so_acontece_ao_soltar_depois_de_2_s(void) {
    pressionarPor(2500);
    // Ainda pressionado: nada acontece, mas a largada já está armada.
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
    TEST_ASSERT_TRUE(botao.largadaArmada());

    soltarPor(100);
    TEST_ASSERT_EQUAL_UINT8(1, largadas);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_FALSE(botao.largadaArmada());
}

void test_segurar_sem_soltar_nunca_da_largada(void) {
    pressionarPor(10000);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);
    TEST_ASSERT_TRUE(botao.largadaArmada());
}

void test_largada_armada_so_a_partir_de_2_s(void) {
    pressionarPor(1900);
    TEST_ASSERT_TRUE(botao.pressionado());
    TEST_ASSERT_FALSE(botao.largadaArmada());
    pressionarPor(200);
    TEST_ASSERT_TRUE(botao.largadaArmada());
}

void test_repique_ao_soltar_depois_de_2_s_gera_uma_largada(void) {
    pressionarPor(2500);
    repicar(40);
    soltarPor(1000);
    TEST_ASSERT_EQUAL_UINT8(1, largadas);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
}

// Com passo de 1 ms as bordas estáveis atrasam igualmente o início e o fim da
// pressão, então a duração medida é exatamente a duração física.
static void pressaoExata(uint32_t duracaoMs) {
    setUp();
    pressionarPor(duracaoMs, 1);
    soltarPor(600, 1);
}

void test_limites_exatos_de_1_s_e_2_s(void) {
    pressaoExata(999);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);

    pressaoExata(1000);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);

    pressaoExata(1999);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_EQUAL_UINT8(0, largadas);

    pressaoExata(2000);
    TEST_ASSERT_EQUAL_UINT8(0, sequencias);
    TEST_ASSERT_EQUAL_UINT8(1, largadas);
}

void test_toques_depois_de_pressao_longa_sao_contados(void) {
    pressionarPor(1500); // ignorada
    soltarPor(100);
    pressionarPor(100);
    soltarPor(150);
    pressionarPor(100);
    soltarPor(150);
    pressionarPor(100);
    soltarPor(600);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
    TEST_ASSERT_EQUAL_UINT8(3, toquesDaUltimaSequencia);
}

void test_millis_voltando_a_zero_durante_a_pressao(void) {
    agora = 0xFFFFFFFFu - 1000;
    pressionarPor(2500);
    TEST_ASSERT_TRUE(agora < 2000); // o contador deu a volta
    TEST_ASSERT_TRUE(botao.largadaArmada());
    soltarPor(100);
    TEST_ASSERT_EQUAL_UINT8(1, largadas);
}

void test_millis_voltando_a_zero_durante_a_janela_de_toques(void) {
    agora = 0xFFFFFFFFu - 300;
    pressionarPor(100);
    soltarPor(150);
    pressionarPor(100);
    soltarPor(600);
    TEST_ASSERT_EQUAL_UINT8(1, sequencias);
    TEST_ASSERT_EQUAL_UINT8(2, toquesDaUltimaSequencia);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sem_acionamento_nao_gera_evento);
    RUN_TEST(test_pulso_menor_que_o_debounce_e_ignorado);
    RUN_TEST(test_estado_so_muda_depois_de_25_ms_estavel);
    RUN_TEST(test_repique_ao_pressionar_e_ao_soltar_conta_um_toque);
    RUN_TEST(test_um_dois_e_tres_toques_na_mesma_janela);
    RUN_TEST(test_toques_so_sao_entregues_apos_400_ms_de_silencio);
    RUN_TEST(test_toques_em_janelas_separadas_geram_eventos_separados);
    RUN_TEST(test_pressao_curta_conta_como_toque);
    RUN_TEST(test_pressao_intermediaria_e_ignorada);
    RUN_TEST(test_largada_so_acontece_ao_soltar_depois_de_2_s);
    RUN_TEST(test_segurar_sem_soltar_nunca_da_largada);
    RUN_TEST(test_largada_armada_so_a_partir_de_2_s);
    RUN_TEST(test_repique_ao_soltar_depois_de_2_s_gera_uma_largada);
    RUN_TEST(test_limites_exatos_de_1_s_e_2_s);
    RUN_TEST(test_toques_depois_de_pressao_longa_sao_contados);
    RUN_TEST(test_millis_voltando_a_zero_durante_a_pressao);
    RUN_TEST(test_millis_voltando_a_zero_durante_a_janela_de_toques);
    return UNITY_END();
}
