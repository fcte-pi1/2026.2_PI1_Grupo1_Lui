#include <math.h>
#include <stdint.h>
#include <unity.h>
#include "../../src/controle/ControleCurva.h"

void setUp(void) {}
void tearDown(void) {}

static const float LIMITE_RF_SFT01_GRAUS = 3.0f;  // erro angular acumulado maximo

// ---------------------------------------------------------------------------
// Auxiliares dos testes de unidade
// ---------------------------------------------------------------------------

static EntradaControleCurva leitura(float yaw, uint32_t agoraMs = 0) {
    EntradaControleCurva e;
    e.yawGraus = yaw;
    e.imuValido = true;
    e.encEsq = 0;
    e.encDir = 0;
    e.agoraMs = agoraMs;
    return e;
}

// ---------------------------------------------------------------------------
// Sentido de giro e velocidades
// ---------------------------------------------------------------------------

void test_curva_a_esquerda_gira_roda_esquerda_para_tras(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    const SaidaControleCurva s = c.atualizar(leitura(0.0f, 10));
    TEST_ASSERT_TRUE(s.velEsqMmS < 0.0f);
    TEST_ASSERT_TRUE(s.velDirMmS > 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -s.velEsqMmS, s.velDirMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, s.erroGraus);
}

void test_curva_a_direita_gira_roda_direita_para_tras(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_DIREITA, 0.0f, 0, 0, 0);
    const SaidaControleCurva s = c.atualizar(leitura(0.0f, 10));
    TEST_ASSERT_TRUE(s.velEsqMmS > 0.0f);
    TEST_ASSERT_TRUE(s.velDirMmS < 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -90.0f, s.erroGraus);
}

void test_velocidade_e_limitada_pelo_maximo(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    const SaidaControleCurva s = c.atualizar(leitura(0.0f, 10));  // erro de 90 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.velocidadeMaxMmS, s.velDirMmS);
}

void test_velocidade_proporcional_ao_erro_entre_o_piso_e_o_teto(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    const SaidaControleCurva s = c.atualizar(leitura(70.0f, 10));  // erro de 20 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpGiro * 20.0f, s.velDirMmS);
}

void test_perto_do_alvo_a_velocidade_nao_cai_abaixo_do_piso(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    const SaidaControleCurva s = c.atualizar(leitura(88.0f, 10));  // erro de 2 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.velocidadeMinMmS, s.velDirMmS);
}

void test_ultrapassagem_e_corrigida_no_sentido_oposto(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    const SaidaControleCurva s = c.atualizar(leitura(95.0f, 10));  // passou 5 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -5.0f, s.erroGraus);
    TEST_ASSERT_TRUE(s.velEsqMmS > 0.0f);
    TEST_ASSERT_TRUE(s.velDirMmS < 0.0f);
}

// ---------------------------------------------------------------------------
// Conclusao, assentamento e orientacao
// ---------------------------------------------------------------------------

void test_dentro_da_tolerancia_para_e_so_conclui_apos_o_assentamento(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);

    SaidaControleCurva s = c.atualizar(leitura(89.5f, 500));
    TEST_ASSERT_EQUAL(CURVA_GIRANDO, s.estado);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velDirMmS);

    s = c.atualizar(leitura(89.6f, 500 + p.assentamentoMs - 1));
    TEST_ASSERT_EQUAL(CURVA_GIRANDO, s.estado);

    s = c.atualizar(leitura(89.6f, 500 + p.assentamentoMs));
    TEST_ASSERT_EQUAL(CURVA_CONCLUIDA, s.estado);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velDirMmS);
}

void test_inercia_que_leva_para_fora_da_tolerancia_e_corrigida_antes_de_concluir(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    c.atualizar(leitura(89.5f, 500));                              // entra na tolerancia
    SaidaControleCurva s = c.atualizar(leitura(92.0f, 550));       // a inercia levou 2 graus alem
    TEST_ASSERT_EQUAL(CURVA_GIRANDO, s.estado);
    TEST_ASSERT_TRUE(s.velEsqMmS > 0.0f);                          // volta
    s = c.atualizar(leitura(90.0f, 600));                          // recomeca o assentamento
    TEST_ASSERT_EQUAL(CURVA_GIRANDO, s.estado);
    s = c.atualizar(leitura(90.0f, 700));
    TEST_ASSERT_EQUAL(CURVA_CONCLUIDA, s.estado);
}

