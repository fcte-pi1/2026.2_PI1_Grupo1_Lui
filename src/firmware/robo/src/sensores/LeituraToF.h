#ifndef LEITURA_TOF_H
#define LEITURA_TOF_H

#include <stdint.h>
#include "../hardware/Pinos.h"

// Estado da leitura de um sensor ToF, sem dependência do Arduino
// (docs/firmware/integracao-loop.md, achados B10 e R11).
//
// O driver (drivers/SensoresToF.h) não espera o sensor: a cada consulta, se houver
// amostra pronta, chama registrar(); se não houver, não chama nada. Esta classe decide se
// a última amostra ainda vale: uma leitura sem amostra nova há mais de validadeMs é
// inválida, como se o sensor tivesse parado de responder. Assim um sensor que morre no
// meio da prova vira "leitura inválida" em 100 ms, sem travar o laço.
//
// O VL53L0X devolve 8190 (ou mais) quando não há alvo ao alcance, e 0 em erro: as duas
// leituras são registradas como amostra (o sensor respondeu), mas não como distância
// válida.

class LeituraToF {
public:
    explicit LeituraToF(uint16_t validadeMs = TOF_VALIDADE_MS) : validadeMs_(validadeMs) {
        reiniciar();
    }

    void reiniciar() {
        distanciaMm_ = TOF_LEITURA_INVALIDA_MM;
        instanteMs_ = 0;
        temAmostra_ = false;
        nova_ = false;
    }

    // Amostra recebida do sensor no instante agoraMs.
    void registrar(uint16_t distanciaMm, uint32_t agoraMs) {
        distanciaMm_ = distanciaMm;
        instanteMs_ = agoraMs;
        temAmostra_ = true;
        nova_ = true;
    }

    // A última amostra é recente e traz uma distância dentro do alcance.
    bool valida(uint32_t agoraMs) const {
        if (!temAmostra_) return false;
        if ((uint32_t)(agoraMs - instanteMs_) > validadeMs_) return false;
        return distanciaMm_ > 0 && distanciaMm_ < TOF_LEITURA_INVALIDA_MM;
    }

    // O sensor respondeu há pouco, mesmo que sem alvo ao alcance (distância 8190): serve
    // para distinguir "não há parede" de "sensor parado".
    bool respondendo(uint32_t agoraMs) const {
        return temAmostra_ && (uint32_t)(agoraMs - instanteMs_) <= validadeMs_;
    }

    // A amostra serve para decidir se há parede: o sensor respondeu há pouco e não deu
    // erro. Sem alvo ao alcance (8190) conta, e é classificado como "não há parede"; para o
    // controle de reta, que precisa da distância, use valida().
    bool validaParaParede(uint32_t agoraMs) const {
        return respondendo(agoraMs) && distanciaMm_ > 0;
    }

    uint16_t distanciaMm() const { return distanciaMm_; }

    // Instante da última amostra registrada. Serve para exigir uma amostra feita depois de
    // o robô parar (o laço só classifica as paredes com amostras novas da célula).
    uint32_t instanteMs() const { return instanteMs_; }
    bool temAmostra() const { return temAmostra_; }

    // Distância no formato dos controles (EntradaControleReta): em mm, ou -1 se inválida.
    float distanciaParaControle(uint32_t agoraMs) const {
        return valida(agoraMs) ? (float)distanciaMm_ : -1.0f;
    }

    // Há amostra que ainda não foi consumida (o ControleReta só deve rodar com amostra nova
    // dos laterais, R11). consumir() marca como lida.
    bool nova() const { return nova_; }
    void consumir() { nova_ = false; }

private:
    uint16_t validadeMs_;
    uint16_t distanciaMm_;
    uint32_t instanteMs_;
    bool temAmostra_;
    bool nova_;
};

#endif
