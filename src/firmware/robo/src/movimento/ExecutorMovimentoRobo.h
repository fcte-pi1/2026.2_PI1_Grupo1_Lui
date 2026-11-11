#ifndef EXECUTOR_MOVIMENTO_ROBO_H
#define EXECUTOR_MOVIMENTO_ROBO_H

#include <stdint.h>
#include "InterfacesMovimento.h"
#include "../app/InterfacesHardware.h"           // ISensoresToF
#include "../controle/ControleCurva.h"
#include "../controle/ControleReta.h"
#include "../controle/ConversorVelocidadePwm.h"
#include "../drivers/Motores.h"
#include "../hardware/Pinos.h"
#include "../mapa/Direcao.h"
#include "../odometria/Odometria.h"

// Executor de movimento do robô, sem bloqueio e sem dependência do Arduino
// (docs/firmware/integracao-loop.md, seção 3.5, achados B3, B5, B7 e R11).
//
// Implementa ExecutorMovimentoNaoBloqueante sobre ControleCurva, ControleReta, Odometria,
// ConversorVelocidadePwm e Motores. O laço chama atualizar() a cada tick de controle
// (10 ms); cada chamada faz uma volta de controle e devolve MOV_EM_ANDAMENTO até o fim.
//
// Giro (90 graus para cada lado; meia-volta = duas curvas seguidas para a direita):
//  - Sem IMU, cada curva é relativa (B5): antes de cada uma, o ControleCurva é recriado e
//    ressincronizado com o rumo nominal da orientação atual, e esse mesmo valor é o yaw
//    passado em iniciar(), com imuValido = false. O ângulo sai só dos encoders.
//  - CURVA_CONCLUIDA -> freio; CURVA_FALHOU (tempo) -> freio e FALHA_TEMPO.
//
// Avanço de uma célula:
//  - ControleReta com as distâncias laterais, rodado só quando chega amostra nova de um dos
//    sensores laterais (R11); entre amostras vale o último comando.
//  - Termina pela odometria (média das rodas >= alvo em pulsos) ou pelo encosto frontal
//    (parede à frente a DIST_ENCOSTO_FRONTAL_MM, depois de metade do avanço).
//  - Emergência: frontal <= DIST_MINIMA_SEGURA_MM antes de 60 % do avanço ->
//    FALHA_EMERGENCIA_FRONTAL. Tempo acima do limite -> FALHA_TEMPO.
//
// Fim de todo movimento, como no hw/firm: freio ativo, espera de tempoFreioMs (200 ms) para
// o robô assentar e as leituras da célula nova serem feitas parado, e roda livre. Só então
// o resultado é entregue.
//
// Trinco (B7): se os motores forem bloqueados por fora (MonitorTravamento ou
// ParadaObjetivo), o movimento acaba na hora com FALHA_TRACAO_CORTADA, sem novo comando.

struct ParametrosExecutor {
    int32_t alvoAvancoPulsos;       // pulsos de cada roda (média) para avançar uma célula
    uint32_t tempoLimiteAvancoMs;   // desiste do avanço depois disso
    uint32_t tempoLimiteCurvaMs;    // desiste de cada curva de 90 graus depois disso
    uint32_t tempoFreioMs;          // freio e assentamento no fim de cada movimento
    uint16_t distEncostoFrontalMm;  // encosto frontal: fim do avanço com parede à frente
    uint16_t distMinimaSeguraMm;    // emergência frontal
    float fracaoEmergencia;         // a emergência só vale antes desta fração do alvo
    float fracaoMinimaEncosto;      // o encosto só vale depois desta fração do alvo
};

// Rumo nominal de cada orientação de grade, no referencial do yaw (anti-horário positivo,
// leste = 0, como na largada).
inline float rumoNominalGraus(Direcao d) {
    switch (d) {
        case DIR_LESTE: return 0.0f;
        case DIR_NORTE: return 90.0f;
        case DIR_OESTE: return 180.0f;
        case DIR_SUL:   return -90.0f;
    }
    return 0.0f;
}

class ExecutorMovimentoRobo : public ExecutorMovimentoNaoBloqueante {
public:
    // Valores do hw/firm, o único código que já moveu o robô (testes 4 a 6 do menu):
    // 3 s de limite no avanço, 2 s por curva, freio e 200 ms de espera no fim. O alvo do
    // avanço é a célula de 180 mm em pulsos (o hw/firm usa 168 mm, o vão do corredor).
    static ParametrosExecutor parametrosPadrao() {
        ParametrosExecutor p;
        p.alvoAvancoPulsos = Odometria::pulsosParaMm((float)CELULA_MM);
        p.tempoLimiteAvancoMs = 3000;
        p.tempoLimiteCurvaMs = 2000;
        p.tempoFreioMs = 200;
        p.distEncostoFrontalMm = DIST_ENCOSTO_FRONTAL_MM;
        p.distMinimaSeguraMm = DIST_MINIMA_SEGURA_MM;
        p.fracaoEmergencia = 0.6f;
        p.fracaoMinimaEncosto = 0.5f;
        return p;
    }

