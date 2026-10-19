#include <math.h>
#include <stdint.h>
#include <unity.h>
#include "../../src/controle/ControleReta.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Auxiliares dos testes de unidade
// ---------------------------------------------------------------------------

static EntradaControleReta leitura(float esq, float dir, uint32_t agoraMs = 0) {
    EntradaControleReta e;
    e.distEsqMm = esq;
    e.distDirMm = dir;
    e.yawGraus = 0.0f;
    e.imuValido = true;
    e.encEsq = 0;
    e.encDir = 0;
    e.agoraMs = agoraMs;
    return e;
}

// Correção aplicada pelo controle: positivo = virar para a esquerda. Só vale sem saturação.
static float giroDe(const SaidaControleReta& s) { return (s.velDirMmS - s.velEsqMmS) / 2.0f; }

// ---------------------------------------------------------------------------
// MODO_PAREDES: controle PD sobre o erro lateral
// ---------------------------------------------------------------------------

void test_centralizado_mantem_a_mesma_velocidade_nas_duas_rodas(void) {
    ControleReta c;
    const SaidaControleReta s = c.atualizar(leitura(20.0f, 20.0f));
    TEST_ASSERT_EQUAL(MODO_PAREDES, s.modo);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.erroLateralMm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, ControleReta::parametrosPadrao().velocidadeBaseMmS, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, s.velEsqMmS, s.velDirMmS);
}

void test_mais_perto_da_parede_direita_vira_para_a_esquerda(void) {
    ControleReta c;
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    const SaidaControleReta s = c.atualizar(leitura(30.0f, 20.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, s.erroLateralMm);
    TEST_ASSERT_TRUE(s.velDirMmS > s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.velocidadeBaseMmS - p.kpParede * 10.0f, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.velocidadeBaseMmS + p.kpParede * 10.0f, s.velDirMmS);
}

void test_mais_perto_da_parede_esquerda_vira_para_a_direita(void) {
    ControleReta c;
    const SaidaControleReta s = c.atualizar(leitura(20.0f, 30.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f, s.erroLateralMm);
    TEST_ASSERT_TRUE(s.velEsqMmS > s.velDirMmS);
}

void test_correcao_e_proporcional_ao_erro(void) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    p.kdParede = 0.0f;
    ControleReta c1(p);
    ControleReta c2(p);
    const float giro10 = giroDe(c1.atualizar(leitura(30.0f, 20.0f)));
    const float giro20 = giroDe(c2.atualizar(leitura(40.0f, 20.0f)));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f * giro10, giro20);
}

void test_correcao_e_limitada(void) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    p.kdParede = 0.0f;
    ControleReta c(p);
    const SaidaControleReta s = c.atualizar(leitura(90.0f, 10.0f));  // erro de 80 mm
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.limiteGiroMmS, giroDe(s));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.velocidadeBaseMmS - p.limiteGiroMmS, s.velEsqMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.velocidadeBaseMmS + p.limiteGiroMmS, s.velDirMmS);
}

void test_velocidade_das_rodas_nunca_fica_negativa(void) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    p.kdParede = 0.0f;
    p.velocidadeBaseMmS = 30.0f;  // menor que o limite de giro
    ControleReta c(p);
    const SaidaControleReta s = c.atualizar(leitura(90.0f, 10.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.velEsqMmS);
    TEST_ASSERT_TRUE(s.velDirMmS > 0.0f);
}

void test_primeira_amostra_nao_tem_derivada(void) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    p.kdParede = 10.0f;
    ControleReta c(p);
    const SaidaControleReta s = c.atualizar(leitura(30.0f, 20.0f, 5000));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 10.0f, giroDe(s));
}

void test_derivada_filtrada_reduz_a_correcao_quando_o_erro_diminui(void) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.atualizar(leitura(40.0f, 20.0f, 0));                         // erro 20
    const SaidaControleReta s = c.atualizar(leitura(30.0f, 20.0f, 50));  // erro 10, 50 ms depois
    const float derivadaBruta = (10.0f - 20.0f) / 0.05f;          // -200 mm/s
    const float derivadaFiltrada = p.filtroDerivada * derivadaBruta;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 10.0f + p.kdParede * derivadaFiltrada, giroDe(s));
}

