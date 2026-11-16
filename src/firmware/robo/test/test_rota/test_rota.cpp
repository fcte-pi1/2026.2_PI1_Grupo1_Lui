#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "../../src/navegacao/RotaMaisCurta.h"
#include "../../src/navegacao/Navegador.h"
#include "../comum/SimuladorLabirinto.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Utilitários
// ---------------------------------------------------------------------------

static GeometriaLabirinto geometriaDe(uint8_t linhas) {
    return linhas == 4 ? LAB_4X4 : linhas == 8 ? LAB_8X4 : LAB_12X4;
}

// Copia para o mapa conhecido todos os lados internos do labirinto verdadeiro (como se a
// exploração tivesse observado tudo), exceto o lado (pulaL, pulaC, pulaDir), que fica
// desconhecido. O perímetro já vem como parede conhecida do Mapa.
static void conhecerTudo(const SimuladorLabirinto& sim, Mapa& mapa,
                         int pulaL = -1, int pulaC = -1, Direcao pulaDir = DIR_SUL) {
    for (uint8_t l = 0; l < sim.linhas; l++)
        for (uint8_t c = 0; c < SimuladorLabirinto::COLUNAS; c++) {
            const Direcao lados[2] = {DIR_SUL, DIR_LESTE};  // cada lado interno uma vez
            for (uint8_t k = 0; k < 2; k++) {
                uint8_t lv, cv;
                if (!mapa.vizinha(l, c, lados[k], lv, cv)) continue;
                if ((int)l == pulaL && (int)c == pulaC && lados[k] == pulaDir) continue;
                if (sim.temParedeReal(l, c, lados[k])) mapa.registrarParede(l, c, lados[k]);
                else mapa.registrarPassagem(l, c, lados[k]);
            }
        }
}

static Mapa* novoMapa(Mapa& m, uint8_t linhas) {
    m.setGeometria(geometriaDe(linhas));
    return &m;
}

// Oráculo independente da BFS: relaxamento repetido de distâncias (estilo Bellman-Ford)
// sobre as passagens confirmadas do mapa. Retorna -1 se o destino não é alcançável.
static int distanciaOraculo(const Mapa& mapa, uint8_t lo, uint8_t co, uint8_t ld, uint8_t cd) {
    int dist[12][4];
    for (uint8_t l = 0; l < 12; l++)
        for (uint8_t c = 0; c < 4; c++) dist[l][c] = 1000;
    dist[lo][co] = 0;
    for (int volta = 0; volta < 60; volta++) {
        bool mudou = false;
        for (uint8_t l = 0; l < mapa.getMaxLinhas(); l++)
            for (uint8_t c = 0; c < mapa.getMaxColunas(); c++) {
                if (dist[l][c] >= 1000) continue;
                for (uint8_t d = 0; d < 4; d++) {
                    const Direcao dir = (Direcao)d;
                    if (!mapa.ladoConhecido(l, c, dir) || mapa.temParede(l, c, dir)) continue;
                    uint8_t lv, cv;
                    if (!mapa.vizinha(l, c, dir, lv, cv)) continue;
                    if (dist[l][c] + 1 < dist[lv][cv]) {
                        dist[lv][cv] = dist[l][c] + 1;
                        mudou = true;
                    }
                }
            }
        if (!mudou) break;
    }
    return dist[ld][cd] >= 1000 ? -1 : dist[ld][cd];
}

