#ifndef CRONOMETRO_H
#define CRONOMETRO_H

#include <stdint.h>

// Cronômetro da prova, sem dependência do Arduino: conta o tempo desde a largada,
// que é o campo 'timestamp_ms' da telemetria. Antes da largada vale 0. O valor é
// limitado a 'limiteMs' para não sair da faixa aceita pelo contrato de dados
// (0 a 600000 ms); pacotes fora dela seriam descartados pelo gateway.
// A subtração unsigned de 32 bits continua correta quando millis() volta a zero.
class Cronometro {
public:
    explicit Cronometro(uint32_t limiteMs)
        : limiteMs_(limiteMs), inicioMs_(0), emAndamento_(false) {}

    // Marca o instante da largada. Chamar de novo reinicia a contagem.
    void iniciar(uint32_t agoraMs) {
        inicioMs_ = agoraMs;
        emAndamento_ = true;
    }

    // Volta ao estado anterior à largada.
    void zerar() {
        inicioMs_ = 0;
        emAndamento_ = false;
    }

    bool emAndamento() const { return emAndamento_; }

    uint32_t decorridoMs(uint32_t agoraMs) const {
        if (!emAndamento_) return 0;
        const uint32_t decorrido = agoraMs - inicioMs_;
        return decorrido < limiteMs_ ? decorrido : limiteMs_;
    }

private:
    uint32_t limiteMs_;
    uint32_t inicioMs_;
    bool emAndamento_;
};

#endif
