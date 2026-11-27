#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>
#include "GatewayBridge.h"
#include "TelemetriaCodec.h"
#include "../../src/app/CicloProva.h"
#include "../../src/movimento/ExecutorMovimentoRobo.h"
#include "../comum/LeitorPistas.h"
#include "../comum/MundoVirtual.h"
#include "../comum/PistasSlides.h"
#include "../comum/SimuladorLabirinto.h"

// Robô virtual (docs/firmware/integracao-loop.md, seção 3.10, nível 2): o firmware do robô,
// o mesmo do main.cpp (CicloProva, ExecutorMovimentoRobo, Motores, Odometria), roda tick a
// tick com relógio de 1 ms sobre o MundoVirtual, nas três pistas dos slides. Só o botão, a
// bateria, o LED e o rádio são dublês.
//
// Com a variável de ambiente ROBO_VIRTUAL_SAIDA apontando para uma pasta, as provas nas
// três pistas gravam ali as linhas JSON que o gateway geraria (robo_virtual_4x4.jsonl e
// assim por diante), para tools/painel_virtual.py enviar ao backend.

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Dublês do que não é físico
// ---------------------------------------------------------------------------

struct BotaoVirtual : IEntradaBotao {
    bool apertado = false;
    bool pressionado() override { return apertado; }
};

struct BateriaVirtual : IBateria {
    uint16_t mv = 7800;
    uint16_t lerMilivolts() override { return mv; }
};

struct LedVirtual : ILedEstado {
    EstadoRobo estado = AGUARDANDO;
    int avisos = 0;
    void mostrarEstado(EstadoRobo e) override { estado = e; }
    void piscar(CorLed, uint8_t, uint32_t) override { avisos++; }
    void atualizar(uint32_t, bool) override {}
};

// Rádio: confere cada pacote e, se pedido, grava a linha JSON do gateway.
struct RadioVirtual : IRadioTelemetria {
    int enviados = 0;
    int invalidos = 0;
    TelemetriaPacket ultimo;
    FILE* arquivo = nullptr;
    GatewayBridge ponte;

    RadioVirtual() { memset(&ultimo, 0, sizeof(ultimo)); }
    ~RadioVirtual() {
        if (arquivo != nullptr) fclose(arquivo);
    }

    bool enviar(const TelemetriaPacket& p) override {
        if (telemetriaValidar(p) != TELEMETRIA_OK) invalidos++;
        ultimo = p;
        enviados++;
        if (arquivo != nullptr) {
            uint8_t quadro[TELEMETRIA_TAMANHO_BYTES];
            const size_t n = telemetriaCodificar(p, quadro, sizeof(quadro));
            char linha[512];
            if (n > 0 && ponte.processar(quadro, n, -50, linha, sizeof(linha)) > 0) fputs(linha, arquivo);
        }
        return true;
    }
};

// ---------------------------------------------------------------------------
// Robô virtual montado
// ---------------------------------------------------------------------------

struct RoboVirtual {
    SimuladorLabirinto sim;  // só as paredes verdadeiras; quem se move é o MundoVirtual
    MundoVirtual mundo;
    Motores motores;
    Odometria odo;
    ExecutorMovimentoRobo exec;
    BotaoVirtual botao;
    BateriaVirtual bat;
    LedVirtual led;
    RadioVirtual radio;
    CicloProva ciclo;
    uint32_t t;

    explicit RoboVirtual(const char* pista,
                         const ParametrosMundo& pm = MundoVirtual::parametrosPadrao())
        : sim(SimuladorLabirinto::LINHAS_MAX),
          mundo(sim, pm),
          motores(mundo),
          exec(motores, odo, mundo),
          ciclo(DependenciasProva{botao, mundo, mundo, bat, led, motores, odo, exec, &radio, nullptr}),
          t(0) {
        TEST_ASSERT_TRUE(carregarPista(pista, sim));
        ciclo.iniciar(0);
    }