// Valida a rota passo a passo: começa na origem, termina no destino, cada passo fica
// dentro do mapa, usa um lado conhecido e sem parede, e a célula seguinte é adjacente.
static void validarRota(const Mapa& mapa, const Rota& r, uint8_t ld, uint8_t cd) {
    uint8_t l = r.linhaOrigem, c = r.colunaOrigem;
    for (uint8_t i = 0; i < r.numPassos; i++) {
        const Direcao dir = r.passos[i];
        TEST_ASSERT_TRUE(mapa.dentroDosLimites(l, c));
        TEST_ASSERT_TRUE(mapa.ladoConhecido(l, c, dir));
        TEST_ASSERT_FALSE(mapa.temParede(l, c, dir));
        uint8_t lv, cv;
        TEST_ASSERT_TRUE(mapa.vizinha(l, c, dir, lv, cv));
        TEST_ASSERT_EQUAL_INT(1, (l > lv ? l - lv : lv - l) + (c > cv ? c - cv : cv - c));
        uint8_t lc, cc;
        TEST_ASSERT_TRUE(r.celula(i + 1, lc, cc));
        TEST_ASSERT_EQUAL_UINT8(lv, lc);
        TEST_ASSERT_EQUAL_UINT8(cv, cc);
        l = lv;
        c = cv;
    }
    TEST_ASSERT_EQUAL_UINT8(ld, l);
    TEST_ASSERT_EQUAL_UINT8(cd, c);
}

static void esperarPassos(const Rota& r, const char* esperado) {
    TEST_ASSERT_EQUAL_UINT8((uint8_t)strlen(esperado), r.numPassos);
    for (uint8_t i = 0; i < r.numPassos; i++) {
        const char ch = r.passos[i] == DIR_NORTE ? 'N' : r.passos[i] == DIR_SUL ? 'S'
                      : r.passos[i] == DIR_LESTE ? 'L' : 'O';
        TEST_ASSERT_EQUAL_CHAR(esperado[i], ch);
    }
}

// ---------------------------------------------------------------------------
// Cenários obrigatórios (3.1)
// ---------------------------------------------------------------------------

void test_caminho_unico(void) {
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "LLLSSS");
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    esperarPassos(r, "LLLSSS");
    validarRota(mapa, r, 3, 3);
}

void test_dois_caminhos_de_comprimentos_diferentes_escolhe_o_mais_curto(void) {
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "SSSLNLSL");  // rota longa, 8 passos
    sim.abrirCaminho(0, 0, "LLLSSS");    // rota curta, 6 passos
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    TEST_ASSERT_EQUAL_UINT8(6, r.numPassos);
    validarRota(mapa, r, 3, 3);
}

void test_multiplas_rotas_minimas_retorna_uma_valida(void) {
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    TEST_ASSERT_EQUAL_UINT8(6, r.numPassos);  // 20 rotas de 6 passos; qualquer uma serve
    validarRota(mapa, r, 3, 3);
    for (uint8_t i = 0; i < r.numPassos; i++)  // só avanços para o sul e o leste
        TEST_ASSERT_TRUE(r.passos[i] == DIR_SUL || r.passos[i] == DIR_LESTE);
}

void test_parede_bloqueando_a_passagem_nao_e_atravessada(void) {
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    sim.fechar(0, 0, DIR_LESTE);
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    TEST_ASSERT_EQUAL(DIR_SUL, r.passos[0]);
    TEST_ASSERT_EQUAL_UINT8(6, r.numPassos);
    validarRota(mapa, r, 3, 3);
}

void test_celulas_e_lados_desconhecidos_nao_sao_usados(void) {
    // Serpentina conhecida (12 passos) e um atalho (1,3)-(2,3) que existe no labirinto
    // verdadeiro, mas nunca foi observado.
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "LLLSOOOSLLLS");
    sim.abrir(1, 3, DIR_SUL);
    Mapa mapa;
    conhecerTudo(sim, mapa, 1, 3, DIR_SUL);
    TEST_ASSERT_FALSE(mapa.ladoConhecido(1, 3, DIR_SUL));
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    TEST_ASSERT_EQUAL_UINT8(12, r.numPassos);  // nao usa o atalho desconhecido
    validarRota(mapa, r, 3, 3);

    // Depois de observado, o atalho passa a valer e a rota encurta.
    mapa.registrarPassagem(1, 3, DIR_SUL);
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    esperarPassos(r, "LLLSSS");
}