void test_derivada_correta_na_volta_do_millis(void) {
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.atualizar(leitura(40.0f, 20.0f, 0xFFFFFFFFu - 20));          // erro 20
    const SaidaControleReta s = c.atualizar(leitura(30.0f, 20.0f, 29));  // 50 ms depois, millis() voltou a zero
    const float derivadaFiltrada = p.filtroDerivada * ((10.0f - 20.0f) / 0.05f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 10.0f + p.kdParede * derivadaFiltrada, giroDe(s));
}

void test_duas_leituras_no_mesmo_instante_nao_dividem_por_zero(void) {
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.atualizar(leitura(40.0f, 20.0f, 0));    // erro 20
    c.atualizar(leitura(30.0f, 20.0f, 50));   // erro 10
    const SaidaControleReta s = c.atualizar(leitura(30.0f, 20.0f, 50));  // mesmo instante
    const float derivadaFiltrada = p.filtroDerivada * ((10.0f - 20.0f) / 0.05f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 10.0f + p.kdParede * derivadaFiltrada, giroDe(s));
}

void test_amostra_anterior_muito_antiga_nao_gera_derivada(void) {
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.atualizar(leitura(40.0f, 20.0f, 0));
    const SaidaControleReta s = c.atualizar(leitura(30.0f, 20.0f, 1000));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 10.0f, giroDe(s));
}

// ---------------------------------------------------------------------------
// Validade das leituras laterais
// ---------------------------------------------------------------------------

void test_limites_da_faixa_valida_de_parede(void) {
    const ControleReta c;
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    TEST_ASSERT_TRUE(c.paredeValida(p.distMinParedeMm));
    TEST_ASSERT_FALSE(c.paredeValida(p.distMinParedeMm - 0.1f));
    TEST_ASSERT_TRUE(c.paredeValida(p.distMaxParedeMm));
    TEST_ASSERT_FALSE(c.paredeValida(p.distMaxParedeMm + 0.1f));
    TEST_ASSERT_FALSE(c.paredeValida(-1.0f));
}

void test_falta_de_parede_em_qualquer_lado_muda_para_o_rumo(void) {
    ControleReta c;
    TEST_ASSERT_EQUAL(MODO_RUMO, c.atualizar(leitura(250.0f, 20.0f)).modo);  // sem parede à esquerda
    TEST_ASSERT_EQUAL(MODO_RUMO, c.atualizar(leitura(20.0f, 250.0f)).modo);  // sem parede à direita
    TEST_ASSERT_EQUAL(MODO_RUMO, c.atualizar(leitura(250.0f, 250.0f)).modo); // sem nenhuma
    TEST_ASSERT_EQUAL(MODO_RUMO, c.atualizar(leitura(-1.0f, 20.0f)).modo);   // timeout do ToF
    TEST_ASSERT_EQUAL(MODO_PAREDES, c.atualizar(leitura(20.0f, 20.0f)).modo);
}

void test_no_modo_rumo_o_erro_lateral_e_zero_e_nao_se_compara_as_paredes(void) {
    ControleReta c;
    // Com só uma parede por perto, a diferença entre as leituras não pode virar correção.
    const SaidaControleReta s = c.atualizar(leitura(250.0f, 20.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.erroLateralMm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, s.velEsqMmS, s.velDirMmS);
}

// ---------------------------------------------------------------------------
// MODO_RUMO: IMU e encoders
// ---------------------------------------------------------------------------

void test_rumo_pelo_imu_corrige_o_desvio_para_a_esquerda(void) {
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.iniciar(0.0f, 0, 0, 0);
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.yawGraus = 4.0f;  // virou 4° para a esquerda: tem de voltar para a direita
    const SaidaControleReta s = c.atualizar(e);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, s.erroRumoGraus);
    TEST_ASSERT_TRUE(s.velEsqMmS > s.velDirMmS);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -p.kpRumo * 4.0f, giroDe(s));
}

void test_rumo_pelo_imu_corrige_o_desvio_para_a_direita(void) {
    ControleReta c;
    c.iniciar(0.0f, 0, 0, 0);
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.yawGraus = -4.0f;
    const SaidaControleReta s = c.atualizar(e);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -4.0f, s.erroRumoGraus);
    TEST_ASSERT_TRUE(s.velDirMmS > s.velEsqMmS);
}

