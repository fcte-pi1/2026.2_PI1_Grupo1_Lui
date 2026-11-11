#include <stdint.h>
#include <unity.h>
#include "TelemetriaCodec.h"
#include "../../src/app/CicloProva.h"
#include "../comum/ExecutorTemporizado.h"
#include "../comum/SimuladorLabirinto.h"

// Testes do CicloProva (docs/firmware/integracao-loop.md, seções 3.3 a 3.8): o laço inteiro
// roda no ambiente nativo, tick a tick com relógio falso de 1 ms, sobre dublês do hardware.
// A navegação usa o SimuladorLabirinto, com ToF falsos que leem as paredes verdadeiras.

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Dublês do hardware
// ---------------------------------------------------------------------------

struct PonteFalsa : IPonteH {
    int freios, solturas;
    PonteFalsa() : freios(0), solturas(0) {}
    void aplicar(int16_t, int16_t) override {}
    void frear() override { freios++; }
    void soltar() override { solturas++; }
};

struct BotaoFalso : IEntradaBotao {
    bool apertado;
    BotaoFalso() : apertado(false) {}
    bool pressionado() override { return apertado; }
};

struct EncodersFalsos : IEncoders {
    int32_t esq, dir;
    bool pausado;
    int pausas, retomadas;
    EncodersFalsos() : esq(0), dir(0), pausado(false), pausas(0), retomadas(0) {}
    void ler(int32_t& e, int32_t& d) override {
        e = esq;
        d = dir;
    }
    void pausarEsquerdo() override {
        pausado = true;
        pausas++;
    }
    void retomarEsquerdo() override {
        pausado = false;
        retomadas++;
    }
};

// ToF que lê as paredes verdadeiras da célula onde o robô simulado está, a cada 33 ms.
struct ToFDoSimulador : ISensoresToF {
    SimuladorLabirinto& sim;
    LeituraToF esq, frente, dir;
    bool mudo;
    bool primeira;
    uint32_t ultimaMs;
    explicit ToFDoSimulador(SimuladorLabirinto& s) : sim(s), mudo(false), primeira(true), ultimaMs(0) {}

    bool parede(LadoRobo lado) const {
        return sim.temParedeReal(sim.linhaReal, sim.colunaReal, direcaoAbsoluta(sim.orientacaoReal, lado));
    }

    void atualizar(uint32_t agoraMs) override {
        if (mudo) return;
        if (!primeira && (uint32_t)(agoraMs - ultimaMs) < 33) return;
        primeira = false;
        ultimaMs = agoraMs;
        esq.registrar(parede(LADO_ESQUERDA) ? 40 : 300, agoraMs);
        dir.registrar(parede(LADO_DIREITA) ? 40 : 300, agoraMs);
        frente.registrar(parede(LADO_FRENTE) ? 45 : TOF_LEITURA_INVALIDA_MM, agoraMs);
    }
    LeituraToF& esquerdo() override { return esq; }
    LeituraToF& frontal() override { return frente; }
    LeituraToF& direito() override { return dir; }
};

struct BateriaFalsa : IBateria {
    EncodersFalsos& enc;
    uint16_t mv;
    int leituras;
    bool sempreComEncoderPausado;
    explicit BateriaFalsa(EncodersFalsos& e) : enc(e), mv(7800), leituras(0), sempreComEncoderPausado(true) {}
    uint16_t lerMilivolts() override {
        leituras++;
        if (!enc.pausado) sempreComEncoderPausado = false;
        return mv;
    }
};

struct LedFalso : ILedEstado {
    EstadoRobo estado;
    int avisos;
    CorLed corAviso;
    uint8_t vezesAviso;
    LedFalso() : estado(AGUARDANDO), avisos(0), corAviso(corLed(0, 0, 0)), vezesAviso(0) {}
    void mostrarEstado(EstadoRobo e) override { estado = e; }
    void piscar(CorLed cor, uint8_t vezes, uint32_t) override {
        avisos++;
        corAviso = cor;
        vezesAviso = vezes;
    }
    void atualizar(uint32_t, bool) override {}
};

