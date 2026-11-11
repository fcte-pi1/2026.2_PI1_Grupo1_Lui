#include <stdint.h>
#include <unity.h>
#include "../../src/drivers/Motores.h"
#include "../../src/parada/ParadaObjetivo.h"
#include "../../src/seguranca/MonitorTravamento.h"

// Testes da lógica dos motores: trinco de parada (achado B7), tração por roda (R1) e
// inversão de montagem (R7) de docs/firmware/integracao-loop.md.

void setUp(void) {}
void tearDown(void) {}

// Ponte H falsa: guarda o último comando físico e conta as chamadas.
struct PonteFalsa : IPonteH {
    int16_t esq, dir;
    int aplicacoes, freios, solturas;
    PonteFalsa() : esq(0), dir(0), aplicacoes(0), freios(0), solturas(0) {}
    void aplicar(int16_t e, int16_t d) override {
        esq = e;
        dir = d;
        aplicacoes++;
    }
    void frear() override {
        esq = dir = 0;
        freios++;
    }
    void soltar() override {
        esq = dir = 0;
        solturas++;
    }
};

// ---------------------------------------------------------------------------
// Comando e tração por roda
// ---------------------------------------------------------------------------

void test_comando_chega_a_ponte(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    TEST_ASSERT_TRUE(m.comandar(90, -90));
    TEST_ASSERT_EQUAL_INT16(90, ponte.esq);
    TEST_ASSERT_EQUAL_INT16(-90, ponte.dir);
    TEST_ASSERT_EQUAL(1, ponte.aplicacoes);
}

void test_comando_e_saturado(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.comandar(400, -400);
    TEST_ASSERT_EQUAL_INT16(255, ponte.esq);
    TEST_ASSERT_EQUAL_INT16(-255, ponte.dir);
    TEST_ASSERT_EQUAL_INT16(255, m.pwmEsq());
}

void test_tracao_por_roda_segue_o_ultimo_pwm(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    TEST_ASSERT_FALSE(m.tracaoAtiva());
    m.comandar(0, 60);
    TEST_ASSERT_TRUE(m.tracaoAtiva());
    TEST_ASSERT_FALSE(m.tracaoEsqAtiva());
    TEST_ASSERT_TRUE(m.tracaoDirAtiva());
    m.comandar(-60, 0);  // ré também é tração
    TEST_ASSERT_TRUE(m.tracaoEsqAtiva());
    TEST_ASSERT_FALSE(m.tracaoDirAtiva());
    m.comandar(0, 0);
    TEST_ASSERT_FALSE(m.tracaoAtiva());
}

// ---------------------------------------------------------------------------
// Inversão de montagem (R7)
// ---------------------------------------------------------------------------

void test_inversao_troca_o_sinal_so_na_ponte(void) {
    PonteFalsa ponte;
    Motores m(ponte, true, false);
    m.comandar(90, 90);
    TEST_ASSERT_EQUAL_INT16(-90, ponte.esq);
    TEST_ASSERT_EQUAL_INT16(90, ponte.dir);
    TEST_ASSERT_EQUAL_INT16(90, m.pwmEsq());  // o valor lógico continua "para a frente"

    Motores n(ponte, false, true);
    n.comandar(-255, -255);
    TEST_ASSERT_EQUAL_INT16(-255, ponte.esq);
    TEST_ASSERT_EQUAL_INT16(255, ponte.dir);
}

void test_padrao_usa_as_constantes_de_pinos(void) {
    PonteFalsa ponte;
    Motores m(ponte);
    m.comandar(50, 50);
    TEST_ASSERT_EQUAL_INT16(MOTOR_ESQ_INVERTIDO ? -50 : 50, ponte.esq);
    TEST_ASSERT_EQUAL_INT16(MOTOR_DIR_INVERTIDO ? -50 : 50, ponte.dir);
}

// ---------------------------------------------------------------------------
// Trinco (B7)
// ---------------------------------------------------------------------------

void test_parar_freia_e_bloqueia_novos_comandos(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.comandar(90, 90);
    m.parar();
    TEST_ASSERT_TRUE(m.bloqueado());
    TEST_ASSERT_TRUE(m.freiando());
    TEST_ASSERT_EQUAL(1, ponte.freios);
    TEST_ASSERT_FALSE(m.tracaoAtiva());

    // O controle tenta reaplicar PWM no tick seguinte: ignorado.
    TEST_ASSERT_FALSE(m.comandar(90, 90));
    TEST_ASSERT_EQUAL(1, ponte.aplicacoes);
    TEST_ASSERT_EQUAL_INT16(0, ponte.esq);
    TEST_ASSERT_FALSE(m.tracaoAtiva());
}

void test_parar_repetido_e_inocuo(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.parar();
    m.parar();
    TEST_ASSERT_EQUAL(1, ponte.freios);
}