void test_rumo_e_medido_em_relacao_ao_inicio_da_reta(void) {
    ControleReta c;
    c.iniciar(90.0f, 0, 0, 0);  // reta começa apontando para o leste
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.yawGraus = 90.0f;
    const SaidaControleReta s = c.atualizar(e);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.erroRumoGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, s.velEsqMmS, s.velDirMmS);
}

void test_erro_de_rumo_atravessa_a_fronteira_de_180_graus(void) {
    ControleReta c;
    c.iniciar(179.0f, 0, 0, 0);
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.yawGraus = -179.0f;  // 2° à esquerda, e não 358°
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, c.atualizar(e).erroRumoGraus);
}

void test_normalizar_graus(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -170.0f, ControleReta::normalizarGraus(190.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 170.0f, ControleReta::normalizarGraus(-190.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, ControleReta::normalizarGraus(360.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, ControleReta::normalizarGraus(730.0f));
}

void test_sem_imu_o_rumo_vem_dos_encoders(void) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.iniciar(0.0f, 0, 0, 0);
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.imuValido = false;
    e.encEsq = 0;
    e.encDir = 100;  // roda direita andou 100 pulsos a mais: o robô girou para a esquerda
    const SaidaControleReta s = c.atualizar(e);
    const float esperado = (100.0f * p.mmPorPulso / p.bitolaMm) * 57.29578f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, esperado, s.erroRumoGraus);
    TEST_ASSERT_TRUE(s.velEsqMmS > s.velDirMmS);
}

void test_contagem_dos_encoders_e_relativa_ao_inicio_da_reta(void) {
    ControleReta c;
    c.iniciar(0.0f, 5000, 5200, 0);  // as contagens já eram diferentes na largada da reta
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.imuValido = false;
    e.encEsq = 5300;
    e.encDir = 5500;  // as duas rodas andaram 300 pulsos desde iniciar()
    const SaidaControleReta s = c.atualizar(e);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.erroRumoGraus);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, s.velEsqMmS, s.velDirMmS);
}

void test_com_imu_valido_os_encoders_nao_interferem_no_rumo(void) {
    ControleReta c;
    c.iniciar(0.0f, 0, 0, 0);
    EntradaControleReta e = leitura(250.0f, 250.0f, 50);
    e.encDir = 500;  // patinagem de uma roda
    const SaidaControleReta s = c.atualizar(e);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.erroRumoGraus);
}

// ---------------------------------------------------------------------------
// Transições e reinício
// ---------------------------------------------------------------------------

void test_volta_as_paredes_nao_herda_derivada_antiga(void) {
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.atualizar(leitura(20.0f, 20.0f, 0));      // paredes, erro 0
    c.atualizar(leitura(250.0f, 250.0f, 50));   // cruzamento
    const SaidaControleReta s = c.atualizar(leitura(40.0f, 20.0f, 100));  // volta com erro 20
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 20.0f, giroDe(s));
}

void test_iniciar_descarta_a_derivada_da_reta_anterior(void) {
    const ParametrosControleReta p = ControleReta::parametrosPadrao();
    ControleReta c(p);
    c.atualizar(leitura(20.0f, 20.0f, 0));
    c.atualizar(leitura(40.0f, 20.0f, 50));
    c.iniciar(0.0f, 0, 0, 100);
    const SaidaControleReta s = c.atualizar(leitura(40.0f, 20.0f, 150));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, p.kpParede * 20.0f, giroDe(s));
}

// ---------------------------------------------------------------------------
// Simulação em malha fechada: 3 células em reta (critérios de aceitação da HU-01)
//
// Modelo simples do robô num corredor de 168 mm (células de 180 mm, RNF-SFT02): posição
// lateral y em relação à linha de centro, rumo, motores com atraso de primeira ordem e
// leituras laterais de ToF com ruído uniforme. O modelo é uma suposição: ele valida a
// lógica de controle, mas não substitui o ensaio CT-01 no robô.
// ---------------------------------------------------------------------------

static const float PI_F = 3.14159265f;
static const float CORREDOR_MM = 168.0f;
static const float CELULA_MM = 180.0f;
static const float DESVIO_MAXIMO_MM = 15.0f;  // critério de aceitação do RF-SFT01

struct Gerador {
    uint32_t estado;
    // Uniforme em [-1, 1], determinístico para os testes serem reprodutíveis.
    float proximo() {
        estado = estado * 1664525u + 1013904223u;
        return ((float)((estado >> 8) & 0xFFFF) / 32768.0f) - 1.0f;
    }
};

