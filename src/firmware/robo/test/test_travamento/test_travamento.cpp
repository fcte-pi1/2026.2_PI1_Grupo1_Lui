#include <unity.h>

#include "../../src/seguranca/DetectorTravamento.h"
#include "../../src/seguranca/MonitorTravamento.h"
#include "../../src/parada/AdaptadorCronometro.h"
#include "TelemetriaCodec.h"

// ---------------------------------------------------------------------------
// Dublês de teste
// ---------------------------------------------------------------------------

struct EventosTravamento {
    char ordem[16];
    uint8_t n;
    EventosTravamento() : n(0) { ordem[0] = '\0'; }
    void add(char c) {
        if (n < 15) {
            ordem[n++] = c;
            ordem[n] = '\0';
        }
    }
};

struct TracaoFalsaTravamento : IFonteTracao {
    EventosTravamento& ev;
    bool ativa;
    int chamadasParar;
    explicit TracaoFalsaTravamento(EventosTravamento& e, bool inicialAtiva = false)
        : ev(e), ativa(inicialAtiva), chamadasParar(0) {}

    bool tracaoAtiva() const override { return ativa; }
    void parar() override {
        ativa = false;
        chamadasParar++;
        ev.add('P');
    }
};

struct OdometriaFalsaTravamento : IFontePulsos {
    uint32_t ultimoPulsoMs;
    explicit OdometriaFalsaTravamento(uint32_t inicialMs = 0) : ultimoPulsoMs(inicialMs) {}
    uint32_t getUltimoPulsoMs() const override { return ultimoPulsoMs; }
};

struct CronometroFalsoTravamento : ICronometro {
    EventosTravamento& ev;
    int chamadasCongelar;
    explicit CronometroFalsoTravamento(EventosTravamento& e) : ev(e), chamadasCongelar(0) {}
    void congelar() override {
        chamadasCongelar++;
        ev.add('C');
    }
};

struct SinalizacaoFalsaTravamento : ISinalizacaoEstado {
    EventosTravamento& ev;
    EstadoRobo ultimoEstado;
    int chamadas;
    explicit SinalizacaoFalsaTravamento(EventosTravamento& e)
        : ev(e), ultimoEstado(AGUARDANDO), chamadas(0) {}
    void mostrarEstado(EstadoRobo estado) override {
        ultimoEstado = estado;
        chamadas++;
        ev.add('S');
    }
};

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Testes do DetectorTravamento (lógica pura — Guia do MVP p. 25)
// ---------------------------------------------------------------------------

void test_nao_dispara_com_pulsos_regulares(void) {
    DetectorTravamento detector;
    TEST_ASSERT_EQUAL_UINT32(TEMPO_TRAVAMENTO_MS, detector.limiarMs());

    // Robô avançando com pulsos a cada 100 ms durante 5 segundos
    uint32_t ultimoPulso = 0;
    for (uint32_t t = 0; t <= 5000; t += 50) {
        if (t % 100 == 0) ultimoPulso = t;
        TEST_ASSERT_FALSE(detector.verificar(true, ultimoPulso, t));
        TEST_ASSERT_FALSE(detector.travamentoDetectado());
    }
}

void test_dispara_aos_1000ms_sem_pulso_com_tracao_ativa(void) {
    DetectorTravamento detector;

    // Largada em t = 1000 ms com um pulso inicial em 1000 ms
    TEST_ASSERT_FALSE(detector.verificar(true, 1000, 1000));
    TEST_ASSERT_EQUAL_UINT32(0, detector.tempoSemPulsoMs());

    // Em 1999 ms (999 ms sem pulso): ainda não deve disparar
    TEST_ASSERT_FALSE(detector.verificar(true, 1000, 1999));
    TEST_ASSERT_EQUAL_UINT32(999, detector.tempoSemPulsoMs());

    // Em 2000 ms (exatos 1000 ms sem pulso): travamento confirmado
    TEST_ASSERT_TRUE(detector.verificar(true, 1000, 2000));
    TEST_ASSERT_TRUE(detector.travamentoDetectado());
    TEST_ASSERT_EQUAL_UINT32(1000, detector.tempoSemPulsoMs());
}

void test_nao_dispara_com_tracao_desligada(void) {
    DetectorTravamento detector;

    TEST_ASSERT_FALSE(detector.verificar(false, 0, 0));
    TEST_ASSERT_FALSE(detector.verificar(false, 0, 1000));
    TEST_ASSERT_FALSE(detector.verificar(false, 0, 10000));
    TEST_ASSERT_EQUAL_UINT32(0, detector.tempoSemPulsoMs());
    TEST_ASSERT_FALSE(detector.travamentoDetectado());
}

