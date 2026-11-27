#ifndef CICLO_PROVA_H
#define CICLO_PROVA_H

#include <stdint.h>
#include <string.h>
#include "TelemetriaCodec.h"        // telemetriaValidar, TELEMETRIA_TIMESTAMP_MAX_MS
#include "TelemetriaConfig.h"       // TELEMETRIA_PERIODO_MS
#include "TelemetriaPacket.h"
#include "TemporizadorPeriodico.h"
#include "InterfacesHardware.h"
#include "../botao/Botao.h"
#include "../cronometro/Cronometro.h"
#include "../drivers/Motores.h"
#include "../mapa/Mapa.h"
#include "../movimento/InterfacesMovimento.h"
#include "../navegacao/Navegador.h"
#include "../navegacao/RotaMaisCurta.h"
#include "../navegacao/VelocidadeMedia.h"
#include "../odometria/Odometria.h"
#include "../parada/ParadaObjetivo.h"
#include "../seguranca/MonitorTravamento.h"
#include "../sensores/BateriaConversao.h"
#include "../sensores/ClassificadorParedes.h"
#include "../sinalizacao/PadraoLed.h"

// Laço da prova, sem dependência do Arduino (docs/firmware/integracao-loop.md, seções 3.3
// a 3.8). O main.cpp só cria os drivers e chama tick(millis()) em todo loop(); nos testes
// nativos e no robô virtual, tick() roda com relógio falso.

enum SubfaseMapeamento : uint8_t {
    SUBFASE_ESPERA_LARGADA, // pausa pós-soltar botão para afastar a mão antes de ler paredes
    SUBFASE_LENDO_PAREDES,
    SUBFASE_GIRANDO,
    SUBFASE_ALINHANDO_TRAS, // wall-squaring contra parede traseira conhecida após curva
    SUBFASE_AVANCANDO,
    SUBFASE_DESATOLANDO     // recuo curto antes de nova tentativa após falha de avanço
};

struct DependenciasProva {
    IEntradaBotao& botao;
    IEncoders& encoders;
    ISensoresToF& tof;
    IBateria& bateria;
    ILedEstado& led;
    Motores& motores;
    Odometria& odometria;
    ExecutorMovimentoNaoBloqueante& executor;
    IRadioTelemetria* radio;
    IRegistro* registro;
};

class CicloProva {
public:
    static const uint32_t PERIODO_CONTROLE_MS = 10;
    static const uint32_t PERIODO_LED_MS = 50;
    static const uint32_t ESPERA_LEITURA_PAREDES_MS = 250;
    static const uint32_t AMOSTRAS_PAREDE_CELULA = 2;
    static const uint32_t FREIO_TERMINAL_MS = 300;
    static const uint32_t ESPERA_LARGADA_HARDWARE_MS = 1200;

    explicit CicloProva(const DependenciasProva& d, uint32_t esperaLargadaMs = 0)
        : d_(d),
          esperaLargadaMs_(esperaLargadaMs),
          cronometro_(TELEMETRIA_TIMESTAMP_MAX_MS),
          navegador_(mapa_),
          congelador_(cronometro_, agoraMs_),
          parada_(d.motores, congelador_),
          monitor_(d.motores, d.odometria, &congelador_, &d.led),
          temporizadorControle_(PERIODO_CONTROLE_MS),
          temporizadorSeguranca_(PERIODO_SEGURANCA_MS),
          temporizadorTelemetria_(TELEMETRIA_PERIODO_MS),
          temporizadorLed_(PERIODO_LED_MS),
          estado_(AGUARDANDO),
          subfase_(SUBFASE_LENDO_PAREDES),
          giroPendente_(GIRO_NENHUM),
          inicioEsperaLargadaMs_(0),
          inicioLeituraMs_(0),
          amostrasEsqInicio_(0),
          amostrasFrenteInicio_(0),
          amostrasDirInicio_(0),
          terminalDesdeMs_(0),
          rodasSoltas_(false),
          causaUltimaFalha_(FALHA_NENHUMA),
          agoraMs_(0),
          bateriaMv_(0),
          pacotesEnviados_(0),
          pacotesInvalidos_(0),
          temRotaSpeedRun_(false),
          speedRunArmado_(false),
          emModoSpeedRun_(false),
          speedRunContinuo_(false),
          emCurvaSpeedRun_(false),
          srEntradaFinal_(false),
          passoSpeedRun_(0),
          linhaSr_(0),
          colunaSr_(0),
          orientacaoSr_(DIR_LESTE),
          deslocamentosSr_(0),
          ultimoTempoExploracaoMs_(0),
          ultimoTempoSpeedRunMs_(0) {
        memset(&ultimoPacote_, 0, sizeof(ultimoPacote_));
    }

