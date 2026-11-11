#include <math.h>
#include <stdint.h>
#include <unity.h>
#include "../../src/movimento/ExecutorMovimentoRobo.h"

// Testes do ExecutorMovimentoRobo (docs/firmware/integracao-loop.md, seção 3.5) sobre uma
// bancada simulada: os encoders contam a partir do PWM aplicado na ponte (o inverso do
// ConversorVelocidadePwm, sem inércia) e os ToF entregam uma amostra a cada 33 ms.

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Bancada simulada
// ---------------------------------------------------------------------------

struct PonteFalsa : IPonteH {
    int16_t esq, dir;
    int aplicacoes, freios, solturas;
    int16_t maiorPwm;
    PonteFalsa() : esq(0), dir(0), aplicacoes(0), freios(0), solturas(0), maiorPwm(0) {}
    void aplicar(int16_t e, int16_t d) override {
        esq = e;
        dir = d;
        aplicacoes++;
        const int16_t me = e < 0 ? (int16_t)-e : e;
        const int16_t md = d < 0 ? (int16_t)-d : d;
        if (me > maiorPwm) maiorPwm = me;
        if (md > maiorPwm) maiorPwm = md;
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

struct ToFFalso : ISensoresToF {
    LeituraToF esq, frente, dir;
    void atualizar(uint32_t) override {}
    LeituraToF& esquerdo() override { return esq; }
    LeituraToF& frontal() override { return frente; }
    LeituraToF& direito() override { return dir; }
};

struct Bancada {
    PonteFalsa ponte;
    Motores motores;
    Odometria odo;
    ToFFalso tof;
    ExecutorMovimentoRobo exec;
    float encEsq, encDir;
    uint32_t t;
    bool paredeFrente;
    float frontalInicialMm;  // distância à parede da frente no início
    uint16_t frontalFixoMm;  // obstáculo parado à frente (0 = nenhum)
    bool rodasTravadas;
    uint32_t ultimoMovimentoMs;

    Bancada()
        : motores(ponte, false, false), exec(motores, odo, tof), encEsq(0), encDir(0), t(0),
          paredeFrente(false), frontalInicialMm(0), frontalFixoMm(0), rodasTravadas(false),
          ultimoMovimentoMs(0) {
        odo.reiniciar(0, 0, 0);
    }

    // Inverso do conversor: velocidade da roda para o PWM físico aplicado.
    static float velocidadeDoPwm(int16_t pwm) {
        const float m = (float)(pwm < 0 ? -pwm : pwm);
        if (m < PWM_MINIMO) return 0.0f;
        const float v = (m - PWM_MINIMO) / PWM_POR_MM_S;
        return pwm < 0 ? -v : v;
    }

    void passo1ms() {
        t++;
        if (!rodasTravadas) {
            encEsq += velocidadeDoPwm(ponte.esq) / 1000.0f / RODA_MM_POR_PULSO;
            encDir += velocidadeDoPwm(ponte.dir) / 1000.0f / RODA_MM_POR_PULSO;
        }
        if (ponte.esq != 0 || ponte.dir != 0) ultimoMovimentoMs = t;
        odo.atualizar((int32_t)encEsq, (int32_t)encDir, t);
        if (t % 33 == 0) {
            tof.esq.registrar(40, t);
            tof.dir.registrar(40, t);
            if (frontalFixoMm > 0) {
                tof.frente.registrar(frontalFixoMm, t);
            } else if (paredeFrente) {
                tof.frente.registrar((uint16_t)(frontalInicialMm - odo.deslocamentoMm()), t);
            } else {
                tof.frente.registrar(TOF_LEITURA_INVALIDA_MM, t);  // sem alvo ao alcance
            }
        }
    }

    // Roda até o movimento terminar (ou maxMs), chamando o executor a cada 10 ms.
    ResultadoMovimento rodar(uint32_t maxMs = 10000) {
        const uint32_t fim = t + maxMs;
        while (t < fim) {
            passo1ms();
            if (t % 10 == 0) {
                const ResultadoMovimento r = exec.atualizar(t);
                if (r == MOV_CONCLUIDO || r == MOV_FALHOU) return r;
            }
        }
        return MOV_EM_ANDAMENTO;
    }
};

// ---------------------------------------------------------------------------
// Parâmetros (valores do hw/firm)
// ---------------------------------------------------------------------------

void test_parametros_padrao_partem_do_hw_firm(void) {
    const ParametrosExecutor p = ExecutorMovimentoRobo::parametrosPadrao();
    TEST_ASSERT_EQUAL_INT32(645, p.alvoAvancoPulsos);  // 180 mm em pulsos (o hw/firm usa 168)
    TEST_ASSERT_EQUAL_UINT32(3000, p.tempoLimiteAvancoMs);
    TEST_ASSERT_EQUAL_UINT32(2000, p.tempoLimiteCurvaMs);
    TEST_ASSERT_EQUAL_UINT32(200, p.tempoFreioMs);
    TEST_ASSERT_EQUAL_UINT16(DIST_MINIMA_SEGURA_MM, p.distMinimaSeguraMm);
    TEST_ASSERT_EQUAL_UINT16(DIST_ENCOSTO_FRONTAL_MM, p.distEncostoFrontalMm);
    TEST_ASSERT_EQUAL_FLOAT((float)LIMIAR_PAREDE_MM, ExecutorMovimentoRobo::parametrosReta().distMaxParedeMm);
    TEST_ASSERT_EQUAL_UINT32(2000, ExecutorMovimentoRobo::parametrosCurva(p).tempoLimiteMs);
}

void test_rumo_nominal_de_cada_orientacao(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rumoNominalGraus(DIR_LESTE));
    TEST_ASSERT_EQUAL_FLOAT(90.0f, rumoNominalGraus(DIR_NORTE));
    TEST_ASSERT_EQUAL_FLOAT(180.0f, rumoNominalGraus(DIR_OESTE));
    TEST_ASSERT_EQUAL_FLOAT(-90.0f, rumoNominalGraus(DIR_SUL));
}

// ---------------------------------------------------------------------------
// Giro
// ---------------------------------------------------------------------------

void test_giro_a_esquerda_conclui_em_90_graus(void) {
    Bancada b;
    TEST_ASSERT_TRUE(b.exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 0));
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 90.0f, b.odo.rumoGraus());
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, b.odo.deslocamentoMm());  // no próprio eixo
    TEST_ASSERT_TRUE(b.ponte.maiorPwm <= PWM_MAXIMO_MOVIMENTO);
    TEST_ASSERT_TRUE(b.exec.ocioso());
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, b.exec.motivoUltimaFalha());
}

