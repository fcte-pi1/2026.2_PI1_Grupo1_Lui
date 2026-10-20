#ifndef INTERFACES_NAVEGACAO_H
#define INTERFACES_NAVEGACAO_H

#include <stdint.h>

// Fronteira entre a lógica de navegação (decisão e atualização do mapa) e o hardware.
// O Navegador só conhece estas interfaces: os drivers reais de sensores, motores e
// odometria ainda não existem no firmware (pendência de hardware, ver README da HU-06)
// e, nos testes nativos, quem as implementa é um simulador de labirinto.

// Giro executado no lugar, relativo à orientação atual do robô.
enum Giro : uint8_t {
    GIRO_NENHUM,
    GIRO_DIREITA,
    GIRO_ESQUERDA,
    GIRO_MEIA_VOLTA
};

// Leitura das paredes imediatamente ao redor do robô.
class SensoresParedes {
public:
    virtual ~SensoresParedes() {}

    // Preenche a presença de parede à frente, à esquerda e à direita da célula atual.
    // Retorna false se não foi possível obter uma leitura válida.
    virtual bool lerParedes(bool& frente, bool& esquerda, bool& direita) = 0;
};

// Execução física dos movimentos. Cada método só retorna true quando o movimento foi
// concluído e confirmado (por exemplo, por odometria); é essa confirmação que autoriza
// o Navegador a atualizar posição, orientação e mapa.
class ExecutorMovimento {
public:
    virtual ~ExecutorMovimento() {}

    virtual bool girar(Giro giro) = 0;
    virtual bool avancarCelula() = 0;
};

#endif