struct RadioFalso : IRadioTelemetria {
    int enviados;
    int invalidos;
    TelemetriaPacket ultimo;
    RadioFalso() : enviados(0), invalidos(0) { memset(&ultimo, 0, sizeof(ultimo)); }
    bool enviar(const TelemetriaPacket& p) override {
        if (telemetriaValidar(p) != TELEMETRIA_OK) invalidos++;
        ultimo = p;
        enviados++;
        return true;
    }
};

// Executor que comanda os motores e nunca termina (rodas presas): só sai pelo trinco.
// Com comandarMotores = false, nunca termina e não aciona nada (para o tempo limite).
struct ExecutorEmperrado : ExecutorMovimentoNaoBloqueante {
    Motores& motores;
    bool comandarMotores;
    bool ocupado;
    uint32_t inicioComandoMs;
    explicit ExecutorEmperrado(Motores& m)
        : motores(m), comandarMotores(true), ocupado(false), inicioComandoMs(0) {}
    bool iniciar(uint32_t agoraMs) {
        if (ocupado) return false;
        ocupado = true;
        if (comandarMotores && motores.comandar(90, 90)) inicioComandoMs = agoraMs;
        return true;
    }
    bool iniciarGiro(Giro g, Direcao, uint32_t agoraMs) override {
        return g != GIRO_NENHUM && iniciar(agoraMs);
    }
    bool iniciarAvanco(uint32_t agoraMs) override { return iniciar(agoraMs); }
    ResultadoMovimento atualizar(uint32_t) override {
        if (!ocupado) return MOV_OCIOSO;
        if (motores.bloqueado()) {
            ocupado = false;
            return MOV_FALHOU;
        }
        return MOV_EM_ANDAMENTO;
    }
    void abortar() override {
        ocupado = false;
        motores.frear();
    }
    bool ocioso() const override { return !ocupado; }
    MotivoFalhaMovimento motivoUltimaFalha() const override { return FALHA_TRACAO_CORTADA; }
};

// Simulador em que todo avanço esbarra num obstáculo à frente.
struct SimuladorObstaculo : ExecutorMovimento, InformanteMotivoFalha {
    SimuladorLabirinto& sim;
    explicit SimuladorObstaculo(SimuladorLabirinto& s) : sim(s) {}
    bool girar(Giro g) override { return sim.girar(g); }
    bool avancarCelula() override { return false; }
    MotivoFalhaMovimento motivoFalha() const override { return FALHA_EMERGENCIA_FRONTAL; }
};

enum TipoExecutor { EXEC_SIMULADOR, EXEC_EMPERRADO, EXEC_PARADO, EXEC_OBSTACULO };

// ---------------------------------------------------------------------------
// Robô montado com os dublês
// ---------------------------------------------------------------------------

struct Robo {
    SimuladorLabirinto sim;
    PonteFalsa ponte;
    Motores motores;
    Odometria odo;
    EncodersFalsos enc;
    ToFDoSimulador tof;
    BateriaFalsa bat;
    LedFalso led;
    RadioFalso radio;
    BotaoFalso botao;
    ExecutorTemporizado execSimulador;
    ExecutorEmperrado execEmperrado;
    SimuladorObstaculo obstaculo;
    ExecutorImediato execObstaculo;
    TipoExecutor tipo;
    CicloProva ciclo;
    uint32_t t;

    Robo(uint8_t linhas, TipoExecutor tipoExec = EXEC_SIMULADOR, bool comRadio = true)
        : sim(linhas), motores(ponte, false, false), tof(sim), bat(enc), execSimulador(sim, 5),
          execEmperrado(motores), obstaculo(sim), execObstaculo(obstaculo, &obstaculo),
          tipo(tipoExec),
          ciclo(DependenciasProva{botao, enc, tof, bat, led, motores, odo, executor(),
                                  comRadio ? &radio : nullptr, nullptr}),
          t(0) {
        execEmperrado.comandarMotores = (tipoExec == EXEC_EMPERRADO);
        ciclo.iniciar(0);
    }

    ExecutorMovimentoNaoBloqueante& executor() {
        switch (tipo) {
            case EXEC_EMPERRADO:
            case EXEC_PARADO:    return execEmperrado;
            case EXEC_OBSTACULO: return execObstaculo;
            case EXEC_SIMULADOR: break;
        }
        return execSimulador;
    }

