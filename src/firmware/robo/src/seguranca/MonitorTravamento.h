#ifndef MONITOR_TRAVAMENTO_H
#define MONITOR_TRAVAMENTO_H

// HU-18 / RF-SFT11 — Monitoramento periódico e corte de tração por travamento (stall).
//
// Aplica a regra pura do DetectorTravamento a cada ciclo de segurança (50 ms) e age
// imediatamente ao confirmar o bloqueio mecânico (em ambas ou em qualquer uma das rodas):
//  1. Corta a potência dos motores (ITracao::parar) — garantindo reação entre 1000 e 1050 ms
//     (abaixo do teto de 1200 ms do RF-SFT11);
//  2. Congela o cronômetro da prova (ICronometro::congelar, opcional);
//  3. Aciona o indicador visual local em estado TRAVADO (ISinalizacaoEstado, opcional);
//  4. Trava o estado em TRAVADO (estado terminal na tentativa).
//
// Nota de calibração (zona morta):
//  - O laço de controle do robô deve calibrar e respeitar o patamar mínimo de velocidade
//    operacional, prevenindo acionamento contínuo abaixo da zona morta dos motores.
//
// Na integração no main.cpp:
//   monitorTravamento.verificar(tempoAtual);
//   if (monitorTravamento.travado()) estadoAtual = TRAVADO;

#include <stdint.h>

#include "DetectorTravamento.h"
#include "../parada/ParadaObjetivo.h"  // ITracao, ICronometro
#include "TelemetriaPacket.h"          // EstadoRobo (TRAVADO)

// Fonte de estado da tração e parada dos motores (compatível com Motores da HU-01).
class IFonteTracao : public ITracao {
public:
    virtual ~IFonteTracao() {}
    virtual bool tracaoAtiva() const = 0;
};

// Fonte dos instantes dos últimos pulsos de encoder (compatível com Odometria da HU-02).
class IFontePulsos {
public:
    virtual ~IFontePulsos() {}
    virtual uint32_t getUltimoPulsoMs() const { return 0; }
    virtual uint32_t getUltimoPulsoEsqMs() const { return getUltimoPulsoMs(); }
    virtual uint32_t getUltimoPulsoDirMs() const { return getUltimoPulsoMs(); }
};

// Indicador local de estado (LED RGB / buzzer da HU-05).
class ISinalizacaoEstado {
public:
    virtual ~ISinalizacaoEstado() {}
    virtual void mostrarEstado(EstadoRobo estado) = 0;
};

class MonitorTravamento {
public:
    // Construtor com fontes de tração e odometria injetadas (permite verificar(agoraMs)).
    MonitorTravamento(IFonteTracao& tracao,
                      IFontePulsos& odometria,
                      ICronometro* cronometro = nullptr,
                      ISinalizacaoEstado* sinalizacao = nullptr,
                      uint32_t limiarMs = TEMPO_TRAVAMENTO_MS)
        : tracao_(tracao),
          fonteTracao_(&tracao),
          odometria_(&odometria),
          cronometro_(cronometro),
          sinalizacao_(sinalizacao),
          detector_(limiarMs),
          travado_(false),
          instanteCorteMs_(0) {}

    // Construtor enxuto (no mesmo estilo de ParadaObjetivo), recebendo leituras em verificar().
    explicit MonitorTravamento(ITracao& tracao,
                               ICronometro* cronometro = nullptr,
                               ISinalizacaoEstado* sinalizacao = nullptr,
                               uint32_t limiarMs = TEMPO_TRAVAMENTO_MS)
        : tracao_(tracao),
          fonteTracao_(nullptr),
          odometria_(nullptr),
          cronometro_(cronometro),
          sinalizacao_(sinalizacao),
          detector_(limiarMs),
          travado_(false),
          instanteCorteMs_(0) {}

    // Verificação periódica (chamada a cada 50 ms) lendo das fontes injetadas.
    bool verificar(uint32_t agoraMs) {
        if (travado_) return true;
        if (fonteTracao_ == nullptr || odometria_ == nullptr) return false;
        return verificar(fonteTracao_->tracaoAtiva(),
                         odometria_->getUltimoPulsoEsqMs(),
                         odometria_->getUltimoPulsoDirMs(),
                         agoraMs);
    }

    // Verificação periódica com pulsos de cada roda monitorados separadamente.
    // Na primeira confirmação de travamento: corta os motores, congela o cronômetro e
    // sinaliza TRAVADO. Permanece em estado terminal até iniciarProva().
    bool verificar(bool tracaoAtiva, uint32_t ultimoPulsoEsqMs, uint32_t ultimoPulsoDirMs, uint32_t agoraMs) {
        if (travado_) return true;

        if (!detector_.verificar(tracaoAtiva, ultimoPulsoEsqMs, ultimoPulsoDirMs, agoraMs)) {
            return false;
        }

        // 1. Corte imediato da potência dos motores (prioridade de segurança)
        tracao_.parar();
        instanteCorteMs_ = agoraMs;

        // 2. Congelamento do cronômetro da prova
        if (cronometro_ != nullptr) {
            cronometro_->congelar();
        }

        // 3. Sinalização local (LED vermelho em TRAVADO)
        if (sinalizacao_ != nullptr) {
            sinalizacao_->mostrarEstado(TRAVADO);
        }

        travado_ = true;
        return true;
    }

    // Sobrecarga de conveniência para fonte agregada de pulsos.
    bool verificar(bool tracaoAtiva, uint32_t ultimoPulsoMs, uint32_t agoraMs) {
        return verificar(tracaoAtiva, ultimoPulsoMs, ultimoPulsoMs, agoraMs);
    }

    // Reinicia o monitor para uma nova tentativa/corrida.
    void iniciarProva() {
        travado_ = false;
        instanteCorteMs_ = 0;
        detector_.reiniciar();
    }

    bool travado() const { return travado_; }
    uint32_t instanteCorteMs() const { return instanteCorteMs_; }
    uint32_t tempoSemPulsoMs() const { return detector_.tempoSemPulsoMs(); }
    uint32_t tempoSemPulsoEsqMs() const { return detector_.tempoSemPulsoEsqMs(); }
    uint32_t tempoSemPulsoDirMs() const { return detector_.tempoSemPulsoDirMs(); }
    RodaTravada rodaTravada() const { return detector_.rodaTravada(); }
    EstadoRobo estadoRobo() const { return travado_ ? TRAVADO : MAPEANDO; }

private:
    ITracao& tracao_;
    IFonteTracao* fonteTracao_;
    IFontePulsos* odometria_;
    ICronometro* cronometro_;
    ISinalizacaoEstado* sinalizacao_;
    DetectorTravamento detector_;
    bool travado_;
    uint32_t instanteCorteMs_;
};

#endif  // MONITOR_TRAVAMENTO_H
