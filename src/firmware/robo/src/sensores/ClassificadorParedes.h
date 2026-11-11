#ifndef CLASSIFICADOR_PAREDES_H
#define CLASSIFICADOR_PAREDES_H

#include <stdint.h>
#include "../hardware/Pinos.h"
#include "../navegacao/InterfacesNavegacao.h"  // SensoresParedes
#include "LeituraToF.h"

// Classifica as distâncias dos três ToF em "há parede" ou "não há", sem dependência do
// Arduino (docs/firmware/integracao-loop.md, achado R3). É o único lugar com o limiar de
// parede: LIMIAR_PAREDE_MM e HISTERESE_PAREDE_MM, em hardware/Pinos.h. O mesmo limiar deve
// ser usado como distMaxParedeMm do ControleReta, para os dois concordarem sobre o que é
// parede (quem configura o controle é o executor de movimento).
//
// Histerese por lado: há parede quando a distância cai abaixo de limiar - histerese e
// deixa de haver quando passa de limiar + histerese; entre os dois, mantém o que era. Na
// primeira leitura válida de um lado, a decisão é pelo limiar puro.
//
// Implementa SensoresParedes (o que o Navegador usa) a partir da última amostra entregue em
// atualizar(): lerParedes() devolve false se algum dos três sensores estava inválido nessa
// amostra, e o laço tenta de novo (até 150 ms, seção 3.7 do desenho).

// Uma amostra dos três sensores. Distância inválida: valida = false (o valor é ignorado).
// Sem alvo ao alcance (8190 mm) é válida e vira "não há parede".
struct AmostraParedes {
    uint16_t esqMm;
    uint16_t frenteMm;
    uint16_t dirMm;
    bool esqValida;
    bool frenteValida;
    bool dirValida;
};

// Monta a amostra a partir do estado das três leituras no instante agoraMs.
inline AmostraParedes amostraDasLeituras(const LeituraToF& esq, const LeituraToF& frente,
                                         const LeituraToF& dir, uint32_t agoraMs) {
    AmostraParedes a;
    a.esqMm = esq.distanciaMm();
    a.frenteMm = frente.distanciaMm();
    a.dirMm = dir.distanciaMm();
    a.esqValida = esq.validaParaParede(agoraMs);
    a.frenteValida = frente.validaParaParede(agoraMs);
    a.dirValida = dir.validaParaParede(agoraMs);
    return a;
}

class ClassificadorParedes : public SensoresParedes {
public:
    explicit ClassificadorParedes(uint16_t limiarMm = LIMIAR_PAREDE_MM,
                                  uint16_t histereseMm = HISTERESE_PAREDE_MM)
        : limiarMm_(limiarMm), histereseMm_(histereseMm) {
        reiniciar();
    }

    // Esquece o histórico (por exemplo, na largada).
    void reiniciar() {
        esq_.reiniciar();
        frente_.reiniciar();
        dir_.reiniciar();
    }

    void atualizar(const AmostraParedes& a) {
        esq_.atualizar(a.esqValida, a.esqMm, limiarMm_, histereseMm_);
        frente_.atualizar(a.frenteValida, a.frenteMm, limiarMm_, histereseMm_);
        dir_.atualizar(a.dirValida, a.dirMm, limiarMm_, histereseMm_);
    }

    // --- SensoresParedes ---
    bool lerParedes(bool& frente, bool& esquerda, bool& direita) override {
        if (!esq_.valida || !frente_.valida || !dir_.valida) return false;
        frente = frente_.parede;
        esquerda = esq_.parede;
        direita = dir_.parede;
        return true;
    }

    // Último estado de cada lado (com histerese). Só tem sentido se o lado estiver válido.
    bool paredeEsquerda() const { return esq_.parede; }
    bool paredeFrente() const { return frente_.parede; }
    bool paredeDireita() const { return dir_.parede; }
    bool esquerdaValida() const { return esq_.valida; }
    bool frenteValida() const { return frente_.valida; }
    bool direitaValida() const { return dir_.valida; }

private:
    struct Lado {
        bool parede;
        bool valida;        // a última amostra deste lado era válida
        bool temHistorico;  // já houve uma leitura válida desde reiniciar()

        void reiniciar() {
            parede = false;
            valida = false;
            temHistorico = false;
        }

        void atualizar(bool amostraValida, uint16_t mm, uint16_t limiar, uint16_t histerese) {
            valida = amostraValida;
            if (!amostraValida) return;  // sem informação nova: mantém a decisão anterior
            if (!temHistorico) {
                parede = mm <= limiar;
                temHistorico = true;
            } else if (parede) {
                if (mm > (uint32_t)limiar + histerese) parede = false;
            } else {
                if ((uint32_t)mm + histerese < limiar) parede = true;
            }
        }
    };

    uint16_t limiarMm_;
    uint16_t histereseMm_;
    Lado esq_, frente_, dir_;
};

#endif
