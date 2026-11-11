#include <stdint.h>
#include <unity.h>
#include "../../src/movimento/InterfacesMovimento.h"
#include "../../src/navegacao/Navegador.h"
#include "../comum/ExecutorTemporizado.h"
#include "../comum/SimuladorLabirinto.h"

// Testes da interface não bloqueante do executor, do adaptador ExecutorImediato e do dublê
// ExecutorTemporizado (docs/firmware/integracao-loop.md, achado B3 e seção 3.5).

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Dublê: executor bloqueante que registra as chamadas, devolve o resultado configurado e
// informa o motivo da falha (InformanteMotivoFalha)
// ---------------------------------------------------------------------------

struct ExecutorBloqueanteFalso : ExecutorMovimento, InformanteMotivoFalha {
    int giros, avancos;
    Giro ultimoGiro;
    bool resultado;
    MotivoFalhaMovimento motivo;
    ExecutorBloqueanteFalso()
        : giros(0), avancos(0), ultimoGiro(GIRO_NENHUM), resultado(true), motivo(FALHA_TEMPO) {}
    MotivoFalhaMovimento motivoFalha() const override { return motivo; }
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
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, exec.motivoUltimaFalha());  // recusa só devolve false
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(0, falso.giros);
}

void test_novo_pedido_com_movimento_em_andamento_e_recusado(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorImediato exec(falso);
    TEST_ASSERT_TRUE(exec.iniciarGiro(GIRO_MEIA_VOLTA, DIR_LESTE, 0));
    TEST_ASSERT_FALSE(exec.iniciarAvanco(1));
    TEST_ASSERT_FALSE(exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, 1));
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, exec.motivoUltimaFalha());

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
// Motivo da falha (seção 3.7: COLISAO e TRAVADO)
// ---------------------------------------------------------------------------

void test_motivo_informado_pelo_executor_chega_ao_laco(void) {
    ExecutorBloqueanteFalso falso;
    falso.resultado = false;
    ExecutorImediato exec(falso, &falso);

    falso.motivo = FALHA_EMERGENCIA_FRONTAL;  // vira COLISAO no laço
    exec.iniciarAvanco(0);
    TEST_ASSERT_EQUAL(MOV_FALHOU, exec.atualizar(10));
    TEST_ASSERT_EQUAL(FALHA_EMERGENCIA_FRONTAL, exec.motivoUltimaFalha());

    falso.motivo = FALHA_TRACAO_CORTADA;  // vira TRAVADO no laço
    exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 20);
    TEST_ASSERT_EQUAL(MOV_FALHOU, exec.atualizar(30));
    TEST_ASSERT_EQUAL(FALHA_TRACAO_CORTADA, exec.motivoUltimaFalha());
}

void test_sem_informante_toda_falha_e_por_tempo(void) {
    ExecutorBloqueanteFalso falso;
    falso.resultado = false;
    falso.motivo = FALHA_EMERGENCIA_FRONTAL;  // ignorado: o adaptador não recebeu o informante
    ExecutorImediato exec(falso);
    exec.iniciarAvanco(0);
    exec.atualizar(10);
    TEST_ASSERT_EQUAL(FALHA_TEMPO, exec.motivoUltimaFalha());
}

void test_recusa_nao_apaga_o_motivo_de_uma_falha_nao_lida(void) {
    ExecutorBloqueanteFalso falso;
    falso.resultado = false;
    falso.motivo = FALHA_EMERGENCIA_FRONTAL;
    ExecutorImediato exec(falso, &falso);
    exec.iniciarAvanco(0);
    TEST_ASSERT_EQUAL(MOV_FALHOU, exec.atualizar(10));

    // Pedidos recusados antes de o laço ler o motivo não o sobrescrevem.
    TEST_ASSERT_FALSE(exec.iniciarGiro(GIRO_NENHUM, DIR_LESTE, 20));
    TEST_ASSERT_EQUAL(FALHA_EMERGENCIA_FRONTAL, exec.motivoUltimaFalha());
    TEST_ASSERT_TRUE(exec.iniciarAvanco(30));
    TEST_ASSERT_FALSE(exec.iniciarAvanco(31));
    TEST_ASSERT_FALSE(exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, 31));
    TEST_ASSERT_EQUAL(FALHA_EMERGENCIA_FRONTAL, exec.motivoUltimaFalha());
}

