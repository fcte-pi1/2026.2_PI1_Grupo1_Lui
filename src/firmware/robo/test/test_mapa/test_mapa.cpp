#include <unity.h>
#include "../../src/mapa/Mapa.h"

void setUp(void) {
    // Configuração antes de cada teste
}

void tearDown(void) {
    // Limpeza após cada teste
}

void test_mapa_default_geometry(void) {
    Mapa mapa;
    
    // Testa estado inicial que deve ser 4x4
    TEST_ASSERT_EQUAL(LAB_4X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(4, mapa.getMaxLinhas());
    TEST_ASSERT_EQUAL(4, mapa.getMaxColunas());
}

void test_mapa_change_to_8x4(void) {
    Mapa mapa;
    
    // Troca para 8x4 e verifica
    mapa.setGeometria(LAB_8X4);
    TEST_ASSERT_EQUAL(LAB_8X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(8, mapa.getMaxLinhas());
    TEST_ASSERT_EQUAL(4, mapa.getMaxColunas());
}

void test_mapa_change_to_12x4(void) {
    Mapa mapa;
    
    // Troca para 12x4 e verifica
    mapa.setGeometria(LAB_12X4);
    TEST_ASSERT_EQUAL(LAB_12X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(12, mapa.getMaxLinhas());
    TEST_ASSERT_EQUAL(4, mapa.getMaxColunas());
}

void test_mapa_cycle_geometry(void) {
    Mapa mapa;
    
    // Testa transições
    mapa.setGeometria(LAB_8X4);
    mapa.setGeometria(LAB_12X4);
    mapa.setGeometria(LAB_4X4);
    
    // Ao final deve retornar a 4x4
    TEST_ASSERT_EQUAL(LAB_4X4, mapa.getGeometria());
    TEST_ASSERT_EQUAL(4, mapa.getMaxLinhas());
}

// ---------------------------------------------------------------------------
// Registro de paredes (HU-03)
// ---------------------------------------------------------------------------

static const Direcao DIRECOES[] = {DIR_NORTE, DIR_SUL, DIR_LESTE, DIR_OESTE};

static uint8_t perimetroEsperado(uint8_t linha, uint8_t coluna, uint8_t linhas) {
    uint8_t m = 0;
    if (linha == 0) m |= PAREDE_NORTE;
    if (linha == linhas - 1) m |= PAREDE_SUL;
    if (coluna == 0) m |= PAREDE_OESTE;
    if (coluna == 3) m |= PAREDE_LESTE;
    return m;
}

void test_perimetro_registrado_nas_tres_geometrias(void) {
    const GeometriaLabirinto geometrias[] = {LAB_4X4, LAB_8X4, LAB_12X4};
    Mapa mapa;
    for (uint8_t g = 0; g < 3; g++) {
        mapa.setGeometria(geometrias[g]);
        const uint8_t linhas = mapa.getMaxLinhas();
        for (uint8_t l = 0; l < linhas; l++) {
            for (uint8_t c = 0; c < 4; c++) {
                const uint8_t esperado = perimetroEsperado(l, c, linhas);
                TEST_ASSERT_EQUAL_HEX8(esperado, mapa.getParedes(l, c));
                // Só o perímetro é conhecido no início; os lados internos não foram observados.
                TEST_ASSERT_EQUAL_HEX8(esperado, mapa.getLadosConhecidos(l, c));
                TEST_ASSERT_FALSE(mapa.foiVisitada(l, c));
            }
        }
    }
}

void test_trocar_geometria_e_limpar_reiniciam_o_mapa(void) {
    Mapa mapa;
    mapa.registrarParede(1, 1, DIR_LESTE);
    mapa.marcarVisitada(1, 1);

    mapa.setGeometria(LAB_8X4);
    TEST_ASSERT_EQUAL_HEX8(0, mapa.getParedes(1, 1));
    TEST_ASSERT_EQUAL_HEX8(0, mapa.getLadosConhecidos(1, 1));
    TEST_ASSERT_FALSE(mapa.foiVisitada(1, 1));
    // Na 8x4 a linha 3 deixou de ser borda: o antigo perímetro sul some.
    TEST_ASSERT_FALSE(mapa.temParede(3, 0, DIR_SUL));
    TEST_ASSERT_TRUE(mapa.temParede(7, 0, DIR_SUL));

    mapa.registrarParede(2, 2, DIR_NORTE);
    mapa.limpar();
    TEST_ASSERT_EQUAL_HEX8(0, mapa.getParedes(2, 2));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_NORTE | PAREDE_OESTE, mapa.getParedes(0, 0));
}

void test_parede_gravada_tambem_na_vizinha_nas_quatro_direcoes(void) {
    for (uint8_t i = 0; i < 4; i++) {
        const Direcao d = DIRECOES[i];
        Mapa mapa;
        uint8_t lv = 0, cv = 0;
        TEST_ASSERT_TRUE(mapa.vizinha(1, 1, d, lv, cv));

        TEST_ASSERT_TRUE(mapa.registrarParede(1, 1, d));
        TEST_ASSERT_TRUE(mapa.temParede(1, 1, d));
        TEST_ASSERT_TRUE(mapa.temParede(lv, cv, direcaoOposta(d)));
        TEST_ASSERT_TRUE(mapa.ladoConhecido(lv, cv, direcaoOposta(d)));
        // Os outros lados internos da vizinha continuam desconhecidos.
        TEST_ASSERT_EQUAL_HEX8(mascaraParede(direcaoOposta(d)) | perimetroEsperado(lv, cv, 4),
                               mapa.getLadosConhecidos(lv, cv));

        TEST_ASSERT_TRUE(mapa.registrarPassagem(1, 1, d));
        TEST_ASSERT_FALSE(mapa.temParede(1, 1, d));
        TEST_ASSERT_FALSE(mapa.temParede(lv, cv, direcaoOposta(d)));
        TEST_ASSERT_TRUE(mapa.ladoConhecido(1, 1, d));
        TEST_ASSERT_TRUE(mapa.ladoConhecido(lv, cv, direcaoOposta(d)));
    }
}

void test_exemplo_de_simetria_da_secao_5_1(void) {
    // Parede a leste de (r, c) é a mesma parede a oeste de (r, c+1).
    Mapa mapa;
    mapa.registrarParede(2, 1, DIR_LESTE);
    TEST_ASSERT_TRUE(mapa.temParede(2, 2, DIR_OESTE));
}

void test_vizinha_respeita_os_limites_da_geometria(void) {
    Mapa mapa;
    uint8_t l = 99, c = 99;
    TEST_ASSERT_TRUE(mapa.vizinha(0, 0, DIR_SUL, l, c));
    TEST_ASSERT_EQUAL_UINT8(1, l);
    TEST_ASSERT_EQUAL_UINT8(0, c);

    l = 99;
    c = 99;
    TEST_ASSERT_FALSE(mapa.vizinha(0, 0, DIR_NORTE, l, c));
    TEST_ASSERT_FALSE(mapa.vizinha(0, 0, DIR_OESTE, l, c));
    TEST_ASSERT_FALSE(mapa.vizinha(3, 3, DIR_SUL, l, c));
    TEST_ASSERT_FALSE(mapa.vizinha(3, 3, DIR_LESTE, l, c));
    TEST_ASSERT_FALSE(mapa.vizinha(4, 0, DIR_NORTE, l, c));
    // Em caso de falha as saídas não são alteradas.
    TEST_ASSERT_EQUAL_UINT8(99, l);
    TEST_ASSERT_EQUAL_UINT8(99, c);

    mapa.setGeometria(LAB_8X4);
    TEST_ASSERT_TRUE(mapa.vizinha(3, 3, DIR_SUL, l, c));
    TEST_ASSERT_EQUAL_UINT8(4, l);
}

void test_parede_no_perimetro_nao_altera_outras_celulas(void) {
    Mapa mapa;
    TEST_ASSERT_TRUE(mapa.registrarParede(0, 0, DIR_NORTE));
    TEST_ASSERT_TRUE(mapa.registrarParede(3, 3, DIR_LESTE));
    for (uint8_t l = 0; l < 4; l++) {
        for (uint8_t c = 0; c < 4; c++) {
            TEST_ASSERT_EQUAL_HEX8(perimetroEsperado(l, c, 4), mapa.getParedes(l, c));
            TEST_ASSERT_EQUAL_HEX8(perimetroEsperado(l, c, 4), mapa.getLadosConhecidos(l, c));
        }
    }
}

void test_passagem_no_perimetro_e_recusada(void) {
    Mapa mapa;
    TEST_ASSERT_FALSE(mapa.registrarPassagem(0, 0, DIR_NORTE));
    TEST_ASSERT_FALSE(mapa.registrarPassagem(3, 2, DIR_SUL));
    TEST_ASSERT_TRUE(mapa.temParede(0, 0, DIR_NORTE));
    TEST_ASSERT_TRUE(mapa.temParede(3, 2, DIR_SUL));
}

void test_celula_fora_da_geometria_e_rejeitada(void) {
    Mapa mapa; // 4x4: a linha 4 existe na matriz, mas não na geometria
    TEST_ASSERT_FALSE(mapa.dentroDosLimites(4, 0));
    TEST_ASSERT_FALSE(mapa.dentroDosLimites(0, 4));
    TEST_ASSERT_FALSE(mapa.registrarParede(4, 0, DIR_NORTE));
    TEST_ASSERT_FALSE(mapa.registrarPassagem(4, 0, DIR_NORTE));
    TEST_ASSERT_FALSE(mapa.marcarVisitada(4, 0));
    TEST_ASSERT_FALSE(mapa.foiVisitada(4, 0));
    TEST_ASSERT_FALSE(mapa.registrarLeituras(0, 4, DIR_NORTE, true, true, true));
    TEST_ASSERT_FALSE(mapa.registrarMovimento(4, 0, DIR_NORTE));
    TEST_ASSERT_EQUAL_HEX8(0, mapa.getParedes(4, 0));
    TEST_ASSERT_EQUAL_HEX8(0, mapa.getLadosConhecidos(0, 4));
    // A tentativa de parede ao norte de (4, 0) não pode vazar para a borda sul de (3, 0).
    TEST_ASSERT_EQUAL_HEX8(PAREDE_SUL | PAREDE_OESTE, mapa.getLadosConhecidos(3, 0));
}

void test_marcar_e_consultar_celula_visitada(void) {
    Mapa mapa;
    TEST_ASSERT_FALSE(mapa.foiVisitada(2, 3));
    TEST_ASSERT_TRUE(mapa.marcarVisitada(2, 3));
    TEST_ASSERT_TRUE(mapa.foiVisitada(2, 3));
    TEST_ASSERT_FALSE(mapa.foiVisitada(3, 2));
}

void test_lado_nao_observado_e_diferente_de_passagem(void) {
    Mapa mapa;
    // Lado interno nunca observado: sem parede, mas desconhecido.
    TEST_ASSERT_FALSE(mapa.temParede(1, 1, DIR_LESTE));
    TEST_ASSERT_FALSE(mapa.ladoConhecido(1, 1, DIR_LESTE));

    mapa.registrarPassagem(1, 1, DIR_LESTE);
    TEST_ASSERT_FALSE(mapa.temParede(1, 1, DIR_LESTE));
    TEST_ASSERT_TRUE(mapa.ladoConhecido(1, 1, DIR_LESTE));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_LESTE, mapa.getLadosConhecidos(1, 1));
}

void test_leituras_convertidas_para_direcoes_absolutas(void) {
    Mapa mapa;
    // Robô em (1, 1) virado para o sul: frente = sul, esquerda = leste, direita = oeste.
    TEST_ASSERT_TRUE(mapa.registrarLeituras(1, 1, DIR_SUL, true, false, true));

    TEST_ASSERT_EQUAL_HEX8(PAREDE_SUL | PAREDE_OESTE, mapa.getParedes(1, 1));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_SUL | PAREDE_LESTE | PAREDE_OESTE, mapa.getLadosConhecidos(1, 1));
    TEST_ASSERT_FALSE(mapa.ladoConhecido(1, 1, DIR_NORTE)); // atrás do robô
    TEST_ASSERT_TRUE(mapa.temParede(2, 1, DIR_NORTE));
    TEST_ASSERT_TRUE(mapa.temParede(1, 0, DIR_LESTE));
    TEST_ASSERT_FALSE(mapa.temParede(1, 2, DIR_OESTE));
    TEST_ASSERT_TRUE(mapa.ladoConhecido(1, 2, DIR_OESTE));
}

