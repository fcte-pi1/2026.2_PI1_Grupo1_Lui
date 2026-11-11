#include <stdint.h>
#include <unity.h>
#include "../../src/odometria/Odometria.h"
#include "../../src/seguranca/MonitorTravamento.h"

// Testes da Odometria (docs/firmware/integracao-loop.md, achados R6 e B4).

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Distância e constantes (B4: tudo deriva de ENCODER_PULSOS_POR_VOLTA)
// ---------------------------------------------------------------------------

void test_uma_volta_da_roda_percorre_a_circunferencia(void) {
    Odometria o;
    o.atualizar(ENCODER_PULSOS_POR_VOLTA, ENCODER_PULSOS_POR_VOLTA, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.14159265f * RODA_DIAMETRO_MM, o.deslocamentoEsqMm());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.14159265f * RODA_DIAMETRO_MM, o.deslocamentoDirMm());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.14159265f * RODA_DIAMETRO_MM, o.deslocamentoMm());
}

void test_alvo_de_uma_celula_em_pulsos(void) {
    // 180 / (pi x 22 / 360) = 937,6 pulsos.
    TEST_ASSERT_EQUAL_INT32(938, Odometria::pulsosParaMm((float)CELULA_MM));
    TEST_ASSERT_EQUAL_INT32(-938, Odometria::pulsosParaMm(-(float)CELULA_MM));
    TEST_ASSERT_EQUAL_INT32(0, Odometria::pulsosParaMm(0.0f));
}

void test_alvo_acompanha_a_constante_de_pulsos_por_volta(void) {
    // Se a bancada mostrar 4x menos pulsos por volta (B4), o alvo cai 4x junto.
    const float mmPorPulsoMedido = 3.14159265f * RODA_DIAMETRO_MM / 90.0f;
    TEST_ASSERT_EQUAL_INT32(234, Odometria::pulsosParaMm((float)CELULA_MM, mmPorPulsoMedido));
    Odometria o(mmPorPulsoMedido);
    o.atualizar(234, 234, 10);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 180.0f, o.deslocamentoMm());
}

void test_reiniciar_toma_as_contagens_atuais_como_origem(void) {
    Odometria o;
    o.atualizar(1000, 2000, 10);
    o.reiniciar(1000, 2000, 20);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, o.deslocamentoMm());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, o.rumoGraus());
    o.atualizar(1100, 2100, 30);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100 * RODA_MM_POR_PULSO, o.deslocamentoMm());
    TEST_ASSERT_EQUAL_INT32(1100, o.encEsq());
    TEST_ASSERT_EQUAL_INT32(2100, o.encDir());
}

// ---------------------------------------------------------------------------
// Rumo odométrico
// ---------------------------------------------------------------------------

void test_giro_no_lugar_de_90_graus_a_esquerda(void) {
    // Cada roda percorre um quarto da circunferência de raio bitola/2: pi x 80 / 4.
    Odometria o;
    const int32_t p = Odometria::pulsosParaMm(3.14159265f * BITOLA_MM / 4.0f);
    o.atualizar(-p, p, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 90.0f, o.rumoGraus());
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, o.deslocamentoMm());
}

void test_giro_a_direita_tem_rumo_negativo(void) {
    Odometria o;
    const int32_t p = Odometria::pulsosParaMm(3.14159265f * BITOLA_MM / 4.0f);
    o.atualizar(p, -p, 10);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, -90.0f, o.rumoGraus());
}

// ---------------------------------------------------------------------------
// Velocidade (janela de 200 ms)
// ---------------------------------------------------------------------------

void test_velocidade_constante_na_janela(void) {
    Odometria o;
    // 150 mm/s nas duas rodas: em 200 ms, 30 mm.
    const int32_t pulsos30mm = Odometria::pulsosParaMm(30.0f);
    for (uint32_t t = 0; t <= 200; t += 10) {
        const int32_t p = (int32_t)((int64_t)pulsos30mm * t / 200);
        o.atualizar(p, p, t);
    }
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 150.0f, o.velocidadeMmS());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 150.0f, o.velocidadeEsqMmS());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 150.0f, o.velocidadeDirMmS());
}

void test_velocidade_so_muda_ao_fechar_a_janela(void) {
    Odometria o;
    o.atualizar(100, 100, 100);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, o.velocidadeMmS());  // janela ainda aberta
    o.atualizar(100, 100, 200);
    TEST_ASSERT_TRUE(o.velocidadeMmS() > 0.0f);
}

void test_parado_por_uma_janela_a_velocidade_volta_a_zero(void) {
    Odometria o;
    o.atualizar(100, 100, 200);
    TEST_ASSERT_TRUE(o.velocidadeMmS() > 0.0f);
    o.atualizar(100, 100, 400);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, o.velocidadeMmS());
}

void test_giro_no_lugar_tem_velocidade_media_nula(void) {
    Odometria o;
    o.atualizar(-50, 50, 200);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, o.velocidadeMmS());
    TEST_ASSERT_TRUE(o.velocidadeEsqMmS() < 0.0f);
    TEST_ASSERT_TRUE(o.velocidadeDirMmS() > 0.0f);
}