    // Chamar uma vez no setup(), com os drivers já iniciados.
    void iniciar(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        mapa_.setGeometria(LAB_4X4);
        d_.motores.liberar();
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
        d_.executor.reiniciar();
        mudarEstado(AGUARDANDO);
    }

    void tick(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        const bool tickControle = temporizadorControle_.vencido(agoraMs);

        // 1. Entradas
        const EventoBotao evento = botao_.atualizar(d_.botao.pressionado(), agoraMs);
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.atualizar(esq, dir, agoraMs);
        d_.tof.atualizar(agoraMs);

        // 2. Segurança, antes da máquina: um corte é visto por ela no mesmo tick.
        if (temporizadorSeguranca_.vencido(agoraMs)) verificarSeguranca(agoraMs);

        // 3. Máquina da prova
        switch (estado_) {
            case AGUARDANDO:
                tratarAguardando(evento, agoraMs);
                break;
            case MAPEANDO:
                tratarMapeando(agoraMs, tickControle);
                break;
            case SPEED_RUN:
                tratarSpeedRun(agoraMs, tickControle);
                break;
            case CONCLUIDO:
            case COLISAO:
            case TIMEOUT:
            case TRAVADO:
                tratarTerminal(evento, agoraMs);
                break;
        }

        // 4. Telemetria
        if (temporizadorTelemetria_.vencido(agoraMs)) enviarTelemetria(agoraMs);

        // 5. LED
        if (temporizadorLed_.vencido(agoraMs)) {
            const bool piscandoLargada =
                (estado_ == AGUARDANDO && botao_.largadaArmada()) ||
                ((estado_ == MAPEANDO || estado_ == SPEED_RUN) && subfase_ == SUBFASE_ESPERA_LARGADA);
            d_.led.atualizar(agoraMs, piscandoLargada);
        }
    }

    // Largada remota (via Serial ou comando 'k' ESP-NOW do Gateway).
    // Só inicia a prova quando o robô está em AGUARDANDO.
    void largadaRemota(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        if (estado_ != AGUARDANDO) return;
        registrar("Largada remota recebida.");
        darLargada(agoraMs);
    }

    // Alterna a geometria do labirinto (4x4 -> 8x4 -> 12x4) quando em AGUARDANDO.
    void alternarGeometria(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        if (estado_ != AGUARDANDO) return;
        proximaGeometria(agoraMs);
    }

    // Reinicia para AGUARDANDO a partir de um estado terminal.
    void reiniciarRemoto(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        if (estado_ == AGUARDANDO || estado_ == MAPEANDO || estado_ == SPEED_RUN) return;
        reiniciar(agoraMs);
    }

    // Arma o Speed Run mantendo o mapa conhecido e a rota calculada (HU-16).
    // Pode ser chamado em AGUARDANDO ou em qualquer estado terminal com rota valida.
    bool prepararSpeedRun(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        if (!temRotaSpeedRun_) return false;
        if (estado_ == MAPEANDO || estado_ == SPEED_RUN) return false;
        d_.executor.abortar();
        cronometro_.zerar();
        parada_.iniciarProva();
        monitor_.iniciarProva();
        d_.motores.liberar();
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
        d_.executor.reiniciar();
        causaUltimaFalha_ = FALHA_NENHUMA;
        speedRunArmado_ = true;
        emModoSpeedRun_ = false;
        linhaSr_ = 0;
        colunaSr_ = 0;
        orientacaoSr_ = DIR_LESTE;
        deslocamentosSr_ = 0;
        mudarEstado(AGUARDANDO);
        d_.led.piscar(corLed(0, 255, 255), 2, agoraMs);
        registrar("Speed run armado: reposicione em A1 e de a largada.");
        return true;
    }

    // Parada de emergência remota (via Serial ou comando 's' ESP-NOW do Gateway).
    void paradaEmergencia(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        cortar(agoraMs);
        registrar("Parada de emergencia remota.");
        if (estado_ == MAPEANDO || estado_ == SPEED_RUN) {
            entrarTerminal(TRAVADO, agoraMs);
        }
    }