struct Cenario {
    float y0Mm;               // posição lateral inicial (positivo = à esquerda do centro)
    float rumo0Graus;         // rumo inicial em relação ao corredor (positivo = à esquerda)
    float ruidoToFMm;         // ruído uniforme de ±ruidoToFMm em cada ToF lateral
    bool semParedeEsq;        // trecho sem parede à esquerda entre lacunaIniMm e lacunaFimMm
    bool semParedeDir;
    float lacunaIniMm;
    float lacunaFimMm;
    bool imuValido;
    float viesGiroscopioGrausPorS;
    float velocidadeMmS;
    float bitolaMm;
    float larguraRoboMm;
    float constanteMotorS;    // atraso de primeira ordem dos motores
    float periodoControleS;   // intervalo entre leituras dos ToF
    bool controleLigado;
};

static Cenario cenarioBase() {
    Cenario c;
    c.y0Mm = 0.0f;
    c.rumo0Graus = 0.0f;
    c.ruidoToFMm = 5.0f;
    c.semParedeEsq = false;
    c.semParedeDir = false;
    c.lacunaIniMm = CELULA_MM;           // a lacuna ocupa a segunda célula
    c.lacunaFimMm = 2.0f * CELULA_MM;
    c.imuValido = true;
    c.viesGiroscopioGrausPorS = 0.0f;
    c.velocidadeMmS = 150.0f;
    c.bitolaMm = 78.0f;                  // doc 4.1
    c.larguraRoboMm = 86.0f;             // doc 4.1: folga de 41 mm por lado no corredor de 168 mm
    c.constanteMotorS = 0.08f;
    c.periodoControleS = 0.05f;          // 20 Hz, o mínimo do doc 4.4
    c.controleLigado = true;
    return c;
}

struct Resultado {
    float desvioMaxMm;       // maior |y| ao longo do percurso
    uint32_t ciclosParedes;  // ciclos em MODO_PAREDES
    uint32_t ciclosRumo;     // ciclos em MODO_RUMO
};

static Resultado simular(const Cenario& c) {
    ParametrosControleReta p = ControleReta::parametrosPadrao();
    p.velocidadeBaseMmS = c.velocidadeMmS;
    p.bitolaMm = c.bitolaMm;
    if (!c.controleLigado) {
        p.kpParede = 0.0f;
        p.kdParede = 0.0f;
        p.kpRumo = 0.0f;
    }
    ControleReta ctl(p);
    Gerador rng = {777u};

    const float passoSim = 0.005f;
    float x = 0.0f, y = c.y0Mm, rumo = c.rumo0Graus * PI_F / 180.0f;
    float velEsq = c.velocidadeMmS, velDir = c.velocidadeMmS;
    float percEsq = 0.0f, percDir = 0.0f, vies = 0.0f;
    uint32_t agoraMs = 0;
    Resultado r = {fabsf(y), 0, 0};

    ctl.iniciar(c.rumo0Graus, 0, 0, agoraMs);
    for (int ciclo = 0; x < 3.0f * CELULA_MM && ciclo < 2000; ciclo++) {
        // Leitura dos ToF laterais: distância medida na perpendicular ao robô até a parede.
        const float cosRumo = cosf(rumo);
        float dEsq = (CORREDOR_MM / 2.0f - y) / cosRumo - c.larguraRoboMm / 2.0f;
        float dDir = (CORREDOR_MM / 2.0f + y) / cosRumo - c.larguraRoboMm / 2.0f;
        const bool lacuna = x >= c.lacunaIniMm && x < c.lacunaFimMm;
        dEsq = (c.semParedeEsq && lacuna) ? 250.0f : dEsq + c.ruidoToFMm * rng.proximo();
        dDir = (c.semParedeDir && lacuna) ? 250.0f : dDir + c.ruidoToFMm * rng.proximo();

        vies += c.viesGiroscopioGrausPorS * c.periodoControleS;
        EntradaControleReta e;
        e.distEsqMm = dEsq;
        e.distDirMm = dDir;
        e.yawGraus = rumo * 180.0f / PI_F + vies + 0.1f * rng.proximo();
        e.imuValido = c.imuValido;
        e.encEsq = (int32_t)(percEsq / p.mmPorPulso);
        e.encDir = (int32_t)(percDir / p.mmPorPulso);
        e.agoraMs = agoraMs;
        const SaidaControleReta s = ctl.atualizar(e);
        if (s.modo == MODO_PAREDES) r.ciclosParedes++; else r.ciclosRumo++;

        // Dinâmica do robô durante um período de controle.
        for (float t = 0.0f; t < c.periodoControleS - 1e-6f; t += passoSim) {
            velEsq += (s.velEsqMmS - velEsq) * passoSim / c.constanteMotorS;
            velDir += (s.velDirMmS - velDir) * passoSim / c.constanteMotorS;
            rumo += (velDir - velEsq) / c.bitolaMm * passoSim;
            const float v = (velEsq + velDir) / 2.0f;
            x += v * cosf(rumo) * passoSim;
            y += v * sinf(rumo) * passoSim;
            percEsq += velEsq * passoSim;
            percDir += velDir * passoSim;
            if (fabsf(y) > r.desvioMaxMm) r.desvioMaxMm = fabsf(y);
        }
        agoraMs += (uint32_t)(c.periodoControleS * 1000.0f + 0.5f);
    }
    return r;
}

