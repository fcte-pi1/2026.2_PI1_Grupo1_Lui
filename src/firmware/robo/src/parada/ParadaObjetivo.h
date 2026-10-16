#ifndef PARADA_OBJETIVO_H
#define PARADA_OBJETIVO_H

// HU-07 / RF-SFT04 — Parar automaticamente ao chegar no objetivo.
//
// Módulo só de cabeçalho (como o Mapa), sem dependência de Arduino, sem
// alocação dinâmica e sem STL. Tração, cronômetro e relógio entram por
// interfaces; a telemetria é atendida preenchendo o TelemetriaPacket
// (lib telemetria) com status CONCLUIDO e desafio cumprido.

#include <stdint.h>

#include "TelemetriaPacket.h"  // EstadoRobo, TelemetriaPacket (lib/telemetria)

// Célula da área objetivo, no mesmo padrão do Mapa: (linha, coluna).
struct CelulaObjetivo {
    uint8_t linha;
    uint8_t coluna;
};

// Tração: parar() deve cortar o comando dos motores imediatamente.
class ITracao {
public:
    virtual ~ITracao() {}
    virtual void parar() = 0;
};

// Cronômetro da prova: congelar() interrompe a contagem no instante da chamada.
class ICronometro {
public:
    virtual ~ICronometro() {}
    virtual void congelar() = 0;
};

// Relógio em microssegundos (opcional), só para medir a latência de software
// entre detectar a chegada e emitir o comando de parar.
class IRelogioUs {
public:
    virtual ~IRelogioUs() {}
    virtual uint32_t agoraUs() = 0;
};

class ParadaObjetivo {
public:
    // A área objetivo pode ter mais de uma célula (ex.: região 2x2).
    enum : uint8_t { MAX_CELULAS_OBJETIVO = 4 };
    // Critério de aceitação do RF-SFT04: tração interrompida em < 200 ms.
    enum : uint32_t { LIMITE_PARADA_US = 200000UL };
    // Coluna D (índice interno 3), onde fica a chegada em todas as geometrias.
    enum : uint8_t { COLUNA_D = 3 };

    ParadaObjetivo(ITracao& tracao, ICronometro& cronometro, IRelogioUs* relogio = nullptr)
        : tracao_(tracao),
          cronometro_(cronometro),
          relogio_(relogio),
          numObjetivo_(0),
          concluido_(false),
          latenciaUs_(0) {
        for (uint8_t i = 0; i < MAX_CELULAS_OBJETIVO; i++) {
            objetivo_[i].linha = 0;
            objetivo_[i].coluna = 0;
        }
    }

    // Define a área objetivo. Retorna false (mantendo a configuração anterior)
    // se `celulas` for nulo, `n` for 0 ou maior que MAX_CELULAS_OBJETIVO.
    bool configurarObjetivo(const CelulaObjetivo* celulas, uint8_t n) {
        if (celulas == nullptr || n == 0 || n > MAX_CELULAS_OBJETIVO) return false;
        for (uint8_t i = 0; i < n; i++) objetivo_[i] = celulas[i];
        numObjetivo_ = n;
        return true;
    }

    // Configura a chegada oficial da prova: última linha da coluna D
    // (D4 na 4x4, D8 na 8x4, D12 na 12x4), isto é, (linhas - 1, 3).
    // A partida é A1 = (0, 0). Retorna false para geometria desconhecida,
    // mantendo a configuração anterior.
    bool configurarObjetivoPorGeometria(GeometriaLabirinto geometria) {
        uint8_t linhas = 0;
        switch (geometria) {
            case LAB_4X4:  linhas = 4;  break;
            case LAB_8X4:  linhas = 8;  break;
            case LAB_12X4: linhas = 12; break;
            default:       return false;
        }
        const CelulaObjetivo chegada = {(uint8_t)(linhas - 1), COLUNA_D};
        return configurarObjetivo(&chegada, 1);
    }

    // Reinicia o estado para uma nova prova.
    void iniciarProva() {
        concluido_ = false;
        latenciaUs_ = 0;
    }

    // Chamar a cada ciclo de controle com a célula atual do robô.
    // Na primeira vez em que a célula estiver na área objetivo: para a tração e
    // congela o cronômetro, nessa ordem. Depois disso fica travado em
    // "concluído" até iniciarProva(): não repete comandos nem libera a tração.
    void atualizar(uint8_t linha, uint8_t coluna) {
        if (concluido_) return;
        if (!naAreaObjetivo(linha, coluna)) return;

        const uint32_t t0 = (relogio_ != nullptr) ? relogio_->agoraUs() : 0;

        tracao_.parar();  // primeiro a tração: é o critério com limite de 200 ms

        if (relogio_ != nullptr) {
            latenciaUs_ = relogio_->agoraUs() - t0;  // sem sinal: correto mesmo com overflow
        }

        cronometro_.congelar();  // no mesmo ciclo, logo após parar a tração
        concluido_ = true;
    }

    bool concluido() const { return concluido_; }
    bool desafioCumprido() const { return concluido_; }

    // Latência de software da última parada, em µs (0 se não houver relógio ou parada).
    uint32_t latenciaUltimaParadaUs() const { return latenciaUs_; }

    // Para o módulo de telemetria chamar antes de codificar o pacote: se a prova
    // foi concluída, grava status_id = CONCLUIDO e desafio_cumprido = 1.
    // Antes disso não altera nada, para não atrapalhar MAPEANDO/SPEED_RUN etc.
    void aplicarNoPacote(TelemetriaPacket& pacote) const {
        if (!concluido_) return;
        pacote.status_id = (uint8_t)CONCLUIDO;
        pacote.desafio_cumprido = 1;
    }

private:
    bool naAreaObjetivo(uint8_t linha, uint8_t coluna) const {
        for (uint8_t i = 0; i < numObjetivo_; i++) {
            if (objetivo_[i].linha == linha && objetivo_[i].coluna == coluna) return true;
        }
        return false;
    }

    ITracao& tracao_;
    ICronometro& cronometro_;
    IRelogioUs* relogio_;

    CelulaObjetivo objetivo_[MAX_CELULAS_OBJETIVO];
    uint8_t numObjetivo_;
    bool concluido_;
    uint32_t latenciaUs_;
};

#endif  // PARADA_OBJETIVO_H