void test_leituras_nas_quatro_orientacoes(void) {
    for (uint8_t i = 0; i < 4; i++) {
        const Direcao orientacao = DIRECOES[i];
        Mapa mapa;
        // Só a frente tem parede.
        TEST_ASSERT_TRUE(mapa.registrarLeituras(1, 1, orientacao, true, false, false));

        TEST_ASSERT_EQUAL_HEX8(mascaraParede(orientacao), mapa.getParedes(1, 1));
        const uint8_t atras = mascaraParede(direcaoOposta(orientacao));
        TEST_ASSERT_EQUAL_HEX8(PAREDE_TODAS & (uint8_t)~atras, mapa.getLadosConhecidos(1, 1));
    }
}

void test_leitura_de_passagem_no_perimetro_e_ignorada(void) {
    Mapa mapa;
    // Em (0, 0) virado para o leste a esquerda é o norte, que é perímetro.
    TEST_ASSERT_FALSE(mapa.registrarLeituras(0, 0, DIR_LESTE, true, false, false));
    TEST_ASSERT_TRUE(mapa.temParede(0, 0, DIR_NORTE));
    // As outras leituras foram registradas.
    TEST_ASSERT_TRUE(mapa.temParede(0, 0, DIR_LESTE));
    TEST_ASSERT_TRUE(mapa.ladoConhecido(0, 0, DIR_SUL));
}