// Combinações de partida: robô deslocado até 8 mm e desalinhado até 3° (o RF-SFT01 admite
// ±3° de erro angular depois de uma curva).
static const float DESLOCAMENTOS_MM[3] = {-8.0f, 0.0f, 8.0f};
static const float DESALINHAMENTOS_GRAUS[3] = {-3.0f, 0.0f, 3.0f};

void test_tres_celulas_em_reta_ficam_abaixo_de_15_mm_de_desvio(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Cenario c = cenarioBase();
            c.y0Mm = DESLOCAMENTOS_MM[i];
            c.rumo0Graus = DESALINHAMENTOS_GRAUS[j];
            const Resultado r = simular(c);
            TEST_ASSERT_TRUE_MESSAGE(r.desvioMaxMm < DESVIO_MAXIMO_MM, "desvio lateral >= 15 mm em 3 células");
            TEST_ASSERT_EQUAL_UINT32(0, r.ciclosRumo);
        }
    }
}

void test_nao_encosta_nas_paredes_com_o_erro_maximo_do_sensor(void) {
    // RNF-ELE02: erro de até ±10 mm nas leituras. A folga lateral do robô (doc 4.1) é de 41 mm
    // por lado, mas o critério de 15 mm da HU-01 é mais exigente e vale também com esse ruído.
    const float folgaMm = (CORREDOR_MM - cenarioBase().larguraRoboMm) / 2.0f;
    const float velocidades[2] = {150.0f, 250.0f};
    for (int v = 0; v < 2; v++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                Cenario c = cenarioBase();
                c.ruidoToFMm = 10.0f;
                c.velocidadeMmS = velocidades[v];
                c.y0Mm = DESLOCAMENTOS_MM[i];
                c.rumo0Graus = DESALINHAMENTOS_GRAUS[j];
                const Resultado r = simular(c);
                TEST_ASSERT_TRUE_MESSAGE(r.desvioMaxMm < folgaMm, "robô encostaria na parede lateral");
                TEST_ASSERT_TRUE_MESSAGE(r.desvioMaxMm < DESVIO_MAXIMO_MM, "desvio >= 15 mm com o erro máximo do sensor");
            }
        }
    }
}

void test_trecho_sem_parede_lateral_mantem_o_rumo_pelo_imu(void) {
    // Segunda célula sem parede à esquerda, à direita e dos dois lados, com deriva do giroscópio.
    for (int caso = 0; caso < 3; caso++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                Cenario c = cenarioBase();
                c.semParedeEsq = (caso == 0 || caso == 2);
                c.semParedeDir = (caso == 1 || caso == 2);
                c.viesGiroscopioGrausPorS = 0.05f;
                c.y0Mm = DESLOCAMENTOS_MM[i];
                c.rumo0Graus = DESALINHAMENTOS_GRAUS[j] * (2.0f / 3.0f);  // até 2° nesse trecho
                const Resultado r = simular(c);
                TEST_ASSERT_TRUE_MESSAGE(r.ciclosRumo >= 10, "o trecho sem parede não foi percorrido em MODO_RUMO");
                TEST_ASSERT_TRUE_MESSAGE(r.desvioMaxMm < DESVIO_MAXIMO_MM, "desvio >= 15 mm no trecho sem parede");
            }
        }
    }
}