void test_orientacao_so_muda_ao_concluir(void) {
    ControleCurva c;  // parte virado para o leste
    TEST_ASSERT_EQUAL(ORIENTACAO_LESTE, c.orientacao());
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    c.atualizar(leitura(30.0f, 100));
    TEST_ASSERT_EQUAL(ORIENTACAO_LESTE, c.orientacao());
    c.atualizar(leitura(90.0f, 200));
    TEST_ASSERT_EQUAL(ORIENTACAO_LESTE, c.orientacao());  // ainda assentando
    c.atualizar(leitura(90.0f, 300));
    TEST_ASSERT_EQUAL(CURVA_CONCLUIDA, c.estado());
    TEST_ASSERT_EQUAL(ORIENTACAO_NORTE, c.orientacao());
}

static void girarIdeal(ControleCurva& c, SentidoCurva sentido, float& yaw, uint32_t& t) {
    c.iniciar(sentido, yaw, 0, 0, t);
    yaw = ControleCurva::normalizarGraus(yaw + (sentido == SENTIDO_ESQUERDA ? 90.0f : -90.0f));
    c.atualizar(leitura(yaw, t + 100));
    c.atualizar(leitura(yaw, t + 300));
    t += 1000;
}

void test_sequencia_de_curvas_percorre_as_quatro_orientacoes(void) {
    ControleCurva c;  // leste
    float yaw = 0.0f;
    uint32_t t = 0;
    const Orientacao esperadoEsq[4] = {ORIENTACAO_NORTE, ORIENTACAO_OESTE, ORIENTACAO_SUL, ORIENTACAO_LESTE};
    for (int i = 0; i < 4; i++) {
        girarIdeal(c, SENTIDO_ESQUERDA, yaw, t);
        TEST_ASSERT_EQUAL(esperadoEsq[i], c.orientacao());
    }
    const Orientacao esperadoDir[4] = {ORIENTACAO_SUL, ORIENTACAO_OESTE, ORIENTACAO_NORTE, ORIENTACAO_LESTE};
    for (int i = 0; i < 4; i++) {
        girarIdeal(c, SENTIDO_DIREITA, yaw, t);
        TEST_ASSERT_EQUAL(esperadoDir[i], c.orientacao());
    }
}

void test_ids_da_orientacao_batem_com_o_codec_de_telemetria(void) {
    // telemetriaNomeOrientacao(): {"NORTE", "SUL", "LESTE", "OESTE"}
    TEST_ASSERT_EQUAL_UINT8(0, ORIENTACAO_NORTE);
    TEST_ASSERT_EQUAL_UINT8(1, ORIENTACAO_SUL);
    TEST_ASSERT_EQUAL_UINT8(2, ORIENTACAO_LESTE);
    TEST_ASSERT_EQUAL_UINT8(3, ORIENTACAO_OESTE);
}

void test_estado_parado_e_concluido_mantem_as_rodas_paradas(void) {
    ControleCurva c;
    SaidaControleCurva s = c.atualizar(leitura(10.0f, 0));
    TEST_ASSERT_EQUAL(CURVA_PARADA, s.estado);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velDirMmS);

    float yaw = 0.0f;
    uint32_t t = 0;
    girarIdeal(c, SENTIDO_ESQUERDA, yaw, t);
    s = c.atualizar(leitura(40.0f, t));  // mesmo com leitura estranha, nao volta a girar
    TEST_ASSERT_EQUAL(CURVA_CONCLUIDA, s.estado);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velDirMmS);
}

// ---------------------------------------------------------------------------
// Alvo absoluto: o erro de uma curva nao passa para a seguinte
// ---------------------------------------------------------------------------

void test_segunda_curva_mira_no_rumo_ideal_e_nao_no_medido(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    c.atualizar(leitura(89.1f, 100));  // terminou 0,9 grau antes do alvo, dentro da tolerancia
    c.atualizar(leitura(89.1f, 200));
    TEST_ASSERT_EQUAL(CURVA_CONCLUIDA, c.estado());

    c.iniciar(SENTIDO_ESQUERDA, 89.1f, 0, 0, 1000);
    const SaidaControleCurva s = c.atualizar(leitura(89.1f, 1010));
    // Rumo ideal = 180 graus. Se somasse 90 ao medido, o erro seria 90 e o alvo, 179,1.
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.9f, s.erroGraus);
}