    // O controle de reta usa o mesmo limiar de parede do ClassificadorParedes (R3).
    static ParametrosControleReta parametrosReta() {
        ParametrosControleReta r = ControleReta::parametrosPadrao();
        r.distMaxParedeMm = (float)LIMIAR_PAREDE_MM;
        return r;
    }

    static ParametrosControleCurva parametrosCurva(const ParametrosExecutor& p) {
        ParametrosControleCurva c = ControleCurva::parametrosPadrao();
        c.tempoLimiteMs = p.tempoLimiteCurvaMs;
        return c;
    }

    ExecutorMovimentoRobo(Motores& motores, Odometria& odometria, ISensoresToF& tof,
                          const ParametrosExecutor& p = parametrosPadrao(),
                          const ConversorVelocidadePwm& conversor = ConversorVelocidadePwm())
        : motores_(motores), odometria_(odometria), tof_(tof), p_(p),
          parametrosCurva_(parametrosCurva(p)), conversor_(conversor),
          reta_(parametrosReta()), curva_(parametrosCurva_), fase_(FASE_OCIOSO),
          sentido_(SENTIDO_ESQUERDA), curvasRestantes_(0), rumoGraus_(0.0f), inicioMs_(0),
          encEsq0_(0), encDir0_(0), primeiroComando_(false), freioDesdeMs_(0),
          resultadoPendente_(MOV_OCIOSO), motivoPendente_(FALHA_NENHUMA),
          motivo_(FALHA_NENHUMA) {}

    bool iniciarGiro(Giro giro, Direcao orientacaoAtual, uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO || giro == GIRO_NENHUM) return false;
        sentido_ = (giro == GIRO_ESQUERDA) ? SENTIDO_ESQUERDA : SENTIDO_DIREITA;
        curvasRestantes_ = (giro == GIRO_MEIA_VOLTA) ? 2 : 1;
        rumoGraus_ = rumoNominalGraus(orientacaoAtual);
        comecarCurva(agoraMs);
        fase_ = FASE_GIRANDO;
        return true;
    }

    bool iniciarAvanco(uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO) return false;
        encEsq0_ = odometria_.encEsq();
        encDir0_ = odometria_.encDir();
        reta_.iniciar(0.0f, encEsq0_, encDir0_, agoraMs);
        inicioMs_ = agoraMs;
        primeiroComando_ = true;
        fase_ = FASE_AVANCANDO;
        return true;
    }

    ResultadoMovimento atualizar(uint32_t agoraMs) override {
        if (fase_ == FASE_OCIOSO) return MOV_OCIOSO;

        if ((fase_ == FASE_GIRANDO || fase_ == FASE_AVANCANDO) && motores_.bloqueado()) {
            fase_ = FASE_OCIOSO;  // corte de segurança por fora: nenhum comando novo
            motivo_ = FALHA_TRACAO_CORTADA;
            return MOV_FALHOU;
        }

        switch (fase_) {
            case FASE_GIRANDO:   return atualizarGiro(agoraMs);
            case FASE_AVANCANDO: return atualizarAvanco(agoraMs);
            case FASE_FREANDO:   return atualizarFreio(agoraMs);
            case FASE_OCIOSO:    break;
        }
        return MOV_OCIOSO;
    }

    void abortar() override {
        if (fase_ == FASE_OCIOSO) return;
        motores_.frear();  // ignorado se o trinco estiver fechado, que já freou
        curva_ = ControleCurva(parametrosCurva_);
        fase_ = FASE_OCIOSO;
    }

    bool ocioso() const override { return fase_ == FASE_OCIOSO; }
    MotivoFalhaMovimento motivoUltimaFalha() const override { return motivo_; }

    // Para log e testes: pulsos médios andados no avanço atual (ou no último).
    int32_t progressoAvancoPulsos() const {
        return ((odometria_.encEsq() - encEsq0_) + (odometria_.encDir() - encDir0_)) / 2;
    }
    const ParametrosExecutor& parametros() const { return p_; }

