#ifndef MAPA_H
#define MAPA_H

#include <stdint.h>
#include "TelemetriaPacket.h"
#include "Direcao.h"

// Mapa do labirinto em memória (doc 4.4, seção 5.1). Linhas e colunas começam em 0
// e a largada é (0, 0), no canto superior esquerdo. Para cada célula o mapa guarda
// as paredes, os lados já conhecidos e se a célula foi visitada. Toda parede ou
// passagem registrada em uma célula também é gravada na vizinha correspondente.
class Mapa {
private:
    static const uint8_t LINHAS_MAX = 12;
    static const uint8_t COLUNAS_MAX = 4;

    GeometriaLabirinto geometria;
    uint8_t max_linhas;
    uint8_t max_colunas;

    // Alocação estática no tamanho máximo (12x4); só a área da geometria atual é usada.
    // 'paredes' e 'conhecidas' usam o bitmask do contrato (PAREDE_NORTE etc.).
    // Lado conhecido sem parede é passagem confirmada; lado desconhecido ainda não
    // foi observado e aparece como sem parede em getParedes().
    uint8_t paredes[LINHAS_MAX][COLUNAS_MAX];
    uint8_t conhecidas[LINHAS_MAX][COLUNAS_MAX];
    bool visitadas[LINHAS_MAX][COLUNAS_MAX];

public:
    Mapa() : geometria(LAB_4X4), max_linhas(4), max_colunas(4) {
        limpar();
    }

    void setGeometria(GeometriaLabirinto geo) {
        geometria = geo;
        max_colunas = 4; // Colunas A a D (sempre 4 nas geometrias oficiais)
        switch(geo) {
            case LAB_4X4: max_linhas = 4; break;
            case LAB_8X4: max_linhas = 8; break;
            case LAB_12X4: max_linhas = 12; break;
        }
        limpar();
    }

    GeometriaLabirinto getGeometria() const {
        return geometria;
    }

    uint8_t getMaxLinhas() const { return max_linhas; }
    uint8_t getMaxColunas() const { return max_colunas; }

    // Compatibilidade temporária se necessário
    uint8_t getMaxX() const { return max_linhas; }
    uint8_t getMaxY() const { return max_colunas; }

    // Volta ao estado inicial da geometria atual: nenhuma célula visitada, perímetro
    // registrado como parede conhecida (toda pista oficial é fechada nas bordas) e
    // todos os lados internos desconhecidos.
    void limpar() {
        for (uint8_t l = 0; l < LINHAS_MAX; l++) {
            for (uint8_t c = 0; c < COLUNAS_MAX; c++) {
                paredes[l][c] = 0;
                conhecidas[l][c] = 0;
                visitadas[l][c] = false;
            }
        }
        registrarPerimetro();
    }

    bool dentroDosLimites(uint8_t linha, uint8_t coluna) const {
        return linha < max_linhas && coluna < max_colunas;
    }

    // Célula vizinha na direção indicada. Retorna false, sem alterar as saídas, se a
    // célula estiver fora da geometria ou se a vizinha ficar além do perímetro.
    bool vizinha(uint8_t linha, uint8_t coluna, Direcao direcao,
                 uint8_t& linhaVizinha, uint8_t& colunaVizinha) const {
        if (!dentroDosLimites(linha, coluna)) return false;
        const int l = linha + deltaLinha(direcao);
        const int c = coluna + deltaColuna(direcao);
        if (l < 0 || c < 0 || !dentroDosLimites((uint8_t)l, (uint8_t)c)) return false;
        linhaVizinha = (uint8_t)l;
        colunaVizinha = (uint8_t)c;
        return true;
    }

    bool marcarVisitada(uint8_t linha, uint8_t coluna) {
        if (!dentroDosLimites(linha, coluna)) return false;
        visitadas[linha][coluna] = true;
        return true;
    }

    bool foiVisitada(uint8_t linha, uint8_t coluna) const {
        return dentroDosLimites(linha, coluna) && visitadas[linha][coluna];
    }

    // Registra parede no lado indicado da célula e no lado oposto da vizinha.
    // Retorna false se a célula estiver fora da geometria.
    bool registrarParede(uint8_t linha, uint8_t coluna, Direcao direcao) {
        return registrarLado(linha, coluna, direcao, true);
    }

    // Registra passagem livre no lado indicado da célula e no lado oposto da vizinha.
    // Retorna false, sem alterar o mapa, se a célula estiver fora da geometria ou se o
    // lado for do perímetro, que é sempre parede.
    bool registrarPassagem(uint8_t linha, uint8_t coluna, Direcao direcao) {
        return registrarLado(linha, coluna, direcao, false);
    }