    void avancar(uint32_t ms) {
        for (uint32_t i = 0; i < ms; i++) ciclo.tick(++t);
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
        avancar(500);  // fecha a janela de toques
    }

    // Toques triplos até a geometria das 'linhas' pedidas.
    void escolherGeometria(uint8_t linhas) {
        const GeometriaLabirinto alvo = linhas == 4 ? LAB_4X4 : linhas == 8 ? LAB_8X4 : LAB_12X4;
        for (int i = 0; i < 3 && ciclo.mapa().getGeometria() != alvo; i++) tocar(3);
    }

    bool rodarAte(EstadoRobo e, uint32_t maxMs) {
        const uint32_t fim = t + maxMs;
        while (t < fim) {
            avancar(1);
            if (ciclo.estado() == e) return true;
        }
        return false;
    }
};

static void labirintoBecos4(SimuladorLabirinto& s) {
    s.abrirCaminho(0, 0, "LLLSSS");
    s.abrirCaminho(0, 1, "SSS");
    s.abrirCaminho(0, 2, "S");
}

// ---------------------------------------------------------------------------
// AGUARDANDO, telemetria e botão
// ---------------------------------------------------------------------------

void test_inicia_em_aguardando_e_publica_a1_leste(void) {
    Robo r(4);
    r.avancar(1);
    TEST_ASSERT_EQUAL(AGUARDANDO, r.ciclo.estado());
    TEST_ASSERT_EQUAL(1, r.radio.enviados);  // o primeiro pacote sai no primeiro tick
    const TelemetriaPacket& p = r.radio.ultimo;
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaValidar(p));
    TEST_ASSERT_EQUAL_UINT8(AGUARDANDO, p.status_id);
    TEST_ASSERT_EQUAL_UINT32(0, p.timestamp_ms);
    TEST_ASSERT_EQUAL_UINT8(LAB_4X4, p.labirinto_id);
    TEST_ASSERT_EQUAL_UINT8(0, p.linha);
    TEST_ASSERT_EQUAL_UINT8(0, p.coluna);
    TEST_ASSERT_EQUAL_UINT8(DIR_LESTE, p.orientacao);
    TEST_ASSERT_EQUAL_UINT8(0, p.desafio_cumprido);
    TEST_ASSERT_EQUAL_UINT16(7800, p.bateria_mv);
    TEST_ASSERT_EQUAL_UINT8(porcentagemBateria(7800), p.bateria_pct);
    TEST_ASSERT_EQUAL(AGUARDANDO, r.led.estado);
}

void test_telemetria_a_1_hz(void) {
    Robo r(4);
    r.avancar(5000);
    TEST_ASSERT_EQUAL(5, r.radio.enviados);
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
}

void test_toque_triplo_troca_a_geometria_com_piscadas(void) {
    Robo r(4);
    r.tocar(3);
    TEST_ASSERT_EQUAL(LAB_8X4, r.ciclo.mapa().getGeometria());
    TEST_ASSERT_EQUAL_UINT8(2, r.led.vezesAviso);
    TEST_ASSERT_EQUAL_UINT8(255, r.led.corAviso.b);  // ciano
    r.tocar(3);
    TEST_ASSERT_EQUAL(LAB_12X4, r.ciclo.mapa().getGeometria());
    TEST_ASSERT_EQUAL_UINT8(3, r.led.vezesAviso);
    r.tocar(3);
    TEST_ASSERT_EQUAL(LAB_4X4, r.ciclo.mapa().getGeometria());
    TEST_ASSERT_EQUAL_UINT8(1, r.led.vezesAviso);
    r.avancar(1000);
    TEST_ASSERT_EQUAL_UINT8(LAB_4X4, r.radio.ultimo.labirinto_id);
    TEST_ASSERT_EQUAL(AGUARDANDO, r.ciclo.estado());
}

