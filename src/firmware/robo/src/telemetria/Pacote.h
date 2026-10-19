// include/telemetria/Telemetria.h
#ifndef TELEMETRIA_H
#define TELEMETRIA_H

#include <stdint.h>
#include "Pacote.h"

class Telemetria {
public:
    // Retorna true se a inicialização e o pareamento com o Gateway forem bem-sucedidos
    static bool iniciar(const uint8_t macGateway[6]);
    
    // Retorna true se o envio do pacote via ESP-NOW for bem-sucedido
    static bool enviar(const TelemetriaPacket& p);
};

#endif // TELEMETRIA_H