void test_referencia_inicial_pode_ser_qualquer_yaw(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 37.0f, 0, 0, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, c.atualizar(leitura(37.0f, 10)).erroGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, c.atualizar(leitura(127.0f, 20)).erroGraus);
}

void test_ressincronizar_absorve_a_deriva_do_giroscopio(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    c.atualizar(leitura(90.0f, 100));
    c.atualizar(leitura(90.0f, 200));
    // Depois do realinhamento pelos ToF, o giroscopio le 93 com o robo alinhado.
    c.ressincronizar(93.0f);
    c.iniciar(SENTIDO_ESQUERDA, 93.0f, 0, 0, 1000);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, c.atualizar(leitura(93.0f, 1010)).erroGraus);
}

void test_yaw_atravessa_a_fronteira_de_180_graus(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 170.0f, 0, 0, 0);  // alvo: 260 = -100 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, c.atualizar(leitura(170.0f, 10)).erroGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 40.0f, c.atualizar(leitura(-140.0f, 20)).erroGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, c.atualizar(leitura(-100.0f, 30)).erroGraus);
}

void test_curva_a_direita_atravessa_a_fronteira_de_menos_180(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_DIREITA, -170.0f, 0, 0, 0);  // alvo: -260 = 100 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -90.0f, c.atualizar(leitura(-170.0f, 10)).erroGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f, c.atualizar(leitura(110.0f, 20)).erroGraus);
}

void test_iniciar_durante_uma_curva_e_ignorado(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    c.iniciar(SENTIDO_ESQUERDA, 30.0f, 0, 0, 100);  // nao pode somar outros 90 graus
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 90.0f, c.atualizar(leitura(0.0f, 110)).erroGraus);
}

// ---------------------------------------------------------------------------
// Falha por tempo
// ---------------------------------------------------------------------------

void test_sensor_travado_falha_e_nao_atualiza_a_orientacao(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    SaidaControleCurva s = c.atualizar(leitura(0.0f, p.tempoLimiteMs));
    TEST_ASSERT_EQUAL(CURVA_GIRANDO, s.estado);
    s = c.atualizar(leitura(0.0f, p.tempoLimiteMs + 1));
    TEST_ASSERT_EQUAL(CURVA_FALHOU, s.estado);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velDirMmS);
    TEST_ASSERT_EQUAL(ORIENTACAO_LESTE, c.orientacao());
}

void test_tempo_limite_correto_na_volta_do_millis(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0xFFFFFFFFu - 500);
    TEST_ASSERT_EQUAL(CURVA_GIRANDO, c.atualizar(leitura(0.0f, 1000)).estado);  // 1.501 ms depois
    TEST_ASSERT_EQUAL(CURVA_FALHOU, c.atualizar(leitura(0.0f, 3000)).estado);   // 3.501 ms depois
}

// ---------------------------------------------------------------------------
// Encoders: conferencia e reserva do IMU
// ---------------------------------------------------------------------------

void test_giro_pelos_encoders_e_reportado(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 5000, 5200, 0);  // contagens ja diferentes na largada
    EntradaControleCurva e = leitura(10.0f, 10);
    e.encEsq = 5000 - 100;
    e.encDir = 5200 + 100;  // rodas em sentidos opostos: giro a esquerda
    const SaidaControleCurva s = c.atualizar(e);
    const float esperado = (200.0f * p.mmPorPulso / p.bitolaMm) * 57.29578f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, esperado, s.giroEncodersGraus);
}

void test_discrepancia_entre_imu_e_encoders_e_reportada(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    EntradaControleCurva e = leitura(30.0f, 10);
    e.encEsq = -200;
    e.encDir = 200;  // patinagem: os encoders dizem que girou mais do que o IMU mediu
    const SaidaControleCurva s = c.atualizar(e);
    const float giroEnc = (400.0f * p.mmPorPulso / p.bitolaMm) * 57.29578f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f - giroEnc, s.discrepanciaGraus);
}

void test_com_imu_valido_os_encoders_nao_interferem_no_controle(void) {
    ControleCurva c;
    c.iniciar(SENTIDO_ESQUERDA, 0.0f, 0, 0, 0);
    EntradaControleCurva e = leitura(30.0f, 10);
    e.encDir = 5000;  // patinagem
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 60.0f, c.atualizar(e).erroGraus);
}

