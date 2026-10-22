#ifndef DETECTOR_TRAVAMENTO_H
#define DETECTOR_TRAVAMENTO_H

// HU-18 / RF-SFT11 — Detecção pura de travamento mecânico (stall) das rodas.
//
// Regra de negócio (doc 4.4, seção 5.3 e Guia do MVP, seção 5):
//  - Com comando de tração ativo e sem rotação em qualquer uma das rodas (nenhum
//    pulso de encoder na roda esquerda ou na roda direita) por pelo menos 1000 ms
//    (TEMPO_TRAVAMENTO_MS), o travamento é confirmado.
//  - O monitoramento de cada roda é independente: se apenas uma roda travar
//    (por exemplo, bloqueada contra uma parede enquanto a outra gira livre),
//    o travamento ainda assim é confirmado (não mascarado pelos pulsos da outra).
//  - Quando a tração liga com o robô parado, o último pulso pode ser antigo.
//    O tempo sem pulsos é contado a partir do maior instante entre o último pulso
//    da respectiva roda e o momento em que a tração foi ligada, evitando falso
//    disparo na largada.
//
// Nota de calibração (motores / zona morta):
//  - Caso o controle envie comandos de velocidade com PWM abaixo da zona morta dos
//    motores (~10% a 15% do ciclo ativo, conforme doc 4.3), as rodas não vencerão o
//    atrito estático, nenhum pulso de encoder será gerado e o detector acusará
//    travamento após 1,0 s de tração ativa. O laço de controle de velocidade deve
//    garantir que qualquer comando de avanço com tração ativa respeite a velocidade
//    mínima operacional do robô.
//
// Módulo puro de cabeçalho, sem dependência de Arduino.h nem de hardware,
// testável nativamente com Unity.

#include <stdint.h>

constexpr uint32_t TEMPO_TRAVAMENTO_MS = 1000UL;
constexpr uint32_t PERIODO_SEGURANCA_MS = 50UL;
constexpr uint32_t LIMITE_CORTE_TRAVAMENTO_MS = 1200UL;

enum RodaTravada : uint8_t {
    RODA_NENHUMA = 0,
    RODA_ESQUERDA = 1,
    RODA_DIREITA = 2,
    AMBAS_RODAS = 3
};

class DetectorTravamento {
public:
    explicit DetectorTravamento(uint32_t limiarMs = TEMPO_TRAVAMENTO_MS)
        : limiarMs_(limiarMs),
          inicioTracaoMs_(0),
          tempoSemPulsoEsqMs_(0),
          tempoSemPulsoDirMs_(0),
          tempoSemPulsoMs_(0),
          tracaoEstavaAtiva_(false),
          rodaTravada_(RODA_NENHUMA),
          travado_(false) {}

    // Avalia se houve travamento monitorando os pulsos de cada roda independentemente.
    // Devolve true quando a tração está ativa e qualquer uma das rodas atinge `limiarMs_`
    // sem novos pulsos.
    bool verificar(bool tracaoAtiva, uint32_t ultimoPulsoEsqMs, uint32_t ultimoPulsoDirMs, uint32_t agoraMs) {
        if (!tracaoAtiva) {
            tracaoEstavaAtiva_ = false;
            tempoSemPulsoEsqMs_ = 0;
            tempoSemPulsoDirMs_ = 0;
            tempoSemPulsoMs_ = 0;
            rodaTravada_ = RODA_NENHUMA;
            travado_ = false;
            return false;
        }

        if (!tracaoEstavaAtiva_) {
            tracaoEstavaAtiva_ = true;
            inicioTracaoMs_ = agoraMs;
        }

        const uint32_t desdeInicioTracaoMs = (uint32_t)(agoraMs - inicioTracaoMs_);

        const int32_t diffPulsoEsq = (int32_t)(agoraMs - ultimoPulsoEsqMs);
        const uint32_t desdePulsoEsqMs = (diffPulsoEsq > 0) ? (uint32_t)diffPulsoEsq : 0UL;

        const int32_t diffPulsoDir = (int32_t)(agoraMs - ultimoPulsoDirMs);
        const uint32_t desdePulsoDirMs = (diffPulsoDir > 0) ? (uint32_t)diffPulsoDir : 0UL;

        // Equivale a contar a partir de max(ultimoPulso, inicioTracaoMs_),
        // preservando aritmética modular de 32 bits e protegendo a largada.
        tempoSemPulsoEsqMs_ = (desdePulsoEsqMs < desdeInicioTracaoMs)
                                  ? desdePulsoEsqMs
                                  : desdeInicioTracaoMs;

        tempoSemPulsoDirMs_ = (desdePulsoDirMs < desdeInicioTracaoMs)
                                  ? desdePulsoDirMs
                                  : desdeInicioTracaoMs;

        tempoSemPulsoMs_ = (tempoSemPulsoEsqMs_ > tempoSemPulsoDirMs_)
                               ? tempoSemPulsoEsqMs_
                               : tempoSemPulsoDirMs_;

        const bool esqTravada = (tempoSemPulsoEsqMs_ >= limiarMs_);
        const bool dirTravada = (tempoSemPulsoDirMs_ >= limiarMs_);

        if (esqTravada && dirTravada) {
            rodaTravada_ = AMBAS_RODAS;
        } else if (esqTravada) {
            rodaTravada_ = RODA_ESQUERDA;
        } else if (dirTravada) {
            rodaTravada_ = RODA_DIREITA;
        } else {
            rodaTravada_ = RODA_NENHUMA;
        }

        travado_ = (rodaTravada_ != RODA_NENHUMA);
        return travado_;
    }

    // Sobrecarga de conveniência para quando houver uma única fonte agregada de pulsos.
    bool verificar(bool tracaoAtiva, uint32_t ultimoPulsoMs, uint32_t agoraMs) {
        return verificar(tracaoAtiva, ultimoPulsoMs, ultimoPulsoMs, agoraMs);
    }

    // Reinicia o histórico do detector para uma nova tentativa/corrida.
    void reiniciar() {
        inicioTracaoMs_ = 0;
        tempoSemPulsoEsqMs_ = 0;
        tempoSemPulsoDirMs_ = 0;
        tempoSemPulsoMs_ = 0;
        tracaoEstavaAtiva_ = false;
        rodaTravada_ = RODA_NENHUMA;
        travado_ = false;
    }

    uint32_t tempoSemPulsoMs() const { return tempoSemPulsoMs_; }
    uint32_t tempoSemPulsoEsqMs() const { return tempoSemPulsoEsqMs_; }
    uint32_t tempoSemPulsoDirMs() const { return tempoSemPulsoDirMs_; }
    uint32_t limiarMs() const { return limiarMs_; }
    RodaTravada rodaTravada() const { return rodaTravada_; }
    bool travamentoDetectado() const { return travado_; }

private:
    uint32_t limiarMs_;
    uint32_t inicioTracaoMs_;
    uint32_t tempoSemPulsoEsqMs_;
    uint32_t tempoSemPulsoDirMs_;
    uint32_t tempoSemPulsoMs_;
    bool tracaoEstavaAtiva_;
    RodaTravada rodaTravada_;
    bool travado_;
};

#endif  // DETECTOR_TRAVAMENTO_H