void test_trecho_sem_parede_com_imu_invalido_usa_os_encoders(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Cenario c = cenarioBase();
            c.semParedeEsq = true;
            c.semParedeDir = true;
            c.imuValido = false;
            c.y0Mm = DESLOCAMENTOS_MM[i];
            c.rumo0Graus = DESALINHAMENTOS_GRAUS[j] * (2.0f / 3.0f);
            const Resultado r = simular(c);
            TEST_ASSERT_TRUE_MESSAGE(r.ciclosRumo >= 10, "o trecho sem parede não foi percorrido em MODO_RUMO");
            TEST_ASSERT_TRUE_MESSAGE(r.desvioMaxMm < DESVIO_MAXIMO_MM, "desvio >= 15 mm usando só encoders");
        }
    }
}

void test_sem_controle_o_desvio_passa_de_15_mm(void) {
    // Controle do próprio simulador: com o controle desligado, 3° de desalinhamento já
    // estouram o critério. Se este teste passar a falhar, a simulação perdeu sensibilidade.
    Cenario c = cenarioBase();
    c.rumo0Graus = 3.0f;
    c.controleLigado = false;
    TEST_ASSERT_TRUE(simular(c).desvioMaxMm > DESVIO_MAXIMO_MM);
}

int main(void) {
    UNITY_BEGIN();
    // Modo paredes
    RUN_TEST(test_centralizado_mantem_a_mesma_velocidade_nas_duas_rodas);
    RUN_TEST(test_mais_perto_da_parede_direita_vira_para_a_esquerda);
    RUN_TEST(test_mais_perto_da_parede_esquerda_vira_para_a_direita);
    RUN_TEST(test_correcao_e_proporcional_ao_erro);
    RUN_TEST(test_correcao_e_limitada);
    RUN_TEST(test_velocidade_das_rodas_nunca_fica_negativa);
    RUN_TEST(test_primeira_amostra_nao_tem_derivada);
    RUN_TEST(test_derivada_filtrada_reduz_a_correcao_quando_o_erro_diminui);
    RUN_TEST(test_derivada_correta_na_volta_do_millis);
    RUN_TEST(test_duas_leituras_no_mesmo_instante_nao_dividem_por_zero);
    RUN_TEST(test_amostra_anterior_muito_antiga_nao_gera_derivada);
    // Validade das leituras
    RUN_TEST(test_limites_da_faixa_valida_de_parede);
    RUN_TEST(test_falta_de_parede_em_qualquer_lado_muda_para_o_rumo);
    RUN_TEST(test_no_modo_rumo_o_erro_lateral_e_zero_e_nao_se_compara_as_paredes);
    // Modo rumo
    RUN_TEST(test_rumo_pelo_imu_corrige_o_desvio_para_a_esquerda);
    RUN_TEST(test_rumo_pelo_imu_corrige_o_desvio_para_a_direita);
    RUN_TEST(test_rumo_e_medido_em_relacao_ao_inicio_da_reta);
    RUN_TEST(test_erro_de_rumo_atravessa_a_fronteira_de_180_graus);
    RUN_TEST(test_normalizar_graus);
    RUN_TEST(test_sem_imu_o_rumo_vem_dos_encoders);
    RUN_TEST(test_contagem_dos_encoders_e_relativa_ao_inicio_da_reta);
    RUN_TEST(test_com_imu_valido_os_encoders_nao_interferem_no_rumo);
    // Transições
    RUN_TEST(test_volta_as_paredes_nao_herda_derivada_antiga);
    RUN_TEST(test_iniciar_descarta_a_derivada_da_reta_anterior);
    // Malha fechada (critérios da HU-01)
    RUN_TEST(test_tres_celulas_em_reta_ficam_abaixo_de_15_mm_de_desvio);
    RUN_TEST(test_nao_encosta_nas_paredes_com_o_erro_maximo_do_sensor);
    RUN_TEST(test_trecho_sem_parede_lateral_mantem_o_rumo_pelo_imu);
    RUN_TEST(test_trecho_sem_parede_com_imu_invalido_usa_os_encoders);
    RUN_TEST(test_sem_controle_o_desvio_passa_de_15_mm);
    return UNITY_END();
}
