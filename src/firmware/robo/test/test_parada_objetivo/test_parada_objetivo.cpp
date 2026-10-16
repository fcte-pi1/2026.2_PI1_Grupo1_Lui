#include <unity.h>

#include "../../src/parada/ParadaObjetivo.h"

// ---------------------------------------------------------------------------
// Dublês de teste
// ---------------------------------------------------------------------------

// Registra a ordem dos eventos: 'P' = parar tração, 'C' = congelar cronômetro.
struct Eventos {
    char ordem[16];
    uint8_t n;
    Eventos() : n(0) { ordem[0] = '\0'; }
    void add(char c) {
        if (n < 15) {
            ordem[n++] = c;
            ordem[n] = '\0';
        }
    }
};

struct RelogioFalso : IRelogioUs {
    uint32_t agora;
    RelogioFalso() : agora(0) {}
    uint32_t agoraUs() override { return agora; }
};

struct TracaoFalsa : ITracao {
    Eventos& ev;
    RelogioFalso* relogio;
    uint32_t custoUs;
    int chamadas;
    explicit TracaoFalsa(Eventos& e, RelogioFalso* r = nullptr, uint32_t custo = 0)
        : ev(e), relogio(r), custoUs(custo), chamadas(0) {}
    void parar() override {
        chamadas++;
        ev.add('P');
        if (relogio != nullptr) relogio->agora += custoUs;  // simula o tempo gasto em parar()
    }
};

struct CronometroFalso : ICronometro {
    Eventos& ev;
    int chamadas;
    explicit CronometroFalso(Eventos& e) : ev(e), chamadas(0) {}
    void congelar() override {
        chamadas++;
        ev.add('C');
    }
};

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Critérios de aceitação da HU-07 (parada da tração e cronômetro)
// ---------------------------------------------------------------------------

void test_nao_para_fora_da_area_objetivo(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo obj[] = {{3, 3}};
    TEST_ASSERT_TRUE(parada.configurarObjetivo(obj, 1));

    parada.atualizar(0, 0);
    parada.atualizar(2, 3);
    parada.atualizar(3, 2);

    TEST_ASSERT_EQUAL(0, tracao.chamadas);
    TEST_ASSERT_EQUAL(0, cron.chamadas);
    TEST_ASSERT_FALSE(parada.concluido());
    TEST_ASSERT_FALSE(parada.desafioCumprido());
}

void test_para_tracao_e_congela_cronometro_ao_entrar_no_objetivo(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo obj[] = {{3, 3}};
    parada.configurarObjetivo(obj, 1);

    parada.atualizar(2, 3);
    parada.atualizar(3, 3);

    TEST_ASSERT_EQUAL(1, tracao.chamadas);
    TEST_ASSERT_EQUAL(1, cron.chamadas);
    TEST_ASSERT_TRUE(parada.concluido());
    TEST_ASSERT_TRUE(parada.desafioCumprido());
}

void test_tracao_para_antes_do_cronometro_no_mesmo_ciclo(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo obj[] = {{1, 1}};
    parada.configurarObjetivo(obj, 1);

    parada.atualizar(1, 1);

    TEST_ASSERT_EQUAL_STRING("PC", ev.ordem);
}

// ---------------------------------------------------------------------------
// Comportamento do módulo
// ---------------------------------------------------------------------------

void test_nao_repete_comandos_e_nao_libera_a_tracao(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo obj[] = {{2, 2}};
    parada.configurarObjetivo(obj, 1);

    parada.atualizar(2, 2);
    parada.atualizar(2, 2);
    parada.atualizar(2, 2);
    parada.atualizar(2, 3);  // leitura de posição fora do objetivo depois de concluir

    TEST_ASSERT_EQUAL(1, tracao.chamadas);
    TEST_ASSERT_EQUAL(1, cron.chamadas);
    TEST_ASSERT_TRUE(parada.concluido());
}

void test_linha_e_coluna_nao_sao_confundidas(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo obj[] = {{5, 2}};  // linha 5, coluna 2
    parada.configurarObjetivo(obj, 1);

    parada.atualizar(2, 5);  // invertido: não é o objetivo
    TEST_ASSERT_FALSE(parada.concluido());

    parada.atualizar(5, 2);
    TEST_ASSERT_TRUE(parada.concluido());
}

