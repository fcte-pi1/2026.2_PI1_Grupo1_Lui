#ifndef TELEMETRIA_CODEC_H
#define TELEMETRIA_CODEC_H

#include <stddef.h>
#include <stdint.h>
#include "TelemetriaPacket.h"

// Codificação, decodificação e validação do pacote de telemetria de 18 bytes.
// A serialização é explícita em little-endian, campo a campo, e portanto não
// depende de alinhamento nem da endianness da máquina (robô, gateway e testes nativos).

#define TELEMETRIA_TAMANHO_BYTES      18
#define TELEMETRIA_TIMESTAMP_MAX_MS   600000UL  // Limite de 10 min de prova (doc 4.4, seção 6.5)

enum TelemetriaResultado {
    TELEMETRIA_OK = 0,
    TELEMETRIA_ERRO_PONTEIRO,
    TELEMETRIA_ERRO_TAMANHO,
    TELEMETRIA_ERRO_TIMESTAMP,
    TELEMETRIA_ERRO_LABIRINTO,
    TELEMETRIA_ERRO_STATUS,
    TELEMETRIA_ERRO_DESAFIO,
    TELEMETRIA_ERRO_BATERIA,
    TELEMETRIA_ERRO_POSICAO,
    TELEMETRIA_ERRO_ORIENTACAO,
    TELEMETRIA_ERRO_PAREDES
};

inline const char* telemetriaNomeResultado(TelemetriaResultado r) {
    switch (r) {
        case TELEMETRIA_OK:              return "OK";
        case TELEMETRIA_ERRO_PONTEIRO:   return "PONTEIRO";
        case TELEMETRIA_ERRO_TAMANHO:    return "TAMANHO";
        case TELEMETRIA_ERRO_TIMESTAMP:  return "TIMESTAMP";
        case TELEMETRIA_ERRO_LABIRINTO:  return "LABIRINTO";
        case TELEMETRIA_ERRO_STATUS:     return "STATUS";
        case TELEMETRIA_ERRO_DESAFIO:    return "DESAFIO";
        case TELEMETRIA_ERRO_BATERIA:    return "BATERIA";
        case TELEMETRIA_ERRO_POSICAO:    return "POSICAO";
        case TELEMETRIA_ERRO_ORIENTACAO: return "ORIENTACAO";
        case TELEMETRIA_ERRO_PAREDES:    return "PAREDES";
    }
    return "DESCONHECIDO";
}

// Número de linhas da geometria (4, 8 ou 12), ou 0 se o id for inválido.
inline uint8_t telemetriaLinhasDoLabirinto(uint8_t labirinto_id) {
    switch (labirinto_id) {
        case LAB_4X4:  return 4;
        case LAB_8X4:  return 8;
        case LAB_12X4: return 12;
    }
    return 0;
}

// Nomes textuais exigidos pelo dicionário de dados (doc 4.4, seção 6.5).
// Retornam nullptr se o valor estiver fora do domínio.
inline const char* telemetriaNomeLabirinto(uint8_t id) {
    static const char* const nomes[] = {"4x4", "8x4", "12x4"};
    return (id >= 1 && id <= 3) ? nomes[id - 1] : nullptr;
}

inline const char* telemetriaNomeStatus(uint8_t id) {
    static const char* const nomes[] = {"AGUARDANDO", "MAPEANDO", "SPEED_RUN", "COLISAO",
                                        "CONCLUIDO", "TIMEOUT", "TRAVADO"};
    return (id <= 6) ? nomes[id] : nullptr;
}

inline const char* telemetriaNomeOrientacao(uint8_t id) {
    static const char* const nomes[] = {"NORTE", "SUL", "LESTE", "OESTE"};
    return (id <= 3) ? nomes[id] : nullptr;
}

inline const char* telemetriaLetraColuna(uint8_t coluna) {
    static const char* const letras[] = {"A", "B", "C", "D"};
    return (coluna <= 3) ? letras[coluna] : nullptr;
}