    // Grava as linhas JSON se ROBO_VIRTUAL_SAIDA estiver definida.
    void gravarEm(const char* nomeArquivo) {
        const char* pasta = getenv("ROBO_VIRTUAL_SAIDA");
        if (pasta == nullptr || pasta[0] == '\0') return;
        char caminho[512];
        snprintf(caminho, sizeof(caminho), "%s/%s", pasta, nomeArquivo);
        radio.arquivo = fopen(caminho, "w");
    }

    void avancar(uint32_t ms) {
        for (uint32_t i = 0; i < ms; i++) {
            mundo.passo1ms();
            ciclo.tick(++t);
        }
    }

    void segurarESoltar() {
        botao.apertado = true;
        avancar(2100);
        botao.apertado = false;
        avancar(60);
    }

    void tocar(uint8_t vezes) {
        for (uint8_t i = 0; i < vezes; i++) {
            botao.apertado = true;
            avancar(80);
            botao.apertado = false;
            avancar(120);
        }
        avancar(500);
    }

    void escolherGeometria(uint8_t linhas) {
        const GeometriaLabirinto alvo = linhas == 4 ? LAB_4X4 : linhas == 8 ? LAB_8X4 : LAB_12X4;
        for (int i = 0; i < 3 && ciclo.mapa().getGeometria() != alvo; i++) tocar(3);
        TEST_ASSERT_EQUAL(alvo, ciclo.mapa().getGeometria());
    }

    bool rodarAte(EstadoRobo e, uint32_t maxMs) {
        const uint32_t fim = t + maxMs;
        while (t < fim) {
            avancar(1);
            if (ciclo.estado() == e) return true;
        }
        return false;
    }

    // Roda até o executor estar avançando com as duas rodas acionadas para a frente.
    bool rodarAteAvancar(uint32_t maxMs) {
        const uint32_t fim = t + maxMs;
        while (t < fim) {
            avancar(1);
            if (ciclo.subfase() == SUBFASE_AVANCANDO && mundo.pwmEsq() > 0 && mundo.pwmDir() > 0) return true;
        }
        return false;
    }
};

// Células alcançáveis a partir de A1 no labirinto verdadeiro.
static uint8_t contarAlcancaveis(const SimuladorLabirinto& s, bool& chegadaAlcancavel) {
    bool visto[SimuladorLabirinto::LINHAS_MAX][4] = {};
    uint8_t fila[SimuladorLabirinto::LINHAS_MAX * 4][2];
    uint8_t ini = 0, fim = 0, total = 0;
    visto[0][0] = true;
    fila[fim][0] = 0;
    fila[fim][1] = 0;
    fim++;
    const Direcao dirs[4] = {DIR_NORTE, DIR_SUL, DIR_LESTE, DIR_OESTE};
    while (ini < fim) {
        const uint8_t l = fila[ini][0], c = fila[ini][1];
        ini++;
        total++;
        for (int i = 0; i < 4; i++) {
            if (s.temParedeReal(l, c, dirs[i])) continue;
            const int nl = l + deltaLinha(dirs[i]), nc = c + deltaColuna(dirs[i]);
            if (nl < 0 || nl >= s.linhas || nc < 0 || nc >= 4 || visto[nl][nc]) continue;
            visto[nl][nc] = true;
            fila[fim][0] = (uint8_t)nl;
            fila[fim][1] = (uint8_t)nc;
            fim++;
        }
    }
    chegadaAlcancavel = visto[s.linhas - 1][3];
    return total;
}

// ---------------------------------------------------------------------------
// Pistas e mundo
// ---------------------------------------------------------------------------