void test_area_objetivo_com_quatro_celulas(void) {
    const CelulaObjetivo area[] = {{7, 1}, {7, 2}, {8, 1}, {8, 2}};
    for (uint8_t i = 0; i < 4; i++) {
        Eventos ev;
        TracaoFalsa tracao(ev);
        CronometroFalso cron(ev);
        ParadaObjetivo parada(tracao, cron);
        TEST_ASSERT_TRUE(parada.configurarObjetivo(area, 4));

        parada.atualizar(area[i].linha, area[i].coluna);

        TEST_ASSERT_TRUE(parada.concluido());
        TEST_ASSERT_EQUAL(1, tracao.chamadas);
    }
}

void test_sem_objetivo_configurado_nunca_conclui(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);

    // Sem configurarObjetivo(): nem a célula (0, 0) deve ser tratada como objetivo.
    parada.atualizar(0, 0);
    parada.atualizar(3, 3);

    TEST_ASSERT_EQUAL(0, tracao.chamadas);
    TEST_ASSERT_FALSE(parada.concluido());
}

void test_configuracao_invalida_e_rejeitada_e_mantem_a_anterior(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo ok[] = {{3, 3}};
    const CelulaObjetivo muitas[5] = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {1, 0}};

    TEST_ASSERT_FALSE(parada.configurarObjetivo(nullptr, 1));
    TEST_ASSERT_FALSE(parada.configurarObjetivo(ok, 0));
    TEST_ASSERT_FALSE(parada.configurarObjetivo(muitas, 5));

    TEST_ASSERT_TRUE(parada.configurarObjetivo(ok, 1));
    TEST_ASSERT_FALSE(parada.configurarObjetivo(muitas, 5));  // não pode apagar a válida

    parada.atualizar(3, 3);
    TEST_ASSERT_TRUE(parada.concluido());
}

void test_iniciar_prova_permite_nova_parada(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    const CelulaObjetivo obj[] = {{4, 1}};
    parada.configurarObjetivo(obj, 1);

    parada.atualizar(4, 1);
    TEST_ASSERT_TRUE(parada.concluido());

    parada.iniciarProva();
    TEST_ASSERT_FALSE(parada.concluido());
    TEST_ASSERT_FALSE(parada.desafioCumprido());

    parada.atualizar(4, 1);
    TEST_ASSERT_EQUAL(2, tracao.chamadas);
    TEST_ASSERT_TRUE(parada.concluido());
}

void test_latencia_de_software_abaixo_de_200ms(void) {
    Eventos ev;
    RelogioFalso relogio;
    TracaoFalsa tracao(ev, &relogio, 150);  // parar() leva 150 us no dublê
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron, &relogio);
    const CelulaObjetivo obj[] = {{3, 3}};
    parada.configurarObjetivo(obj, 1);

    relogio.agora = 4294967000UL;  // perto do overflow de uint32 (a subtração deve resistir)
    parada.atualizar(3, 3);

    TEST_ASSERT_EQUAL_UINT32(150, parada.latenciaUltimaParadaUs());
    TEST_ASSERT_LESS_THAN_UINT32(ParadaObjetivo::LIMITE_PARADA_US, parada.latenciaUltimaParadaUs());
}

// ---------------------------------------------------------------------------
// Chegada oficial: última célula da coluna D (D4 / D8 / D12)
// ---------------------------------------------------------------------------

static const GeometriaLabirinto GEOMETRIAS[] = {LAB_4X4, LAB_8X4, LAB_12X4};
static const uint8_t LINHAS[] = {4, 8, 12};

void test_chegada_por_geometria_e_a_ultima_celula_da_coluna_d(void) {
    for (uint8_t g = 0; g < 3; g++) {
        Eventos ev;
        TracaoFalsa tracao(ev);
        CronometroFalso cron(ev);
        ParadaObjetivo parada(tracao, cron);
        TEST_ASSERT_TRUE(parada.configurarObjetivoPorGeometria(GEOMETRIAS[g]));

        parada.atualizar(LINHAS[g] - 1, 3);  // D4 / D8 / D12

        TEST_ASSERT_TRUE(parada.concluido());
        TEST_ASSERT_EQUAL(1, tracao.chamadas);
        TEST_ASSERT_EQUAL(1, cron.chamadas);
    }
}

