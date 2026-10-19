// src/firmware/gateway/include/Conversor.h
#ifndef CONVERSOR_H
#define CONVERSOR_H

#include <stddef.h>   // Necessário para o tipo size_t
#include "Pacote.h"   // Necessário para a estrutura TelemetriaPacket

/**
 * @brief Converte a estrutura binária de telemetria para uma string JSON.
 * 
 * @param p Referência constante para o pacote recebido via ESP-NOW.
 * @param saida Buffer de caracteres onde o JSON resultante será salvo.
 * @param tamanho Tamanho máximo do buffer de saída (para evitar buffer overflow).
 * @return size_t O número de caracteres escritos no buffer (excluindo o caractere nulo terminador), 
 *                semelhante ao retorno da função snprintf.
 */
size_t converterParaJson(const TelemetriaPacket& p, char* saida, size_t tamanho);

#endif // CONVERSOR_H