void test_pistas_dos_slides_sao_lidas_como_no_backend(void) {
    const char* pistas[3] = {PISTA_SLIDES_4X4, PISTA_SLIDES_8X4, PISTA_SLIDES_12X4};
    const uint8_t linhas[3] = {4, 8, 12};
    const uint8_t fechadas[3] = {0, 7, 8};  // regiões sem acesso, como diz pistas.txt
    for (int i = 0; i < 3; i++) {
        SimuladorLabirinto s(SimuladorLabirinto::LINHAS_MAX);
        TEST_ASSERT_TRUE(carregarPista(pistas[i], s));
        TEST_ASSERT_EQUAL_UINT8(linhas[i], s.linhas);
        // Largada em A1 num beco com a única abertura para o leste.
        TEST_ASSERT_TRUE(s.temParedeReal(0, 0, DIR_NORTE));
        TEST_ASSERT_TRUE(s.temParedeReal(0, 0, DIR_SUL));
        TEST_ASSERT_TRUE(s.temParedeReal(0, 0, DIR_OESTE));
        TEST_ASSERT_FALSE(s.temParedeReal(0, 0, DIR_LESTE));
        bool chegada = false;
        const uint8_t alcancaveis = contarAlcancaveis(s, chegada);
        TEST_ASSERT_TRUE(chegada);
        TEST_ASSERT_EQUAL_UINT8(linhas[i] * 4 - fechadas[i], alcancaveis);
    }
    SimuladorLabirinto s(SimuladorLabirinto::LINHAS_MAX);
    TEST_ASSERT_FALSE(carregarPista("+---+\n|   |\n", s));  // quantidade de linhas inválida
}

void test_sensores_no_centro_da_celula_leem_o_que_o_desenho_supoe(void) {
    SimuladorLabirinto s(SimuladorLabirinto::LINHAS_MAX);
    carregarPista(PISTA_SLIDES_4X4, s);
    MundoVirtual m(s);
    m.atualizar(0);
    TEST_ASSERT_UINT16_WITHIN(3, 40, m.esquerdo().distanciaMm());  // parede norte de A1
    TEST_ASSERT_UINT16_WITHIN(3, 40, m.direito().distanciaMm());   // parede sul de A1
    // À frente, a linha 1 é aberta até a parede leste de D1: 714 - (90 + 39) = 585 mm.
    TEST_ASSERT_UINT16_WITHIN(3, 585, m.frontal().distanciaMm());
}

// ---------------------------------------------------------------------------
// Provas completas nas três pistas
// ---------------------------------------------------------------------------

// Mundo com o motor direito 'ganhoDir' em relação ao esquerdo.
static ParametrosMundo mundoComMotores(float ganhoDir) {
    ParametrosMundo pm = MundoVirtual::parametrosPadrao();
    pm.ganhoDir = ganhoDir;
    return pm;
}

