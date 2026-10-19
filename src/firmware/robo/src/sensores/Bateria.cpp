// src/sensores/Bateria.cpp
#include "sensores/Bateria.h"
#include "Pinos.h"
#include "Config.h"
#include <Arduino.h>

void Bateria::iniciar() {
    // Configuração inicial do ADC (se necessário para atenuadores)
}

uint16_t Bateria::lerMilivolts() {
    uint32_t mvAdc = analogReadMilliVolts(PINO_BATERIA_ADC);
    return (mvAdc * 133) / 33; 
}

uint8_t Bateria::calcularPorcentagem(uint16_t mv) const {
    if (mv <= BATERIA_VAZIA_MV) return 0;
    if (mv >= BATERIA_CHEIA_MV) return 100;
    return ((mv - BATERIA_VAZIA_MV) * 100) / (BATERIA_CHEIA_MV - BATERIA_VAZIA_MV);
}