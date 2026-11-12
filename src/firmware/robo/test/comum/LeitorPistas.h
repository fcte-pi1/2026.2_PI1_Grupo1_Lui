#ifndef LEITOR_PISTAS_H
#define LEITOR_PISTAS_H

#include <stdint.h>
#include <string.h>
#include "SimuladorLabirinto.h"

// Leitor das pistas desenhadas em texto, no mesmo formato de
// src/backend/app/simulador/pistas.txt (o leitor do backend é pistas.py):
//  - cada célula ocupa 3 caracteres de largura; "---" é parede horizontal, "|" é parede
//    vertical e espaço é passagem;
//  - a coluna A fica à esquerda e a linha 1 no topo (norte); o interior das células é
//    ignorado;
//  - linhas curtas são completadas com espaços (editores apagam espaços no fim).
// O desenho tem 2 x linhas + 1 linhas de texto, separadas por '\n'. Assim a prova no
// ambiente nativo usa as mesmas pistas que o painel mostra.

namespace leitor_pistas {

const uint8_t COLUNAS = 4;
const uint8_t LARGURA = 4 * COLUNAS + 1;
const uint8_t LINHAS_TEXTO_MAX = 2 * SimuladorLabirinto::LINHAS_MAX + 1;

// Caractere da coluna x na linha de texto (espaço além do fim da linha).
inline char caractere(const char* inicio, uint16_t tamanho, uint8_t x) {
    return (x < tamanho) ? inicio[x] : ' ';
}

}  // namespace leitor_pistas

// Carrega o desenho no simulador. Retorna false se a quantidade de linhas de texto não
// for 9, 17 ou 25 (geometrias 4x4, 8x4 e 12x4).
inline bool carregarPista(const char* desenho, SimuladorLabirinto& sim) {
    using namespace leitor_pistas;
    const char* inicio[LINHAS_TEXTO_MAX];
    uint16_t tamanho[LINHAS_TEXTO_MAX];
    uint8_t n = 0;
    const char* p = desenho;
    while (*p != '\0') {
        const char* fim = strchr(p, '\n');
        const uint16_t len = (uint16_t)(fim ? (fim - p) : strlen(p));
        uint16_t util = len;
        if (util > 0 && p[util - 1] == '\r') util--;
        if (util > 0) {
            if (n >= LINHAS_TEXTO_MAX) return false;
            inicio[n] = p;
            tamanho[n] = util;
            n++;
        }
        if (fim == nullptr) break;
        p = fim + 1;
    }
    if (n != 9 && n != 17 && n != 25) return false;
    const uint8_t linhas = (uint8_t)((n - 1) / 2);

    sim.reiniciar();  // todas as paredes; abrimos as passagens do desenho
    sim.linhas = linhas;
    for (uint8_t l = 0; l < linhas; l++) {
        const uint8_t acima = (uint8_t)(2 * l), meio = (uint8_t)(2 * l + 1), abaixo = (uint8_t)(2 * l + 2);
        for (uint8_t c = 0; c < COLUNAS; c++) {
            const uint8_t x = (uint8_t)(4 * c);
            if (caractere(inicio[acima], tamanho[acima], x + 1) != '-' && l > 0) sim.abrir(l, c, DIR_NORTE);
            if (caractere(inicio[abaixo], tamanho[abaixo], x + 1) != '-' && l + 1 < linhas) sim.abrir(l, c, DIR_SUL);
            if (caractere(inicio[meio], tamanho[meio], x + 4) != '|' && c + 1 < COLUNAS) sim.abrir(l, c, DIR_LESTE);
            if (caractere(inicio[meio], tamanho[meio], x) != '|' && c > 0) sim.abrir(l, c, DIR_OESTE);
        }
    }
    return true;
}

#endif