static void provaNaPista(const char* pista, uint8_t linhas, const char* nome, const char* arquivo,
                         float ganhoDir, const char* motores) {
    RoboVirtual r(pista, mundoComMotores(ganhoDir));
    if (arquivo != nullptr) r.gravarEm(arquivo);
    r.avancar(1500);  // painel vê AGUARDANDO antes da largada
    r.escolherGeometria(linhas);
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(MAPEANDO, r.ciclo.estado());

    // Roda até concluir, medindo, com o robô parado lendo as paredes de uma célula, o maior
    // desvio do centro, o maior erro de rumo real e o maior erro da estimativa de rumo do
    // executor (rumo estimado - rumo real).
    float maiorDesvio = 0.0f, maiorRumo = 0.0f, maiorErroEstimativa = 0.0f;
    const uint32_t limite = r.t + 600000;
    while (r.t < limite && r.ciclo.estado() == MAPEANDO) {
        r.avancar(1);
        if (r.ciclo.subfase() == SUBFASE_LENDO_PAREDES && r.mundo.parado()) {
            if (r.mundo.desvioDoCentroMm() > maiorDesvio) maiorDesvio = r.mundo.desvioDoCentroMm();
            if (fabsf(r.mundo.desalinhamentoGraus()) > maiorRumo) maiorRumo = fabsf(r.mundo.desalinhamentoGraus());
            const float erroEstimativa = fabsf(r.exec.erroRumoEstimadoGraus() - r.mundo.desalinhamentoGraus());
            if (erroEstimativa > maiorErroEstimativa) maiorErroEstimativa = erroEstimativa;
        }
    }
    printf("[ROBO VIRTUAL] %-5s motores=%-8s fim=%-9s tempo=%5.1f s desloc=%3u retornos=%2u giros=%3u "
           "distancia=%5.0f mm maior_desvio=%4.1f mm maior_rumo=%3.1f graus erro_estimativa=%3.1f graus encostos=%u\n",
           nome, motores, telemetriaNomeStatus((uint8_t)r.ciclo.estado()),
           r.ciclo.cronometro().decorridoMs(r.t) / 1000.0f, r.ciclo.navegador().deslocamentos(),
           r.ciclo.navegador().retornos(), r.ciclo.navegador().giros(),
           r.mundo.distanciaPercorridaMm(), maiorDesvio, maiorRumo, maiorErroEstimativa, r.mundo.encostos());
    TEST_ASSERT_EQUAL_MESSAGE(CONCLUIDO, r.ciclo.estado(), nome);

    // Lógica e física concordam: o robô está de fato na chegada, centrado e alinhado.
    TEST_ASSERT_EQUAL_UINT8(linhas - 1, r.ciclo.navegador().linha());
    TEST_ASSERT_EQUAL_UINT8(3, r.ciclo.navegador().coluna());
    TEST_ASSERT_EQUAL_UINT8(linhas - 1, r.mundo.linha());
    TEST_ASSERT_EQUAL_UINT8(3, r.mundo.coluna());
    TEST_ASSERT_TRUE(r.mundo.desvioDoCentroMm() < 25.0f);
    TEST_ASSERT_TRUE(fabsf(r.mundo.desalinhamentoGraus()) < 5.0f);
    TEST_ASSERT_TRUE(maiorDesvio < 25.0f);          // em nenhuma célula longe do centro
    TEST_ASSERT_TRUE(maiorRumo < 8.0f);             // nem muito desalinhado
    TEST_ASSERT_TRUE(maiorErroEstimativa < 2.5f);   // e o executor sabe quanto está
    TEST_ASSERT_EQUAL_UINT32(0, r.mundo.encostos());  // nunca encostou numa parede
    TEST_ASSERT_EQUAL_UINT16(0, r.ciclo.navegador().falhas());

    // Parada: tração travada, rodas paradas e cronômetro congelado.
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_TRUE(r.ciclo.cronometro().congelado());
    r.avancar(400);
    TEST_ASSERT_TRUE(r.mundo.parado());

    // O mapa conhece as paredes verdadeiras de toda célula visitada (menos a chegada,
    // que não é observada).
    for (uint8_t l = 0; l < linhas; l++) {
        for (uint8_t c = 0; c < 4; c++) {
            if (!r.ciclo.mapa().foiVisitada(l, c) || (l == linhas - 1 && c == 3)) continue;
            TEST_ASSERT_EQUAL_UINT8(r.sim.verdadeiro[l][c], r.ciclo.mapa().getParedes(l, c));
        }
    }

    // Telemetria a 1 Hz, sempre válida, com o estado final e o tempo congelado.
    r.avancar(2000);
    const TelemetriaPacket& p = r.radio.ultimo;
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
    TEST_ASSERT_EQUAL_UINT8(CONCLUIDO, p.status_id);
    TEST_ASSERT_EQUAL_UINT8(1, p.desafio_cumprido);
    TEST_ASSERT_EQUAL_UINT8(linhas - 1, p.linha);
    TEST_ASSERT_EQUAL_UINT8(3, p.coluna);
    TEST_ASSERT_EQUAL_UINT32(r.ciclo.cronometro().decorridoMs(r.t), p.timestamp_ms);
    TEST_ASSERT_TRUE(p.velocidade_media > 0);
    TEST_ASSERT_INT_WITHIN(2, (int)(r.t / 1000), r.radio.enviados);

}

void test_pista_4x4_dos_slides_conclui_em_d4(void) {
    provaNaPista(PISTA_SLIDES_4X4, 4, "4x4", "robo_virtual_4x4.jsonl", 1.02f, "dir +2%");
}

void test_pista_8x4_dos_slides_conclui_em_d8(void) {
    provaNaPista(PISTA_SLIDES_8X4, 8, "8x4", "robo_virtual_8x4.jsonl", 1.02f, "dir +2%");
}