    // --- Consultas (log, autoteste e testes) ---
    EstadoRobo estado() const { return estado_; }
    SubfaseMapeamento subfase() const { return subfase_; }
    const Navegador& navegador() const { return navegador_; }
    const Mapa& mapa() const { return mapa_; }
    const Cronometro& cronometro() const { return cronometro_; }
    const ParadaObjetivo& parada() const { return parada_; }
    const MonitorTravamento& monitor() const { return monitor_; }
    const TelemetriaPacket& ultimoPacote() const { return ultimoPacote_; }
    uint32_t pacotesEnviados() const { return pacotesEnviados_; }
    uint32_t pacotesInvalidos() const { return pacotesInvalidos_; }
    MotivoFalhaMovimento causaUltimaFalha() const { return causaUltimaFalha_; }
    bool temRotaSpeedRun() const { return temRotaSpeedRun_; }
    bool speedRunArmado() const { return speedRunArmado_; }
    const Rota& rotaSpeedRun() const { return rotaSpeedRun_; }
    uint32_t ultimoTempoExploracaoMs() const { return ultimoTempoExploracaoMs_; }
    uint32_t ultimoTempoSpeedRunMs() const { return ultimoTempoSpeedRunMs_; }

private:
    class CongeladorCronometro : public ICronometro {
    public:
        CongeladorCronometro(Cronometro& c, const uint32_t& agoraMs) : c_(c), agoraMs_(agoraMs) {}
        void congelar() override { c_.congelar(agoraMs_); }

    private:
        Cronometro& c_;
        const uint32_t& agoraMs_;
    };

    // ------------------------------------------------------------------ estados

    void mudarEstado(EstadoRobo e) {
        estado_ = e;
        d_.led.mostrarEstado(e);
    }

    void registrar(const char* mensagem) {
        if (d_.registro != nullptr) d_.registro->registrar(mensagem);
    }

    uint16_t lerBateria() {
        d_.encoders.pausarEsquerdo();
        const uint16_t mv = d_.bateria.lerMilivolts();
        d_.encoders.retomarEsquerdo();
        bateriaMv_ = mv;
        return mv;
    }

    void cortar(uint32_t agoraMs) {
        d_.motores.parar();
        d_.executor.abortar();
        if ((estado_ == MAPEANDO || estado_ == SPEED_RUN) &&
            !cronometro_.emAndamento() && !cronometro_.congelado()) {
            cronometro_.iniciar(agoraMs);
        }
        cronometro_.congelar(agoraMs);
    }

    void entrarTerminal(EstadoRobo e, uint32_t agoraMs) {
        terminalDesdeMs_ = agoraMs;
        rodasSoltas_ = false;
        mudarEstado(e);
        enviarTelemetria(agoraMs);
    }

    // ------------------------------------------------------------------ segurança

    void verificarSeguranca(uint32_t agoraMs) {
        if (estado_ != MAPEANDO && estado_ != SPEED_RUN) return;
        if (monitor_.verificar(agoraMs)) {
            d_.executor.abortar();
            registrar("Travamento: tracao cortada.");
            entrarTerminal(TRAVADO, agoraMs);
            return;
        }
        if (cronometro_.decorridoMs(agoraMs) >= TELEMETRIA_TIMESTAMP_MAX_MS) {
            cortar(agoraMs);
            registrar("Tempo limite de 10 min.");
            entrarTerminal(TIMEOUT, agoraMs);
        }
    }

    // ------------------------------------------------------------------ AGUARDANDO

    void tratarAguardando(const EventoBotao& evento, uint32_t agoraMs) {
        if (evento.tipo == BOTAO_LARGADA) {
            darLargada(agoraMs);
        } else if (evento.tipo == BOTAO_TOQUES && evento.toques == 2 && temRotaSpeedRun_) {
            speedRunArmado_ = !speedRunArmado_;
            d_.led.piscar(speedRunArmado_ ? corLed(0, 255, 255) : corLed(0, 255, 0), 2, agoraMs);
            registrar(speedRunArmado_ ? "Modo SPEED_RUN selecionado." : "Modo EXPLORACAO selecionado.");
        } else if (evento.tipo == BOTAO_TOQUES && evento.toques == 3) {
            proximaGeometria(agoraMs);
        }
    }

