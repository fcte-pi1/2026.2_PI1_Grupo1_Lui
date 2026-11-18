#ifndef INTERFACES_MOVIMENTO_H
#define INTERFACES_MOVIMENTO_H

#include <stdint.h>
#include "../mapa/Direcao.h"
#include "../navegacao/InterfacesNavegacao.h"

// Executor de movimento sem bloqueio (docs/firmware/integracao-loop.md, seção 3.5).
//
// O ExecutorMovimento da HU-06 só retorna quando o movimento terminou, o que no robô
// são 1 a 2 s por célula sem telemetria, botão nem monitor de travamento. Esta interface
// divide o movimento em "iniciar" e "atualizar": o laço principal chama atualizar() no
// ritmo de controle e, quando o resultado é MOV_CONCLUIDO ou MOV_FALHOU, confirma ou
// registra a falha no Navegador, como o passo() já fazia. ExecutorMovimento continua
// existindo para os testes e para o SimuladorLabirinto; ExecutorImediato adapta um ao outro.

enum ResultadoMovimento : uint8_t {
    MOV_OCIOSO,        // nenhum movimento em andamento
    MOV_EM_ANDAMENTO,  // chamar atualizar() de novo no próximo ciclo
    MOV_CONCLUIDO,     // movimento confirmado; o laço pode atualizar o Navegador
    MOV_FALHOU         // movimento abortado (tempo, emergência, tração cortada)
};

// Motivo da última falha, para o laço decidir o estado publicado (seção 3.7 do desenho):
// FALHA_EMERGENCIA_FRONTAL leva a COLISAO; FALHA_TRACAO_CORTADA e FALHA_TEMPO levam a
// TRAVADO (a de tempo só quando o Navegador desiste, na terceira falha seguida).
enum MotivoFalhaMovimento : uint8_t {
    FALHA_NENHUMA,
    FALHA_TEMPO,              // o movimento não terminou no tempo limite
    FALHA_EMERGENCIA_FRONTAL, // obstáculo à frente antes de completar o avanço
    FALHA_TRACAO_CORTADA      // a tração foi bloqueada por fora (travamento ou parada)
};

class ExecutorMovimentoNaoBloqueante {
public:
    virtual ~ExecutorMovimentoNaoBloqueante() {}

    // Começa um giro no lugar. 'orientacaoAtual' é a do Navegador (fonte única da
    // orientação) e serve para o executor derivar o rumo nominal da curva. Retorna false,
    // sem mudar mais nada (nem o motivo da última falha), se já houver movimento em
    // andamento ou se o giro for GIRO_NENHUM.
    virtual bool iniciarGiro(Giro giro, Direcao orientacaoAtual, uint32_t agoraMs) = 0;

    // Começa o avanço de uma célula na orientação atual. Retorna false, sem mudar mais
    // nada, se já houver movimento em andamento.
    virtual bool iniciarAvanco(uint32_t agoraMs) = 0;

    // Avança o movimento em andamento. Depois de devolver MOV_CONCLUIDO ou MOV_FALHOU,
    // volta a MOV_OCIOSO na chamada seguinte, sem repetir o resultado.
    virtual ResultadoMovimento atualizar(uint32_t agoraMs) = 0;

    // Interrompe o que estiver em andamento e para os motores; o executor fica ocioso.
    virtual void abortar() = 0;

    virtual bool ocioso() const = 0;

    // Largada ou nova tentativa: o robô está parado e alinhado à grade. Quem estima o rumo
    // (ExecutorMovimentoRobo) zera a estimativa; os demais não precisam fazer nada.
    virtual void reiniciar() {}

    // Motivo da última falha entregue por atualizar(); FALHA_NENHUMA depois de um
    // movimento concluído.
    virtual MotivoFalhaMovimento motivoUltimaFalha() const = 0;
};

// Opcional para um ExecutorMovimento bloqueante: informa por que o último movimento
// falhou. Sem ela, o ExecutorImediato reporta toda falha como FALHA_TEMPO. Com ela, os
// testes no ambiente nativo exercitam COLISAO e TRAVADO (seção 3.7 do desenho).
class InformanteMotivoFalha {
public:
    virtual ~InformanteMotivoFalha() {}
    virtual MotivoFalhaMovimento motivoFalha() const = 0;
};

// Adapta um ExecutorMovimento bloqueante (por exemplo, o SimuladorLabirinto dos testes)
// à interface sem bloqueio: o movimento é executado inteiro no primeiro atualizar().
// É o que permite rodar o laço completo no ambiente nativo sobre o simulador.
class ExecutorImediato : public ExecutorMovimentoNaoBloqueante {
public:
    explicit ExecutorImediato(ExecutorMovimento& executor,
                              const InformanteMotivoFalha* informante = nullptr)
        : executor_(executor), informante_(informante), pendente_(PENDENTE_NADA),
          giro_(GIRO_NENHUM), motivo_(FALHA_NENHUMA) {}

    bool iniciarGiro(Giro giro, Direcao, uint32_t) override {
        if (pendente_ != PENDENTE_NADA || giro == GIRO_NENHUM) return false;
        pendente_ = PENDENTE_GIRO;
        giro_ = giro;
        return true;
    }

    bool iniciarAvanco(uint32_t) override {
        if (pendente_ != PENDENTE_NADA) return false;
        pendente_ = PENDENTE_AVANCO;
        return true;
    }

    ResultadoMovimento atualizar(uint32_t) override {
        if (pendente_ == PENDENTE_NADA) return MOV_OCIOSO;
        const bool ok = (pendente_ == PENDENTE_GIRO) ? executor_.girar(giro_)
                                                     : executor_.avancarCelula();
        pendente_ = PENDENTE_NADA;
        if (ok) {
            motivo_ = FALHA_NENHUMA;
            return MOV_CONCLUIDO;
        }
        motivo_ = (informante_ != nullptr) ? informante_->motivoFalha() : FALHA_TEMPO;
        return MOV_FALHOU;
    }

    void abortar() override { pendente_ = PENDENTE_NADA; }

    bool ocioso() const override { return pendente_ == PENDENTE_NADA; }
    MotivoFalhaMovimento motivoUltimaFalha() const override { return motivo_; }

private:
    enum Pendente : uint8_t { PENDENTE_NADA, PENDENTE_GIRO, PENDENTE_AVANCO };

    ExecutorMovimento& executor_;
    const InformanteMotivoFalha* informante_;
    Pendente pendente_;
    Giro giro_;
    MotivoFalhaMovimento motivo_;
};

#endif