void test_sem_imu_o_giro_vem_dos_encoders(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    ControleCurva c(p);
    c.iniciar(SENTIDO_ESQUERDA, 20.0f, 0, 0, 0);  // yaw de partida 20; alvo ideal 110
    EntradaControleCurva e = leitura(0.0f, 10);   // o yaw lido agora e lixo
    e.imuValido = false;
    e.encEsq = -200;
    e.encDir = 200;
    const SaidaControleCurva s = c.atualizar(e);
    const float giroEnc = (400.0f * p.mmPorPulso / p.bitolaMm) * 57.29578f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 110.0f - (20.0f + giroEnc), s.erroGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.discrepanciaGraus);
}

// ---------------------------------------------------------------------------
// Simulacao em malha fechada: curvas no proprio eixo (criterio de +-3 graus do RF-SFT01)
//
// Modelo simples: cada roda segue a velocidade pedida com atraso de primeira ordem, o
// robo gira com (velDir - velEsq) / bitola, o IMU le o angulo real com ruido e os
// encoders contam o deslocamento de cada roda. O modelo e uma suposicao: valida a logica
// de controle, mas nao substitui o ensaio no robo.
// ---------------------------------------------------------------------------

struct Gerador {
    uint32_t estado;
    // Uniforme em [-1, 1], deterministico para os testes serem reprodutiveis.
    float proximo() {
        estado = estado * 1664525u + 1013904223u;
        return ((float)((estado >> 8) & 0xFFFF) / 32768.0f) - 1.0f;
    }
};

struct Cenario {
    float periodoS;          // intervalo entre leituras do IMU
    float constanteMotorS;   // atraso de primeira ordem dos motores
    float escalaMotor;       // velocidade real / velocidade pedida (erro de ganho)
    float escalaEncoder;     // pulsos lidos / pulsos reais (patinagem)
    float ruidoImuGraus;     // ruido uniforme de +-ruidoImuGraus na leitura
    bool imuValido;
    float anguloInicialGraus;
};

static Cenario cenarioBase() {
    Cenario c;
    c.periodoS = 0.01f;       // 100 Hz, doc 4.3
    c.constanteMotorS = 0.08f;
    c.escalaMotor = 1.0f;
    c.escalaEncoder = 1.0f;
    c.ruidoImuGraus = 0.1f;
    c.imuValido = true;
    c.anguloInicialGraus = 0.0f;
    return c;
}

struct Robo {
    float anguloReal;  // sem normalizar, para somar voltas
    float velEsq, velDir;
    float percEsq, percDir;
    uint32_t agoraMs;
    Gerador rng;
};

static Robo novoRobo(const Cenario& c) {
    Robo r;
    r.anguloReal = c.anguloInicialGraus;
    r.velEsq = r.velDir = 0.0f;
    r.percEsq = r.percDir = 0.0f;
    r.agoraMs = 0;
    r.rng.estado = 4242u;
    return r;
}

static EntradaControleCurva ler(const Robo& r, Robo& rw, const Cenario& c, float mmPorPulso) {
    EntradaControleCurva e;
    e.yawGraus = ControleCurva::normalizarGraus(r.anguloReal + c.ruidoImuGraus * rw.rng.proximo());
    e.imuValido = c.imuValido;
    e.encEsq = (int32_t)(r.percEsq * c.escalaEncoder / mmPorPulso);
    e.encDir = (int32_t)(r.percDir * c.escalaEncoder / mmPorPulso);
    e.agoraMs = r.agoraMs;
    return e;
}

// Executa uma curva ate o controle sair de CURVA_GIRANDO. Devolve o estado final.
static EstadoControleCurva executarCurva(ControleCurva& ctl, Robo& r, const Cenario& c,
                                         SentidoCurva sentido, float mmPorPulso, float bitolaMm) {
    EntradaControleCurva e0 = ler(r, r, c, mmPorPulso);
    ctl.iniciar(sentido, e0.yawGraus, e0.encEsq, e0.encDir, r.agoraMs);

    const float passoSim = 0.0025f;
    const int passosPorCiclo = (int)(c.periodoS / passoSim + 0.5f);
    for (int ciclo = 0; ciclo < 1000 && ctl.estado() == CURVA_GIRANDO; ciclo++) {
        const EntradaControleCurva e = ler(r, r, c, mmPorPulso);
        const SaidaControleCurva s = ctl.atualizar(e);
        for (int k = 0; k < passosPorCiclo; k++) {
            r.velEsq += (s.velEsqMmS * c.escalaMotor - r.velEsq) * passoSim / c.constanteMotorS;
            r.velDir += (s.velDirMmS * c.escalaMotor - r.velDir) * passoSim / c.constanteMotorS;
            r.anguloReal += ((r.velDir - r.velEsq) / bitolaMm) * 57.29578f * passoSim;
            r.percEsq += r.velEsq * passoSim;
            r.percDir += r.velDir * passoSim;
        }
        r.agoraMs += (uint32_t)(c.periodoS * 1000.0f + 0.5f);
    }
    return ctl.estado();
}

