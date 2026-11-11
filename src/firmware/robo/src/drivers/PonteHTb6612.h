#ifndef PONTE_H_TB6612_H
#define PONTE_H_TB6612_H

#include <stdint.h>
#include "Motores.h"  // IPonteH

// Driver da ponte H TB6612FNG (porte de hw/firm/src/tb6612_driver.cpp com os pinos de
// hardware/Pinos.h). Só fala com o hardware: trinco, inversão de montagem e tração por roda
// ficam em Motores (drivers/Motores.h), que é testado no ambiente nativo.
//
// PWM positivo = IN1 alto e IN2 baixo (sentido "frente" da fiação); 0 = roda livre.
// Freio ativo = IN1 e IN2 altos com PWM máximo, como no hw/firm.
class PonteHTb6612 : public IPonteH {
public:
    // Configura os pinos e o PWM (20 kHz, 8 bits), habilita a ponte e deixa as rodas livres.
    void iniciar();

    void aplicar(int16_t pwmEsq, int16_t pwmDir) override;
    void frear() override;
    void soltar() override;

    // Standby da ponte (STBY baixo desliga a saída). Ativo só fora da prova.
    void standby(bool ativo);
};

#endif
