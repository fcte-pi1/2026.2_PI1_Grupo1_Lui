#include "drivers/SinalizacaoLed.h"

// O LED de estado fisico esta ligado diretamente ao barramento de 3.3 V (indicador de energia),
// sem controle por GPIO. Este driver mantem o contrato ILedEstado sem acionar nenhum pino.

void SinalizacaoLed::iniciar() {}

void SinalizacaoLed::atualizar(uint32_t agoraMs, bool largadaArmada) {
    (void)largadaArmada;
    const uint32_t desdeAvisoMs = agoraMs - avisoInicioMs_;
    if (avisoVezes_ > 0 && !avisoEmAndamento(avisoVezes_, desdeAvisoMs)) {
        avisoVezes_ = 0;
    }
}

void SinalizacaoLed::apagar() {
    avisoVezes_ = 0;
}