void test_mapa_sem_nenhuma_informacao_nao_tem_rota(void) {
    Mapa mapa;  // so o perimetro e conhecido
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_INEXISTENTE, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    TEST_ASSERT_EQUAL_UINT8(0, r.numPassos);
}

void test_objetivo_inalcancavel(void) {
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "LLLSS");  // (3,3) fica isolada de (2,3)
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_INEXISTENTE, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    TEST_ASSERT_EQUAL_UINT8(0, r.numPassos);
}

void test_origem_igual_ao_destino_tem_comprimento_zero(void) {
    Mapa mapa;
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcular(mapa, 2, 1, 2, 1, r));
    TEST_ASSERT_EQUAL_UINT8(0, r.numPassos);
    TEST_ASSERT_EQUAL_UINT8(2, r.linhaOrigem);
    TEST_ASSERT_EQUAL_UINT8(1, r.colunaOrigem);
    uint8_t l, c;
    TEST_ASSERT_TRUE(r.celula(0, l, c));
    TEST_ASSERT_EQUAL_UINT8(2, l);
    TEST_ASSERT_EQUAL_UINT8(1, c);
    TEST_ASSERT_FALSE(r.celula(1, l, c));
}

void test_mapa_com_obstaculos_calcula_a_menor_rota(void) {
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "LLLSOOOSLLLS");  // serpentina: unico caminho, 12 passos
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    esperarPassos(r, "LLLSOOOSLLLS");
    validarRota(mapa, r, 3, 3);
}

void test_limites_do_mapa(void) {
    Mapa m4;
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_ENTRADA_INVALIDA, RotaMaisCurta::calcular(m4, 4, 0, 0, 0, r));
    TEST_ASSERT_EQUAL(ROTA_ENTRADA_INVALIDA, RotaMaisCurta::calcular(m4, 0, 0, 0, 4, r));
    TEST_ASSERT_EQUAL(ROTA_ENTRADA_INVALIDA, RotaMaisCurta::calcular(m4, 0, 0, 7, 3, r));  // D8 em 4x4
    TEST_ASSERT_EQUAL_UINT8(0, r.numPassos);
    Mapa m8;
    m8.setGeometria(LAB_8X4);
    TEST_ASSERT_EQUAL(ROTA_ENTRADA_INVALIDA, RotaMaisCurta::calcular(m8, 0, 0, 8, 0, r));
    TEST_ASSERT_EQUAL(ROTA_INEXISTENTE, RotaMaisCurta::calcular(m8, 0, 0, 7, 3, r));  // valido, sem info
}

void test_destino_oficial_nas_tres_geometrias(void) {
    const uint8_t linhas[3] = {4, 8, 12};
    const uint8_t passosEsperados[3] = {6, 10, 14};  // |linhas-1| + 3 em pista aberta
    for (uint8_t i = 0; i < 3; i++) {
        SimuladorLabirinto sim(linhas[i]);
        sim.abrirTudo();
        Mapa mapa;
        novoMapa(mapa, linhas[i]);
        conhecerTudo(sim, mapa);
        Rota r;
        TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
        TEST_ASSERT_EQUAL_UINT8(passosEsperados[i], r.numPassos);
        validarRota(mapa, r, (uint8_t)(linhas[i] - 1), 3);
    }
}

void test_rota_mais_longa_cabe_na_capacidade(void) {
    // Serpentina que passa por todas as 48 celulas do 12x4: 47 passos, o maximo.
    SimuladorLabirinto sim(12);
    for (uint8_t l = 0; l < 12; l++) {
        if (l % 2 == 0) sim.abrirCaminho(l, 0, "LLL"); else sim.abrirCaminho(l, 3, "OOO");
        if (l < 11) sim.abrirCaminho(l, (l % 2 == 0) ? 3 : 0, "S");
    }
    Mapa mapa;
    novoMapa(mapa, 12);
    conhecerTudo(sim, mapa);
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcular(mapa, 0, 0, 11, 0, r));
    TEST_ASSERT_EQUAL_UINT8(ROTA_MAX_PASSOS, r.numPassos);
    validarRota(mapa, r, 11, 0);
}