    void proximaGeometria(uint32_t agoraMs) {
        GeometriaLabirinto geo = mapa_.getGeometria();
        uint8_t piscadas;
        if (geo == LAB_4X4) {
            geo = LAB_8X4;
            piscadas = 2;
        } else if (geo == LAB_8X4) {
            geo = LAB_12X4;
            piscadas = 3;
        } else {
            geo = LAB_4X4;
            piscadas = 1;
        }
        mapa_.setGeometria(geo);
        temRotaSpeedRun_ = false;
        speedRunArmado_ = false;
        emModoSpeedRun_ = false;
        d_.led.piscar(corLed(0, 255, 255), piscadas, agoraMs);
        registrar(geo == LAB_4X4 ? "Geometria 4x4." : geo == LAB_8X4 ? "Geometria 8x4." : "Geometria 12x4.");
    }

    void darLargada(uint32_t agoraMs) {
        if (!bateriaPermiteLargada(lerBateria())) {
            d_.led.piscar(corLed(255, 0, 0), 3, agoraMs);
            registrar("Largada recusada: bateria abaixo do minimo.");
            return;
        }
        const GeometriaLabirinto geo = mapa_.getGeometria();
        const bool iniciarSpeedRun = speedRunArmado_ && temRotaSpeedRun_;
        if (!iniciarSpeedRun) {
            mapa_.setGeometria(geo);
            temRotaSpeedRun_ = false;
        }
        parada_.configurarObjetivoPorGeometria(geo);
        parada_.iniciarProva();
        monitor_.iniciarProva();
        d_.motores.liberar();
        if (!iniciarSpeedRun) navegador_.iniciar();
        cronometro_.zerar();
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
        d_.executor.reiniciar();
        classificador_.reiniciar();
        causaUltimaFalha_ = FALHA_NENHUMA;

        if (iniciarSpeedRun) {
            speedRunArmado_ = false;
            emModoSpeedRun_ = true;
            linhaSr_ = 0;
            colunaSr_ = 0;
            orientacaoSr_ = DIR_LESTE;
            deslocamentosSr_ = 0;
            mudarEstado(SPEED_RUN);
            if (esperaLargadaMs_ > 0) {
                inicioEsperaLargadaMs_ = agoraMs;
                subfase_ = SUBFASE_ESPERA_LARGADA;
                registrar("Speed run confirmado: afaste a mao dos sensores...");
                return;
            }
            cronometro_.iniciar(agoraMs);
            registrar("Largada: SPEED_RUN.");
            iniciarPercursoSpeedRun(agoraMs);
            return;
        }

        emModoSpeedRun_ = false;
        mudarEstado(MAPEANDO);

        if (esperaLargadaMs_ > 0) {
            inicioEsperaLargadaMs_ = agoraMs;
            subfase_ = SUBFASE_ESPERA_LARGADA;
            registrar("Largada confirmada: afaste a mao dos sensores...");
            return;
        }
        cronometro_.iniciar(agoraMs);
        registrar("Largada: MAPEANDO.");
        comecarLeitura(agoraMs);
    }

    // ------------------------------------------------------------------ MAPEANDO

    void comecarLeitura(uint32_t agoraMs) {
        subfase_ = SUBFASE_LENDO_PAREDES;
        inicioLeituraMs_ = agoraMs;
        amostrasEsqInicio_ = d_.tof.esquerdo().amostras();
        amostrasFrenteInicio_ = d_.tof.frontal().amostras();
        amostrasDirInicio_ = d_.tof.direito().amostras();
    }

    bool amostrasDaCelula() {
        return amostrasNovas(d_.tof.esquerdo(), amostrasEsqInicio_) &&
               amostrasNovas(d_.tof.frontal(), amostrasFrenteInicio_) &&
               amostrasNovas(d_.tof.direito(), amostrasDirInicio_);
    }

    bool amostrasNovas(const LeituraToF& l, uint32_t base) const {
        return l.temAmostra() &&
               (uint32_t)(l.amostras() - base) >= AMOSTRAS_PAREDE_CELULA &&
               (int32_t)(l.instanteMs() - inicioLeituraMs_) >= 0;
    }

    void tratarMapeando(uint32_t agoraMs, bool tickControle) {
        switch (subfase_) {
            case SUBFASE_ESPERA_LARGADA:
                tratarEsperaLargada(agoraMs);
                break;
            case SUBFASE_LENDO_PAREDES:
                lerParedes(agoraMs);
                break;
            case SUBFASE_GIRANDO:
                if (tickControle) acompanharGiro(agoraMs);
                break;
            case SUBFASE_ALINHANDO_TRAS:
                if (tickControle) acompanharAlinhamentoTras(agoraMs);
                break;
            case SUBFASE_AVANCANDO:
                if (tickControle) acompanharAvanco(agoraMs);
                break;
            case SUBFASE_DESATOLANDO:
                if (tickControle) acompanharDesatolamento(agoraMs);
                break;
        }
    }

