#ifndef EXECUTOR_TEMPORIZADO_H
#define EXECUTOR_TEMPORIZADO_H

#include <stdint.h>
#include "../../src/movimento/InterfacesMovimento.h"

// Dublê de teste: como o ExecutorImediato, embrulha um ExecutorMovimento bloqueante (por
// exemplo, o SimuladorLabirinto), mas cada movimento leva 'ciclosPorMovimento' chamadas a
// atualizar() devolvendo MOV_EM_ANDAMENTO antes do resultado. Exercita de verdade o laço
// que espera o movimento terminar (é o ExecutorVirtualTemporizado da seção 3.5 de
// docs/firmware/integracao-loop.md, para o robô virtual da etapa 6).
class ExecutorTemporizado : public ExecutorMovimentoNaoBloqueante {
public:
    ExecutorTemporizado(ExecutorMovimento& executor, uint16_t ciclosPorMovimento,
                        const InformanteMotivoFalha* informante = nullptr)
        : executor_(executor), informante_(informante), ciclos_(ciclosPorMovimento),
          restantes_(0), pendente_(PENDENTE_NADA), giro_(GIRO_NENHUM), motivo_(FALHA_NENHUMA),
          emAndamento_(0), movimentos_(0), abortados_(0) {}

    bool iniciarGiro(Giro giro, Direcao, uint32_t) override {
        if (pendente_ != PENDENTE_NADA || giro == GIRO_NENHUM) return false;
        pendente_ = PENDENTE_GIRO;
        giro_ = giro;
        restantes_ = ciclos_;
        return true;
    }

    bool iniciarAvanco(uint32_t) override {
        if (pendente_ != PENDENTE_NADA) return false;
        pendente_ = PENDENTE_AVANCO;
        restantes_ = ciclos_;
        return true;
    }

    ResultadoMovimento atualizar(uint32_t) override {
        if (pendente_ == PENDENTE_NADA) return MOV_OCIOSO;
        if (restantes_ > 0) {
            restantes_--;
            emAndamento_++;
            return MOV_EM_ANDAMENTO;
        }
        const bool ok = (pendente_ == PENDENTE_GIRO) ? executor_.girar(giro_)
                                                     : executor_.avancarCelula();
        pendente_ = PENDENTE_NADA;
        movimentos_++;
        if (ok) {
            motivo_ = FALHA_NENHUMA;
            return MOV_CONCLUIDO;
        }
        motivo_ = (informante_ != nullptr) ? informante_->motivoFalha() : FALHA_TEMPO;
        return MOV_FALHOU;
    }

    void abortar() override {
        if (pendente_ != PENDENTE_NADA) abortados_++;
        pendente_ = PENDENTE_NADA;
        restantes_ = 0;
    }

    bool ocioso() const override { return pendente_ == PENDENTE_NADA; }
    MotivoFalhaMovimento motivoUltimaFalha() const override { return motivo_; }

    // Métricas para os testes.
    uint32_t vezesEmAndamento() const { return emAndamento_; }
    uint32_t movimentosTerminados() const { return movimentos_; }
    uint32_t movimentosAbortados() const { return abortados_; }

private:
    enum Pendente : uint8_t { PENDENTE_NADA, PENDENTE_GIRO, PENDENTE_AVANCO };

    ExecutorMovimento& executor_;
    const InformanteMotivoFalha* informante_;
    uint16_t ciclos_;
    uint16_t restantes_;
    Pendente pendente_;
    Giro giro_;
    MotivoFalhaMovimento motivo_;
    uint32_t emAndamento_;
    uint32_t movimentos_;
    uint32_t abortados_;
};

#endif
