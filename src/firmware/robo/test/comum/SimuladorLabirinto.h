#ifndef SIMULADOR_LABIRINTO_H
#define SIMULADOR_LABIRINTO_H

#include <stdint.h>
#include "../../src/mapa/Mapa.h"
#include "../../src/navegacao/InterfacesNavegacao.h"

// Ambiente de simulação dos testes da HU-06. Guarda o labirinto VERDADEIRO (que o
// Navegador nunca vê) e implementa os sensores e o executor sobre ele. O Navegador só
// recebe o que os sensores simulados leem na célula em que o robô realmente está.
class SimuladorLabirinto : public SensoresParedes, public ExecutorMovimento {
public:
    static const uint8_t LINHAS_MAX = 12;
    static const uint8_t COLUNAS = 4;
    static const uint16_t TRACO_MAX = 4096;

    uint8_t linhas;
    uint8_t verdadeiro[LINHAS_MAX][COLUNAS];  // bitmask PAREDE_* de cada célula

    // Posição e orientação físicas (independentes do estado lógico do Navegador).
    uint8_t linhaReal, colunaReal;
    Direcao orientacaoReal;

    // Métricas e injeção de falhas.
    uint16_t avancos, girosFeitos, movimentosInvalidos, leituras;
    uint8_t falhasDeAvancoPendentes;
    uint8_t falhasDeGiroPendentes;
    uint8_t falhasDeLeituraPendentes;

    // Sequência de células ocupadas (para o teste de determinismo).
    uint8_t traco[TRACO_MAX];
    uint16_t tamanhoTraco;

    explicit SimuladorLabirinto(uint8_t numLinhas) : linhas(numLinhas) { reiniciar(); }

    void reiniciar() {
        for (uint8_t l = 0; l < LINHAS_MAX; l++)
            for (uint8_t c = 0; c < COLUNAS; c++) verdadeiro[l][c] = PAREDE_TODAS;
        linhaReal = 0;
        colunaReal = 0;
        orientacaoReal = DIR_LESTE;
        avancos = girosFeitos = movimentosInvalidos = leituras = 0;
        falhasDeAvancoPendentes = falhasDeGiroPendentes = falhasDeLeituraPendentes = 0;
        tamanhoTraco = 0;
    }

    // Todo o interior aberto; só o perímetro fica fechado.
    void abrirTudo() {
        for (uint8_t l = 0; l < linhas; l++)
            for (uint8_t c = 0; c < COLUNAS; c++) {
                uint8_t p = 0;
                if (l == 0) p |= PAREDE_NORTE;
                if (l == linhas - 1) p |= PAREDE_SUL;
                if (c == 0) p |= PAREDE_OESTE;
                if (c == COLUNAS - 1) p |= PAREDE_LESTE;
                verdadeiro[l][c] = p;
            }
    }

    void abrir(uint8_t l, uint8_t c, Direcao d) {
        const int nl = l + deltaLinha(d), nc = c + deltaColuna(d);
        verdadeiro[l][c] &= (uint8_t)~mascaraParede(d);
        verdadeiro[nl][nc] &= (uint8_t)~mascaraParede(direcaoOposta(d));
    }

    void fechar(uint8_t l, uint8_t c, Direcao d) {
        verdadeiro[l][c] |= mascaraParede(d);
        const int nl = l + deltaLinha(d), nc = c + deltaColuna(d);
        if (nl >= 0 && nl < linhas && nc >= 0 && nc < COLUNAS)
            verdadeiro[nl][nc] |= mascaraParede(direcaoOposta(d));
    }

    // Abre um corredor a partir de (l, c) seguindo a sequência 'N', 'S', 'L', 'O'.
    void abrirCaminho(uint8_t l, uint8_t c, const char* movimentos) {
        for (; *movimentos; movimentos++) {
            const Direcao d = *movimentos == 'N' ? DIR_NORTE : *movimentos == 'S' ? DIR_SUL
                            : *movimentos == 'L' ? DIR_LESTE : DIR_OESTE;
            abrir(l, c, d);
            l = (uint8_t)(l + deltaLinha(d));
            c = (uint8_t)(c + deltaColuna(d));
        }
    }

    bool temParedeReal(uint8_t l, uint8_t c, Direcao d) const {
        return (verdadeiro[l][c] & mascaraParede(d)) != 0;
    }

    bool naChegada() const { return linhaReal == linhas - 1 && colunaReal == COLUNAS - 1; }

    // --- SensoresParedes ---
    bool lerParedes(bool& frente, bool& esquerda, bool& direita) override {
        if (falhasDeLeituraPendentes > 0) {
            falhasDeLeituraPendentes--;
            return false;
        }
        leituras++;
        frente = temParedeReal(linhaReal, colunaReal, direcaoAbsoluta(orientacaoReal, LADO_FRENTE));
        esquerda = temParedeReal(linhaReal, colunaReal, direcaoAbsoluta(orientacaoReal, LADO_ESQUERDA));
        direita = temParedeReal(linhaReal, colunaReal, direcaoAbsoluta(orientacaoReal, LADO_DIREITA));
        return true;
    }

    // --- ExecutorMovimento ---
    bool girar(Giro giro) override {
        if (falhasDeGiroPendentes > 0) {
            falhasDeGiroPendentes--;
            return false;
        }
        switch (giro) {
            case GIRO_DIREITA:    orientacaoReal = girarDireita(orientacaoReal); break;
            case GIRO_ESQUERDA:   orientacaoReal = girarEsquerda(orientacaoReal); break;
            case GIRO_MEIA_VOLTA: orientacaoReal = direcaoOposta(orientacaoReal); break;
            case GIRO_NENHUM:     return true;
        }
        girosFeitos++;
        return true;
    }

    bool avancarCelula() override {
        if (falhasDeAvancoPendentes > 0) {
            falhasDeAvancoPendentes--;
            return false;
        }
        if (temParedeReal(linhaReal, colunaReal, orientacaoReal)) {
            movimentosInvalidos++;  // o robô tentou atravessar uma parede verdadeira
            return false;
        }
        linhaReal = (uint8_t)(linhaReal + deltaLinha(orientacaoReal));
        colunaReal = (uint8_t)(colunaReal + deltaColuna(orientacaoReal));
        avancos++;
        if (tamanhoTraco < TRACO_MAX) traco[tamanhoTraco++] = (uint8_t)(linhaReal * COLUNAS + colunaReal);
        return true;
    }
};

#endif
