#ifndef SINALIZACAO_LED_H
#define SINALIZACAO_LED_H

#include <stdint.h>
#include "../app/InterfacesHardware.h"  // ILedEstado (e ISinalizacaoEstado)
#include "../sinalizacao/PadraoLed.h"
#include "TelemetriaPacket.h"           // EstadoRobo

// LED de estado do robô (implementa ILedEstado para manter o contrato com CicloProva
// e MonitorTravamento). No hardware atual o LED de estado esta ligado diretamente em
// 3.3 V (indicador de energia), portanto iniciar()/atualizar()/apagar() nao acionam
// nenhum GPIO.
class SinalizacaoLed : public ILedEstado {
public:
    void iniciar();

    void mostrarEstado(EstadoRobo estado) override { estado_ = estado; }

    void piscar(CorLed cor, uint8_t vezes, uint32_t agoraMs) override {
        avisoCor_ = cor;
        avisoVezes_ = vezes;
        avisoInicioMs_ = agoraMs;
    }

    // largadaArmada: botão pressionado há 2 s em AGUARDANDO (pode soltar).
    void atualizar(uint32_t agoraMs, bool largadaArmada) override;

    // Apaga o LED (por exemplo, enquanto outra rotina usa o pino).
    void apagar();

    EstadoRobo estado() const { return estado_; }

private:
    EstadoRobo estado_ = AGUARDANDO;
    CorLed avisoCor_ = {0, 0, 0};
    uint8_t avisoVezes_ = 0;
    uint32_t avisoInicioMs_ = 0;
};

#endif
