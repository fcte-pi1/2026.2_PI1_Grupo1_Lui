#ifndef FILTRO_TOF_H
#define FILTRO_TOF_H

#include <stdint.h>
#include "../hardware/Pinos.h"

// Pré-processamento dos sensores VL53L0X (sem dependência do Arduino, testável no host):
//  1. Decodificação de RESULT_RANGE_STATUS (bits 6:3): distingue medição válida (11) de
//     "sem alvo / corredor longo" (sigma/sinal/fase fracos: 6, 7, 8, 9, 12..15) e de
//     falha de hardware (1..3).
//  2. Aplicação do offset individual de calibração (mm que o sensor lê acima do real).
//  3. Filtro de mediana de 3 amostras para eliminar picos isolados sem atrasar a resposta.

// Resultado classificado de uma amostra bruta do VL53L0X:
//  - 1..TOF_DIST_MAX_VALIDA_MM: distância válida em mm (já com offset descontado);
//  - TOF_LEITURA_INVALIDA_MM (8190): sensor respondeu, mas sem alvo ao alcance (aberto);
//  - 0: erro de hardware / medição inválida (não conta como passagem livre).
inline uint16_t aplicarOffsetToF(uint16_t brutaMm, int16_t offsetMm) {
    const int32_t corrigida = (int32_t)brutaMm - (int32_t)offsetMm;
    if (corrigida < 1) return 1;
    if (corrigida > 65535) return 65535;
    return (uint16_t)corrigida;
}

inline uint16_t classificarLeituraVL53(uint8_t rawStatusByte, uint16_t brutaMm, int16_t offsetMm) {
    const uint8_t interno = (uint8_t)((rawStatusByte & 0x78u) >> 3);

    // 11 = Range Valid no registrador 0x14 do VL53L0X.
    if (interno == 11) {
        if (brutaMm == 0) return 0;
        if (brutaMm >= TOF_LEITURA_INVALIDA_MM || brutaMm > TOF_DIST_MAX_VALIDA_MM) {
            return TOF_LEITURA_INVALIDA_MM;
        }
        return aplicarOffsetToF(brutaMm, offsetMm);
    }

    // 4 ou 5 = Min Range Fail (alvo encostado/perto demais): nunca devolver "sem alvo",
    // pois isso faria o classificador achar que o caminho está aberto!
    if (interno == 4 || interno == 5) {
        return TOF_DIST_MIN_VALIDA_MM;
    }

    // 6, 7, 8, 9 (sigma/signal fail) ou 0, 10, 12..15 (phase fail / out-of-range):
    // em corredor aberto sem parede ao alcance o VL53L0X devolve estes estados.
    if (interno == 6 || interno == 7 || interno == 8 || interno == 9 ||
        interno == 0 || interno >= 10) {
        return TOF_LEITURA_INVALIDA_MM;
    }

    // 1, 2, 3 = Hardware fail: leitura inválida (erro de sensor).
    return 0;
}

// Converte duração do pulso ECHO (us) do sensor ultrassônico frontal (HC-SR04) em mm
// classificados para LeituraToF.
inline uint16_t classificarLeituraUltrassonico(uint32_t duracaoUs, int16_t offsetMm) {
    if (duracaoUs == 0 || duracaoUs > ULTRASSONICO_TIMEOUT_US) {
        return TOF_LEITURA_INVALIDA_MM;
    }
    const uint32_t mmBruto = (duracaoUs * 343UL + 1000UL) / 2000UL;
    if (mmBruto > TOF_DIST_MAX_VALIDA_MM) {
        return TOF_LEITURA_INVALIDA_MM;
    }
    const uint16_t baseMm = (mmBruto < ULTRASSONICO_DIST_MIN_MM)
                                ? ULTRASSONICO_DIST_MIN_MM
                                : (uint16_t)mmBruto;
    return aplicarOffsetToF(baseMm, offsetMm);
}

// Filtro de mediana de 3 amostras válidas. Descarta picos isolados e reinicia o histórico
// após 3 amostras seguidas sem alvo ou com erro, evitando arrastar valores antigos para
// dentro de uma abertura lateral.
class FiltroMedianaToF {
public:
    FiltroMedianaToF() { reiniciar(); }

    void reiniciar() {
        janela_[0] = janela_[1] = janela_[2] = 0;
        indice_ = 0;
        quantidade_ = 0;
        invalidasSeguidas_ = 0;
    }

    uint16_t atualizar(uint16_t amostraMm) {
        if (amostraMm == 0 || amostraMm >= TOF_LEITURA_INVALIDA_MM) {
            if (++invalidasSeguidas_ >= 3) reiniciar();
            return amostraMm;
        }
        invalidasSeguidas_ = 0;
        janela_[indice_] = amostraMm;
        indice_ = (uint8_t)((indice_ + 1) % 3);
        if (quantidade_ < 3) quantidade_++;

        if (quantidade_ == 1) return janela_[0];
        if (quantidade_ == 2) return (uint16_t)(((uint32_t)janela_[0] + janela_[1] + 1) / 2);

        const uint16_t a = janela_[0];
        const uint16_t b = janela_[1];
        const uint16_t c = janela_[2];
        const uint16_t menor = a < b ? a : b;
        const uint16_t maior = a < b ? b : a;
        const uint16_t meio = c < maior ? c : maior;
        return menor > meio ? menor : meio;
    }

private:
    uint16_t janela_[3];
    uint8_t indice_;
    uint8_t quantidade_;
    uint8_t invalidasSeguidas_;
};

#endif