void test_movimento_registra_passagem_e_visita_o_destino(void) {
    Mapa mapa;
    mapa.registrarParede(0, 0, DIR_LESTE); // leitura anterior equivocada

    TEST_ASSERT_TRUE(mapa.registrarMovimento(0, 0, DIR_LESTE));
    TEST_ASSERT_TRUE(mapa.foiVisitada(0, 1));
    TEST_ASSERT_FALSE(mapa.foiVisitada(0, 0)); // a origem é marcada por quem a visitou
    TEST_ASSERT_FALSE(mapa.temParede(0, 0, DIR_LESTE));
    TEST_ASSERT_FALSE(mapa.temParede(0, 1, DIR_OESTE));
    TEST_ASSERT_TRUE(mapa.ladoConhecido(0, 0, DIR_LESTE));
    TEST_ASSERT_TRUE(mapa.ladoConhecido(0, 1, DIR_OESTE));
}

void test_movimento_para_fora_da_grade_e_recusado(void) {
    Mapa mapa;
    TEST_ASSERT_FALSE(mapa.registrarMovimento(0, 3, DIR_LESTE));
    TEST_ASSERT_FALSE(mapa.registrarMovimento(3, 0, DIR_SUL));
    TEST_ASSERT_TRUE(mapa.temParede(0, 3, DIR_LESTE));
    TEST_ASSERT_TRUE(mapa.temParede(3, 0, DIR_SUL));
}

