#include "Conversor.h"
#include <stdio.h>

size_t converterParaJson(const TelemetriaPacket& p, char* saida, size_t tamanho) {
    const char* labs[] = {"", "4x4", "8x4", "12x4"};
    const char* status_str[] = {"AGUARDANDO", "MAPEANDO", "SPEED RUN", "COLISAO", "CONCLUIDO", "TIMEOUT", "TRAVADO"};
    const char* orientacao_str[] = {"NORTE", "SUL", "LESTE", "OESTE"};

    char col = 'A' + p.coluna;
    int lin = p.linha + 1;
    float vel_atual_cm = p.velocidade_atual / 10.0f;
    float vel_media_cm = p.velocidade_media / 10.0f;
    float volts = p.bateria_mv / 1000.0f;

    bool pN = (p.paredes & (1 << 0)) != 0;
    bool pS = (p.paredes & (1 << 1)) != 0;
    bool pL = (p.paredes & (1 << 2)) != 0;
    bool pO = (p.paredes & (1 << 3)) != 0;

    return snprintf(saida, tamanho,
        "{\"timestamp_ms\": %u, \"labirinto\": \"%s\", \"status\": \"%s\", \"desafio_cumprido\": %s, "
        "\"bateria pct\": %u, \"bateria_volts\": %.2f, \"velocidade_atual\": %.1f, \"velocidade media\": %.1f, "
        "\"posicao\": { \"coluna\": \"%c\", \"linha\": %d}, \"orientacao\": \"%s\", "
        "\"paredes\": { \"norte\": %s, \"sul\": %s, \"leste\": %s, \"oeste\": %s } }\n",
        p.timestamp_ms,
        (p.labirinto_id >= 1 && p.labirinto_id <= 3) ? labs[p.labirinto_id] : "DESCONHECIDO",
        (p.status_id <= 6) ? status_str[p.status_id] : "DESCONHECIDO",
        p.desafio_cumprido ? "true" : "false",
        p.bateria_pct, volts, vel_atual_cm, vel_media_cm,
        col, lin,
        (p.orientacao <= 3) ? orientacao_str[p.orientacao] : "DESCONHECIDO",
        pN ? "true" : "false", pS ? "true" : "false", pL ? "true" : "false", pO ? "true" : "false"
    );
}