    // Registra as leituras dos sensores feitas na célula com o robô virado para
    // 'orientacao': cada lado vira parede (true) ou passagem (false), convertido para
    // a direção absoluta. Leituras mais recentes substituem as anteriores. O lado de
    // trás não é lido: ele é registrado por registrarMovimento() ao entrar na célula.
    // Retorna false se a célula estiver fora da geometria ou se alguma leitura indicar
    // passagem no perímetro; essa leitura é ignorada e as demais são registradas.
    bool registrarLeituras(uint8_t linha, uint8_t coluna, Direcao orientacao,
                           bool paredeFrente, bool paredeEsquerda, bool paredeDireita) {
        if (!dentroDosLimites(linha, coluna)) return false;
        bool ok = registrarLado(linha, coluna, direcaoAbsoluta(orientacao, LADO_FRENTE), paredeFrente);
        ok = registrarLado(linha, coluna, direcaoAbsoluta(orientacao, LADO_ESQUERDA), paredeEsquerda) && ok;
        ok = registrarLado(linha, coluna, direcaoAbsoluta(orientacao, LADO_DIREITA), paredeDireita) && ok;
        return ok;
    }

    // Registra o avanço de uma célula, a partir de (linha, coluna), na direção indicada:
    // o lado atravessado vira passagem conhecida nas duas células, mesmo que houvesse
    // parede registrada, e a célula de destino é marcada como visitada. Retorna false,
    // sem alterar o mapa, se a origem estiver fora da geometria ou o destino além do
    // perímetro. A posição de destino pode ser obtida com vizinha().
    bool registrarMovimento(uint8_t linha, uint8_t coluna, Direcao direcao) {
        uint8_t linhaDestino, colunaDestino;
        if (!vizinha(linha, coluna, direcao, linhaDestino, colunaDestino)) return false;
        registrarLado(linha, coluna, direcao, false);
        visitadas[linhaDestino][colunaDestino] = true;
        return true;
    }

    // Paredes da célula no formato do campo 'paredes' do contrato de dados (bits 0 a 3),
    // pronto para a telemetria. Lados não observados aparecem como sem parede.
    // Retorna 0 fora da geometria.
    uint8_t getParedes(uint8_t linha, uint8_t coluna) const {
        return dentroDosLimites(linha, coluna) ? paredes[linha][coluna] : 0;
    }

    // Lados da célula já observados (parede ou passagem), no mesmo bitmask.
    // Retorna 0 fora da geometria.
    uint8_t getLadosConhecidos(uint8_t linha, uint8_t coluna) const {
        return dentroDosLimites(linha, coluna) ? conhecidas[linha][coluna] : 0;
    }

    bool temParede(uint8_t linha, uint8_t coluna, Direcao direcao) const {
        return (getParedes(linha, coluna) & mascaraParede(direcao)) != 0;
    }

    bool ladoConhecido(uint8_t linha, uint8_t coluna, Direcao direcao) const {
        return (getLadosConhecidos(linha, coluna) & mascaraParede(direcao)) != 0;
    }

private:
    // Grava o lado na célula e, se houver vizinha, o lado oposto nela. Nas bordas não
    // há vizinha: o perímetro já é parede, então só passagem é recusada.
    bool registrarLado(uint8_t linha, uint8_t coluna, Direcao direcao, bool parede) {
        if (!dentroDosLimites(linha, coluna)) return false;
        uint8_t linhaVizinha, colunaVizinha;
        if (!vizinha(linha, coluna, direcao, linhaVizinha, colunaVizinha)) return parede;
        gravarLado(linha, coluna, direcao, parede);
        gravarLado(linhaVizinha, colunaVizinha, direcaoOposta(direcao), parede);
        return true;
    }

    void gravarLado(uint8_t linha, uint8_t coluna, Direcao direcao, bool parede) {
        const uint8_t mascara = mascaraParede(direcao);
        if (parede) {
            paredes[linha][coluna] |= mascara;
        } else {
            paredes[linha][coluna] &= (uint8_t)~mascara;
        }
        conhecidas[linha][coluna] |= mascara;
    }

    void registrarPerimetro() {
        for (uint8_t l = 0; l < max_linhas; l++) {
            for (uint8_t c = 0; c < max_colunas; c++) {
                if (l == 0) gravarLado(l, c, DIR_NORTE, true);
                if (l == max_linhas - 1) gravarLado(l, c, DIR_SUL, true);
                if (c == 0) gravarLado(l, c, DIR_OESTE, true);
                if (c == max_colunas - 1) gravarLado(l, c, DIR_LESTE, true);
            }
        }
    }
};

#endif