private:
    enum Fase : uint8_t { FASE_OCIOSO, FASE_GIRANDO, FASE_AVANCANDO, FASE_FREANDO };

    void comecarCurva(uint32_t agoraMs) {
        curva_ = ControleCurva(parametrosCurva_);  // sem resto de uma curva anterior ou abortada
        curva_.ressincronizar(rumoGraus_);
        curva_.iniciar(sentido_, rumoGraus_, odometria_.encEsq(), odometria_.encDir(), agoraMs);
    }

    // Converte e aplica; com as duas rodas em zero, freia em vez de soltar (assentamento).
    void comandar(float velEsqMmS, float velDirMmS) {
        const int16_t pwmEsq = conversor_.converter(velEsqMmS);
        const int16_t pwmDir = conversor_.converter(velDirMmS);
        if (pwmEsq == 0 && pwmDir == 0) {
            motores_.frear();
        } else {
            motores_.comandar(pwmEsq, pwmDir);
        }
    }

    ResultadoMovimento terminar(uint32_t agoraMs, ResultadoMovimento resultado,
                                MotivoFalhaMovimento motivo) {
        motores_.frear();
        freioDesdeMs_ = agoraMs;
        resultadoPendente_ = resultado;
        motivoPendente_ = motivo;
        fase_ = FASE_FREANDO;
        return MOV_EM_ANDAMENTO;
    }

    ResultadoMovimento atualizarGiro(uint32_t agoraMs) {
        EntradaControleCurva e;
        e.yawGraus = rumoGraus_;
        e.imuValido = false;
        e.encEsq = odometria_.encEsq();
        e.encDir = odometria_.encDir();
        e.agoraMs = agoraMs;
        const SaidaControleCurva s = curva_.atualizar(e);

        if (s.estado == CURVA_CONCLUIDA) {
            rumoGraus_ = ControleCurva::normalizarGraus(
                rumoGraus_ + (sentido_ == SENTIDO_ESQUERDA ? 90.0f : -90.0f));
            if (--curvasRestantes_ > 0) {  // segunda metade da meia-volta
                comecarCurva(agoraMs);
                motores_.frear();
                return MOV_EM_ANDAMENTO;
            }
            return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        }
        if (s.estado == CURVA_FALHOU) return terminar(agoraMs, MOV_FALHOU, FALHA_TEMPO);

        comandar(s.velEsqMmS, s.velDirMmS);
        return MOV_EM_ANDAMENTO;
    }

    ResultadoMovimento atualizarAvanco(uint32_t agoraMs) {
        const int32_t progresso = progressoAvancoPulsos();
        const float alvo = (float)p_.alvoAvancoPulsos;

        LeituraToF& frontal = tof_.frontal();
        if (frontal.valida(agoraMs)) {
            const uint16_t d = frontal.distanciaMm();
            if (progresso < alvo * p_.fracaoEmergencia && d <= p_.distMinimaSeguraMm) {
                return terminar(agoraMs, MOV_FALHOU, FALHA_EMERGENCIA_FRONTAL);
            }
            if (progresso >= alvo * p_.fracaoMinimaEncosto && d <= p_.distEncostoFrontalMm) {
                return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);  // encosto frontal
            }
        }
        if (progresso >= p_.alvoAvancoPulsos) return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        if ((uint32_t)(agoraMs - inicioMs_) > p_.tempoLimiteAvancoMs) {
            return terminar(agoraMs, MOV_FALHOU, FALHA_TEMPO);
        }

        LeituraToF& esq = tof_.esquerdo();
        LeituraToF& dir = tof_.direito();
        if (primeiroComando_ || esq.nova() || dir.nova()) {
            esq.consumir();
            dir.consumir();
            primeiroComando_ = false;
            EntradaControleReta e;
            e.distEsqMm = esq.distanciaParaControle(agoraMs);
            e.distDirMm = dir.distanciaParaControle(agoraMs);
            e.yawGraus = 0.0f;
            e.imuValido = false;
            e.encEsq = odometria_.encEsq();
            e.encDir = odometria_.encDir();
            e.agoraMs = agoraMs;
            const SaidaControleReta s = reta_.atualizar(e);
            comandar(s.velEsqMmS, s.velDirMmS);
        }
        return MOV_EM_ANDAMENTO;
    }

    ResultadoMovimento atualizarFreio(uint32_t agoraMs) {
        if ((uint32_t)(agoraMs - freioDesdeMs_) < p_.tempoFreioMs) return MOV_EM_ANDAMENTO;
        motores_.soltar();  // roda livre depois do freio, como no hw/firm
        fase_ = FASE_OCIOSO;
        motivo_ = motivoPendente_;
        return resultadoPendente_;
    }

    Motores& motores_;
    Odometria& odometria_;
    ISensoresToF& tof_;
    ParametrosExecutor p_;
    ParametrosControleCurva parametrosCurva_;
    ConversorVelocidadePwm conversor_;
    ControleReta reta_;
    ControleCurva curva_;
    Fase fase_;
    SentidoCurva sentido_;
    uint8_t curvasRestantes_;
    float rumoGraus_;
    uint32_t inicioMs_;
    int32_t encEsq0_, encDir0_;
    bool primeiroComando_;
    uint32_t freioDesdeMs_;
    ResultadoMovimento resultadoPendente_;
    MotivoFalhaMovimento motivoPendente_;
    MotivoFalhaMovimento motivo_;
};

#endif