void test_velocidade_na_volta_do_millis(void) {
    Odometria o;
    o.reiniciar(0, 0, 0xFFFFFF00u);
    o.atualizar(100, 100, 0xFFFFFF00u + 200u);  // atravessa o zero
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 100 * RODA_MM_POR_PULSO / 0.2f, o.velocidadeMmS());
}

// ---------------------------------------------------------------------------
// Instante do último pulso (IFontePulsos, R6)
// ---------------------------------------------------------------------------

void test_registra_o_instante_em_que_cada_contagem_mudou(void) {
    Odometria o;
    o.reiniciar(0, 0, 0);
    o.atualizar(0, 0, 50);
    TEST_ASSERT_EQUAL_UINT32(0, o.getUltimoPulsoEsqMs());
    TEST_ASSERT_EQUAL_UINT32(0, o.getUltimoPulsoDirMs());
    o.atualizar(1, 0, 60);
    TEST_ASSERT_EQUAL_UINT32(60, o.getUltimoPulsoEsqMs());
    TEST_ASSERT_EQUAL_UINT32(0, o.getUltimoPulsoDirMs());
    o.atualizar(1, -1, 70);  // pulso para trás também é pulso
    TEST_ASSERT_EQUAL_UINT32(60, o.getUltimoPulsoEsqMs());
    TEST_ASSERT_EQUAL_UINT32(70, o.getUltimoPulsoDirMs());
    TEST_ASSERT_EQUAL_UINT32(70, o.getUltimoPulsoMs());
}

void test_reiniciar_renova_o_instante_do_ultimo_pulso(void) {
    Odometria o;
    o.reiniciar(5, 5, 1000);
    TEST_ASSERT_EQUAL_UINT32(1000, o.getUltimoPulsoEsqMs());
    TEST_ASSERT_EQUAL_UINT32(1000, o.getUltimoPulsoDirMs());
}

void test_ultimo_pulso_geral_na_volta_do_millis(void) {
    Odometria o;
    o.reiniciar(0, 0, 0xFFFFFFF0u);
    o.atualizar(0, 1, 0x10u);  // a direita pulsou depois da volta
    TEST_ASSERT_EQUAL_UINT32(0x10u, o.getUltimoPulsoMs());
}

// Fonte de tração mínima para o monitor.
struct TracaoFixa : IFonteTracao {
    bool esq, dir;
    int paradas;
    TracaoFixa(bool e, bool d) : esq(e), dir(d), paradas(0) {}
    bool tracaoAtiva() const override { return esq || dir; }
    bool tracaoEsqAtiva() const override { return esq; }
    bool tracaoDirAtiva() const override { return dir; }
    void parar() override {
        esq = dir = false;
        paradas++;
    }
};

void test_monitor_de_travamento_sobre_a_odometria(void) {
    Odometria o;
    o.reiniciar(0, 0, 0);
    TracaoFixa tracao(true, true);
    MonitorTravamento monitor(tracao, o);
    int32_t enc = 0;
    // Rodas girando por 2 s: sem corte.
    for (uint32_t t = 0; t <= 2000; t += 10) {
        o.atualizar(enc, enc, t);
        enc += 2;
        if (t % 50 == 0) TEST_ASSERT_FALSE(monitor.verificar(t));
    }
    // A roda direita trava: corte em até 1,2 s.
    const int32_t encDirTravado = enc;
    uint32_t t = 2010;
    for (; t <= 3500; t += 10) {
        o.atualizar(enc, encDirTravado, t);
        enc += 2;
        if (t % 50 == 0 && monitor.verificar(t)) break;
    }
    TEST_ASSERT_TRUE(monitor.travado());
    TEST_ASSERT_EQUAL(RODA_DIREITA, monitor.rodaTravada());
    TEST_ASSERT_TRUE(t - 2000 <= 1200);
    TEST_ASSERT_EQUAL(1, tracao.paradas);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_uma_volta_da_roda_percorre_a_circunferencia);
    RUN_TEST(test_alvo_de_uma_celula_em_pulsos);
    RUN_TEST(test_alvo_acompanha_a_constante_de_pulsos_por_volta);
    RUN_TEST(test_reiniciar_toma_as_contagens_atuais_como_origem);
    RUN_TEST(test_giro_no_lugar_de_90_graus_a_esquerda);
    RUN_TEST(test_giro_a_direita_tem_rumo_negativo);
    RUN_TEST(test_velocidade_constante_na_janela);
    RUN_TEST(test_velocidade_so_muda_ao_fechar_a_janela);
    RUN_TEST(test_parado_por_uma_janela_a_velocidade_volta_a_zero);
    RUN_TEST(test_giro_no_lugar_tem_velocidade_media_nula);
    RUN_TEST(test_velocidade_na_volta_do_millis);
    RUN_TEST(test_registra_o_instante_em_que_cada_contagem_mudou);
    RUN_TEST(test_reiniciar_renova_o_instante_do_ultimo_pulso);
    RUN_TEST(test_ultimo_pulso_geral_na_volta_do_millis);
    RUN_TEST(test_monitor_de_travamento_sobre_a_odometria);
    return UNITY_END();
}