void test_partida_a1_nao_dispara_em_nenhuma_geometria(void) {
    for (uint8_t g = 0; g < 3; g++) {
        Eventos ev;
        TracaoFalsa tracao(ev);
        CronometroFalso cron(ev);
        ParadaObjetivo parada(tracao, cron);
        parada.configurarObjetivoPorGeometria(GEOMETRIAS[g]);

        parada.atualizar(0, 0);  // A1: robô começa aqui

        TEST_ASSERT_FALSE(parada.concluido());
        TEST_ASSERT_EQUAL(0, tracao.chamadas);
    }
}

void test_celulas_vizinhas_da_chegada_nao_disparam(void) {
    for (uint8_t g = 0; g < 3; g++) {
        Eventos ev;
        TracaoFalsa tracao(ev);
        CronometroFalso cron(ev);
        ParadaObjetivo parada(tracao, cron);
        parada.configurarObjetivoPorGeometria(GEOMETRIAS[g]);
        const uint8_t ultima = LINHAS[g] - 1;

        parada.atualizar(ultima - 1, 3);  // acima da chegada (D3 / D7 / D11)
        parada.atualizar(ultima, 2);      // ao lado da chegada (C4 / C8 / C12)
        parada.atualizar(ultima, 0);      // A4 / A8 / A12
        TEST_ASSERT_FALSE(parada.concluido());

        parada.atualizar(ultima, 3);
        TEST_ASSERT_TRUE(parada.concluido());
    }
}

void test_chegada_da_8x4_nao_e_a_da_4x4(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);
    parada.configurarObjetivoPorGeometria(LAB_8X4);

    parada.atualizar(3, 3);  // D4 não é chegada na 8x4
    TEST_ASSERT_FALSE(parada.concluido());

    parada.atualizar(7, 3);  // D8 é
    TEST_ASSERT_TRUE(parada.concluido());
}

void test_geometria_invalida_e_rejeitada_e_mantem_a_anterior(void) {
    Eventos ev;
    TracaoFalsa tracao(ev);
    CronometroFalso cron(ev);
    ParadaObjetivo parada(tracao, cron);

    TEST_ASSERT_FALSE(parada.configurarObjetivoPorGeometria((GeometriaLabirinto)0));
    TEST_ASSERT_FALSE(parada.configurarObjetivoPorGeometria((GeometriaLabirinto)99));

    TEST_ASSERT_TRUE(parada.configurarObjetivoPorGeometria(LAB_4X4));
    TEST_ASSERT_FALSE(parada.configurarObjetivoPorGeometria((GeometriaLabirinto)99));

    parada.atualizar(3, 3);  // a chegada da 4x4 continua valendo
    TEST_ASSERT_TRUE(parada.concluido());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_nao_para_fora_da_area_objetivo);
    RUN_TEST(test_para_tracao_e_congela_cronometro_ao_entrar_no_objetivo);
    RUN_TEST(test_tracao_para_antes_do_cronometro_no_mesmo_ciclo);
    RUN_TEST(test_nao_repete_comandos_e_nao_libera_a_tracao);
    RUN_TEST(test_linha_e_coluna_nao_sao_confundidas);
    RUN_TEST(test_area_objetivo_com_quatro_celulas);
    RUN_TEST(test_sem_objetivo_configurado_nunca_conclui);
    RUN_TEST(test_configuracao_invalida_e_rejeitada_e_mantem_a_anterior);
    RUN_TEST(test_iniciar_prova_permite_nova_parada);
    RUN_TEST(test_latencia_de_software_abaixo_de_200ms);
    RUN_TEST(test_chegada_por_geometria_e_a_ultima_celula_da_coluna_d);
    RUN_TEST(test_partida_a1_nao_dispara_em_nenhuma_geometria);
    RUN_TEST(test_celulas_vizinhas_da_chegada_nao_disparam);
    RUN_TEST(test_chegada_da_8x4_nao_e_a_da_4x4);
    RUN_TEST(test_geometria_invalida_e_rejeitada_e_mantem_a_anterior);

    return UNITY_END();
}
