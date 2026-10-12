#ifndef TELEMETRIA_JSON_H
#define TELEMETRIA_JSON_H

#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>
#include "TelemetriaCodec.h"

// Converte um pacote válido no JSON do contrato de dados (doc 4.4, seções 6.4 e 6.5),
// acrescido dos campos do gateway: "seq" (número de sequência) e "rssi" (dBm).
// Conversões: coluna 0..3 -> "A".."D"; linha 0..11 -> 1..12; mm/s -> cm/s; mV -> V;
// bitmask de paredes -> quatro booleanos.
// Escreve o JSON terminado em '\0' em 'saida'. Retorna o tamanho (sem o '\0'),
// ou 0 se o pacote for inválido ou o buffer for insuficiente.
inline size_t telemetriaParaJson(const TelemetriaPacket& p, uint32_t seq, int rssi,
                                 char* saida, size_t capacidade) {
    if (saida == nullptr || capacidade == 0) return 0;
    if (telemetriaValidar(p) != TELEMETRIA_OK) return 0;

    JsonDocument doc;
    doc["seq"] = seq;
    doc["rssi"] = rssi;
    doc["timestamp_ms"] = p.timestamp_ms;
    doc["labirinto"] = telemetriaNomeLabirinto(p.labirinto_id);
    doc["status"] = telemetriaNomeStatus(p.status_id);
    doc["desafio_cumprido"] = (p.desafio_cumprido == 1);
    doc["bateria_pct"] = p.bateria_pct;
    doc["bateria_volts"] = p.bateria_mv / 1000.0f;
    doc["velocidade_atual"] = p.velocidade_atual / 10.0f;
    doc["velocidade_media"] = p.velocidade_media / 10.0f;

    JsonObject posicao = doc["posicao"].to<JsonObject>();
    posicao["coluna"] = telemetriaLetraColuna(p.coluna);
    posicao["linha"] = p.linha + 1;

    doc["orientacao"] = telemetriaNomeOrientacao(p.orientacao);

    JsonObject paredes = doc["paredes"].to<JsonObject>();
    paredes["norte"] = (p.paredes & 0x01) != 0;
    paredes["sul"] = (p.paredes & 0x02) != 0;
    paredes["leste"] = (p.paredes & 0x04) != 0;
    paredes["oeste"] = (p.paredes & 0x08) != 0;

    const size_t n = serializeJson(doc, saida, capacidade);
    if (n == 0 || n >= capacidade) {
        saida[0] = '\0';
        return 0;
    }
    return n;
}

#endif