void test_frear_sem_trinco_permite_continuar(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.comandar(90, 90);
    m.frear();
    TEST_ASSERT_FALSE(m.bloqueado());
    TEST_ASSERT_TRUE(m.freiando());
    TEST_ASSERT_FALSE(m.tracaoAtiva());
    TEST_ASSERT_TRUE(m.comandar(60, 60));
    TEST_ASSERT_FALSE(m.freiando());
    TEST_ASSERT_EQUAL_INT16(60, ponte.esq);
}

void test_soltar_nao_abre_o_trinco(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.parar();
    m.soltar();  // estado terminal: roda livre depois do tempo de freio
    TEST_ASSERT_EQUAL(1, ponte.solturas);
    TEST_ASSERT_TRUE(m.bloqueado());
    TEST_ASSERT_FALSE(m.comandar(90, 90));
}

void test_frear_com_trinco_fechado_nao_faz_nada(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.parar();
    m.frear();
    TEST_ASSERT_EQUAL(1, ponte.freios);
}

void test_liberar_na_largada_reabre_o_trinco_em_roda_livre(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    m.parar();
    m.liberar();
    TEST_ASSERT_FALSE(m.bloqueado());
    TEST_ASSERT_FALSE(m.freiando());
    TEST_ASSERT_EQUAL(1, ponte.solturas);
    TEST_ASSERT_TRUE(m.comandar(90, 90));
    TEST_ASSERT_EQUAL_INT16(90, ponte.esq);
}

// ---------------------------------------------------------------------------
// Integração com quem chama parar(): ParadaObjetivo e MonitorTravamento
// ---------------------------------------------------------------------------

struct CronometroFalso : ICronometro {
    int congelamentos;
    CronometroFalso() : congelamentos(0) {}
    void congelar() override { congelamentos++; }
};

struct PulsosParados : IFontePulsos {
    uint32_t getUltimoPulsoEsqMs() const override { return 0; }
    uint32_t getUltimoPulsoDirMs() const override { return 0; }
};

void test_corte_do_monitor_nao_e_desfeito_pelo_controle(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    PulsosParados pulsos;
    CronometroFalso cron;
    MonitorTravamento monitor(m, pulsos, &cron);

    // Rodas comandadas e sem pulsos: o monitor corta em 1 s. O "controle" reaplica PWM em
    // todo tick; depois do corte nenhum comando pode passar.
    bool cortou = false;
    for (uint32_t t = 0; t <= 2000; t += 10) {
        m.comandar(90, 90);
        if (t % 50 == 0 && monitor.verificar(t)) cortou = true;
        if (cortou) {
            TEST_ASSERT_TRUE(m.bloqueado());
            TEST_ASSERT_FALSE(m.tracaoAtiva());
            TEST_ASSERT_EQUAL_INT16(0, ponte.esq);
            TEST_ASSERT_EQUAL_INT16(0, ponte.dir);
        }
    }
    TEST_ASSERT_TRUE(cortou);
    TEST_ASSERT_EQUAL(1, ponte.freios);
    TEST_ASSERT_EQUAL(1, cron.congelamentos);
}

void test_parada_no_objetivo_trava_os_motores(void) {
    PonteFalsa ponte;
    Motores m(ponte, false, false);
    CronometroFalso cron;
    ParadaObjetivo parada(m, cron);
    parada.configurarObjetivoPorGeometria(LAB_4X4);
    parada.iniciarProva();
    m.comandar(90, 90);
    parada.atualizar(3, 3);  // D4
    TEST_ASSERT_TRUE(parada.concluido());
    TEST_ASSERT_TRUE(m.bloqueado());
    TEST_ASSERT_FALSE(m.comandar(90, 90));
    TEST_ASSERT_EQUAL(1, cron.congelamentos);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_comando_chega_a_ponte);
    RUN_TEST(test_comando_e_saturado);
    RUN_TEST(test_tracao_por_roda_segue_o_ultimo_pwm);
    RUN_TEST(test_inversao_troca_o_sinal_so_na_ponte);
    RUN_TEST(test_padrao_usa_as_constantes_de_pinos);
    RUN_TEST(test_parar_freia_e_bloqueia_novos_comandos);
    RUN_TEST(test_parar_repetido_e_inocuo);
    RUN_TEST(test_frear_sem_trinco_permite_continuar);
    RUN_TEST(test_soltar_nao_abre_o_trinco);
    RUN_TEST(test_frear_com_trinco_fechado_nao_faz_nada);
    RUN_TEST(test_liberar_na_largada_reabre_o_trinco_em_roda_livre);
    RUN_TEST(test_corte_do_monitor_nao_e_desfeito_pelo_controle);
    RUN_TEST(test_parada_no_objetivo_trava_os_motores);
    return UNITY_END();
}
