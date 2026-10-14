#ifndef DIRECAO_H
#define DIRECAO_H

#include <stdint.h>

// Direções absolutas do labirinto (doc 4.4, seção 5.1): o norte é o topo da grade,
// ir para o sul aumenta a linha e ir para o leste aumenta a coluna.
// Os valores são os do campo 'orientacao' do contrato de dados (src/README.md).
enum Direcao : uint8_t {
    DIR_NORTE = 0,
    DIR_SUL   = 1,
    DIR_LESTE = 2,
    DIR_OESTE = 3
};

// Lados do robô em que há sensor de distância (frente e laterais).
enum LadoRobo : uint8_t {
    LADO_FRENTE,
    LADO_ESQUERDA,
    LADO_DIREITA
};

// Máscaras do campo 'paredes' do contrato de dados. O bit de cada parede é o
// próprio valor da direção: bit0 = norte, bit1 = sul, bit2 = leste, bit3 = oeste.
const uint8_t PAREDE_NORTE = 0x01;
const uint8_t PAREDE_SUL   = 0x02;
const uint8_t PAREDE_LESTE = 0x04;
const uint8_t PAREDE_OESTE = 0x08;
const uint8_t PAREDE_TODAS = 0x0F;

inline uint8_t mascaraParede(Direcao d) {
    return (uint8_t)(1u << d);
}

static_assert(PAREDE_NORTE == (1u << DIR_NORTE) && PAREDE_SUL == (1u << DIR_SUL) &&
                  PAREDE_LESTE == (1u << DIR_LESTE) && PAREDE_OESTE == (1u << DIR_OESTE),
              "Máscaras de parede devem seguir os valores de Direcao (contrato de dados)");

inline Direcao direcaoOposta(Direcao d) {
    switch (d) {
        case DIR_NORTE: return DIR_SUL;
        case DIR_SUL:   return DIR_NORTE;
        case DIR_LESTE: return DIR_OESTE;
        case DIR_OESTE: return DIR_LESTE;
    }
    return d;
}

// Giro de 90 graus no sentido horário: norte -> leste -> sul -> oeste -> norte.
inline Direcao girarDireita(Direcao d) {
    switch (d) {
        case DIR_NORTE: return DIR_LESTE;
        case DIR_LESTE: return DIR_SUL;
        case DIR_SUL:   return DIR_OESTE;
        case DIR_OESTE: return DIR_NORTE;
    }
    return d;
}

// Giro de 90 graus no sentido anti-horário: norte -> oeste -> sul -> leste -> norte.
inline Direcao girarEsquerda(Direcao d) {
    switch (d) {
        case DIR_NORTE: return DIR_OESTE;
        case DIR_OESTE: return DIR_SUL;
        case DIR_SUL:   return DIR_LESTE;
        case DIR_LESTE: return DIR_NORTE;
    }
    return d;
}

// Converte um lado do robô na direção absoluta, a partir da orientação atual.
inline Direcao direcaoAbsoluta(Direcao orientacao, LadoRobo lado) {
    switch (lado) {
        case LADO_FRENTE:   return orientacao;
        case LADO_ESQUERDA: return girarEsquerda(orientacao);
        case LADO_DIREITA:  return girarDireita(orientacao);
    }
    return orientacao;
}

// Deslocamento na grade ao avançar uma célula na direção indicada.
inline int8_t deltaLinha(Direcao d) {
    return (d == DIR_SUL) ? 1 : (d == DIR_NORTE) ? -1 : 0;
}

inline int8_t deltaColuna(Direcao d) {
    return (d == DIR_LESTE) ? 1 : (d == DIR_OESTE) ? -1 : 0;
}

#endif