    void tratarEsperaLargada(uint32_t agoraMs) {
        if ((uint32_t)(agoraMs - inicioEsperaLargadaMs_) < esperaLargadaMs_) return;
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
        d_.executor.reiniciar();
        cronometro_.iniciar(agoraMs);
        if (estado_ == SPEED_RUN) {
            registrar("Largada: SPEED_RUN.");
            iniciarPercursoSpeedRun(agoraMs);
            return;
        }
        registrar("Largada: MAPEANDO.");
        comecarLeitura(agoraMs);
    }

    void lerParedes(uint32_t agoraMs) {
        if (amostrasDaCelula()) {
            const AmostraParedes amostra = amostraDasLeituras(d_.tof.esquerdo(), d_.tof.frontal(),
                                                              d_.tof.direito(), agoraMs);
            classificador_.atualizar(amostra);
            bool frente, esquerda, direita;
            if (classificador_.lerParedes(frente, esquerda, direita)) {
                navegador_.observar(frente, esquerda, direita);
                enviarTelemetria(agoraMs);
                decidir(agoraMs);
                return;
            }
        }
        if ((uint32_t)(agoraMs - inicioLeituraMs_) >= ESPERA_LEITURA_PAREDES_MS) {
            causaUltimaFalha_ = FALHA_NENHUMA;
            registrar("Falha de leitura das paredes.");
            navegador_.registrarFalha();
            if (navegador_.estado() == NAV_FALHA) {
                falhaTerminal(agoraMs);
                return;
            }
            comecarLeitura(agoraMs);
        }
    }

