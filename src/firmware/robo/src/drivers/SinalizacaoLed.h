#ifndef SINALIZACAO_LED_H
#define SINALIZACAO_LED_H

#include <stdint.h>
#include "../seguranca/MonitorTravamento.h"  // ISinalizacaoEstado
#include "TelemetriaPacket.h"                // EstadoRobo

// LED de estado do robô (achado R5 de docs/firmware/integracao-loop.md). Implementa
// ISinalizacaoEstado, a interface que o MonitorTravamento já usa, nas duas variantes de
// hardware possíveis no GPIO PINO_LED_ESTADO: WS2812B (cores) ou LED comum (padrões de
// piscada). A variante sai de TIPO_LED_ESTADO em hardware/Pinos.h; os padrões estão em
// sinalizacao/PadraoLed.h, testados no nativo.
//
// mostrarEstado() só guarda o estado. atualizar() desenha o LED e deve ser chamado no ritmo
// do LED (100 ms), sem delay(): as piscadas saem do relógio.
class SinalizacaoLed : public ISinalizacaoEstado {
public:
    void iniciar();

    void mostrarEstado(EstadoRobo estado) override { estado_ = estado; }

    // largadaArmada: botão pressionado há 2 s em AGUARDANDO (pode soltar).
    void atualizar(uint32_t agoraMs, bool largadaArmada);

    // Apaga o LED (por exemplo, enquanto outra rotina usa o pino).
    void apagar();

    EstadoRobo estado() const { return estado_; }

private:
    EstadoRobo estado_ = AGUARDANDO;
};

#endif