void test_giro_a_direita_conclui_em_menos_90_graus(void) {
    Bancada b;
    b.exec.iniciarGiro(GIRO_DIREITA, DIR_NORTE, 0);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, -90.0f, b.odo.rumoGraus());
}

void test_meia_volta_sao_duas_curvas(void) {
    Bancada b;
    b.exec.iniciarGiro(GIRO_MEIA_VOLTA, DIR_LESTE, 0);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_FLOAT_WITHIN(3.0f, -180.0f, b.odo.rumoGraus());
}

void test_curvas_seguidas_sao_relativas_sem_imu(void) {
    // B5: cada curva parte do rumo nominal da orientação atual; sem isso a segunda mira 180.
    Bancada b;
    const Direcao ordem[4] = {DIR_LESTE, DIR_NORTE, DIR_OESTE, DIR_SUL};
    for (int i = 0; i < 4; i++) {
        const float antes = b.odo.rumoGraus();
        TEST_ASSERT_TRUE(b.exec.iniciarGiro(GIRO_ESQUERDA, ordem[i], b.t));
        TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
        TEST_ASSERT_FLOAT_WITHIN(2.0f, 90.0f, b.odo.rumoGraus() - antes);
    }
    TEST_ASSERT_FLOAT_WITHIN(4.0f, 360.0f, b.odo.rumoGraus());
}

