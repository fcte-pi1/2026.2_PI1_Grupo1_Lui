#ifndef ENCODERS_H
#define ENCODERS_H

#include <stdint.h>
#include "../app/InterfacesHardware.h"  // IEncoders

// Driver dos dois encoders em quadratura (porte de hw/firm/src/encoder_driver.cpp com os
// pinos de hardware/Pinos.h). Só conta pulsos: uma interrupção na borda de subida do canal
// A de cada roda soma ou subtrai 1 conforme o nível do canal B. Milímetros, velocidade e o
// instante do último pulso ficam na Odometria (odometria/Odometria.h), que é testada no
// ambiente nativo e recebe estas contagens a cada tick (achado R6).
//
// Contagem 1x (só a borda de subida do canal A), igual à do hw/firm, que foi a usada nas
// medições da Eletrônica. Não trocar para 2x ou 4x: ENCODER_PULSOS_POR_VOLTA em
// hardware/Pinos.h é medido nessa mesma forma de contar (achado B4).
// Sentido: as contagens devem crescer com o robô andando para a frente; se uma roda contar
// ao contrário na bancada, troque ENCODER_ESQ_INVERTIDO ou ENCODER_DIR_INVERTIDO (R7).
//
// Os canais dos GPIO 35, 36 e 39 não têm pull-up interno: a placa dos encoders precisa
// fornecer o nível alto (pendência da Eletrônica, PR #188).
//
// GPIO 36 e 39 com o ADC ligado (pendência 9 do desenho): pausarEsquerdo() e
// retomarEsquerdo() desligam e religam só a interrupção do canal A esquerdo; o laço as
// chama em volta da leitura da bateria.
class Encoders : public IEncoders {
public:
    // Configura os pinos, zera as contagens e liga as interrupções.
    void iniciar();

    // Contagens acumuladas, lidas de forma atômica em relação às interrupções.
    int32_t esq() const;
    int32_t dir() const;
    void ler(int32_t& esq, int32_t& dir) override;  // as duas no mesmo instante

    void pausarEsquerdo() override;
    void retomarEsquerdo() override;

    void zerar();
};

#endif