// Exemplo da tabela da seção 5.1 do doc 4.4: pista 4x4, partida em (0, 0) virado
// para o leste, avanço até (0, 2) e parede à frente.
void test_exemplo_da_tabela_da_secao_5_1(void) {
    Mapa mapa;
    mapa.marcarVisitada(0, 0);
    mapa.registrarLeituras(0, 0, DIR_LESTE, false, true, false);
    mapa.registrarMovimento(0, 0, DIR_LESTE);
    mapa.registrarLeituras(0, 1, DIR_LESTE, false, true, false);
    mapa.registrarMovimento(0, 1, DIR_LESTE);
    mapa.registrarLeituras(0, 2, DIR_LESTE, true, true, false);

    TEST_ASSERT_EQUAL_HEX8(PAREDE_NORTE | PAREDE_OESTE, mapa.getParedes(0, 0));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_NORTE, mapa.getParedes(0, 1));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_NORTE | PAREDE_LESTE, mapa.getParedes(0, 2));
    for (uint8_t c = 0; c < 3; c++) {
        TEST_ASSERT_TRUE(mapa.foiVisitada(0, c));
        TEST_ASSERT_EQUAL_HEX8(PAREDE_TODAS, mapa.getLadosConhecidos(0, c));
    }

    // Demais células não visitadas; só se sabe o que veio do perímetro e das vizinhas.
    TEST_ASSERT_FALSE(mapa.foiVisitada(0, 3));
    TEST_ASSERT_TRUE(mapa.temParede(0, 3, DIR_OESTE));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_NORTE | PAREDE_LESTE | PAREDE_OESTE, mapa.getLadosConhecidos(0, 3));
    TEST_ASSERT_EQUAL_HEX8(PAREDE_NORTE, mapa.getLadosConhecidos(1, 1));
    TEST_ASSERT_EQUAL_HEX8(0, mapa.getLadosConhecidos(2, 2));
}

