#ifndef GATEWAY_BRIDGE_H
#define GATEWAY_BRIDGE_H

#include <stddef.h>
#include <stdint.h>
#include "TelemetriaJson.h"

// Lógica do gateway independente de hardware (testável nativamente):
// recebe os bytes de um quadro ESP-NOW + RSSI, valida, numera e gera uma linha JSON.
class GatewayBridge {
public:
    GatewayBridge() : seq_(0), descartados_(0), ultimoErro_(TELEMETRIA_OK) {}

    // Processa um quadro. Em caso de sucesso escreve em 'saida' uma linha JSON terminada
    // em "\n" + '\0' e retorna seu tamanho (sem o '\0'). Retorna 0 se o quadro foi
    // descartado (tamanho/campos inválidos ou buffer insuficiente); nesse caso o
    // número de sequência NÃO é consumido e o motivo fica em ultimoErro().
    size_t processar(const uint8_t* dados, size_t tamanho, int rssi, char* saida, size_t capacidade) {
        TelemetriaPacket pacote;
        const TelemetriaResultado r = telemetriaDecodificar(dados, tamanho, pacote);
        if (r != TELEMETRIA_OK) {
            registrarDescarte(r);
            return 0;
        }
        const size_t n = telemetriaParaJson(pacote, seq_ + 1, rssi, saida, capacidade);
        if (n == 0 || n + 2 > capacidade) {
            registrarDescarte(TELEMETRIA_ERRO_TAMANHO);
            return 0;
        }
        saida[n] = '\n';
        saida[n + 1] = '\0';
        seq_++;
        return n + 1;
    }

    uint32_t sequencia() const { return seq_; }              // Pacotes válidos entregues
    uint32_t descartados() const { return descartados_; }    // Quadros rejeitados
    TelemetriaResultado ultimoErro() const { return ultimoErro_; }

private:
    void registrarDescarte(TelemetriaResultado r) {
        descartados_++;
        ultimoErro_ = r;
    }

    uint32_t seq_;
    uint32_t descartados_;
    TelemetriaResultado ultimoErro_;
};

#endif
