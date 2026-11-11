#include <stdint.h>
#include <unity.h>
#include "../../src/movimento/InterfacesMovimento.h"
#include "../../src/navegacao/Navegador.h"
#include "../comum/SimuladorLabirinto.h"

// Testes da interface não bloqueante do executor e do adaptador ExecutorImediato
// (docs/firmware/integracao-loop.md, achado B3 e seção 3.5).

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Dublê: executor bloqueante que registra as chamadas e devolve o resultado configurado
// ---------------------------------------------------------------------------

struct ExecutorBloqueanteFalso : ExecutorMovimento {
    int giros, avancos;
    Giro ultimoGiro;
    bool resultado;
    ExecutorBloqueanteFalso() : giros(0), avancos(0), ultimoGiro(GIRO_NENHUM), resultado(true) {}
    bool girar(Giro giro) override {
        giros++;
        ultimoGiro = giro;
        return resultado;
    }
    bool avancarCelula() override {
        avancos++;
        return resultado;
    }
};

// ---------------------------------------------------------------------------
// Contrato do adaptador
// ---------------------------------------------------------------------------

void test_comeca_ocioso_e_atualizar_sem_pedido_nao_move(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    TEST_ASSERT_TRUE(exec.ocioso());
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(0));
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, exec.motivoUltimaFalha());
    TEST_ASSERT_EQUAL(0, falso.giros);
    TEST_ASSERT_EQUAL(0, falso.avancos);
}

void test_giro_so_e_executado_no_atualizar(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    TEST_ASSERT_TRUE(exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 0));
    TEST_ASSERT_FALSE(exec.ocioso());
    TEST_ASSERT_EQUAL(0, falso.giros);  // iniciar não executa nada

    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(1, falso.giros);
    TEST_ASSERT_EQUAL(GIRO_ESQUERDA, falso.ultimoGiro);
    TEST_ASSERT_TRUE(exec.ocioso());
}

void test_avanco_so_e_executado_no_atualizar(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    TEST_ASSERT_TRUE(exec.iniciarAvanco(0));
    TEST_ASSERT_EQUAL(0, falso.avancos);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(1, falso.avancos);
    TEST_ASSERT_EQUAL(0, falso.giros);
}

void test_resultado_final_e_entregue_uma_vez_so(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    exec.iniciarAvanco(0);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(20));
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(30));
    TEST_ASSERT_EQUAL(1, falso.avancos);  // não repete o movimento
}

void test_falha_do_executor_bloqueante_vira_mov_falhou_por_tempo(void) {
    ExecutorBloqueanteFalso falso;
    falso.resultado = false;
    ExecutorImediato exec(falso);
    exec.iniciarGiro(GIRO_DIREITA, DIR_NORTE, 0);
    TEST_ASSERT_EQUAL(MOV_FALHOU, exec.atualizar(10));
    TEST_ASSERT_EQUAL(FALHA_TEMPO, exec.motivoUltimaFalha());
    TEST_ASSERT_TRUE(exec.ocioso());

    // Um movimento bem-sucedido depois limpa o motivo.
    falso.resultado = true;
    exec.iniciarAvanco(20);
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, exec.atualizar(30));
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, exec.motivoUltimaFalha());
}

void test_giro_nenhum_e_recusado(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    TEST_ASSERT_FALSE(exec.iniciarGiro(GIRO_NENHUM, DIR_LESTE, 0));
    TEST_ASSERT_TRUE(exec.ocioso());
    TEST_ASSERT_EQUAL(FALHA_PEDIDO_INVALIDO, exec.motivoUltimaFalha());
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(0, falso.giros);
}

void test_novo_pedido_com_movimento_em_andamento_e_recusado(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    TEST_ASSERT_TRUE(exec.iniciarGiro(GIRO_MEIA_VOLTA, DIR_LESTE, 0));
    TEST_ASSERT_FALSE(exec.iniciarAvanco(1));
    TEST_ASSERT_FALSE(exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, 1));
    TEST_ASSERT_EQUAL(FALHA_PEDIDO_INVALIDO, exec.motivoUltimaFalha());

    // O pedido original continua valendo.
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(GIRO_MEIA_VOLTA, falso.ultimoGiro);
    TEST_ASSERT_EQUAL(1, falso.giros);
    TEST_ASSERT_EQUAL(0, falso.avancos);
}

void test_abortar_descarta_o_pedido_sem_executar(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    exec.iniciarAvanco(0);
    exec.abortar();
    TEST_ASSERT_TRUE(exec.ocioso());
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(0, falso.avancos);
    TEST_ASSERT_TRUE(exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, 20));  // aceita novo pedido
}

// ---------------------------------------------------------------------------
// Navegação pelas três camadas do Navegador, como o laço do robô fará (seção 3.3)
// ---------------------------------------------------------------------------

struct ResultadoCamadas {
    uint16_t ciclos;
    bool divergiu;
};