static float erroAbsoluto(const Robo& r, float idealGraus) {
    return fabsf(ControleCurva::normalizarGraus(r.anguloReal - idealGraus));
}

// Padrao misto de esquerda e direita, com mais esquerdas para o rumo dar voltas completas.
static SentidoCurva sentidoDaCurva(int i) { return (i % 3 != 2) ? SENTIDO_ESQUERDA : SENTIDO_DIREITA; }

void test_uma_curva_de_90_graus_fica_dentro_de_3_graus(void) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    const SentidoCurva sentidos[2] = {SENTIDO_ESQUERDA, SENTIDO_DIREITA};
    for (int i = 0; i < 2; i++) {
        const Cenario c = cenarioBase();
        Robo r = novoRobo(c);
        ControleCurva ctl(p);
        const EstadoControleCurva fim = executarCurva(ctl, r, c, sentidos[i], p.mmPorPulso, p.bitolaMm);
        TEST_ASSERT_EQUAL(CURVA_CONCLUIDA, fim);
        const float ideal = (sentidos[i] == SENTIDO_ESQUERDA) ? 90.0f : -90.0f;
        TEST_ASSERT_TRUE_MESSAGE(erroAbsoluto(r, ideal) <= LIMITE_RF_SFT01_GRAUS, "curva fora de +-3 graus");
    }
}

// Varias curvas seguidas: o erro de cada uma e medido contra o ideal ACUMULADO.
static void rodarSequencia(const Cenario& c, int curvas, const char* mensagem) {
    const ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    Robo r = novoRobo(c);
    ControleCurva ctl(p);
    float ideal = c.anguloInicialGraus;
    for (int i = 0; i < curvas; i++) {
        const SentidoCurva sentido = sentidoDaCurva(i);
        ideal += (sentido == SENTIDO_ESQUERDA) ? 90.0f : -90.0f;
        const EstadoControleCurva fim = executarCurva(ctl, r, c, sentido, p.mmPorPulso, p.bitolaMm);
        TEST_ASSERT_EQUAL_MESSAGE(CURVA_CONCLUIDA, fim, mensagem);
        TEST_ASSERT_TRUE_MESSAGE(erroAbsoluto(r, ideal) <= LIMITE_RF_SFT01_GRAUS, mensagem);
    }
}

void test_24_curvas_seguidas_nao_acumulam_erro(void) {
    rodarSequencia(cenarioBase(), 24, "erro acumulado >= 3 graus em 24 curvas");
}

void test_o_yaw_do_imu_atravessando_180_graus_nao_atrapalha(void) {
    Cenario c = cenarioBase();
    c.anguloInicialGraus = 170.0f;
    rodarSequencia(c, 12, "erro apos atravessar a fronteira de 180 graus");
}

void test_erro_de_ganho_dos_motores_nao_acumula(void) {
    const float escalas[2] = {0.8f, 1.2f};
    for (int i = 0; i < 2; i++) {
        Cenario c = cenarioBase();
        c.escalaMotor = escalas[i];
        rodarSequencia(c, 12, "erro de ganho dos motores");
    }
}

void test_ruido_de_leitura_do_imu(void) {
    Cenario c = cenarioBase();
    c.ruidoImuGraus = 0.5f;
    rodarSequencia(c, 12, "ruido de +-0,5 grau no IMU");
}

void test_patinagem_nos_encoders_nao_afeta_a_curva_pelo_imu(void) {
    Cenario c = cenarioBase();
    c.escalaEncoder = 1.3f;  // os encoders contam 30% a mais do que as rodas andaram
    rodarSequencia(c, 12, "encoders com patinagem");
}

void test_funciona_com_leituras_a_20_hz(void) {
    Cenario c = cenarioBase();
    c.periodoS = 0.05f;  // mesmo ritmo minimo dos ToF laterais
    rodarSequencia(c, 12, "controle a 20 Hz");
}