void test_largada_recusada_com_bateria_baixa(void) {
    Robo r(4);
    r.sim.abrirTudo();  // fechado, o destino seria inalcançável logo depois da largada
    r.bat.mv = BATERIA_MINIMA_LARGADA_MV - 1;
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(AGUARDANDO, r.ciclo.estado());
    TEST_ASSERT_EQUAL(1, r.led.avisos);
    TEST_ASSERT_EQUAL_UINT8(3, r.led.vezesAviso);
    TEST_ASSERT_EQUAL_UINT8(255, r.led.corAviso.r);  // vermelho
    TEST_ASSERT_EQUAL_UINT8(0, r.led.corAviso.g);
    // Com a bateria no mínimo, a largada é aceita.
    r.bat.mv = BATERIA_MINIMA_LARGADA_MV;
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(MAPEANDO, r.ciclo.estado());
}

void test_bateria_e_lida_com_o_encoder_esquerdo_pausado(void) {
    // Pendência 9 do desenho: GPIO 36 e 39 com o ADC ligado.
    Robo r(4);
    r.avancar(3000);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.bat.leituras >= 4);  // telemetria e largada
    TEST_ASSERT_TRUE(r.bat.sempreComEncoderPausado);
    TEST_ASSERT_EQUAL(r.enc.pausas, r.enc.retomadas);
    TEST_ASSERT_EQUAL(r.bat.leituras, r.enc.pausas);
    TEST_ASSERT_FALSE(r.enc.pausado);
}

void test_velocidade_atual_vem_da_odometria(void) {
    Robo r(4);
    const float pulsosPorMs = 150.0f / 1000.0f / RODA_MM_POR_PULSO;  // 150 mm/s
    for (int i = 0; i < 2000; i++) {
        r.enc.esq = r.enc.dir = (int32_t)(r.t * pulsosPorMs);
        r.avancar(1);
    }
    TEST_ASSERT_INT16_WITHIN(10, 150, r.radio.ultimo.velocidade_atual);
}

// ---------------------------------------------------------------------------
// Prova completa
// ---------------------------------------------------------------------------

void test_navega_os_becos_ate_d4_e_conclui(void) {
    Robo r(4);
    labirintoBecos4(r.sim);
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(MAPEANDO, r.ciclo.estado());
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 120000));
    TEST_ASSERT_EQUAL_UINT8(3, r.ciclo.navegador().linha());
    TEST_ASSERT_EQUAL_UINT8(3, r.ciclo.navegador().coluna());
    TEST_ASSERT_EQUAL_UINT8(3, r.sim.linhaReal);  // a posição lógica não divergiu da real
    TEST_ASSERT_EQUAL_UINT8(3, r.sim.colunaReal);
    TEST_ASSERT_EQUAL_UINT16(0, r.sim.movimentosInvalidos);
    TEST_ASSERT_TRUE(r.motores.bloqueado());       // ParadaObjetivo travou a tração
    TEST_ASSERT_TRUE(r.ciclo.cronometro().congelado());
    TEST_ASSERT_TRUE(r.ciclo.parada().concluido());
    TEST_ASSERT_EQUAL(CONCLUIDO, r.led.estado);

    // A telemetria continua a 1 Hz com o estado terminal e o tempo congelado.
    r.avancar(1000);
    const TelemetriaPacket p1 = r.radio.ultimo;
    r.avancar(2000);
    const TelemetriaPacket p2 = r.radio.ultimo;
    TEST_ASSERT_EQUAL_UINT8(CONCLUIDO, p2.status_id);
    TEST_ASSERT_EQUAL_UINT8(1, p2.desafio_cumprido);
    TEST_ASSERT_EQUAL_UINT32(p1.timestamp_ms, p2.timestamp_ms);
    TEST_ASSERT_TRUE(p2.timestamp_ms > 0);
    TEST_ASSERT_EQUAL_UINT8(3, p2.linha);
    TEST_ASSERT_EQUAL_UINT8(3, p2.coluna);
    TEST_ASSERT_TRUE(p2.velocidade_media > 0);
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
}

static void concluirNaGeometria(uint8_t linhas) {
    Robo r(linhas);
    r.sim.abrirTudo();
    r.escolherGeometria(linhas);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 300000));
    TEST_ASSERT_EQUAL_UINT8(linhas - 1, r.ciclo.navegador().linha());
    TEST_ASSERT_EQUAL_UINT8(3, r.ciclo.navegador().coluna());
    r.avancar(1000);
    TEST_ASSERT_EQUAL_UINT8(linhas == 4 ? LAB_4X4 : linhas == 8 ? LAB_8X4 : LAB_12X4,
                            r.radio.ultimo.labirinto_id);
    TEST_ASSERT_EQUAL_UINT8(linhas - 1, r.radio.ultimo.linha);
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
}

