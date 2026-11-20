#ifndef RADIO_TELEMETRIA_H
#define RADIO_TELEMETRIA_H

#include "TelemetriaPacket.h"

// Transmissor ESP-NOW do robô. Envia em broadcast (FF:FF:FF:FF:FF:FF) no canal
// TELEMETRIA_CANAL_WIFI, de modo que qualquer gateway no mesmo canal receba
// sem precisar parear endereços MAC.

// Inicializa Wi-Fi (modo STA), canal e ESP-NOW. Retorna false em caso de falha.
bool radioTelemetriaInit();

// Codifica e envia o pacote (18 bytes). Retorna false se a codificação ou o envio falhar.
bool radioTelemetriaEnviar(const TelemetriaPacket& pacote);

// Lê um comando de 1 byte recebido do Gateway via ESP-NOW (ex.: 's' parada de emergência).
// Retorna true se havia comando pendente na fila.
bool radioTelemetriaLerComando(char& cmd);

#endif