void test_sem_imu_os_encoders_fecham_a_curva_dentro_de_3_graus(void) {
    Cenario c = cenarioBase();
    c.imuValido = false;
    rodarSequencia(c, 12, "so com encoders");
}

void test_sem_ganho_a_curva_nao_conclui(void) {
    // Controle do proprio simulador: com ganho e piso zerados o robo nao sai do lugar e
    // a curva tem de falhar por tempo. Se este teste passar a falhar, a simulacao
    // perdeu sensibilidade.
    ParametrosControleCurva p = ControleCurva::parametrosPadrao();
    p.kpGiro = 0.0f;
    p.velocidadeMinMmS = 0.0f;
    const Cenario c = cenarioBase();
    Robo r = novoRobo(c);
    ControleCurva ctl(p);
    TEST_ASSERT_EQUAL(CURVA_FALHOU, executarCurva(ctl, r, c, SENTIDO_ESQUERDA, p.mmPorPulso, p.bitolaMm));
}

int main(void) {
    UNITY_BEGIN();
    // Sentido e velocidades
    RUN_TEST(test_curva_a_esquerda_gira_roda_esquerda_para_tras);
    RUN_TEST(test_curva_a_direita_gira_roda_direita_para_tras);
    RUN_TEST(test_velocidade_e_limitada_pelo_maximo);
    RUN_TEST(test_velocidade_proporcional_ao_erro_entre_o_piso_e_o_teto);
    RUN_TEST(test_perto_do_alvo_a_velocidade_nao_cai_abaixo_do_piso);
    RUN_TEST(test_ultrapassagem_e_corrigida_no_sentido_oposto);
    // Conclusao e orientacao
    RUN_TEST(test_dentro_da_tolerancia_para_e_so_conclui_apos_o_assentamento);
    RUN_TEST(test_inercia_que_leva_para_fora_da_tolerancia_e_corrigida_antes_de_concluir);
    RUN_TEST(test_orientacao_so_muda_ao_concluir);
    RUN_TEST(test_sequencia_de_curvas_percorre_as_quatro_orientacoes);
    RUN_TEST(test_ids_da_orientacao_batem_com_o_codec_de_telemetria);
    RUN_TEST(test_estado_parado_e_concluido_mantem_as_rodas_paradas);
    // Alvo absoluto
    RUN_TEST(test_segunda_curva_mira_no_rumo_ideal_e_nao_no_medido);
    RUN_TEST(test_referencia_inicial_pode_ser_qualquer_yaw);
    RUN_TEST(test_ressincronizar_absorve_a_deriva_do_giroscopio);
    RUN_TEST(test_yaw_atravessa_a_fronteira_de_180_graus);
    RUN_TEST(test_curva_a_direita_atravessa_a_fronteira_de_menos_180);
    RUN_TEST(test_iniciar_durante_uma_curva_e_ignorado);
    // Falha por tempo
    RUN_TEST(test_sensor_travado_falha_e_nao_atualiza_a_orientacao);
    RUN_TEST(test_tempo_limite_correto_na_volta_do_millis);
    // Encoders
    RUN_TEST(test_giro_pelos_encoders_e_reportado);
    RUN_TEST(test_discrepancia_entre_imu_e_encoders_e_reportada);
    RUN_TEST(test_com_imu_valido_os_encoders_nao_interferem_no_controle);
    RUN_TEST(test_sem_imu_o_giro_vem_dos_encoders);
    // Malha fechada (criterio de +-3 graus do RF-SFT01)
    RUN_TEST(test_uma_curva_de_90_graus_fica_dentro_de_3_graus);
    RUN_TEST(test_24_curvas_seguidas_nao_acumulam_erro);
    RUN_TEST(test_o_yaw_do_imu_atravessando_180_graus_nao_atrapalha);
    RUN_TEST(test_erro_de_ganho_dos_motores_nao_acumula);
    RUN_TEST(test_ruido_de_leitura_do_imu);
    RUN_TEST(test_patinagem_nos_encoders_nao_afeta_a_curva_pelo_imu);
    RUN_TEST(test_funciona_com_leituras_a_20_hz);
    RUN_TEST(test_sem_imu_os_encoders_fecham_a_curva_dentro_de_3_graus);
    RUN_TEST(test_sem_ganho_a_curva_nao_conclui);
    return UNITY_END();
}