void test_conclui_nas_tres_geometrias(void) {
    concluirNaGeometria(4);
    concluirNaGeometria(8);
    concluirNaGeometria(12);
}

void test_sem_radio_a_prova_roda(void) {
    Robo r(4, EXEC_SIMULADOR, false);
    labirintoBecos4(r.sim);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 120000));
    TEST_ASSERT_EQUAL(0, r.radio.enviados);
    TEST_ASSERT_EQUAL_UINT32(0, r.ciclo.pacotesEnviados());
}

void test_gestos_ignorados_durante_a_prova(void) {
    Robo r(4, EXEC_PARADO);
    r.sim.abrirTudo();
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(MAPEANDO, r.ciclo.estado());
    r.tocar(3);
    TEST_ASSERT_EQUAL(LAB_4X4, r.ciclo.mapa().getGeometria());
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(MAPEANDO, r.ciclo.estado());  // RNF-SFT01: sem intervenção
}

// ---------------------------------------------------------------------------
// Falhas (seção 3.7)
// ---------------------------------------------------------------------------

void test_travamento_corta_em_ate_1200_ms(void) {
    Robo r(4, EXEC_EMPERRADO);
    r.sim.abrirTudo();
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(TRAVADO, 5000));
    const uint32_t corte = r.t - r.execEmperrado.inicioComandoMs;
    TEST_ASSERT_TRUE(corte >= TEMPO_TRAVAMENTO_MS);
    TEST_ASSERT_TRUE(corte <= LIMITE_CORTE_TRAVAMENTO_MS);
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_FALSE(r.motores.tracaoAtiva());
    TEST_ASSERT_TRUE(r.ciclo.cronometro().congelado());
    TEST_ASSERT_EQUAL(TRAVADO, r.led.estado);
    r.avancar(1000);
    TEST_ASSERT_EQUAL_UINT8(TRAVADO, r.radio.ultimo.status_id);
    TEST_ASSERT_EQUAL_UINT8(0, r.radio.ultimo.desafio_cumprido);
}

void test_tempo_limite_de_10_minutos(void) {
    Robo r(4, EXEC_PARADO);
    r.sim.abrirTudo();
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(TIMEOUT, TELEMETRIA_TIMESTAMP_MAX_MS + 1000));
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    r.avancar(1000);
    TEST_ASSERT_EQUAL_UINT8(TIMEOUT, r.radio.ultimo.status_id);
    TEST_ASSERT_EQUAL_UINT32(TELEMETRIA_TIMESTAMP_MAX_MS, r.radio.ultimo.timestamp_ms);
    TEST_ASSERT_EQUAL(0, r.radio.invalidos);
}

void test_tres_emergencias_frontais_viram_colisao(void) {
    Robo r(4, EXEC_OBSTACULO);
    r.sim.abrirTudo();
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(COLISAO, 10000));
    TEST_ASSERT_EQUAL(FALHA_EMERGENCIA_FRONTAL, r.ciclo.causaUltimaFalha());
    TEST_ASSERT_EQUAL(NAV_FALHA, r.ciclo.navegador().estado());
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_TRUE(r.ciclo.cronometro().congelado());
}

void test_destino_inalcancavel_vira_travado(void) {
    Robo r(4);  // o simulador começa com todas as paredes: A1 fechada
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(TRAVADO, 2000));
    TEST_ASSERT_EQUAL(NAV_INALCANCAVEL, r.ciclo.navegador().estado());
    TEST_ASSERT_TRUE(r.motores.bloqueado());
    TEST_ASSERT_TRUE(r.ciclo.cronometro().congelado());
}