void test_pista_12x4_dos_slides_conclui_em_d12(void) {
    provaNaPista(PISTA_SLIDES_12X4, 12, "12x4", "robo_virtual_12x4.jsonl", 1.02f, "dir +2%");
}

void test_tres_pistas_com_o_motor_direito_2_porcento_mais_lento(void) {
    provaNaPista(PISTA_SLIDES_4X4, 4, "4x4", nullptr, 0.98f, "dir -2%");
    provaNaPista(PISTA_SLIDES_8X4, 8, "8x4", nullptr, 0.98f, "dir -2%");
    provaNaPista(PISTA_SLIDES_12X4, 12, "12x4", nullptr, 0.98f, "dir -2%");
}

void test_tres_pistas_com_motores_iguais(void) {
    provaNaPista(PISTA_SLIDES_4X4, 4, "4x4", nullptr, 1.0f, "iguais");
    provaNaPista(PISTA_SLIDES_8X4, 8, "8x4", nullptr, 1.0f, "iguais");
    provaNaPista(PISTA_SLIDES_12X4, 12, "12x4", nullptr, 1.0f, "iguais");
}

// ---------------------------------------------------------------------------
// Falhas
// ---------------------------------------------------------------------------

void test_rodas_presas_cortam_a_tracao_em_ate_1200_ms(void) {
    // As duas rodas presas no meio de um avanço (robô preso contra um obstáculo baixo).
    RoboVirtual r(PISTA_SLIDES_4X4);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAteAvancar(5000));
    r.avancar(300);
    const uint32_t presa = r.t;
    r.mundo.travaEsq = true;
    r.mundo.travaDir = true;
    TEST_ASSERT_TRUE(r.rodarAte(TRAVADO, 3000));
    const uint32_t corte = r.t - presa;
    TEST_ASSERT_TRUE(corte >= 900);  // o último pulso foi pouco antes de prender
    TEST_ASSERT_TRUE(corte <= LIMITE_CORTE_TRAVAMENTO_MS);
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_EQUAL(AMBAS_RODAS, r.ciclo.monitor().rodaTravada());
    r.avancar(1000);
    TEST_ASSERT_EQUAL_UINT8(TRAVADO, r.radio.ultimo.status_id);
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
}

void test_roda_esquerda_presa_nao_conta_a_celula_e_trava(void) {
    // Com uma roda presa, o robô gira sobre ela. O avanço não conclui pela outra roda (a
    // célula não é contada); o sensor frontal encontra a parede lateral e o avanço termina
    // por emergência; no movimento seguinte, a roda presa fica 1 s sem pulso com tração e
    // o monitor corta. A tração nunca fica mais de 1,2 s ligada sem pulso.
    RoboVirtual r(PISTA_SLIDES_4X4);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAteAvancar(5000));
    const uint16_t celulasAntes = r.ciclo.navegador().deslocamentos();
    const uint8_t colunaAntes = r.ciclo.navegador().coluna();
    r.avancar(300);
    r.mundo.travaEsq = true;
    TEST_ASSERT_TRUE(r.rodarAte(TRAVADO, 5000));
    TEST_ASSERT_EQUAL_UINT16(celulasAntes, r.ciclo.navegador().deslocamentos());
    TEST_ASSERT_EQUAL_UINT8(colunaAntes, r.ciclo.navegador().coluna());
    TEST_ASSERT_EQUAL(RODA_ESQUERDA, r.ciclo.monitor().rodaTravada());
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_EQUAL_UINT32(0, r.mundo.encostos());
}