// ---------------------------------------------------------------------------
// Optimalidade (3.2): comparação com um oráculo independente em mapas aleatórios
// ---------------------------------------------------------------------------

static uint32_t semente = 12345;
static uint32_t proximo(void) {
    semente = semente * 1664525u + 1013904223u;  // LCG deterministico
    return semente >> 16;
}

void test_rota_e_minima_em_mapas_aleatorios(void) {
    const uint8_t linhas[3] = {4, 8, 12};
    uint16_t comRota = 0, semRota = 0;
    for (uint8_t g = 0; g < 3; g++) {
        for (uint8_t rodada = 0; rodada < 150; rodada++) {
            SimuladorLabirinto sim(linhas[g]);
            sim.abrirTudo();
            const uint8_t pctParede = (uint8_t)(15 + proximo() % 40);
            for (uint8_t l = 0; l < linhas[g]; l++)
                for (uint8_t c = 0; c < 4; c++) {
                    if (l + 1 < linhas[g] && proximo() % 100 < pctParede) sim.fechar(l, c, DIR_SUL);
                    if (c + 1 < 4 && proximo() % 100 < pctParede) sim.fechar(l, c, DIR_LESTE);
                }
            // Mapa conhecido: a maioria dos lados observada, alguns deixados desconhecidos.
            Mapa mapa;
            novoMapa(mapa, linhas[g]);
            for (uint8_t l = 0; l < linhas[g]; l++)
                for (uint8_t c = 0; c < 4; c++) {
                    const Direcao lados[2] = {DIR_SUL, DIR_LESTE};
                    for (uint8_t k = 0; k < 2; k++) {
                        uint8_t lv, cv;
                        if (!mapa.vizinha(l, c, lados[k], lv, cv)) continue;
                        if (proximo() % 100 < 20) continue;  // lado nao observado
                        if (sim.temParedeReal(l, c, lados[k])) mapa.registrarParede(l, c, lados[k]);
                        else mapa.registrarPassagem(l, c, lados[k]);
                    }
                }
            const uint8_t ld = (uint8_t)(linhas[g] - 1);
            Rota r;
            const ResultadoRota res = RotaMaisCurta::calcularParaObjetivo(mapa, r);
            const int esperado = distanciaOraculo(mapa, 0, 0, ld, 3);
            if (esperado < 0) {
                TEST_ASSERT_EQUAL(ROTA_INEXISTENTE, res);
                TEST_ASSERT_EQUAL_UINT8(0, r.numPassos);
                semRota++;
            } else {
                TEST_ASSERT_EQUAL(ROTA_OK, res);
                TEST_ASSERT_EQUAL_INT(esperado, r.numPassos);  // minima
                validarRota(mapa, r, ld, 3);                   // valida e so com lados conhecidos
                // Como os lados conhecidos coincidem com o labirinto verdadeiro, a rota
                // tambem e fisicamente livre.
                uint8_t l = 0, c = 0;
                for (uint8_t i = 0; i < r.numPassos; i++) {
                    TEST_ASSERT_FALSE(sim.temParedeReal(l, c, r.passos[i]));
                    l = (uint8_t)(l + deltaLinha(r.passos[i]));
                    c = (uint8_t)(c + deltaColuna(r.passos[i]));
                }
                comRota++;
            }
        }
    }
    printf("[ALEATORIOS] 450 mapas: %u com rota (minima confirmada pelo oraculo), %u sem rota\n",
           comRota, semRota);
    TEST_ASSERT_TRUE(comRota > 50);
    TEST_ASSERT_TRUE(semRota > 50);
}

void test_resultado_deterministico(void) {
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota a, b;
    RotaMaisCurta::calcularParaObjetivo(mapa, a);
    RotaMaisCurta::calcularParaObjetivo(mapa, b);
    TEST_ASSERT_EQUAL_UINT8(a.numPassos, b.numPassos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)a.passos, (const uint8_t*)b.passos, a.numPassos);
}