void test_sensor_sem_resposta_vira_travado(void) {
    Robo r(4);
    r.sim.abrirTudo();
    r.tof.mudo = true;
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(TRAVADO, 2000));
    TEST_ASSERT_EQUAL(FALHA_NENHUMA, r.ciclo.causaUltimaFalha());  // falha de leitura
    TEST_ASSERT_EQUAL_UINT16(3, r.ciclo.navegador().falhas());
    TEST_ASSERT_TRUE(r.motores.bloqueado());
}

// ---------------------------------------------------------------------------
// Estados terminais e nova tentativa (R9)
// ---------------------------------------------------------------------------

void test_terminal_freia_e_solta_as_rodas_depois_de_300_ms(void) {
    Robo r(4);
    labirintoBecos4(r.sim);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 120000));
    const int solturas = r.ponte.solturas;
    r.avancar(CicloProva::FREIO_TERMINAL_MS - 1);
    TEST_ASSERT_EQUAL(solturas, r.ponte.solturas);
    r.avancar(1);
    TEST_ASSERT_EQUAL(solturas + 1, r.ponte.solturas);
    TEST_ASSERT_TRUE(r.motores.bloqueado());  // soltar não abre o trinco
    r.avancar(1000);
    TEST_ASSERT_EQUAL(solturas + 1, r.ponte.solturas);  // uma vez só
}

void test_reinicio_depois_de_concluir_permite_nova_prova(void) {
    Robo r(4);
    labirintoBecos4(r.sim);
    r.segurarESoltar();
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 120000));

    // Robô reposicionado na largada; segurar 2 s e soltar volta a AGUARDANDO.
    r.sim.linhaReal = 0;
    r.sim.colunaReal = 0;
    r.sim.orientacaoReal = DIR_LESTE;
    r.segurarESoltar();
    TEST_ASSERT_EQUAL(AGUARDANDO, r.ciclo.estado());
    TEST_ASSERT_FALSE(r.motores.bloqueado());
    TEST_ASSERT_FALSE(r.ciclo.mapa().foiVisitada(3, 3));  // mapa limpo
    r.avancar(1000);
    const TelemetriaPacket& p = r.radio.ultimo;
    TEST_ASSERT_EQUAL_UINT8(AGUARDANDO, p.status_id);
    TEST_ASSERT_EQUAL_UINT32(0, p.timestamp_ms);  // o backend abre nova tentativa
    TEST_ASSERT_EQUAL_UINT8(0, p.desafio_cumprido);
    TEST_ASSERT_EQUAL_UINT8(0, p.linha);
    TEST_ASSERT_EQUAL_INT16(0, p.velocidade_media);

    r.segurarESoltar();
    TEST_ASSERT_EQUAL(MAPEANDO, r.ciclo.estado());
    TEST_ASSERT_TRUE(r.rodarAte(CONCLUIDO, 120000));
}

int main(void) {
    UNITY_BEGIN();
    // AGUARDANDO, telemetria e botão
    RUN_TEST(test_inicia_em_aguardando_e_publica_a1_leste);
    RUN_TEST(test_telemetria_a_1_hz);
    RUN_TEST(test_toque_triplo_troca_a_geometria_com_piscadas);
    RUN_TEST(test_largada_recusada_com_bateria_baixa);
    RUN_TEST(test_bateria_e_lida_com_o_encoder_esquerdo_pausado);
    RUN_TEST(test_velocidade_atual_vem_da_odometria);
    // Prova completa
    RUN_TEST(test_navega_os_becos_ate_d4_e_conclui);
    RUN_TEST(test_conclui_nas_tres_geometrias);
    RUN_TEST(test_sem_radio_a_prova_roda);
    RUN_TEST(test_gestos_ignorados_durante_a_prova);
    // Falhas
    RUN_TEST(test_travamento_corta_em_ate_1200_ms);
    RUN_TEST(test_tempo_limite_de_10_minutos);
    RUN_TEST(test_tres_emergencias_frontais_viram_colisao);
    RUN_TEST(test_destino_inalcancavel_vira_travado);
    RUN_TEST(test_sensor_sem_resposta_vira_travado);
    // Terminais e nova tentativa
    RUN_TEST(test_terminal_freia_e_solta_as_rodas_depois_de_300_ms);
    RUN_TEST(test_reinicio_depois_de_concluir_permite_nova_prova);
    return UNITY_END();
}
