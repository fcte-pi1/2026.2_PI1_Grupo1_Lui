#ifndef TEMPORIZADOR_PERIODICO_H
#define TEMPORIZADOR_PERIODICO_H

#include <stdint.h>

// Temporizador não bloqueante, sem deriva e seguro contra overflow de millis()
// (aritmética unsigned de 32 bits). Se o laço principal atrasar mais que um período
// (ex.: piscadas bloqueantes do LED), ressincroniza em vez de disparar em rajada.
class TemporizadorPeriodico {
public:
    explicit TemporizadorPeriodico(uint32_t periodoMs)
        : periodo_(periodoMs), ultimo_(0), iniciado_(false) {}

    // Retorna true quando o período venceu. O primeiro disparo é imediato.
    bool vencido(uint32_t agoraMs) {
        if (!iniciado_) {
            iniciado_ = true;
            ultimo_ = agoraMs;
            return true;
        }
        if ((uint32_t)(agoraMs - ultimo_) < periodo_) return false;
        ultimo_ += periodo_;
        if ((uint32_t)(agoraMs - ultimo_) >= periodo_) ultimo_ = agoraMs;
        return true;
    }

private:
    uint32_t periodo_;
    uint32_t ultimo_;
    bool iniciado_;
};

#endif
