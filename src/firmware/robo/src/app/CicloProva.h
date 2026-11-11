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
//
// Ordem dentro do tick (3.4): entradas (botão, encoders na Odometria, ToF), segurança
// (50 ms: travamento e tempo limite), máquina da prova (o executor anda no ritmo de 10 ms),
// telemetria (1 s) e LED (50 ms). Nada espera: nenhuma chamada bloqueia.
//
// Estados publicados são os do contrato (EstadoRobo) e só esta classe os escreve (3.6).
// Dentro de MAPEANDO há subfases internas: ler paredes, decidir, girar, avançar.
//
// Fontes únicas (3.6): orientação e posição vêm do Navegador; a chegada é detectada pelo
// Navegador e executada pelo ParadaObjetivo, e só parada.concluido() leva a CONCLUIDO (B2).

enum SubfaseMapeamento : uint8_t {
    SUBFASE_LENDO_PAREDES,
    SUBFASE_GIRANDO,
    SUBFASE_AVANCANDO
};

// Hardware e peças compartilhadas com o executor. radio e registro podem ser nulos (prova
// sem telemetria, RNF-SFT01; sem log).
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
    static const uint32_t PERIODO_LED_MS = 50;   // 3.4 pede 100 ms; 50 ms deixa as piscadas de 150 ms regulares
    static const uint32_t ESPERA_LEITURA_PAREDES_MS = 150;  // 3.7: sem leitura válida, falha
    static const uint32_t FREIO_TERMINAL_MS = 300;         // 3.3: freio e depois roda livre

    explicit CicloProva(const DependenciasProva& d)
        : d_(d),
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
          inicioLeituraMs_(0),
          terminalDesdeMs_(0),
          rodasSoltas_(false),
          causaUltimaFalha_(FALHA_NENHUMA),
          agoraMs_(0),
          bateriaMv_(0),
          pacotesEnviados_(0),
          pacotesInvalidos_(0) {
        memset(&ultimoPacote_, 0, sizeof(ultimoPacote_));
    }

    // Chamar uma vez no setup(), com os drivers já iniciados.
    void iniciar(uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        mapa_.setGeometria(LAB_4X4);
        d_.motores.liberar();  // roda livre
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
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
            case SPEED_RUN:  // ainda não existe (RF-SFT09); nunca é alcançado
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
            d_.led.atualizar(agoraMs, estado_ == AGUARDANDO && botao_.largadaArmada());
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

private:
    // Congela o cronômetro no instante do tick em andamento. Faz o papel do
    // AdaptadorCronometro sem precisar de uma função global de relógio (R10).
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

    // Leitura da bateria com a interrupção do encoder esquerdo pausada (GPIO 36 e 39,
    // pendência 9 do desenho). Custa no máximo um ou dois pulsos reais por segundo.
    uint16_t lerBateria() {
        d_.encoders.pausarEsquerdo();
        const uint16_t mv = d_.bateria.lerMilivolts();
        d_.encoders.retomarEsquerdo();
        bateriaMv_ = mv;
        return mv;
    }

    // Corte por decisão do laço (tempo, falhas): trava os motores, para o executor e
    // congela o cronômetro.
    void cortar(uint32_t agoraMs) {
        d_.motores.parar();
        d_.executor.abortar();
        cronometro_.congelar(agoraMs);
    }

    void entrarTerminal(EstadoRobo e, uint32_t agoraMs) {
        terminalDesdeMs_ = agoraMs;
        rodasSoltas_ = false;
        mudarEstado(e);
    }

    // ------------------------------------------------------------------ segurança

    void verificarSeguranca(uint32_t agoraMs) {
        if (estado_ != MAPEANDO) return;
        if (monitor_.verificar(agoraMs)) {  // o monitor já travou os motores e o cronômetro
            d_.executor.abortar();
            registrar("Travamento: tracao cortada.");
            entrarTerminal(TRAVADO, agoraMs);
            return;
        }
        if (cronometro_.decorridoMs(agoraMs) >= TELEMETRIA_TIMESTAMP_MAX_MS) {  // B8
            cortar(agoraMs);
            registrar("Tempo limite de 10 min.");
            entrarTerminal(TIMEOUT, agoraMs);
        }
    }

    // ------------------------------------------------------------------ AGUARDANDO

    void tratarAguardando(const EventoBotao& evento, uint32_t agoraMs) {
        if (evento.tipo == BOTAO_LARGADA) {
            darLargada(agoraMs);
        } else if (evento.tipo == BOTAO_TOQUES && evento.toques == 3) {
            proximaGeometria(agoraMs);
        }
        // 1 e 2 toques ficam reservados ao autoteste (doc 4.3).
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
        d_.led.piscar(corLed(0, 255, 255), piscadas, agoraMs);
        registrar(geo == LAB_4X4 ? "Geometria 4x4." : geo == LAB_8X4 ? "Geometria 8x4." : "Geometria 12x4.");
    }

    void darLargada(uint32_t agoraMs) {
        // Bateria medida com os motores parados (R4). Abaixo do mínimo, a largada é recusada.
        if (!bateriaPermiteLargada(lerBateria())) {
            d_.led.piscar(corLed(255, 0, 0), 3, agoraMs);
            registrar("Largada recusada: bateria abaixo do minimo.");
            return;
        }
        // Ordem da seção 3.3.
        const GeometriaLabirinto geo = mapa_.getGeometria();
        mapa_.setGeometria(geo);  // limpa o mapa
        parada_.configurarObjetivoPorGeometria(geo);
        parada_.iniciarProva();
        monitor_.iniciarProva();
        d_.motores.liberar();
        navegador_.iniciar();
        cronometro_.iniciar(agoraMs);
        int32_t esq, dir;
        d_.encoders.ler(esq, dir);
        d_.odometria.reiniciar(esq, dir, agoraMs);
        classificador_.reiniciar();
        causaUltimaFalha_ = FALHA_NENHUMA;
        mudarEstado(MAPEANDO);
        registrar("Largada: MAPEANDO.");
        comecarLeitura(agoraMs);
    }

    // ------------------------------------------------------------------ MAPEANDO

    void comecarLeitura(uint32_t agoraMs) {
        subfase_ = SUBFASE_LENDO_PAREDES;
        inicioLeituraMs_ = agoraMs;
    }

    // Os três sensores têm amostra feita depois de o robô parar nesta célula.
    bool amostrasDaCelula() {
        return amostraDepois(d_.tof.esquerdo()) && amostraDepois(d_.tof.frontal()) &&
               amostraDepois(d_.tof.direito());
    }

    bool amostraDepois(const LeituraToF& l) const {
        return l.temAmostra() && (int32_t)(l.instanteMs() - inicioLeituraMs_) >= 0;
    }

    void tratarMapeando(uint32_t agoraMs, bool tickControle) {
        switch (subfase_) {
            case SUBFASE_LENDO_PAREDES:
                lerParedes(agoraMs);
                break;
            case SUBFASE_GIRANDO:
                if (tickControle) acompanharGiro(agoraMs);
                break;
            case SUBFASE_AVANCANDO:
                if (tickControle) acompanharAvanco(agoraMs);
                break;
        }
    }

    void lerParedes(uint32_t agoraMs) {
        if (amostrasDaCelula()) {
            classificador_.atualizar(amostraDasLeituras(d_.tof.esquerdo(), d_.tof.frontal(),
                                                        d_.tof.direito(), agoraMs));
            bool frente, esquerda, direita;
            if (classificador_.lerParedes(frente, esquerda, direita)) {
                navegador_.observar(frente, esquerda, direita);
                decidir(agoraMs);
                return;
            }
        }
        if ((uint32_t)(agoraMs - inicioLeituraMs_) >= ESPERA_LEITURA_PAREDES_MS) {
            // 3.7: sem leitura válida em 150 ms, falha e nova tentativa (como o passo()).
            causaUltimaFalha_ = FALHA_NENHUMA;  // falha de leitura, não de movimento
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
        if (!d_.executor.ocioso()) d_.executor.abortar();  // não deve acontecer
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
            d_.executor.iniciarAvanco(agoraMs);
            subfase_ = SUBFASE_AVANCANDO;
        } else if (r == MOV_FALHOU) {
            falhaMovimento(agoraMs);
        }
    }

    void acompanharAvanco(uint32_t agoraMs) {
        const ResultadoMovimento r = d_.executor.atualizar(agoraMs);
        if (r == MOV_CONCLUIDO) {
            navegador_.confirmarAvanco();
            // B2: o Navegador detecta a célula; o ParadaObjetivo para, congela e autoriza
            // CONCLUIDO.
            parada_.atualizar(navegador_.linha(), navegador_.coluna());
            if (parada_.concluido()) {
                registrar("Objetivo alcancado: CONCLUIDO.");
                entrarTerminal(CONCLUIDO, agoraMs);
                return;
            }
            comecarLeitura(agoraMs);
        } else if (r == MOV_FALHOU) {
            falhaMovimento(agoraMs);
        }
    }

    void falhaMovimento(uint32_t agoraMs) {
        const MotivoFalhaMovimento motivo = d_.executor.motivoUltimaFalha();
        if (motivo == FALHA_TRACAO_CORTADA) {
            // Trinco fechado por fora sem passar pela segurança (não deve acontecer): para
            // tudo como travamento.
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
        comecarLeitura(agoraMs);
    }

    // Terceira falha seguida (3.7): COLISAO se a última foi emergência frontal, senão TRAVADO.
    void falhaTerminal(uint32_t agoraMs) {
        cortar(agoraMs);
        entrarTerminal(causaUltimaFalha_ == FALHA_EMERGENCIA_FRONTAL ? COLISAO : TRAVADO, agoraMs);
    }

    void semMovimento(uint32_t agoraMs) {
        if (navegador_.chegou()) {
            // O Navegador chegou e o ParadaObjetivo não (B2: não deve acontecer).
            registrar("Chegada sem parada no objetivo.");
        } else if (navegador_.estado() == NAV_INALCANCAVEL) {
            registrar("Destino inalcancavel.");
        } else {
            registrar("Navegacao encerrada sem chegada.");
        }
        cortar(agoraMs);
        entrarTerminal(TRAVADO, agoraMs);
    }

    // ------------------------------------------------------------------ terminais

    void tratarTerminal(const EventoBotao& evento, uint32_t agoraMs) {
        if (!rodasSoltas_ && (uint32_t)(agoraMs - terminalDesdeMs_) >= FREIO_TERMINAL_MS) {
            d_.motores.soltar();  // o trinco continua fechado
            rodasSoltas_ = true;
        }
        // R9: segurar 2 s e soltar volta a AGUARDANDO para uma nova tentativa.
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
        causaUltimaFalha_ = FALHA_NENHUMA;
        mudarEstado(AGUARDANDO);
        registrar("Nova tentativa: AGUARDANDO.");
    }

    // ------------------------------------------------------------------ telemetria

    static int16_t saturarInt16(float v) {
        if (v > 32767.0f) return 32767;
        if (v < -32768.0f) return -32768;
        return (int16_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
    }

    // Origem de cada campo: seção 3.8 do desenho.
    TelemetriaPacket montarTelemetria(uint32_t agoraMs) {
        TelemetriaPacket p;
        memset(&p, 0, sizeof(p));
        p.timestamp_ms = cronometro_.decorridoMs(agoraMs);
        p.labirinto_id = (uint8_t)mapa_.getGeometria();
        p.status_id = (uint8_t)estado_;
        p.desafio_cumprido = parada_.desafioCumprido() ? 1 : 0;
        const uint16_t mv = lerBateria();
        p.bateria_mv = mv;
        p.bateria_pct = porcentagemBateria(mv);
        p.velocidade_atual = saturarInt16(d_.odometria.velocidadeMmS());
        p.velocidade_media = velocidadeMediaMmS(navegador_.deslocamentos(), p.timestamp_ms);
        if (estado_ == AGUARDANDO) {
            // Antes da largada (ou depois de reiniciar): o robô está em A1, virado para o leste.
            p.linha = 0;
            p.coluna = 0;
            p.orientacao = (uint8_t)DIR_LESTE;
            p.paredes = mapa_.getParedes(0, 0);
        } else {
            navegador_.preencherPosicao(p);
        }
        return p;
    }

    void enviarTelemetria(uint32_t agoraMs) {
        if (d_.radio == nullptr) return;  // prova sem telemetria (RNF-SFT01)
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
    // A ordem importa: o Navegador usa o Mapa, o ParadaObjetivo e o monitor usam o
    // congelador, que usa o cronômetro e o relógio do tick.

    DependenciasProva d_;
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
    uint32_t inicioLeituraMs_;
    uint32_t terminalDesdeMs_;
    bool rodasSoltas_;
    MotivoFalhaMovimento causaUltimaFalha_;
    uint32_t agoraMs_;
    uint16_t bateriaMv_;
    TelemetriaPacket ultimoPacote_;
    uint32_t pacotesEnviados_;
    uint32_t pacotesInvalidos_;
};

#endif