// ---------------------------------------------------------------------------
// Dublê com duração: ExecutorTemporizado
// ---------------------------------------------------------------------------

void test_temporizado_fica_em_andamento_pelos_ciclos_configurados(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorTemporizado exec(falso, 3);
    TEST_ASSERT_TRUE(exec.iniciarAvanco(0));
    TEST_ASSERT_EQUAL(MOV_EM_ANDAMENTO, exec.atualizar(10));
    TEST_ASSERT_EQUAL(MOV_EM_ANDAMENTO, exec.atualizar(20));
    TEST_ASSERT_EQUAL(MOV_EM_ANDAMENTO, exec.atualizar(30));
    TEST_ASSERT_EQUAL(0, falso.avancos);  // ainda não terminou
    TEST_ASSERT_FALSE(exec.iniciarGiro(GIRO_DIREITA, DIR_LESTE, 30));  // ocupado
    TEST_ASSERT_EQUAL(MOV_CONCLUIDO, exec.atualizar(40));
    TEST_ASSERT_EQUAL(1, falso.avancos);
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(50));
}

void test_temporizado_abortado_no_meio_nao_executa(void) {
    ExecutorBloqueanteFalso falso;
    ExecutorTemporizado exec(falso, 5);
    exec.iniciarGiro(GIRO_ESQUERDA, DIR_LESTE, 0);
    exec.atualizar(10);
    exec.abortar();
    TEST_ASSERT_TRUE(exec.ocioso());
    TEST_ASSERT_EQUAL(MOV_OCIOSO, exec.atualizar(20));
    TEST_ASSERT_EQUAL(0, falso.giros);
    TEST_ASSERT_EQUAL_UINT32(1, exec.movimentosAbortados());
}

// ---------------------------------------------------------------------------
// Navegação pelas três camadas do Navegador, como o laço do robô fará (seção 3.3)
// ---------------------------------------------------------------------------

struct ResultadoCamadas {
    uint16_t ciclos;
    bool divergiu;
    uint32_t esperas;             // chamadas a atualizar() que devolveram MOV_EM_ANDAMENTO
    uint16_t falhasPorMotivo[4];  // indexado por MotivoFalhaMovimento
};

// Espera o fim do movimento chamando atualizar() no ritmo de controle (10 ms), com limite
// para um dublê com defeito não prender o teste.
static ResultadoMovimento esperarMovimento(ExecutorMovimentoNaoBloqueante& exec, uint32_t& agora,
                                           ResultadoCamadas& r) {
    ResultadoMovimento m;
    uint16_t voltas = 0;
    do {
        agora += 10;
        m = exec.atualizar(agora);
        if (m == MOV_EM_ANDAMENTO) r.esperas++;
    } while (m == MOV_EM_ANDAMENTO && ++voltas < 1000);
    if (m == MOV_FALHOU) r.falhasPorMotivo[exec.motivoUltimaFalha()]++;
    return m;
}

