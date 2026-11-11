#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "TelemetriaPacket.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o rádio Wi-Fi da ESP32 em modo STA e o protocolo ESP-NOW no Canal 1.
 * Configura o endereço de broadcast (FF:FF:FF:FF:FF:FF) para transmissão direta ao Gateway ESP8266
 * e registra callback de recepção para aceitar comandos remotos de bancada.
 * @return true se inicializado com sucesso; false se falhar.
 */
bool radio_telemetria_init(void);

/**
 * @brief Codifica e envia um pacote de 18 bytes via rádio ESP-NOW.
 * O Gateway ESP8266 recebe esse pacote, converte em JSON e envia via USB para o site/backend.
 * @param pacote Ponteiro para a struct TelemetriaPacket preenchida.
 * @return true se o pacote foi transmitido/enfileirado com sucesso; false caso contrário.
 */
bool radio_telemetria_enviar(const TelemetriaPacket* pacote);

/**
 * @brief Retorna se o rádio ESP-NOW está inicializado e pronto para transmissão.
 */
bool radio_telemetria_is_ready(void);

/**
 * @brief Retorna true se houver algum comando de caractere recebido pelo rádio ESP-NOW.
 * @param[out] cmd Ponteiro onde o caractere recebido será armazenado.
 */
bool radio_telemetria_ler_comando(char* cmd);

#ifdef __cplusplus
}
#endif
