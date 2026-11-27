#ifndef ROTA_MAIS_CURTA_H
#define ROTA_MAIS_CURTA_H

#include <stdint.h>
#include "../mapa/Mapa.h"
#include "InterfacesNavegacao.h"

// Rota mais curta sobre o mapa conhecido (HU-15, RF-SFT09). Calculada depois da
// exploração, com BFS: todos os avanços custam uma célula, então a primeira vez que a BFS
// alcança o destino já é por um caminho de menor número de células. Dijkstra e A* dariam o
// mesmo resultado com mais código e memória; o Flood Fill a partir do destino equivale a
// esta BFS, mas exigiria guardar o campo de distâncias inteiro.
//
// Só entram no grafo as passagens confirmadas (lado observado e sem parede). Lado não
// observado nunca é tratado como livre: uma célula nunca visitada só tem conhecido o lado
// por onde foi vista, então a rota nunca atravessa região desconhecida.
//
// Sem alocação dinâmica: dois vetores de 48 bytes na pilha durante o cálculo.

// Cada rota tem no máximo (células - 1) passos; a maior geometria (12x4) tem 48 células.
static const uint8_t ROTA_MAX_PASSOS = 47;

enum ResultadoRota : uint8_t {
    ROTA_OK,                // rota calculada (pode ter zero passos)
    ROTA_INEXISTENTE,       // nenhum caminho de passagens confirmadas até o destino
    ROTA_ENTRADA_INVALIDA   // origem ou destino fora da geometria do mapa
};

// Rota como sequência de direções absolutas, uma por célula percorrida. É o formato que o
// speed run (HU-16) consome: cada passo é "avance uma célula na direção passos[i]", e o
// giro necessário vem de giroDoPasso().
struct Rota {
    uint8_t linhaOrigem;
    uint8_t colunaOrigem;
    uint8_t numPassos;               // número de deslocamentos; células da rota = numPassos + 1
    Direcao passos[ROTA_MAX_PASSOS];

    Rota() : linhaOrigem(0), colunaOrigem(0), numPassos(0) {
        for (uint8_t i = 0; i < ROTA_MAX_PASSOS; i++) passos[i] = DIR_NORTE;
    }

    // Posição da célula alcançada após 'i' passos (i = 0 é a origem).
    // Retorna false, sem alterar as saídas, se i > numPassos.
    bool celula(uint8_t i, uint8_t& linha, uint8_t& coluna) const {
        if (i > numPassos) return false;
        int l = linhaOrigem, c = colunaOrigem;
        for (uint8_t k = 0; k < i; k++) {
            l += deltaLinha(passos[k]);
            c += deltaColuna(passos[k]);
        }
        linha = (uint8_t)l;
        coluna = (uint8_t)c;
        return true;
    }

    // Giro a executar antes do passo 'i', partindo do robô virado para 'orientacaoInicial'
    // na origem. Retorna GIRO_NENHUM se i >= numPassos.
    Giro giroDoPasso(uint8_t i, Direcao orientacaoInicial) const {
        if (i >= numPassos) return GIRO_NENHUM;
        const Direcao atual = (i == 0) ? orientacaoInicial : passos[i - 1];
        const Direcao alvo = passos[i];
        if (alvo == atual) return GIRO_NENHUM;
        if (alvo == girarDireita(atual)) return GIRO_DIREITA;
        if (alvo == girarEsquerda(atual)) return GIRO_ESQUERDA;
        return GIRO_MEIA_VOLTA;
    }
};