void test_calculo_nao_altera_o_mapa(void) {
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "LLLSSS");
    Mapa mapa;
    conhecerTudo(sim, mapa);
    uint8_t antesP[4][4], antesC[4][4];
    for (uint8_t l = 0; l < 4; l++)
        for (uint8_t c = 0; c < 4; c++) {
            antesP[l][c] = mapa.getParedes(l, c);
            antesC[l][c] = mapa.getLadosConhecidos(l, c);
        }
    Rota r;
    RotaMaisCurta::calcularParaObjetivo(mapa, r);
    for (uint8_t l = 0; l < 4; l++)
        for (uint8_t c = 0; c < 4; c++) {
            TEST_ASSERT_EQUAL_UINT8(antesP[l][c], mapa.getParedes(l, c));
            TEST_ASSERT_EQUAL_UINT8(antesC[l][c], mapa.getLadosConhecidos(l, c));
            TEST_ASSERT_FALSE(mapa.foiVisitada(l, c));
        }
}

// ---------------------------------------------------------------------------
// Consumo pela HU-16: auxiliares da rota
// ---------------------------------------------------------------------------

void test_giros_e_celulas_da_rota(void) {
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "LLLSSS");
    Mapa mapa;
    conhecerTudo(sim, mapa);
    Rota r;
    RotaMaisCurta::calcularParaObjetivo(mapa, r);
    const Giro esperados[6] = {GIRO_NENHUM, GIRO_NENHUM, GIRO_NENHUM,
                               GIRO_DIREITA, GIRO_NENHUM, GIRO_NENHUM};
    for (uint8_t i = 0; i < 6; i++)
        TEST_ASSERT_EQUAL(esperados[i], r.giroDoPasso(i, DIR_LESTE));
    TEST_ASSERT_EQUAL(GIRO_NENHUM, r.giroDoPasso(6, DIR_LESTE));
    TEST_ASSERT_EQUAL(GIRO_MEIA_VOLTA, r.giroDoPasso(0, DIR_OESTE));
    TEST_ASSERT_EQUAL(GIRO_ESQUERDA, r.giroDoPasso(0, DIR_SUL));
    uint8_t l, c;
    TEST_ASSERT_TRUE(r.celula(3, l, c));
    TEST_ASSERT_EQUAL_UINT8(0, l);
    TEST_ASSERT_EQUAL_UINT8(3, c);
    TEST_ASSERT_TRUE(r.celula(6, l, c));
    TEST_ASSERT_EQUAL_UINT8(3, l);
    TEST_ASSERT_EQUAL_UINT8(3, c);
}

// ---------------------------------------------------------------------------
// Integração: exploração (HU-06) seguida do cálculo (HU-15) e percurso simulado
// ---------------------------------------------------------------------------

static void explorarEPercorrer(uint8_t linhas, void (*montar)(SimuladorLabirinto&),
                               uint8_t passosMinimosVerdadeiros) {
    // 1. Exploracao com o Navegador (so ve o que os sensores simulados leem).
    SimuladorLabirinto sim(linhas);
    montar(sim);
    Mapa mapa;
    novoMapa(mapa, linhas);
    Navegador nav(mapa);
    nav.iniciar();
    for (uint16_t i = 0; i < 5000 && nav.passo(sim, sim); i++) {}
    TEST_ASSERT_TRUE(nav.chegou());

    // 2. Rota sobre o mapa descoberto.
    Rota r;
    TEST_ASSERT_EQUAL(ROTA_OK, RotaMaisCurta::calcularParaObjetivo(mapa, r));
    validarRota(mapa, r, (uint8_t)(linhas - 1), 3);
    TEST_ASSERT_EQUAL_INT(distanciaOraculo(mapa, 0, 0, (uint8_t)(linhas - 1), 3), r.numPassos);
    TEST_ASSERT_EQUAL_UINT8(passosMinimosVerdadeiros, r.numPassos);

    // 3. Percurso da rota num robo recolocado em A1, como a HU-16 faria: giro e avanco.
    sim.reiniciar();
    montar(sim);
    sim.linhas = linhas;
    Direcao orientacao = DIR_LESTE;
    for (uint8_t i = 0; i < r.numPassos; i++) {
        const Giro g = r.giroDoPasso(i, DIR_LESTE);
        TEST_ASSERT_TRUE(sim.girar(g));
        orientacao = r.passos[i];
        TEST_ASSERT_EQUAL(orientacao, sim.orientacaoReal);
        TEST_ASSERT_TRUE(sim.avancarCelula());
    }
    TEST_ASSERT_TRUE(sim.naChegada());
    TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
    printf("[INTEGRACAO] %ux4: rota de %u passos sobre o mapa explorado, percorrida sem movimento invalido\n",
           linhas, r.numPassos);
}