void test_tempo_limite_com_relogio_acelerado(void) {
    RoboVirtual r(PISTA_SLIDES_8X4);
    r.escolherGeometria(8);
    r.segurarESoltar();
    // Depois de alguns movimentos, com o robô parado lendo as paredes, o relógio salta
    // 10 min (o robô virtual não leva 10 min em nenhuma das pistas).
    r.avancar(5000);
    const uint32_t fim = r.t + 5000;
    while (r.t < fim && r.ciclo.subfase() != SUBFASE_LENDO_PAREDES) r.avancar(1);
    TEST_ASSERT_EQUAL(SUBFASE_LENDO_PAREDES, r.ciclo.subfase());
    r.t += TELEMETRIA_TIMESTAMP_MAX_MS;
    TEST_ASSERT_TRUE(r.rodarAte(TIMEOUT, 100));
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    r.avancar(1500);
    TEST_ASSERT_EQUAL_UINT8(TIMEOUT, r.radio.ultimo.status_id);
    TEST_ASSERT_EQUAL_UINT32(TELEMETRIA_TIMESTAMP_MAX_MS, r.radio.ultimo.timestamp_ms);
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
}

void test_obstaculo_que_surge_a_frente_vira_colisao_sem_encostar(void) {
    RoboVirtual r(PISTA_SLIDES_4X4);
    r.mundo.maoNaFrente = true;
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(COLISAO, 30000));
    TEST_ASSERT_EQUAL(FALHA_EMERGENCIA_FRONTAL, r.ciclo.causaUltimaFalha());
    TEST_ASSERT_EQUAL_UINT32(0, r.mundo.encostos());  // freou antes de tocar
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_EQUAL_UINT8(0, r.mundo.coluna());     // não saiu de A1
}

void test_largada_recusada_com_bateria_baixa(void) {
    RoboVirtual r(PISTA_SLIDES_4X4);
    r.bat.mv = BATERIA_MINIMA_LARGADA_MV - 100;
    r.segurarESoltar();
    r.avancar(2000);
    TEST_ASSERT_EQUAL(AGUARDANDO, r.ciclo.estado());
    TEST_ASSERT_EQUAL(1, r.led.avisos);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, r.mundo.distanciaPercorridaMm());  // não se mexeu
}

// ---------------------------------------------------------------------------
// Nova tentativa (R9)
// ---------------------------------------------------------------------------

void test_nova_tentativa_depois_de_concluir(void) {
    RoboVirtual r(PISTA_SLIDES_4X4);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 300000));
    const uint32_t primeira = r.ciclo.cronometro().decorridoMs(r.t);
    r.avancar(1000);

    r.mundo.posicionarNaLargada();  // a equipe leva o robô de volta à largada
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(AGUARDANDO, r.ciclo.estado());
    r.avancar(1000);
    TEST_ASSERT_EQUAL_UINT8(AGUARDANDO, r.radio.ultimo.status_id);
    TEST_ASSERT_EQUAL_UINT32(0, r.radio.ultimo.timestamp_ms);

    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 300000));
    // Mapa limpo na nova tentativa: a exploração se repete com o mesmo tempo.
    TEST_ASSERT_UINT32_WITHIN(1000, primeira, r.ciclo.cronometro().decorridoMs(r.t));
    TEST_ASSERT_EQUAL_UINT32(0, r.mundo.encostos());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pistas_dos_slides_sao_lidas_como_no_backend);
    RUN_TEST(test_sensores_no_centro_da_celula_leem_o_que_o_desenho_supoe);
    RUN_TEST(test_pista_4x4_dos_slides_conclui_em_d4);
    RUN_TEST(test_pista_8x4_dos_slides_conclui_em_d8);
    RUN_TEST(test_pista_12x4_dos_slides_conclui_em_d12);
    RUN_TEST(test_tres_pistas_com_o_motor_direito_2_porcento_mais_lento);
    RUN_TEST(test_tres_pistas_com_motores_iguais);
    RUN_TEST(test_rodas_presas_cortam_a_tracao_em_ate_1200_ms);
    RUN_TEST(test_roda_esquerda_presa_nao_conta_a_celula_e_trava);
    RUN_TEST(test_tempo_limite_com_relogio_acelerado);
    RUN_TEST(test_obstaculo_que_surge_a_frente_vira_colisao_sem_encostar);
    RUN_TEST(test_largada_recusada_com_bateria_baixa);
    RUN_TEST(test_nova_tentativa_depois_de_concluir);
    return UNITY_END();
}
