#ifndef ADAPTADOR_CRONOMETRO_H
#define ADAPTADOR_CRONOMETRO_H

#include <stdint.h>

#include "ParadaObjetivo.h"
#include "../cronometro/Cronometro.h"

// Liga o Cronometro da prova à interface ICronometro do ParadaObjetivo (HU-07).
// A interface não recebe o instante atual e o Cronometro não lê relógio (para
// continuar sem dependência do Arduino), então o adaptador recebe uma função que
// devolve o tempo em ms: no robô, um invólucro de millis(); nos testes, um
// relógio falso.
class AdaptadorCronometro : public ICronometro {
public:
    typedef uint32_t (*FuncaoAgoraMs)();

    AdaptadorCronometro(Cronometro& cronometro, FuncaoAgoraMs agoraMs)
        : cronometro_(cronometro), agoraMs_(agoraMs) {}

    void congelar() override { cronometro_.congelar(agoraMs_()); }

private:
    Cronometro& cronometro_;
    FuncaoAgoraMs agoraMs_;
};

#endif  // ADAPTADOR_CRONOMETRO_H