void test_fim_do_giro_freia_e_espera_200_ms(void) {
    Bancada b;
    b.exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 0);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_TRUE(b.t - b.ultimoMovimentoMs >= 200);  // resultado só depois do freio
    TEST_ASSERT_TRUE(b.ponte.freios >= 1);
    TEST_ASSERT_TRUE(b.ponte.solturas >= 1);  // roda livre no fim, como no hw/firm
}

void test_curva_com_rodas_travadas_falha_por_tempo(void) {
    Bancada b;
    b.rodasTravadas = true;
    b.exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 0);
    TEST_ASSERT_EQUAL(MOV_FALHOU, b.rodar());
    TEST_ASSERT_EQUAL(FALHA_TEMPO, b.exec.motivoUltimaFalha());
    TEST_ASSERT_TRUE(b.t > 2000);
    TEST_ASSERT_TRUE(b.t <= 2000 + 200 + 20);  // limite da curva + freio
}

// ---------------------------------------------------------------------------
// Avanço
// ---------------------------------------------------------------------------

void test_avanco_termina_pela_odometria_em_uma_celula(void) {
    Bancada b;
    TEST_ASSERT_TRUE(b.exec.iniciarAvanco(0));
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_FLOAT_WITHIN(4.0f, (float)CELULA_MM, b.odo.deslocamentoMm());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 0.0f, b.odo.rumoGraus());  // centrado, sem desvio
    TEST_ASSERT_TRUE(b.ponte.maiorPwm <= PWM_MAXIMO_MOVIMENTO);
    TEST_ASSERT_TRUE(b.t - b.ultimoMovimentoMs >= 200);
}

void test_avanco_com_parede_a_frente_termina_no_encosto(void) {
    Bancada b;
    b.paredeFrente = true;
    b.frontalInicialMm = 210.0f;  // o encosto (45 mm) chega antes dos 180 mm
    b.exec.iniciarAvanco(0);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_TRUE(b.odo.deslocamentoMm() < 175.0f);
    TEST_ASSERT_TRUE(b.odo.deslocamentoMm() > 150.0f);
}

void test_obstaculo_a_frente_e_emergencia_frontal(void) {
    Bancada b;
    b.frontalFixoMm = 25;
    b.exec.iniciarAvanco(0);
    TEST_ASSERT_EQUAL(MOV_FALHOU, b.rodar());
    TEST_ASSERT_EQUAL(FALHA_EMERGENCIA_FRONTAL, b.exec.motivoUltimaFalha());
    TEST_ASSERT_TRUE(b.odo.deslocamentoMm() < 0.6f * CELULA_MM);
    TEST_ASSERT_TRUE(b.ponte.freios >= 1);
}

void test_avanco_com_rodas_travadas_falha_por_tempo(void) {
    Bancada b;
    b.rodasTravadas = true;
    b.exec.iniciarAvanco(0);
    TEST_ASSERT_EQUAL(MOV_FALHOU, b.rodar());
    TEST_ASSERT_EQUAL(FALHA_TEMPO, b.exec.motivoUltimaFalha());
    TEST_ASSERT_TRUE(b.t > 3000);
}

void test_reta_so_roda_com_amostra_nova_dos_laterais(void) {
    // R11: ~30 amostras por segundo, e não um comando a cada tick de 10 ms.
    Bancada b;
    b.exec.iniciarAvanco(0);
    b.rodar();
    const uint32_t duracao = b.ultimoMovimentoMs;
    const int esperadoMaximo = (int)(duracao / 33) + 2;
    TEST_ASSERT_TRUE(b.ponte.aplicacoes <= esperadoMaximo);
    TEST_ASSERT_TRUE(b.ponte.aplicacoes >= (int)(duracao / 33) - 2);
}