    void decidir(uint32_t agoraMs) {
        const DecisaoNavegacao dec = navegador_.decidir();
        if (!dec.movimento) {
            semMovimento(agoraMs);
            return;
        }
        if (!d_.executor.ocioso()) d_.executor.abortar();
        if (dec.giro != GIRO_NENHUM) {
            giroPendente_ = dec.giro;
            d_.executor.iniciarGiro(dec.giro, navegador_.orientacao(), agoraMs);
            subfase_ = SUBFASE_GIRANDO;
        } else {
            d_.executor.iniciarAvanco(agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
        }
    }

    void acompanharGiro(uint32_t agoraMs) {
        const ResultadoMovimento r = d_.executor.atualizar(agoraMs);
        if (r == MOV_CONCLUIDO) {
            navegador_.confirmarGiro(giroPendente_);
            const Direcao tras = direcaoOposta(navegador_.orientacao());
            const bool paredeAtras = mapa_.ladoConhecido(navegador_.linha(), navegador_.coluna(), tras) &&
                                     mapa_.temParede(navegador_.linha(), navegador_.coluna(), tras);
            if (paredeAtras && d_.executor.iniciarAlinhamentoTraseiro(agoraMs)) {
                registrar("Alinhando na parede traseira (wall-squaring).");
                subfase_ = SUBFASE_ALINHANDO_TRAS;
                return;
            }
            d_.executor.iniciarAvanco(agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
        } else if (r == MOV_FALHOU) {
            falhaMovimento(agoraMs, false);
        }
    }

    void acompanharAlinhamentoTras(uint32_t agoraMs) {
        const ResultadoMovimento r = d_.executor.atualizar(agoraMs);
        if (r == MOV_CONCLUIDO || r == MOV_FALHOU) {
            if (r == MOV_FALHOU && d_.executor.motivoUltimaFalha() == FALHA_TRACAO_CORTADA) {
                falhaMovimento(agoraMs, false);
                return;
            }
            d_.executor.iniciarAvanco(agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
        }
    }

    void acompanharAvanco(uint32_t agoraMs) {
        const ResultadoMovimento r = d_.executor.atualizar(agoraMs);
        if (r == MOV_CONCLUIDO) {
            navegador_.confirmarAvanco();
            parada_.atualizar(navegador_.linha(), navegador_.coluna());
            if (parada_.concluido()) {
                ultimoTempoExploracaoMs_ = cronometro_.decorridoMs(agoraMs);
                temRotaSpeedRun_ =
                    (RotaMaisCurta::calcularParaObjetivo(mapa_, rotaSpeedRun_) == ROTA_OK &&
                     rotaSpeedRun_.numPassos > 0);
                registrar("Objetivo alcancado: CONCLUIDO.");
                entrarTerminal(CONCLUIDO, agoraMs);
                return;
            }
            comecarLeitura(agoraMs);
        } else if (r == MOV_FALHOU) {
            falhaMovimento(agoraMs, true);
        }
    }

    void acompanharDesatolamento(uint32_t agoraMs) {
        const ResultadoMovimento r = d_.executor.atualizar(agoraMs);
        if (r == MOV_CONCLUIDO || r == MOV_FALHOU) {
            if (r == MOV_FALHOU && d_.executor.motivoUltimaFalha() == FALHA_TRACAO_CORTADA) {
                cortar(agoraMs);
                registrar("Tracao cortada durante desatolamento.");
                entrarTerminal(TRAVADO, agoraMs);
                return;
            }
            comecarLeitura(agoraMs);
        }
    }

    void falhaMovimento(uint32_t agoraMs, bool tentarDesatolar) {
        const MotivoFalhaMovimento motivo = d_.executor.motivoUltimaFalha();
        if (motivo == FALHA_TRACAO_CORTADA) {
            cortar(agoraMs);
            registrar("Tracao cortada durante o movimento.");
            entrarTerminal(TRAVADO, agoraMs);
            return;
        }
        causaUltimaFalha_ = motivo;
        registrar(motivo == FALHA_EMERGENCIA_FRONTAL ? "Emergencia frontal." : "Movimento nao concluido no tempo.");
        navegador_.registrarFalha();
        if (navegador_.estado() == NAV_FALHA) {
            falhaTerminal(agoraMs);
            return;
        }
        if (tentarDesatolar && d_.executor.iniciarRecuoDesatolamento(agoraMs)) {
            registrar("Recuando para desencostar antes de nova tentativa.");
            subfase_ = SUBFASE_DESATOLANDO;
            return;
        }
        comecarLeitura(agoraMs);
    }

    void falhaTerminal(uint32_t agoraMs) {
        cortar(agoraMs);
        entrarTerminal(causaUltimaFalha_ == FALHA_EMERGENCIA_FRONTAL ? COLISAO : TRAVADO, agoraMs);
    }

    void semMovimento(uint32_t agoraMs) {
        if (navegador_.chegou()) {
            registrar("Chegada sem parada no objetivo.");
        } else if (navegador_.estado() == NAV_INALCANCAVEL) {
            registrar("Destino inalcancavel.");
        } else {
            registrar("Navegacao encerrada sem chegada.");
        }
        cortar(agoraMs);
        entrarTerminal(TRAVADO, agoraMs);
    }

    // ------------------------------------------------------------------ SPEED_RUN

    void iniciarPercursoSpeedRun(uint32_t agoraMs) {
        speedRunContinuo_ = d_.executor.suportaPercursoContinuo();
        passoSpeedRun_ = 0;
        emCurvaSpeedRun_ = false;
        srEntradaFinal_ = false;
        if (rotaSpeedRun_.numPassos == 0) {
            cortar(agoraMs);
            entrarTerminal(TRAVADO, agoraMs);
            return;
        }
        enviarTelemetria(agoraMs);
        if (speedRunContinuo_) {
            const Giro g0 = rotaSpeedRun_.giroDoPasso(0, orientacaoSr_);
            if (g0 != GIRO_NENHUM) {
                emCurvaSpeedRun_ = true;
                d_.executor.iniciarGiro(g0, orientacaoSr_, agoraMs);
                subfase_ = SUBFASE_GIRANDO;
            } else {
                d_.executor.iniciarAvancoRapido(CELULA_MM / 2, false, false, agoraMs);
                subfase_ = SUBFASE_AVANCANDO;
            }
        } else {
            iniciarPassoDiscretoSpeedRun(agoraMs);
        }
    }

    void iniciarPassoDiscretoSpeedRun(uint32_t agoraMs) {
        const Giro g = rotaSpeedRun_.giroDoPasso(passoSpeedRun_, orientacaoSr_);
        if (g != GIRO_NENHUM) {
            emCurvaSpeedRun_ = true;
            d_.executor.iniciarGiro(g, orientacaoSr_, agoraMs);
            subfase_ = SUBFASE_GIRANDO;
        } else {
            emCurvaSpeedRun_ = false;
            d_.executor.iniciarAvancoRapido(CELULA_MM, false, true, agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
        }
    }

    void avancarPosicaoLogicaSpeedRun() {
        orientacaoSr_ = rotaSpeedRun_.passos[passoSpeedRun_];
        mapa_.vizinha(linhaSr_, colunaSr_, orientacaoSr_, linhaSr_, colunaSr_);
        deslocamentosSr_++;
        passoSpeedRun_++;
    }

    void tratarSpeedRun(uint32_t agoraMs, bool tickControle) {
        if (subfase_ == SUBFASE_ESPERA_LARGADA) {
            tratarEsperaLargada(agoraMs);
            return;
        }
        if (!tickControle) return;

        const ResultadoMovimento r = d_.executor.atualizar(agoraMs);
        if (r == MOV_FALHOU) {
            const MotivoFalhaMovimento motivo = d_.executor.motivoUltimaFalha();
            causaUltimaFalha_ = motivo;
            cortar(agoraMs);
            registrar(motivo == FALHA_EMERGENCIA_FRONTAL ? "Emergencia frontal no speed run."
                                                         : "Falha de movimento no speed run.");
            entrarTerminal(motivo == FALHA_EMERGENCIA_FRONTAL ? COLISAO : TRAVADO, agoraMs);
            return;
        }
        if (r != MOV_CONCLUIDO) return;

        if (!speedRunContinuo_) {
            if (emCurvaSpeedRun_) {
                emCurvaSpeedRun_ = false;
                orientacaoSr_ = rotaSpeedRun_.passos[passoSpeedRun_];
                d_.executor.iniciarAvancoRapido(CELULA_MM, false, true, agoraMs);
                subfase_ = SUBFASE_AVANCANDO;
                return;
            }
            avancarPosicaoLogicaSpeedRun();
            parada_.atualizar(linhaSr_, colunaSr_);
            if (parada_.concluido() || passoSpeedRun_ >= rotaSpeedRun_.numPassos) {
                ultimoTempoSpeedRunMs_ = cronometro_.decorridoMs(agoraMs);
                registrar("Speed run concluido: CONCLUIDO.");
                entrarTerminal(CONCLUIDO, agoraMs);
                return;
            }
            iniciarPassoDiscretoSpeedRun(agoraMs);
            return;
        }

        if (emCurvaSpeedRun_) {
            emCurvaSpeedRun_ = false;
            orientacaoSr_ = rotaSpeedRun_.passos[0];
            d_.executor.iniciarAvancoRapido(CELULA_MM / 2, false, false, agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
            return;
        }

        if (srEntradaFinal_) {
            parada_.atualizar(linhaSr_, colunaSr_);
            ultimoTempoSpeedRunMs_ = cronometro_.decorridoMs(agoraMs);
            registrar("Speed run concluido: CONCLUIDO.");
            entrarTerminal(CONCLUIDO, agoraMs);
            return;
        }

        avancarPosicaoLogicaSpeedRun();
        if (passoSpeedRun_ >= rotaSpeedRun_.numPassos) {
            srEntradaFinal_ = true;
            d_.executor.iniciarAvancoRapido(CELULA_MM / 2, true, true, agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
            return;
        }

        const Giro g = rotaSpeedRun_.giroDoPasso(passoSpeedRun_, orientacaoSr_);
        if (g == GIRO_NENHUM) {
            d_.executor.iniciarAvancoRapido(CELULA_MM, true, false, agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
        } else if (g == GIRO_ESQUERDA || g == GIRO_DIREITA) {
            d_.executor.iniciarCurvaContinua(g, orientacaoSr_, agoraMs);
            subfase_ = SUBFASE_GIRANDO;
        } else {
            d_.executor.iniciarGiro(g, orientacaoSr_, agoraMs);
            subfase_ = SUBFASE_GIRANDO;
        }
    }

    // ------------------------------------------------------------------ terminais

    void tratarTerminal(const EventoBotao& evento, uint32_t agoraMs) {
        if (!rodasSoltas_ && (uint32_t)(agoraMs - terminalDesdeMs_) >= FREIO_TERMINAL_MS) {
            d_.motores.soltar();
            rodasSoltas_ = true;
        }
        if (evento.tipo == BOTAO_TOQUES && evento.toques == 2 && temRotaSpeedRun_) {
            prepararSpeedRun(agoraMs);
            return;
        }
        if (evento.tipo == BOTAO_LARGADA) reiniciar(agoraMs);
    }

    void reiniciar(uint32_t agoraMs) {
        d_.executor.abortar();
        mapa_.setGeometria(mapa_.getGeometria());
        cronometro_.zerar();
        parada_.iniciarProva();
        monitor_.iniciarProva();
        d_.motores.liberar();
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
        d_.executor.reiniciar();
        causaUltimaFalha_ = FALHA_NENHUMA;
        temRotaSpeedRun_ = false;
        speedRunArmado_ = false;
        emModoSpeedRun_ = false;
        mudarEstado(AGUARDANDO);
        registrar("Nova tentativa: AGUARDANDO.");
    }

    // ------------------------------------------------------------------ telemetria

    static int16_t saturarInt16(float v) {
        if (v > 32767.0f) return 32767;
        if (v < -32768.0f) return -32768;
        return (int16_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
    }

    TelemetriaPacket montarTelemetria(uint32_t agoraMs) {
        TelemetriaPacket p;
        memset(&p, 0, sizeof(p));
        p.timestamp_ms = cronometro_.decorridoMs(agoraMs);
        p.labirinto_id = (uint8_t)mapa_.getGeometria();
        p.status_id = (uint8_t)estado_;
        p.desafio_cumprido = parada_.desafioCumprido() ? 1 : 0;
        const uint16_t mv = (bateriaMv_ == 0 || d_.executor.ocioso()) ? lerBateria() : bateriaMv_;
        p.bateria_mv = mv;
        p.bateria_pct = porcentagemBateria(mv);
        p.velocidade_atual = saturarInt16(d_.odometria.velocidadeMmS());
        p.velocidade_media = velocidadeMediaMmS(
            emModoSpeedRun_ ? deslocamentosSr_ : navegador_.deslocamentos(), p.timestamp_ms);
        if (estado_ == AGUARDANDO) {
            p.linha = 0;
            p.coluna = 0;
            p.orientacao = (uint8_t)DIR_LESTE;
            p.paredes = mapa_.getParedes(0, 0);
        } else if (emModoSpeedRun_) {
            p.linha = linhaSr_;
            p.coluna = colunaSr_;
            p.orientacao = (uint8_t)orientacaoSr_;
            p.paredes = mapa_.getParedes(linhaSr_, colunaSr_);
        } else {
            navegador_.preencherPosicao(p);
        }
        return p;
    }

    void enviarTelemetria(uint32_t agoraMs) {
        if (d_.radio == nullptr) return;
        const TelemetriaPacket p = montarTelemetria(agoraMs);
        ultimoPacote_ = p;
        if (telemetriaValidar(p) != TELEMETRIA_OK) {
            pacotesInvalidos_++;
            registrar("ERRO: pacote de telemetria invalido (nao enviado).");
            return;
        }
        if (d_.radio->enviar(p)) pacotesEnviados_++;
    }

    // ------------------------------------------------------------------ membros

    DependenciasProva d_;
    uint32_t esperaLargadaMs_;
    Botao botao_;
    Cronometro cronometro_;
    Mapa mapa_;
    Navegador navegador_;
    CongeladorCronometro congelador_;
    ParadaObjetivo parada_;
    MonitorTravamento monitor_;
    ClassificadorParedes classificador_;
    TemporizadorPeriodico temporizadorControle_;
    TemporizadorPeriodico temporizadorSeguranca_;
    TemporizadorPeriodico temporizadorTelemetria_;
    TemporizadorPeriodico temporizadorLed_;
    EstadoRobo estado_;
    SubfaseMapeamento subfase_;
    Giro giroPendente_;
    uint32_t inicioEsperaLargadaMs_;
    uint32_t inicioLeituraMs_;
    uint32_t amostrasEsqInicio_;
    uint32_t amostrasFrenteInicio_;
    uint32_t amostrasDirInicio_;
    uint32_t terminalDesdeMs_;
    bool rodasSoltas_;
    MotivoFalhaMovimento causaUltimaFalha_;
    uint32_t agoraMs_;
    uint16_t bateriaMv_;
    TelemetriaPacket ultimoPacote_;
    uint32_t pacotesEnviados_;
    uint32_t pacotesInvalidos_;
    // Speed run (HU-16 / HU-17)
    Rota rotaSpeedRun_;
    bool temRotaSpeedRun_;
    bool speedRunArmado_;
    bool emModoSpeedRun_;
    bool speedRunContinuo_;
    bool emCurvaSpeedRun_;
    bool srEntradaFinal_;
    uint8_t passoSpeedRun_;
    uint8_t linhaSr_;
    uint8_t colunaSr_;
    Direcao orientacaoSr_;
    uint16_t deslocamentosSr_;
    uint32_t ultimoTempoExploracaoMs_;
    uint32_t ultimoTempoSpeedRunMs_;
};

#endif