static void montarBecos4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "LLLSSS");
    s.abrirCaminho(0, 1, "SSS");
    s.abrirCaminho(0, 2, "S");
}
static void montarAfastar4(SimuladorLabirinto& s) { s.abrirCaminho(0, 0, "SSLNLLSS"); }
static void montarOito(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "SSSSSSSLLL");
    s.abrirCaminho(3, 0, "LL");
}
static void montarDoze(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "SSSSSSSSSSSLLL");
    s.abrirCaminho(2, 0, "LSO");
    s.abrirCaminho(5, 0, "LLLSS");
}

void test_integracao_exploracao_seguida_de_rota_4x4_becos(void)   { explorarEPercorrer(4, montarBecos4, 6); }
void test_integracao_exploracao_seguida_de_rota_4x4_afastar(void) { explorarEPercorrer(4, montarAfastar4, 8); }
void test_integracao_exploracao_seguida_de_rota_8x4(void)         { explorarEPercorrer(8, montarOito, 10); }
void test_integracao_exploracao_seguida_de_rota_12x4(void)        { explorarEPercorrer(12, montarDoze, 14); }

void test_memoria_da_rota(void) {
    printf("[MEMORIA] sizeof(Rota)=%u B\n", (unsigned)sizeof(Rota));
    TEST_ASSERT_TRUE(sizeof(Rota) <= 64);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_caminho_unico);
    RUN_TEST(test_dois_caminhos_de_comprimentos_diferentes_escolhe_o_mais_curto);
    RUN_TEST(test_multiplas_rotas_minimas_retorna_uma_valida);
    RUN_TEST(test_parede_bloqueando_a_passagem_nao_e_atravessada);
    RUN_TEST(test_celulas_e_lados_desconhecidos_nao_sao_usados);
    RUN_TEST(test_mapa_sem_nenhuma_informacao_nao_tem_rota);
    RUN_TEST(test_objetivo_inalcancavel);
    RUN_TEST(test_origem_igual_ao_destino_tem_comprimento_zero);
    RUN_TEST(test_mapa_com_obstaculos_calcula_a_menor_rota);
    RUN_TEST(test_limites_do_mapa);
    RUN_TEST(test_destino_oficial_nas_tres_geometrias);
    RUN_TEST(test_rota_mais_longa_cabe_na_capacidade);
    RUN_TEST(test_rota_e_minima_em_mapas_aleatorios);
    RUN_TEST(test_resultado_deterministico);
    RUN_TEST(test_calculo_nao_altera_o_mapa);
    RUN_TEST(test_giros_e_celulas_da_rota);
    RUN_TEST(test_integracao_exploracao_seguida_de_rota_4x4_becos);
    RUN_TEST(test_integracao_exploracao_seguida_de_rota_4x4_afastar);
    RUN_TEST(test_integracao_exploracao_seguida_de_rota_8x4);
    RUN_TEST(test_integracao_exploracao_seguida_de_rota_12x4);
    RUN_TEST(test_memoria_da_rota);
    return UNITY_END();
}
