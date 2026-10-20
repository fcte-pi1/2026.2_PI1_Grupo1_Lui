#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include "../../src/navegacao/Navegador.h"
#include "SimuladorLabirinto.h"

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Utilitários
// ---------------------------------------------------------------------------

struct Resultado {
    uint16_t iteracoes;
    bool terminou;          // saiu do laço antes do limite de iterações
    bool saiuDosLimites;    // a posição lógica ficou fora da geometria em algum passo
    bool divergiu;          // posição lógica diferente da física em algum passo
};

static const uint16_t LIMITE_ITERACOES = 5000;

static GeometriaLabirinto geometriaDe(uint8_t linhas) {
    return linhas == 4 ? LAB_4X4 : linhas == 8 ? LAB_8X4 : LAB_12X4;
}

// Executa a navegação com limite finito de iterações para detectar laços infinitos.
static Resultado executar(Navegador& nav, Mapa& mapa, SimuladorLabirinto& sim) {
    Resultado r = {0, false, false, false};
    while (r.iteracoes < LIMITE_ITERACOES) {
        const bool continua = nav.passo(sim, sim);
        r.iteracoes++;
        if (!mapa.dentroDosLimites(nav.linha(), nav.coluna())) r.saiuDosLimites = true;
        if (nav.linha() != sim.linhaReal || nav.coluna() != sim.colunaReal) r.divergiu = true;
        if (!continua) {
            r.terminou = true;
            break;
        }
    }
    return r;
}

static void relatar(const char* nome, const Navegador& nav, const Mapa& mapa,
                    const SimuladorLabirinto& sim, const Resultado& r) {
    uint16_t visitadas = 0;
    for (uint8_t l = 0; l < mapa.getMaxLinhas(); l++)
        for (uint8_t c = 0; c < mapa.getMaxColunas(); c++)
            if (mapa.foiVisitada(l, c)) visitadas++;
    const char* fim = nav.estado() == NAV_CHEGOU ? "CHEGOU"
                    : nav.estado() == NAV_INALCANCAVEL ? "INALCANCAVEL"
                    : nav.estado() == NAV_FALHA ? "FALHA" : "OUTRO";
    printf("[CENARIO] %-28s fim=%-12s visitadas=%2u desloc=%3u retornos=%3u decisoes=%3u "
           "giros=%3u invalidos=%u iteracoes=%u\n",
           nome, fim, visitadas, nav.deslocamentos(), nav.retornos(), nav.decisoes(),
           nav.giros(), sim.movimentosInvalidos, r.iteracoes);
}

// ---------------------------------------------------------------------------
// Labirintos de simulação (N, S, L, O a partir da célula indicada)
// ---------------------------------------------------------------------------

static void labirintoBecos4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "LLLSSS");
    s.abrirCaminho(0, 1, "SSS");
    s.abrirCaminho(0, 2, "S");
}

static void labirintoAlternativo4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "SSSLLL");  // rota correta
    s.abrirCaminho(0, 0, "LL");      // rota alternativa tentada primeiro, sem saída
}

static void labirintoCiclos4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "LSON");    // anel 2x2
    s.abrirCaminho(1, 1, "LLSS");
    s.abrirCaminho(1, 2, "SL");      // segundo anel
}

static void labirintoAfastar4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "SSLNLLSS"); // passa por (2,1) e precisa voltar a (1,1)
}

static void labirinto8(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "SSSSSSSLLL");
    s.abrirCaminho(3, 0, "LL");
    s.abrirCaminho(5, 0, "LL");
}

static void labirinto12(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "SSSSSSSSSSSLLL");
    s.abrirCaminho(2, 0, "LSO");      // ciclo
    s.abrirCaminho(5, 0, "LLLSS");    // ramo longo sem saída
}

static void labirintoInacessivel4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "LLLSS");
    s.abrirCaminho(3, 3, "OO");       // chegada isolada de (2,3)
}

// ---------------------------------------------------------------------------
// Testes unitários (01 a 18)
// ---------------------------------------------------------------------------