// ---------------------------------------------------------------------------
// Trinco, recusa e aborto
// ---------------------------------------------------------------------------

void test_trinco_fechado_por_fora_encerra_com_tracao_cortada(void) {
    Bancada b;
    b.exec.iniciarAvanco(0);
    while (b.t < 300) {
        b.passo1ms();
        if (b.t % 10 == 0) b.exec.atualizar(b.t);
    }
    b.motores.parar();  // MonitorTravamento ou ParadaObjetivo
    const int aplicacoes = b.ponte.aplicacoes;
    TEST_ASSERT_EQUAL(MOV_FALHOU, b.exec.atualizar(310));
    TEST_ASSERT_EQUAL(FALHA_TRACAO_CORTADA, b.exec.motivoUltimaFalha());
    TEST_ASSERT_TRUE(b.exec.ocioso());
    TEST_ASSERT_EQUAL(aplicacoes, b.ponte.aplicacoes);  // nenhum comando depois do corte
    TEST_ASSERT_EQUAL(MOV_OCIOSO, b.exec.atualizar(320));
}

void test_pedidos_recusados_so_devolvem_false(void) {
    Bancada b;
    TEST_ASSERT_FALSE(b.exec.iniciarGiro(GIRO_NENHUM, DIR_LESTE, 0));
    TEST_ASSERT_TRUE(b.exec.iniciarAvanco(0));
    TEST_ASSERT_FALSE(b.exec.iniciarAvanco(1));
    TEST_ASSERT_FALSE(b.exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, 1));
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, b.exec.motivoUltimaFalha());
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
}

void test_abortar_freia_e_libera_para_novo_movimento(void) {
    Bancada b;
    b.exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 0);
    while (b.t < 200) {
        b.passo1ms();
        if (b.t % 10 == 0) b.exec.atualizar(b.t);
    }
    const int freios = b.ponte.freios;
    b.exec.abortar();
    TEST_ASSERT_TRUE(b.exec.ocioso());
    TEST_ASSERT_EQUAL(freios + 1, b.ponte.freios);
    TEST_ASSERT_EQUAL(MOV_OCIOSO, b.exec.atualizar(b.t));
    // Uma curva nova depois do aborto é aceita e conclui (o ControleCurva é recriado).
    const float antes = b.odo.rumoGraus();
    TEST_ASSERT_TRUE(b.exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, b.t));
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, b.rodar());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, -90.0f, b.odo.rumoGraus() - antes);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parametros_padrao_partem_do_hw_firm);
    RUN_TEST(test_rumo_nominal_de_cada_orientacao);
    // Giro
    RUN_TEST(test_giro_a_esquerda_conclui_em_90_graus);
    RUN_TEST(test_giro_a_direita_conclui_em_menos_90_graus);
    RUN_TEST(test_meia_volta_sao_duas_curvas);
    RUN_TEST(test_curvas_seguidas_sao_relativas_sem_imu);
    RUN_TEST(test_fim_do_giro_freia_e_espera_200_ms);
    RUN_TEST(test_curva_com_rodas_travadas_falha_por_tempo);
    // Avanço
    RUN_TEST(test_avanco_termina_pela_odometria_em_uma_celula);
    RUN_TEST(test_avanco_com_parede_a_frente_termina_no_encosto);
    RUN_TEST(test_obstaculo_a_frente_e_emergencia_frontal);
    RUN_TEST(test_avanco_com_rodas_travadas_falha_por_tempo);
    RUN_TEST(test_reta_so_roda_com_amostra_nova_dos_laterais);
    // Trinco, recusa e aborto
    RUN_TEST(test_trinco_fechado_por_fora_encerra_com_tracao_cortada);
    RUN_TEST(test_pedidos_recusados_so_devolvem_false);
    RUN_TEST(test_abortar_freia_e_libera_para_novo_movimento);
    return UNITY_END();
}
