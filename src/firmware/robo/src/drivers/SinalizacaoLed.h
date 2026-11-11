#ifndef SINALIZACAO_LED_H
#define SINALIZACAO_LED_H

#include <stdint.h>
#include "../app/InterfacesHardware.h"  // ILedEstado (e ISinalizacaoEstado)
#include "../sinalizacao/PadraoLed.h"
#include "TelemetriaPacket.h"           // EstadoRobo

// LED de estado do robô (achado R5 de docs/firmware/integracao-loop.md). Implementa
// ILedEstado, que estende a ISinalizacaoEstado que o MonitorTravamento já usa, nas duas
// variantes de hardware possíveis no GPIO PINO_LED_ESTADO: WS2812B (cores) ou LED comum
// (padrões de piscada). A variante sai de TIPO_LED_ESTADO em hardware/Pinos.h; os padrões
// estão em sinalizacao/PadraoLed.h, testados no nativo.
//
// mostrarEstado() só guarda o estado e piscar() só guarda o aviso. atualizar() desenha o
// LED e deve ser chamado no ritmo do LED (50 ms), sem delay(): as piscadas saem do
// relógio. Enquanto houver aviso, ele passa por cima do padrão do estado.
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
