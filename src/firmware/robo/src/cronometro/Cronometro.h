#ifndef CRONOMETRO_H
#define CRONOMETRO_H

#include <stdint.h>

// Cronômetro da prova, sem dependência do Arduino: conta o tempo desde a largada,
// que é o campo 'timestamp_ms' da telemetria. Antes da largada vale 0. O valor é
// limitado a 'limiteMs' para não sair da faixa aceita pelo contrato de dados
// (0 a 600000 ms); pacotes fora dela seriam descartados pelo gateway.
// A subtração unsigned de 32 bits continua correta quando millis() volta a zero.
// Ao chegar no objetivo (HU-07), congelar() fixa o tempo da prova.
class Cronometro {
public:
    explicit Cronometro(uint32_t limiteMs)
        : limiteMs_(limiteMs), inicioMs_(0), iniciado_(false), congelado_(false),
          congeladoMs_(0) {}

    // Marca o instante da largada. Chamar de novo reinicia a contagem e desfaz
    // um congelamento anterior (nova prova).
    void iniciar(uint32_t agoraMs) {
        inicioMs_ = agoraMs;
        iniciado_ = true;
        congelado_ = false;
        congeladoMs_ = 0;
    }

    // Volta ao estado anterior à largada, desfazendo um congelamento.
    void zerar() {
        inicioMs_ = 0;
        iniciado_ = false;
        congelado_ = false;
        congeladoMs_ = 0;
    }

    // Fixa o tempo decorrido no instante 'agoraMs': daí em diante decorridoMs()
    // devolve sempre esse valor. Sem efeito antes da largada ou se já congelado.
    void congelar(uint32_t agoraMs) {
        if (!iniciado_ || congelado_) return;
        congeladoMs_ = decorridoMs(agoraMs);
        congelado_ = true;
    }

    // Contando: iniciado e ainda não congelado.
    bool emAndamento() const { return iniciado_ && !congelado_; }

    bool congelado() const { return congelado_; }

    uint32_t decorridoMs(uint32_t agoraMs) const {
        if (!iniciado_) return 0;
        if (congelado_) return congeladoMs_;
        const uint32_t decorrido = agoraMs - inicioMs_;
        return decorrido < limiteMs_ ? decorrido : limiteMs_;
    }

private:
    uint32_t limiteMs_;
    uint32_t inicioMs_;
    bool iniciado_;
    bool congelado_;
    uint32_t congeladoMs_;
};

#endif
