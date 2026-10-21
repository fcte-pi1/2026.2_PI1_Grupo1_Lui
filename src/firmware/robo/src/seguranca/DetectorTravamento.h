#ifndef DETECTOR_TRAVAMENTO_H
#define DETECTOR_TRAVAMENTO_H

// HU-18 / RF-SFT11 — Detecção pura de travamento mecânico (stall) das rodas.
//
// Regra de negócio (doc 4.4, seção 5.3 e Guia do MVP, seção 5):
//  - Com comando de tração ativo e sem rotação das rodas (nenhum pulso de encoder)
//    por pelo menos 1000 ms (TEMPO_TRAVAMENTO_MS), o travamento é confirmado.
//  - Quando a tração liga com o robô parado, o último pulso pode ser antigo.
//    O tempo sem pulsos é contado a partir do maior instante entre o último pulso
//    e o momento em que a tração foi ligada, evitando falso disparo na largada.
//
// Módulo puro de cabeçalho, sem dependência de Arduino.h nem de hardware,
// testável nativamente com Unity.

#include <stdint.h>

constexpr uint32_t TEMPO_TRAVAMENTO_MS = 1000UL;
constexpr uint32_t PERIODO_SEGURANCA_MS = 50UL;
constexpr uint32_t LIMITE_CORTE_TRAVAMENTO_MS = 1200UL;

class DetectorTravamento {
public:
    explicit DetectorTravamento(uint32_t limiarMs = TEMPO_TRAVAMENTO_MS)
        : limiarMs_(limiarMs),
          inicioTracaoMs_(0),
          tempoSemPulsoMs_(0),
          tracaoEstavaAtiva_(false),
          travado_(false) {}

    // Avalia se houve travamento no instante `agoraMs`.
    // Devolve true quando a tração está ativa e o tempo sem pulsos atinge `limiarMs_`.
    bool verificar(bool tracaoAtiva, uint32_t ultimoPulsoMs, uint32_t agoraMs) {
        if (!tracaoAtiva) {
            tracaoEstavaAtiva_ = false;
            tempoSemPulsoMs_ = 0;
            travado_ = false;
            return false;
        }

        if (!tracaoEstavaAtiva_) {
            tracaoEstavaAtiva_ = true;
            inicioTracaoMs_ = agoraMs;
        }

        const uint32_t desdeInicioTracaoMs = (uint32_t)(agoraMs - inicioTracaoMs_);
        const int32_t diffPulso = (int32_t)(agoraMs - ultimoPulsoMs);
        const uint32_t desdeUltimoPulsoMs = (diffPulso > 0) ? (uint32_t)diffPulso : 0UL;

        // Equivale a contar a partir de max(ultimoPulsoMs, inicioTracaoMs_),
        // preservando aritmética modular de 32 bits.
        tempoSemPulsoMs_ = (desdeUltimoPulsoMs < desdeInicioTracaoMs)
                               ? desdeUltimoPulsoMs
                               : desdeInicioTracaoMs;

        travado_ = (tempoSemPulsoMs_ >= limiarMs_);
        return travado_;
    }

    // Reinicia o histórico do detector para uma nova tentativa/corrida.
    void reiniciar() {
        inicioTracaoMs_ = 0;
        tempoSemPulsoMs_ = 0;
        tracaoEstavaAtiva_ = false;
        travado_ = false;
    }

    uint32_t tempoSemPulsoMs() const { return tempoSemPulsoMs_; }
    uint32_t limiarMs() const { return limiarMs_; }
    bool travamentoDetectado() const { return travado_; }

private:
    uint32_t limiarMs_;
    uint32_t inicioTracaoMs_;
    uint32_t tempoSemPulsoMs_;
    bool tracaoEstavaAtiva_;
    bool travado_;
};

#endif  // DETECTOR_TRAVAMENTO_H