// ---------------------------------------------------------------------------
// Percurso simulado: o mapa final deve ser idêntico à pista física
// ---------------------------------------------------------------------------

// Pista 4x4 conhecida. Cada célula ocupa 4 colunas e 2 linhas do desenho; como
// células vizinhas compartilham o mesmo traço, a pista é simétrica por construção.
// Há um ciclo entre (0, 2), (0, 3), (1, 3) e (1, 2).
static const char* const PISTA_4X4[] = {
    "+---+---+---+---+",
    "|       |       |",
    "+---+   +   +   +",
    "|       |       |",
    "+   +---+   +   +",
    "|   |       |   |",
    "+   +   +---+   +",
    "|       |       |",
    "+---+---+---+---+",
};

static uint8_t paredesDaPista(uint8_t linha, uint8_t coluna) {
    const char* acima = PISTA_4X4[2 * linha];
    const char* meio = PISTA_4X4[2 * linha + 1];
    const char* abaixo = PISTA_4X4[2 * linha + 2];
    const int x = 4 * coluna;
    uint8_t m = 0;
    if (acima[x + 2] == '-') m |= PAREDE_NORTE;
    if (abaixo[x + 2] == '-') m |= PAREDE_SUL;
    if (meio[x + 4] == '|') m |= PAREDE_LESTE;
    if (meio[x] == '|') m |= PAREDE_OESTE;
    return m;
}

static bool pistaTemParede(uint8_t linha, uint8_t coluna, Direcao d) {
    return (paredesDaPista(linha, coluna) & mascaraParede(d)) != 0;
}

// Sensores simulados: frente, esquerda e direita lidos da pista física.
static void lerSensores(Mapa& mapa, uint8_t linha, uint8_t coluna, Direcao orientacao) {
    mapa.registrarLeituras(linha, coluna, orientacao,
                           pistaTemParede(linha, coluna, direcaoAbsoluta(orientacao, LADO_FRENTE)),
                           pistaTemParede(linha, coluna, direcaoAbsoluta(orientacao, LADO_ESQUERDA)),
                           pistaTemParede(linha, coluna, direcaoAbsoluta(orientacao, LADO_DIREITA)));
}

// Explorador em profundidade usado só neste teste (a navegação real é da HU-06).
// Ele decide os movimentos apenas pelo que o Mapa já registrou, nunca pela pista.
static uint16_t movimentos;