void test_nao_dispara_logo_apos_ligar_tracao_com_ultimo_pulso_antigo(void) {
    DetectorTravamento detector;

    // Último pulso foi em t = 200 ms, mas o robô ficou parado até t = 5000 ms
    TEST_ASSERT_FALSE(detector.verificar(false, 200, 4950));

    // Tração liga em t = 5000 ms: não pode disparar na largada!
    TEST_ASSERT_FALSE(detector.verificar(true, 200, 5000));
    TEST_ASSERT_EQUAL_UINT32(0, detector.tempoSemPulsoMs());

    // 950 ms após ligar a tração sem nenhum pulso novo: ainda dentro da janela
    TEST_ASSERT_FALSE(detector.verificar(true, 200, 5950));
    TEST_ASSERT_EQUAL_UINT32(950, detector.tempoSemPulsoMs());

    // 1000 ms após ligar a tração sem pulso: agora sim confirma o travamento
    TEST_ASSERT_TRUE(detector.verificar(true, 200, 6000));
}

void test_pulso_simultaneo_ou_adiantado_zera_tempo_sem_pulso(void) {
    DetectorTravamento detector;

    TEST_ASSERT_FALSE(detector.verificar(true, 1000, 1000));
    TEST_ASSERT_FALSE(detector.verificar(true, 1550, 1500));
    TEST_ASSERT_EQUAL_UINT32(0, detector.tempoSemPulsoMs());
}

void test_reiniciar_limpa_historico_do_detector(void) {
    DetectorTravamento detector;

    TEST_ASSERT_FALSE(detector.verificar(true, 0, 0));
    TEST_ASSERT_TRUE(detector.verificar(true, 0, 1000));
    TEST_ASSERT_TRUE(detector.travamentoDetectado());

    detector.reiniciar();
    TEST_ASSERT_FALSE(detector.travamentoDetectado());
    TEST_ASSERT_EQUAL_UINT32(0, detector.tempoSemPulsoMs());
}

// ---------------------------------------------------------------------------
// Testes do MonitorTravamento (orquestração, corte em <= 1,2 s e estado TRAVADO)
// ---------------------------------------------------------------------------

void test_monitor_corta_motores_congela_cronometro_e_sinaliza_travado_em_ordem(void) {
    EventosTravamento ev;
    TracaoFalsaTravamento tracao(ev, true);
    OdometriaFalsaTravamento odom(1000);
    CronometroFalsoTravamento cron(ev);
    SinalizacaoFalsaTravamento sin(ev);

    MonitorTravamento monitor(tracao, odom, &cron, &sin);
    TEST_ASSERT_EQUAL(MAPEANDO, monitor.estadoRobo());

    // Primeiro ciclo em t = 1000 ms
    TEST_ASSERT_FALSE(monitor.verificar(1000));
    TEST_ASSERT_FALSE(monitor.travado());

    // Em t = 2000 ms (1000 ms sem pulso): corta tração, congela cronômetro e sinaliza TRAVADO
    TEST_ASSERT_TRUE(monitor.verificar(2000));
    TEST_ASSERT_TRUE(monitor.travado());
    TEST_ASSERT_EQUAL(TRAVADO, monitor.estadoRobo());
    TEST_ASSERT_EQUAL_UINT32(2000, monitor.instanteCorteMs());
    TEST_ASSERT_EQUAL_STRING("PCS", ev.ordem);
    TEST_ASSERT_EQUAL(1, tracao.chamadasParar);
    TEST_ASSERT_FALSE(tracao.tracaoAtiva());
    TEST_ASSERT_EQUAL(1, cron.chamadasCongelar);
    TEST_ASSERT_EQUAL(1, sin.chamadas);
    TEST_ASSERT_EQUAL(TRAVADO, sin.ultimoEstado);
}