// Espera o fim do movimento chamando atualizar() no ritmo de controle (10 ms).
static ResultadoMovimento esperarMovimento(ExecutorMovimentoNaoBloqueante& exec, uint32_t& agora) {
    ResultadoMovimento m;
    do {
        agora += 10;
        m = exec.atualizar(agora);
    } while (m == MOV_EM_ANDAMENTO);
    return m;
}

// Subfases LENDO_PAREDES, DECIDINDO, GIRANDO e AVANCANDO, sem passo(). O resultado do
// executor é o que autoriza confirmarGiro/confirmarAvanco ou registrarFalha.
static ResultadoCamadas navegarPorCamadas(Navegador& nav, SimuladorLabirinto& sim,
                                          ExecutorMovimentoNaoBloqueante& exec) {
    ResultadoCamadas r = {0, false};
    uint32_t agora = 0;
    while (nav.emExploracao() && r.ciclos < 5000) {
        r.ciclos++;
        bool f, e, d;
        if (!sim.lerParedes(f, e, d)) {
            nav.registrarFalha();
            continue;
        }
        nav.observar(f, e, d);
        const DecisaoNavegacao dec = nav.decidir();
        if (!dec.movimento) break;

        if (dec.giro != GIRO_NENHUM) {
            TEST_ASSERT_TRUE(exec.iniciarGiro(dec.giro, nav.orientacao(), agora));
            if (esperarMovimento(exec, agora) != MOV_CONCLUIDO) {
                nav.registrarFalha();
                continue;
            }
            nav.confirmarGiro(dec.giro);
        }
        TEST_ASSERT_TRUE(exec.iniciarAvanco(agora));
        if (esperarMovimento(exec, agora) != MOV_CONCLUIDO) {
            nav.registrarFalha();
            continue;
        }
        nav.confirmarAvanco();
        if (nav.linha() != sim.linhaReal || nav.coluna() != sim.colunaReal) r.divergiu = true;
    }
    return r;
}

static void labirintoBecos4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "LLLSSS");
    s.abrirCaminho(0, 1, "SSS");
    s.abrirCaminho(0, 2, "S");
}

void test_camadas_com_executor_imediato_chegam_como_o_passo(void) {
    // Referência: o caminho testado na HU-06, com passo().
    Mapa mapaRef;
    SimuladorLabirinto simRef(4);
    labirintoBecos4(simRef);
    Navegador ref(mapaRef);
    ref.iniciar();
    for (uint16_t i = 0; i < 5000 && ref.passo(simRef, simRef); i++) {}
    TEST_ASSERT_TRUE(ref.chegou());

    // Mesmo labirinto pelas camadas, com o adaptador não bloqueante.
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoBecos4(sim);
    ExecutorImediato exec(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const ResultadoCamadas r = navegarPorCamadas(nav, sim, exec);

    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_FALSE(r.divergiu);
    TEST_ASSERT_EQUAL(sim.orientacaoReal, nav.orientacao());  // orientação de fonte única
    TEST_ASSERT_EQUAL_UINT16(0, sim.movimentosInvalidos);
    TEST_ASSERT_EQUAL_UINT16(ref.deslocamentos(), nav.deslocamentos());
    TEST_ASSERT_EQUAL_UINT16(ref.giros(), nav.giros());
    TEST_ASSERT_EQUAL_UINT16(ref.retornos(), nav.retornos());
}

void test_camadas_com_falhas_injetadas_nao_divergem(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoBecos4(sim);
    sim.falhasDeGiroPendentes = 1;
    sim.falhasDeAvancoPendentes = 1;
    ExecutorImediato exec(sim);
    Navegador nav(mapa);
    nav.iniciar();
    const ResultadoCamadas r = navegarPorCamadas(nav, sim, exec);

    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_FALSE(r.divergiu);
    TEST_ASSERT_EQUAL(sim.orientacaoReal, nav.orientacao());
    TEST_ASSERT_EQUAL_UINT16(2, nav.falhas());
}

int main(void) {
    UNITY_BEGIN();
    // Contrato do adaptador
    RUN_TEST(test_comeca_ocioso_e_atualizar_sem_pedido_nao_move);
    RUN_TEST(test_giro_so_e_executado_no_atualizar);
    RUN_TEST(test_avanco_so_e_executado_no_atualizar);
    RUN_TEST(test_resultado_final_e_entregue_uma_vez_so);
    RUN_TEST(test_falha_do_executor_bloqueante_vira_mov_falhou_por_tempo);
    RUN_TEST(test_giro_nenhum_e_recusado);
    RUN_TEST(test_novo_pedido_com_movimento_em_andamento_e_recusado);
    RUN_TEST(test_abortar_descarta_o_pedido_sem_executar);
    // Navegação pelas camadas
    RUN_TEST(test_camadas_com_executor_imediato_chegam_como_o_passo);
    RUN_TEST(test_camadas_com_falhas_injetadas_nao_divergem);
    return UNITY_END();
}