class RotaMaisCurta {
public:
    // Calcula a rota de (linhaOrigem, colunaOrigem) a (linhaDestino, colunaDestino).
    // Em qualquer retorno diferente de ROTA_OK, 'saida' fica vazia (numPassos = 0).
    // Origem igual ao destino devolve ROTA_OK com zero passos. Determinístico: os vizinhos
    // são expandidos sempre na ordem norte, sul, leste, oeste, então, entre rotas de mesmo
    // tamanho, a escolhida é sempre a mesma para o mesmo mapa.
    static ResultadoRota calcular(const Mapa& mapa,
                                  uint8_t linhaOrigem, uint8_t colunaOrigem,
                                  uint8_t linhaDestino, uint8_t colunaDestino,
                                  Rota& saida) {
        saida = Rota();
        if (!mapa.dentroDosLimites(linhaOrigem, colunaOrigem) ||
            !mapa.dentroDosLimites(linhaDestino, colunaDestino)) {
            return ROTA_ENTRADA_INVALIDA;
        }
        saida.linhaOrigem = linhaOrigem;
        saida.colunaOrigem = colunaOrigem;
        if (linhaOrigem == linhaDestino && colunaOrigem == colunaDestino) return ROTA_OK;

        const uint8_t colunas = mapa.getMaxColunas();
        const uint8_t destino = (uint8_t)(linhaDestino * colunas + colunaDestino);
        uint8_t entrou[CELULAS_MAX];  // direção usada para entrar na célula; SEM_ENTRADA = não alcançada
        uint8_t fila[CELULAS_MAX];
        for (uint8_t i = 0; i < CELULAS_MAX; i++) entrou[i] = SEM_ENTRADA;

        const uint8_t origem = (uint8_t)(linhaOrigem * colunas + colunaOrigem);
        entrou[origem] = ORIGEM;
        uint8_t ini = 0, fim = 0;
        fila[fim++] = origem;

        while (ini < fim && entrou[destino] == SEM_ENTRADA) {
            const uint8_t u = fila[ini++];
            const uint8_t lu = (uint8_t)(u / colunas);
            const uint8_t cu = (uint8_t)(u % colunas);
            for (uint8_t d = 0; d < 4; d++) {
                const Direcao dir = (Direcao)d;
                if (!mapa.ladoConhecido(lu, cu, dir) || mapa.temParede(lu, cu, dir)) continue;
                uint8_t lv, cv;
                if (!mapa.vizinha(lu, cu, dir, lv, cv)) continue;
                const uint8_t v = (uint8_t)(lv * colunas + cv);
                if (entrou[v] != SEM_ENTRADA) continue;
                entrou[v] = d;
                fila[fim++] = v;
            }
        }
        if (entrou[destino] == SEM_ENTRADA) return ROTA_INEXISTENTE;

        // Reconstrução de trás para frente: desfaz a direção de entrada em cada célula.
        uint8_t n = 0;
        uint8_t atual = destino;
        while (atual != origem) {
            const Direcao dir = (Direcao)entrou[atual];
            if (n >= ROTA_MAX_PASSOS) {  // defensivo: a BFS não gera caminho maior que 47 passos
                saida = Rota();
                return ROTA_INEXISTENTE;
            }
            saida.passos[n++] = dir;
            uint8_t la, ca;
            mapa.vizinha((uint8_t)(atual / colunas), (uint8_t)(atual % colunas),
                         direcaoOposta(dir), la, ca);
            atual = (uint8_t)(la * colunas + ca);
        }
        for (uint8_t i = 0; i < n / 2; i++) {  // inverte para a ordem origem -> destino
            const Direcao t = saida.passos[i];
            saida.passos[i] = saida.passos[n - 1 - i];
            saida.passos[n - 1 - i] = t;
        }
        saida.numPassos = n;
        return ROTA_OK;
    }

    // Rota da largada A1 (0, 0) ao destino oficial da geometria do mapa: última linha da
    // coluna D (D4, D8 ou D12), igual ao destino do Navegador.
    static ResultadoRota calcularParaObjetivo(const Mapa& mapa, Rota& saida) {
        return calcular(mapa, 0, 0, (uint8_t)(mapa.getMaxLinhas() - 1),
                        (uint8_t)(mapa.getMaxColunas() - 1), saida);
    }

private:
    static const uint8_t CELULAS_MAX = 48;
    static const uint8_t SEM_ENTRADA = 0xFF;
    static const uint8_t ORIGEM = 0xFE;
};

#endif