static void explorar(Mapa& mapa, uint8_t linha, uint8_t coluna, Direcao& orientacao) {
    lerSensores(mapa, linha, coluna, orientacao);
    const Direcao ordem[] = {DIR_NORTE, DIR_LESTE, DIR_SUL, DIR_OESTE};
    for (uint8_t i = 0; i < 4; i++) {
        const Direcao d = ordem[i];
        uint8_t lv, cv;
        // Frente e laterais vieram dos sensores e o lado de trás do movimento de entrada.
        TEST_ASSERT_TRUE(mapa.ladoConhecido(linha, coluna, d));
        if (mapa.temParede(linha, coluna, d)) continue;
        TEST_ASSERT_TRUE(mapa.vizinha(linha, coluna, d, lv, cv));
        if (mapa.foiVisitada(lv, cv)) continue;

        TEST_ASSERT_TRUE(mapa.registrarMovimento(linha, coluna, d));
        movimentos++;
        orientacao = d;
        explorar(mapa, lv, cv, orientacao);

        // Volta para a célula de origem antes de tentar a próxima direção.
        TEST_ASSERT_TRUE(mapa.registrarMovimento(lv, cv, direcaoOposta(d)));
        movimentos++;
        orientacao = direcaoOposta(d);
    }
}

void test_pista_do_teste_e_valida(void) {
    for (uint8_t l = 0; l < 4; l++) {
        for (uint8_t c = 0; c < 4; c++) {
            const uint8_t p = paredesDaPista(l, c);
            TEST_ASSERT_EQUAL_HEX8(perimetroEsperado(l, c, 4), p & perimetroEsperado(l, c, 4));
            if (c < 3) TEST_ASSERT_EQUAL(pistaTemParede(l, c, DIR_LESTE), pistaTemParede(l, c + 1, DIR_OESTE));
            if (l < 3) TEST_ASSERT_EQUAL(pistaTemParede(l, c, DIR_SUL), pistaTemParede(l + 1, c, DIR_NORTE));
        }
    }
}

void test_percurso_simulado_reproduz_a_pista_4x4(void) {
    Mapa mapa;
    Direcao orientacao = DIR_LESTE;
    movimentos = 0;

    mapa.marcarVisitada(0, 0);
    explorar(mapa, 0, 0, orientacao);

    // Árvore de exploração com 16 células: 15 avanços e 15 retornos.
    TEST_ASSERT_EQUAL_UINT16(30, movimentos);
    for (uint8_t l = 0; l < 4; l++) {
        for (uint8_t c = 0; c < 4; c++) {
            TEST_ASSERT_TRUE(mapa.foiVisitada(l, c));
            TEST_ASSERT_EQUAL_HEX8(PAREDE_TODAS, mapa.getLadosConhecidos(l, c));
            TEST_ASSERT_EQUAL_HEX8(paredesDaPista(l, c), mapa.getParedes(l, c));
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mapa_default_geometry);
    RUN_TEST(test_mapa_change_to_8x4);
    RUN_TEST(test_mapa_change_to_12x4);
    RUN_TEST(test_mapa_cycle_geometry);

    RUN_TEST(test_perimetro_registrado_nas_tres_geometrias);
    RUN_TEST(test_trocar_geometria_e_limpar_reiniciam_o_mapa);
    RUN_TEST(test_parede_gravada_tambem_na_vizinha_nas_quatro_direcoes);
    RUN_TEST(test_exemplo_de_simetria_da_secao_5_1);
    RUN_TEST(test_vizinha_respeita_os_limites_da_geometria);
    RUN_TEST(test_parede_no_perimetro_nao_altera_outras_celulas);
    RUN_TEST(test_passagem_no_perimetro_e_recusada);
    RUN_TEST(test_celula_fora_da_geometria_e_rejeitada);
    RUN_TEST(test_marcar_e_consultar_celula_visitada);
    RUN_TEST(test_lado_nao_observado_e_diferente_de_passagem);
    RUN_TEST(test_leituras_convertidas_para_direcoes_absolutas);
    RUN_TEST(test_leituras_nas_quatro_orientacoes);
    RUN_TEST(test_leitura_de_passagem_no_perimetro_e_ignorada);
    RUN_TEST(test_movimento_registra_passagem_e_visita_o_destino);
    RUN_TEST(test_movimento_para_fora_da_grade_e_recusado);
    RUN_TEST(test_exemplo_da_tabela_da_secao_5_1);
    RUN_TEST(test_pista_do_teste_e_valida);
    RUN_TEST(test_percurso_simulado_reproduz_a_pista_4x4);
    return UNITY_END();
}