// Subfases LENDO_PAREDES, DECIDINDO, GIRANDO e AVANCANDO, sem passo(). O resultado do
// executor é o que autoriza confirmarGiro/confirmarAvanco ou registrarFalha.
static ResultadoCamadas navegarPorCamadas(Navegador& nav, SimuladorLabirinto& sim,
                                          ExecutorMovimentoNaoBloqueante& exec) {
    ResultadoCamadas r = {0, false, 0, {0, 0, 0, 0}};
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
            if (esperarMovimento(exec, agora, r) != MOV_CONCLUIDO) {
                nav.registrarFalha();
                continue;
            }
            nav.confirmarGiro(dec.giro);
        }
        TEST_ASSERT_TRUE(exec.iniciarAvanco(agora));
        if (esperarMovimento(exec, agora, r) != MOV_CONCLUIDO) {
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

// Simulador que informa o motivo das falhas injetadas (o SimuladorLabirinto não muda).
struct SimuladorComMotivo : ExecutorMovimento, InformanteMotivoFalha {
    SimuladorLabirinto& sim;
    MotivoFalhaMovimento motivoGiro, motivoAvanco, ultimo;
    explicit SimuladorComMotivo(SimuladorLabirinto& s)
        : sim(s), motivoGiro(FALHA_TEMPO), motivoAvanco(FALHA_TEMPO), ultimo(FALHA_NENHUMA) {}
    bool girar(Giro g) override {
        ultimo = motivoGiro;
        return sim.girar(g);
    }
    bool avancarCelula() override {
        ultimo = motivoAvanco;
        return sim.avancarCelula();
    }
    MotivoFalhaMovimento motivoFalha() const override { return ultimo; }
};

void test_camadas_esperam_o_movimento_temporizado(void) {
    Mapa mapaRef;
    SimuladorLabirinto simRef(4);
    labirintoBecos4(simRef);
    Navegador ref(mapaRef);
    ref.iniciar();
    for (uint16_t i = 0; i < 5000 && ref.passo(simRef, simRef); i++) {}

    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoBecos4(sim);
    ExecutorTemporizado exec(sim, 4);
    Navegador nav(mapa);
    nav.iniciar();
    const ResultadoCamadas r = navegarPorCamadas(nav, sim, exec);

    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_FALSE(r.divergiu);
    TEST_ASSERT_EQUAL_UINT16(ref.deslocamentos(), nav.deslocamentos());
    TEST_ASSERT_EQUAL_UINT16(ref.giros(), nav.giros());
    // Cada movimento passou 4 ciclos em andamento: o laço de espera iterou de verdade.
    const uint32_t movimentos = (uint32_t)nav.deslocamentos() + nav.giros();
    TEST_ASSERT_EQUAL_UINT32(movimentos, exec.movimentosTerminados());
    TEST_ASSERT_EQUAL_UINT32(4 * movimentos, r.esperas);
    TEST_ASSERT_EQUAL_UINT32(r.esperas, exec.vezesEmAndamento());
}

void test_camadas_registram_o_motivo_de_cada_falha(void) {
    Mapa mapa;
    SimuladorLabirinto sim(4);
    labirintoBecos4(sim);
    sim.falhasDeGiroPendentes = 1;
    sim.falhasDeAvancoPendentes = 1;
    SimuladorComMotivo comMotivo(sim);
    comMotivo.motivoGiro = FALHA_TRACAO_CORTADA;
    comMotivo.motivoAvanco = FALHA_EMERGENCIA_FRONTAL;
    ExecutorTemporizado exec(comMotivo, 2, &comMotivo);
    Navegador nav(mapa);
    nav.iniciar();
    const ResultadoCamadas r = navegarPorCamadas(nav, sim, exec);

    TEST_ASSERT_TRUE(nav.chegou());
    TEST_ASSERT_FALSE(r.divergiu);
    TEST_ASSERT_EQUAL_UINT16(2, nav.falhas());
    TEST_ASSERT_EQUAL_UINT16(1, r.falhasPorMotivo[FALHA_EMERGENCIA_FRONTAL]);
    TEST_ASSERT_EQUAL_UINT16(1, r.falhasPorMotivo[FALHA_TRACAO_CORTADA]);
    TEST_ASSERT_EQUAL_UINT16(0, r.falhasPorMotivo[FALHA_TEMPO]);
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
    // Motivo da falha
    RUN_TEST(test_motivo_informado_pelo_executor_chega_ao_laco);
    RUN_TEST(test_sem_informante_toda_falha_e_por_tempo);
    RUN_TEST(test_recusa_nao_apaga_o_motivo_de_uma_falha_nao_lida);
    // Dublê com duração
    RUN_TEST(test_temporizado_fica_em_andamento_pelos_ciclos_configurados);
    RUN_TEST(test_temporizado_abortado_no_meio_nao_executa);
    // Navegação pelas camadas
    RUN_TEST(test_camadas_com_executor_imediato_chegam_como_o_passo);
    RUN_TEST(test_camadas_com_falhas_injetadas_nao_divergem);
    RUN_TEST(test_camadas_esperam_o_movimento_temporizado);
    RUN_TEST(test_camadas_registram_o_motivo_de_cada_falha);
    return UNITY_END();
}