// Teste 01
void test_inicializacao_em_a1_orientado_para_leste(void) {
    Mapa mapa;
    Navegador nav(mapa);
    TEST_ASSERT_EQUAL(NAV_NAO_INICIADO, nav.estado());
    nav.iniciar();
    TEST_ASSERT_EQUAL_UINT8(0, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(0, nav.coluna());
    TEST_ASSERT_EQUAL(DIR_LESTE, nav.orientacao());
    TEST_ASSERT_EQUAL(NAV_EXPLORANDO, nav.estado());
    TEST_ASSERT_TRUE(mapa.foiVisitada(0, 0));
    TEST_ASSERT_EQUAL(MAPEANDO, nav.estadoRobo());
}

static void verificarObjetivo(GeometriaLabirinto geo, uint8_t linhaEsperada) {
    Mapa mapa;
    mapa.setGeometria(geo);
    Navegador nav(mapa);
    nav.iniciar();
    TEST_ASSERT_EQUAL_UINT8(linhaEsperada, nav.linhaObjetivo());
    TEST_ASSERT_EQUAL_UINT8(3, nav.colunaObjetivo());
    TEST_ASSERT_FALSE(nav.noObjetivo());
}

// Testes 02, 03 e 04
void test_objetivo_4x4_e_d4(void)   { verificarObjetivo(LAB_4X4, 3); }
void test_objetivo_8x4_e_d8(void)   { verificarObjetivo(LAB_8X4, 7); }
void test_objetivo_12x4_e_d12(void) { verificarObjetivo(LAB_12X4, 11); }

// Iniciar não pode apagar o que o mapa já tem (a geometria é escolhida antes).
void test_iniciar_preserva_o_mapa(void) {
    Mapa mapa;
    mapa.registrarParede(1, 1, DIR_LESTE);
    Navegador nav(mapa);
    nav.iniciar();
    TEST_ASSERT_TRUE(mapa.temParede(1, 1, DIR_LESTE));
}

// Teste 05
void test_caminho_livre_escolhe_direcao_valida(void) {
    Mapa mapa;
    Navegador nav(mapa);
    nav.iniciar();
    nav.observar(false, true, false);  // frente livre, norte (perimetro) fechado, sul livre
    const DecisaoNavegacao d = nav.decidir();
    TEST_ASSERT_TRUE(d.movimento);
    TEST_ASSERT_TRUE(d.direcao == DIR_LESTE || d.direcao == DIR_SUL);
    TEST_ASSERT_FALSE(mapa.temParede(0, 0, d.direcao));
    TEST_ASSERT_TRUE(mapa.ladoConhecido(0, 0, d.direcao));
    // Decidir não move o robô.
    TEST_ASSERT_EQUAL_UINT8(0, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(0, nav.coluna());
    TEST_ASSERT_EQUAL(DIR_LESTE, nav.orientacao());
}

// Teste 06
void test_parede_a_frente_nao_e_atravessada(void) {
    Mapa mapa;
    Navegador nav(mapa);
    nav.iniciar();
    nav.observar(true, true, false);  // leste e norte fechados, sul livre
    const DecisaoNavegacao d = nav.decidir();
    TEST_ASSERT_TRUE(d.movimento);
    TEST_ASSERT_EQUAL(DIR_SUL, d.direcao);
    TEST_ASSERT_EQUAL(GIRO_DIREITA, d.giro);
    TEST_ASSERT_FALSE(d.retorno);
}

// Teste 07
void test_beco_sem_saida_exige_retorno(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "L");   // beco em (0,1)
    sim.abrirCaminho(0, 0, "SS");  // rota ainda inexplorada, para onde o robo deve voltar
    Navegador nav(mapa);
    nav.iniciar();
    TEST_ASSERT_TRUE(nav.passo(sim, sim));  // (0,0) -> (0,1)
    TEST_ASSERT_EQUAL_UINT8(1, nav.coluna());

    bool f, e, d;
    sim.lerParedes(f, e, d);
    TEST_ASSERT_TRUE(f && e && d);          // beco: tudo fechado, exceto o lado de tras
    nav.observar(f, e, d);
    const DecisaoNavegacao dec = nav.decidir();
    TEST_ASSERT_TRUE(dec.movimento);
    TEST_ASSERT_TRUE(dec.retorno);
    TEST_ASSERT_EQUAL(DIR_OESTE, dec.direcao);
    TEST_ASSERT_EQUAL(GIRO_MEIA_VOLTA, dec.giro);
}

// Teste 08
void test_caminho_alternativo_apos_rota_sem_saida(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoAlternativo4(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const Resultado r = executar(nav, mapa, sim);
    relatar("alternativo 4x4", nav, mapa, sim, r);
    TEST_ASSERT_TRUE(r.terminou);
    TEST_ASSERT_EQUAL(NAV_CHEGOU, nav.estado());
    TEST_ASSERT_TRUE(nav.retornos() > 0);   // voltou pelo caminho conhecido
    TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
}

// Teste 09
void test_decisao_usa_o_historico_de_visitas(void) {
    Mapa mapa;
    Navegador nav(mapa);
    nav.iniciar();
    // Leste e sul abertos; a vizinha leste já foi visitada e é um beco conhecido.
    mapa.registrarPassagem(0, 0, DIR_LESTE);
    mapa.registrarPassagem(0, 0, DIR_SUL);
    mapa.registrarParede(0, 0, DIR_NORTE);
    mapa.marcarVisitada(0, 1);
    mapa.registrarParede(0, 1, DIR_LESTE);
    mapa.registrarParede(0, 1, DIR_SUL);
    const DecisaoNavegacao d = nav.decidir();
    TEST_ASSERT_TRUE(d.movimento);
    TEST_ASSERT_EQUAL(DIR_SUL, d.direcao);  // vai ao lado ainda nao visitado
    TEST_ASSERT_FALSE(d.retorno);
}

// Teste 10
void test_labirinto_com_ciclos_nao_repete_indefinidamente(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoCiclos4(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const Resultado r = executar(nav, mapa, sim);
    relatar("ciclos 4x4", nav, mapa, sim, r);
    TEST_ASSERT_TRUE(r.terminou);
    TEST_ASSERT_EQUAL(NAV_CHEGOU, nav.estado());
    TEST_ASSERT_TRUE(r.iteracoes < 100);
}

// Teste 11
void test_nenhum_movimento_fora_do_mapa(void) {
    const uint8_t geos[3] = {4, 8, 12};
    for (uint8_t i = 0; i < 3; i++) {
        Mapa mapa;
        mapa.setGeometria(geometriaDe(geos[i]));
        SimuladorLabirinto sim(geos[i]);
        sim.abrirTudo();
        Navegador nav(mapa);
        nav.iniciar();
        const Resultado r = executar(nav, mapa, sim);
        TEST_ASSERT_TRUE(r.terminou);
        TEST_ASSERT_FALSE(r.saiuDosLimites);
        TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
    }
    // Uma leitura incoerente de passagem no perimetro nao vira movimento para fora.
    Mapa mapa;
    Navegador nav(mapa);
    nav.iniciar();
    nav.observar(false, false, false);  // norte (esquerda) em A1 e perimetro
    const DecisaoNavegacao d = nav.decidir();
    TEST_ASSERT_TRUE(d.movimento);
    TEST_ASSERT_TRUE(d.direcao != DIR_NORTE);
    TEST_ASSERT_TRUE(d.direcao != DIR_OESTE);
}

// Teste 12
void test_lado_desconhecido_nao_e_passagem_livre(void) {
    Mapa mapa;
    Navegador nav(mapa);
    nav.iniciar();
    // Sem nenhuma leitura, leste e sul de A1 sao desconhecidos: nao ha rota confirmada.
    TEST_ASSERT_FALSE(mapa.ladoConhecido(0, 0, DIR_LESTE));
    const DecisaoNavegacao d = nav.decidir();
    TEST_ASSERT_FALSE(d.movimento);
    TEST_ASSERT_EQUAL(NAV_INALCANCAVEL, nav.estado());
    TEST_ASSERT_EQUAL_UINT8(0, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(0, nav.coluna());
}

// Teste 13
void test_chegada_reconhecida_na_celula_de_destino(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Navegador nav(mapa);
    nav.iniciar();
    const Resultado r = executar(nav, mapa, sim);
    relatar("aberto 4x4", nav, mapa, sim, r);
    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_TRUE(nav.noObjetivo());
    TEST_ASSERT_EQUAL_UINT8(3, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(3, nav.coluna());
    TEST_ASSERT_TRUE(sim.naChegada());
    TEST_ASSERT_EQUAL(CONCLUIDO, nav.estadoRobo());
    TEST_ASSERT_TRUE(mapa.foiVisitada(3, 3));  // informacoes coletadas preservadas
}

// Teste 14
void test_nenhum_movimento_apos_a_conclusao(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Navegador nav(mapa);
    nav.iniciar();
    executar(nav, mapa, sim);
    TEST_ASSERT_TRUE(nav.chegou());

    const uint16_t desloc = nav.deslocamentos();
    const uint16_t avancosReais = sim.avancos;
    const uint16_t giros = sim.girosFeitos;
    TEST_ASSERT_FALSE(nav.decidir().movimento);
    TEST_ASSERT_FALSE(nav.passo(sim, sim));
    TEST_ASSERT_FALSE(nav.confirmarAvanco());
    TEST_ASSERT_EQUAL_UINT16(desloc, nav.deslocamentos());
    TEST_ASSERT_EQUAL_UINT16(avancosReais, sim.avancos);
    TEST_ASSERT_EQUAL_UINT16(giros, sim.girosFeitos);
    TEST_ASSERT_TRUE(nav.chegou());
}

// Teste 15: o laco usa apenas sensores e executor simulados; nenhuma entrada externa.
void test_navegacao_depende_somente_de_leituras_internas(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoBecos4(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const Resultado r = executar(nav, mapa, sim);
    relatar("becos 4x4", nav, mapa, sim, r);
    TEST_ASSERT_TRUE(r.terminou);
    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_FALSE(r.divergiu);
    TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
    // O mapa do robo so conhece o que foi lido: celula (1,0) nao esta no caminho.
    TEST_ASSERT_FALSE(mapa.foiVisitada(1, 0));
    TEST_ASSERT_FALSE(mapa.ladoConhecido(1, 0, DIR_LESTE));
}

// Teste 16
void test_falha_de_avanco_nao_altera_o_estado_logico(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Navegador nav(mapa);
    nav.iniciar();
    sim.falhasDeAvancoPendentes = 1;
    TEST_ASSERT_TRUE(nav.passo(sim, sim));
    TEST_ASSERT_EQUAL_UINT8(0, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(0, nav.coluna());
    TEST_ASSERT_EQUAL_UINT16(0, nav.deslocamentos());
    TEST_ASSERT_EQUAL_UINT16(1, nav.falhas());
    TEST_ASSERT_FALSE(mapa.foiVisitada(0, 1));
    TEST_ASSERT_FALSE(mapa.foiVisitada(1, 0));
    // Uma nova tentativa bem-sucedida avanca normalmente.
    TEST_ASSERT_TRUE(nav.passo(sim, sim));
    TEST_ASSERT_EQUAL_UINT16(1, nav.deslocamentos());
    TEST_ASSERT_EQUAL_UINT8(sim.linhaReal, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(sim.colunaReal, nav.coluna());
}

void test_falha_de_giro_nao_altera_a_orientacao(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirCaminho(0, 0, "S");  // leste fechado: o primeiro passo exige giro a direita
    Navegador nav(mapa);
    nav.iniciar();
    sim.falhasDeGiroPendentes = 1;
    TEST_ASSERT_TRUE(nav.passo(sim, sim));
    TEST_ASSERT_EQUAL(DIR_LESTE, nav.orientacao());
    TEST_ASSERT_EQUAL(DIR_LESTE, sim.orientacaoReal);
    TEST_ASSERT_EQUAL_UINT16(0, nav.giros());
    TEST_ASSERT_EQUAL_UINT16(0, nav.deslocamentos());
    TEST_ASSERT_TRUE(nav.passo(sim, sim));
    TEST_ASSERT_EQUAL(DIR_SUL, nav.orientacao());
    TEST_ASSERT_EQUAL_UINT8(1, nav.linha());
}

void test_falhas_seguidas_encerram_em_falha(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Navegador nav(mapa);
    nav.iniciar();
    sim.falhasDeAvancoPendentes = Navegador::LIMITE_FALHAS;
    Resultado r = executar(nav, mapa, sim);
    TEST_ASSERT_TRUE(r.terminou);
    TEST_ASSERT_EQUAL(NAV_FALHA, nav.estado());
    TEST_ASSERT_EQUAL(TRAVADO, nav.estadoRobo());
    TEST_ASSERT_EQUAL_UINT8(0, nav.linha());
    TEST_ASSERT_EQUAL_UINT8(0, nav.coluna());
}

void test_falha_de_leitura_nao_decide_com_dado_ausente(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Navegador nav(mapa);
    nav.iniciar();
    sim.falhasDeLeituraPendentes = 1;
    TEST_ASSERT_TRUE(nav.passo(sim, sim));
    TEST_ASSERT_EQUAL_UINT16(0, nav.decisoes());
    TEST_ASSERT_EQUAL_UINT16(0, sim.avancos);
    TEST_ASSERT_TRUE(nav.passo(sim, sim));
    TEST_ASSERT_EQUAL_UINT16(1, sim.avancos);
}

// Teste 17
void test_labirinto_impossivel_encerra_sem_repeticao(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoInacessivel4(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const Resultado r = executar(nav, mapa, sim);
    relatar("destino inacessivel 4x4", nav, mapa, sim, r);
    TEST_ASSERT_TRUE(r.terminou);
    TEST_ASSERT_EQUAL(NAV_INALCANCAVEL, nav.estado());
    TEST_ASSERT_FALSE(nav.noObjetivo());
    TEST_ASSERT_FALSE(mapa.foiVisitada(3, 3));
    TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
    TEST_ASSERT_TRUE(r.iteracoes < 100);
    // Todas as celulas acessiveis foram exploradas: (0,0..3), (1,3), (2,3).
    TEST_ASSERT_TRUE(mapa.foiVisitada(2, 3));
}

// Teste 18
void test_comportamento_deterministico(void) {
    uint8_t tracoA[SimuladorLabirinto::TRACO_MAX];
    uint16_t nA = 0, giroA = 0, retA = 0;
    for (uint8_t rodada = 0; rodada < 2; rodada++) {
        Mapa mapa;
        SimuladorLabirinto sim(4);
        labirintoCiclos4(sim);
        Navegador nav(mapa);
        nav.iniciar();
        executar(nav, mapa, sim);
        if (rodada == 0) {
            nA = sim.tamanhoTraco;
            giroA = nav.giros();
            retA = nav.retornos();
            for (uint16_t i = 0; i < nA; i++) tracoA[i] = sim.traco[i];
        } else {
            TEST_ASSERT_EQUAL_UINT16(nA, sim.tamanhoTraco);
            TEST_ASSERT_EQUAL_UINT16(giroA, nav.giros());
            TEST_ASSERT_EQUAL_UINT16(retA, nav.retornos());
            TEST_ASSERT_EQUAL_UINT8_ARRAY(tracoA, sim.traco, nA);
        }
    }
}

// ---------------------------------------------------------------------------
// Integração com o Mapa (HU-03) e a telemetria
// ---------------------------------------------------------------------------

void test_integracao_mapa_registra_paredes_reais_nas_duas_celulas(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoBecos4(sim);
    Navegador nav(mapa);
    nav.iniciar();
    executar(nav, mapa, sim);
    // Toda parede conhecida do mapa existe de verdade e vice-versa nas celulas visitadas.
    for (uint8_t l = 0; l < 4; l++)
        for (uint8_t c = 0; c < 4; c++) {
            // A celula de chegada nao e observada: a navegacao encerra ao entrar nela.
            if (!mapa.foiVisitada(l, c) || (l == nav.linhaObjetivo() && c == nav.colunaObjetivo())) continue;
            TEST_ASSERT_EQUAL_UINT8(PAREDE_TODAS, mapa.getLadosConhecidos(l, c));
            TEST_ASSERT_EQUAL_UINT8(sim.verdadeiro[l][c], mapa.getParedes(l, c));
        }
}

void test_integracao_telemetria_recebe_posicao_e_estado(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    sim.abrirTudo();
    Navegador nav(mapa);
    nav.iniciar();
    executar(nav, mapa, sim);
    TelemetriaPacket p;
    memset(&p, 0, sizeof(p));
    p.labirinto_id = LAB_4X4;
    nav.preencherPosicao(p);
    TEST_ASSERT_EQUAL_UINT8(3, p.linha);
    TEST_ASSERT_EQUAL_UINT8(3, p.coluna);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)nav.orientacao(), p.orientacao);
    TEST_ASSERT_EQUAL_UINT8(mapa.getParedes(3, 3), p.paredes);
    TEST_ASSERT_EQUAL_UINT8(LAB_4X4, p.labirinto_id);  // campos alheios intactos
    TEST_ASSERT_EQUAL(CONCLUIDO, nav.estadoRobo());
}

// ---------------------------------------------------------------------------
// Simulação de labirintos completos (9.3)
// ---------------------------------------------------------------------------

static void rodarCenario(const char* nome, uint8_t linhas, void (*montar)(SimuladorLabirinto&),
                         EstadoNavegacao esperado) {
    Mapa mapa;
    mapa.setGeometria(geometriaDe(linhas));
    SimuladorLabirinto sim(linhas);
    montar(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const Resultado r = executar(nav, mapa, sim);
    relatar(nome, nav, mapa, sim, r);
    TEST_ASSERT_TRUE(r.terminou);
    TEST_ASSERT_EQUAL(esperado, nav.estado());
    TEST_ASSERT_FALSE(r.saiuDosLimites);
    TEST_ASSERT_FALSE(r.divergiu);
    TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
    if (esperado == NAV_CHEGOU) TEST_ASSERT_TRUE(sim.naChegada());
}

static void abrir4(SimuladorLabirinto& s) { s.abrirTudo(); }
static void abrir8(SimuladorLabirinto& s) { s.abrirTudo(); }

void test_cenario_4x4_sem_paredes_internas(void)   { rodarCenario("sem paredes 4x4", 4, abrir4, NAV_CHEGOU); }
void test_cenario_4x4_com_becos(void)              { rodarCenario("becos 4x4", 4, labirintoBecos4, NAV_CHEGOU); }
void test_cenario_4x4_com_alternativas(void)       { rodarCenario("alternativas 4x4", 4, labirintoAlternativo4, NAV_CHEGOU); }
void test_cenario_4x4_com_ciclos(void)             { rodarCenario("ciclos 4x4", 4, labirintoCiclos4, NAV_CHEGOU); }
void test_cenario_4x4_exige_afastar_do_objetivo(void) {
    rodarCenario("afastar do objetivo 4x4", 4, labirintoAfastar4, NAV_CHEGOU);
}
void test_cenario_8x4(void)                        { rodarCenario("labirinto 8x4", 8, labirinto8, NAV_CHEGOU); }
void test_cenario_8x4_aberto(void)                 { rodarCenario("aberto 8x4", 8, abrir8, NAV_CHEGOU); }
void test_cenario_12x4(void)                       { rodarCenario("labirinto 12x4", 12, labirinto12, NAV_CHEGOU); }
void test_cenario_destino_inacessivel(void)        { rodarCenario("destino inacessivel 4x4", 4, labirintoInacessivel4, NAV_INALCANCAVEL); }

// Cenario que exige afastar-se: a distancia ao destino cresce em algum momento.
void test_afastar_do_objetivo_aumenta_a_distancia_temporariamente(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoAfastar4(sim);
    Navegador nav(mapa);
    nav.iniciar();
    int ultima = 6, pior = 0;
    for (uint16_t i = 0; i < LIMITE_ITERACOES && nav.passo(sim, sim); i++) {
        const int d = (3 - nav.linha()) + (3 - nav.coluna());
        if (d > ultima) pior = d;
        ultima = d;
    }
    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_TRUE(pior > 0);
}

// ---------------------------------------------------------------------------
// Desempenho (9.4): processamento, deslocamentos, giros e memória
// ---------------------------------------------------------------------------

void test_desempenho_de_processamento_e_memoria(void) {
    Mapa mapa;
    mapa.setGeometria(LAB_12X4);
    SimuladorLabirinto sim(12);
    labirinto12(sim);
    Navegador nav(mapa);
    nav.iniciar();

    const auto t0 = std::chrono::steady_clock::now();
    const Resultado r = executar(nav, mapa, sim);
    const auto t1 = std::chrono::steady_clock::now();
    const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();

    TEST_ASSERT_TRUE(nav.chegou());
    printf("[DESEMPENHO] 12x4: %u decisoes em %.1f us no PC (media %.2f us/decisao); "
           "deslocamentos=%u giros=%u retornos=%u\n",
           nav.decisoes(), us, us / (nav.decisoes() ? nav.decisoes() : 1),
           nav.deslocamentos(), nav.giros(), nav.retornos());
    printf("[MEMORIA] sizeof(Navegador)=%u B, sizeof(Mapa)=%u B\n",
           (unsigned)sizeof(Navegador), (unsigned)sizeof(Mapa));
    TEST_ASSERT_TRUE(r.iteracoes < Navegador::LIMITE_PASSOS);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_inicializacao_em_a1_orientado_para_leste);
    RUN_TEST(test_objetivo_4x4_e_d4);
    RUN_TEST(test_objetivo_8x4_e_d8);
    RUN_TEST(test_objetivo_12x4_e_d12);
    RUN_TEST(test_iniciar_preserva_o_mapa);
    RUN_TEST(test_caminho_livre_escolhe_direcao_valida);
    RUN_TEST(test_parede_a_frente_nao_e_atravessada);
    RUN_TEST(test_beco_sem_saida_exige_retorno);
    RUN_TEST(test_caminho_alternativo_apos_rota_sem_saida);
    RUN_TEST(test_decisao_usa_o_historico_de_visitas);
    RUN_TEST(test_labirinto_com_ciclos_nao_repete_indefinidamente);
    RUN_TEST(test_nenhum_movimento_fora_do_mapa);
    RUN_TEST(test_lado_desconhecido_nao_e_passagem_livre);
    RUN_TEST(test_chegada_reconhecida_na_celula_de_destino);
    RUN_TEST(test_nenhum_movimento_apos_a_conclusao);
    RUN_TEST(test_navegacao_depende_somente_de_leituras_internas);
    RUN_TEST(test_falha_de_avanco_nao_altera_o_estado_logico);
    RUN_TEST(test_falha_de_giro_nao_altera_a_orientacao);
    RUN_TEST(test_falhas_seguidas_encerram_em_falha);
    RUN_TEST(test_falha_de_leitura_nao_decide_com_dado_ausente);
    RUN_TEST(test_labirinto_impossivel_encerra_sem_repeticao);
    RUN_TEST(test_comportamento_deterministico);
    RUN_TEST(test_integracao_mapa_registra_paredes_reais_nas_duas_celulas);
    RUN_TEST(test_integracao_telemetria_recebe_posicao_e_estado);
    RUN_TEST(test_cenario_4x4_sem_paredes_internas);
    RUN_TEST(test_cenario_4x4_com_becos);
    RUN_TEST(test_cenario_4x4_com_alternativas);
    RUN_TEST(test_cenario_4x4_com_ciclos);
    RUN_TEST(test_cenario_4x4_exige_afastar_do_objetivo);
    RUN_TEST(test_cenario_8x4);
    RUN_TEST(test_cenario_8x4_aberto);
    RUN_TEST(test_cenario_12x4);
    RUN_TEST(test_cenario_destino_inacessivel);
    RUN_TEST(test_afastar_do_objetivo_aumenta_a_distancia_temporariamente);
    RUN_TEST(test_desempenho_de_processamento_e_memoria);
    return UNITY_END();
}