// Valida os campos do pacote conforme o dicionário de dados.
inline TelemetriaResultado telemetriaValidar(const TelemetriaPacket& p) {
    if (p.timestamp_ms > TELEMETRIA_TIMESTAMP_MAX_MS) return TELEMETRIA_ERRO_TIMESTAMP;
    const uint8_t linhas = telemetriaLinhasDoLabirinto(p.labirinto_id);
    if (linhas == 0) return TELEMETRIA_ERRO_LABIRINTO;
    if (p.status_id > TRAVADO) return TELEMETRIA_ERRO_STATUS;
    if (p.desafio_cumprido > 1) return TELEMETRIA_ERRO_DESAFIO;
    if (p.bateria_pct > 100) return TELEMETRIA_ERRO_BATERIA;
    if (p.coluna > 3 || p.linha >= linhas) return TELEMETRIA_ERRO_POSICAO;
    if (p.orientacao > 3) return TELEMETRIA_ERRO_ORIENTACAO;
    if (p.paredes > 0x0F) return TELEMETRIA_ERRO_PAREDES;
    return TELEMETRIA_OK;
}

namespace telemetria_detalhe {
inline void escreve16(uint8_t* d, uint16_t v) {
    d[0] = (uint8_t)(v & 0xFF);
    d[1] = (uint8_t)(v >> 8);
}
inline uint16_t le16(const uint8_t* d) { return (uint16_t)(d[0] | (d[1] << 8)); }
}  // namespace telemetria_detalhe

// Serializa o pacote em 18 bytes. Retorna o número de bytes escritos, ou 0 em erro
// (ponteiro nulo ou buffer menor que 18 bytes). Não valida o conteúdo: use
// telemetriaValidar() antes se necessário.
inline size_t telemetriaCodificar(const TelemetriaPacket& p, uint8_t* buf, size_t capacidade) {
    using namespace telemetria_detalhe;
    if (buf == nullptr || capacidade < TELEMETRIA_TAMANHO_BYTES) return 0;
    buf[0] = (uint8_t)(p.timestamp_ms & 0xFF);
    buf[1] = (uint8_t)((p.timestamp_ms >> 8) & 0xFF);
    buf[2] = (uint8_t)((p.timestamp_ms >> 16) & 0xFF);
    buf[3] = (uint8_t)((p.timestamp_ms >> 24) & 0xFF);
    buf[4] = p.labirinto_id;
    buf[5] = p.status_id;
    buf[6] = p.desafio_cumprido;
    buf[7] = p.bateria_pct;
    escreve16(buf + 8, p.bateria_mv);
    escreve16(buf + 10, (uint16_t)p.velocidade_atual);
    escreve16(buf + 12, (uint16_t)p.velocidade_media);
    buf[14] = p.coluna;
    buf[15] = p.linha;
    buf[16] = p.orientacao;
    buf[17] = p.paredes;
    return TELEMETRIA_TAMANHO_BYTES;
}

// Decodifica e valida um pacote recebido. O tamanho deve ser exatamente 18 bytes.
// Em caso de erro, 'saida' não é modificado.
inline TelemetriaResultado telemetriaDecodificar(const uint8_t* dados, size_t tamanho,
                                                 TelemetriaPacket& saida) {
    using namespace telemetria_detalhe;
    if (dados == nullptr) return TELEMETRIA_ERRO_PONTEIRO;
    if (tamanho != TELEMETRIA_TAMANHO_BYTES) return TELEMETRIA_ERRO_TAMANHO;
    TelemetriaPacket p;
    p.timestamp_ms = (uint32_t)dados[0] | ((uint32_t)dados[1] << 8) |
                     ((uint32_t)dados[2] << 16) | ((uint32_t)dados[3] << 24);
    p.labirinto_id = dados[4];
    p.status_id = dados[5];
    p.desafio_cumprido = dados[6];
    p.bateria_pct = dados[7];
    p.bateria_mv = le16(dados + 8);
    p.velocidade_atual = (int16_t)le16(dados + 10);
    p.velocidade_media = (int16_t)le16(dados + 12);
    p.coluna = dados[14];
    p.linha = dados[15];
    p.orientacao = dados[16];
    p.paredes = dados[17];
    const TelemetriaResultado r = telemetriaValidar(p);
    if (r != TELEMETRIA_OK) return r;
    saida = p;
    return TELEMETRIA_OK;
}

#endif