void test_monitor_corta_em_no_maximo_1200ms_com_verificacao_a_cada_50ms(void) {
    EventosTravamento ev;
    TracaoFalsaTravamento tracao(ev, true);
    OdometriaFalsaTravamento odom(0);

    MonitorTravamento monitor(tracao, odom);

    // Robô anda normalmente até t = 2310 ms, quando a roda trava fisicamente
    for (uint32_t t = 0; t <= 2300; t += PERIODO_SEGURANCA_MS) {
        odom.ultimoPulsoMs = t;
        TEST_ASSERT_FALSE(monitor.verificar(t));
    }
    const uint32_t instanteBloqueioFisicoMs = 2310;
    odom.ultimoPulsoMs = instanteBloqueioFisicoMs;

    // Continua o laço de 50 ms sem novos pulsos até o corte acontecer
    uint32_t t = 2350;
    while (t <= 4000 && !monitor.verificar(t)) {
        t += PERIODO_SEGURANCA_MS;
    }

    TEST_ASSERT_TRUE(monitor.travado());
    const uint32_t atrasoCorteMs = monitor.instanteCorteMs() - instanteBloqueioFisicoMs;
    // Com amostragem de 50 ms e limiar de 1000 ms, o corte ocorre em 1040 ms (entre 1000 e 1050 ms)
    TEST_ASSERT_EQUAL_UINT32(1040, atrasoCorteMs);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(LIMITE_CORTE_TRAVAMENTO_MS, atrasoCorteMs);
}

void test_monitor_estado_travado_e_terminal_ate_iniciar_nova_prova(void) {
    EventosTravamento ev;
    TracaoFalsaTravamento tracao(ev, true);
    OdometriaFalsaTravamento odom(0);
    MonitorTravamento monitor(tracao, odom);

    monitor.verificar(0);
    TEST_ASSERT_TRUE(monitor.verificar(1000));
    TEST_ASSERT_EQUAL(1, tracao.chamadasParar);

    // Chamadas seguintes permanecem em TRAVADO sem repetir parar()
    TEST_ASSERT_TRUE(monitor.verificar(1050));
    TEST_ASSERT_TRUE(monitor.verificar(false, 1050, 1100));
    TEST_ASSERT_EQUAL(1, tracao.chamadasParar);

    // Reinício explícito para nova corrida
    tracao.ativa = true;
    odom.ultimoPulsoMs = 2000;
    monitor.iniciarProva();
    TEST_ASSERT_FALSE(monitor.travado());
    TEST_ASSERT_EQUAL_UINT32(0, monitor.instanteCorteMs());
    TEST_ASSERT_EQUAL_UINT32(0, monitor.tempoSemPulsoMs());
    TEST_ASSERT_FALSE(monitor.verificar(2000));
}

static uint32_t relogioTravamentoMs = 0;
static uint32_t lerRelogioTravamentoMs() { return relogioTravamentoMs; }

void test_monitor_construtor_enxuto_com_cronometro_real(void) {
    EventosTravamento ev;
    TracaoFalsaTravamento tracao(ev, true);
    Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS);
    AdaptadorCronometro adaptador(cronometro, lerRelogioTravamentoMs);
    MonitorTravamento monitor(static_cast<ITracao&>(tracao), &adaptador);

    // Sem fontes injetadas, verificar(agoraMs) retorna false com segurança
    TEST_ASSERT_FALSE(monitor.verificar(0));

    relogioTravamentoMs = 1000;
    cronometro.iniciar(relogioTravamentoMs);

    relogioTravamentoMs = 5000;
    TEST_ASSERT_FALSE(monitor.verificar(true, 5000, 5000));

    relogioTravamentoMs = 6000;
    TEST_ASSERT_TRUE(monitor.verificar(true, 5000, 6000));
    TEST_ASSERT_TRUE(cronometro.congelado());
    TEST_ASSERT_EQUAL_UINT32(5000, cronometro.decorridoMs(9000));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_nao_dispara_com_pulsos_regulares);
    RUN_TEST(test_dispara_aos_1000ms_sem_pulso_com_tracao_ativa);
    RUN_TEST(test_nao_dispara_com_tracao_desligada);
    RUN_TEST(test_nao_dispara_logo_apos_ligar_tracao_com_ultimo_pulso_antigo);
    RUN_TEST(test_pulso_simultaneo_ou_adiantado_zera_tempo_sem_pulso);
    RUN_TEST(test_reiniciar_limpa_historico_do_detector);
    RUN_TEST(test_monitor_corta_motores_congela_cronometro_e_sinaliza_travado_em_ordem);
    RUN_TEST(test_monitor_corta_em_no_maximo_1200ms_com_verificacao_a_cada_50ms);
    RUN_TEST(test_monitor_estado_travado_e_terminal_ate_iniciar_nova_prova);
    RUN_TEST(test_monitor_construtor_enxuto_com_cronometro_real);

    return UNITY_END();
}
