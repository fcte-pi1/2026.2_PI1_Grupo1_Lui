#ifndef EXECUTOR_MOVIMENTO_ROBO_H
#define EXECUTOR_MOVIMENTO_ROBO_H

#include <math.h>
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
// (docs/firmware/integracao-loop.md, seções 2.5 e 3.5, achados B3, B5, B7 e R11).
//
// Implementa ExecutorMovimentoNaoBloqueante sobre ControleCurva, ControleReta, Odometria,
// ConversorVelocidadePwm e Motores. O laço chama atualizar() a cada tick de controle
// (10 ms); cada chamada faz uma volta de controle e devolve MOV_EM_ANDAMENTO até o fim.
//
// Rumo estimado pelas paredes (seção 2.5; achado do robô virtual):
//  - Sem IMU, o executor guarda o erro de rumo do robô em relação à grade (positivo =
//    girado para a esquerda). Durante um movimento ele acompanha o erro pelo giro medido
//    nos encoders (Odometria::rumoGraus()).
//  - Num avanço com as duas paredes laterais, a diferença entre as leituras
//    (esquerda - direita) varia -2 x sen(erro) por mm andado. A inclinação por mínimos
//    quadrados dá o erro médio do trecho, que corrige a estimativa no fim do avanço.
//  - O erro entra nos controles como se fosse o yaw de uma IMU (imuValido = true): a reta
//    começa com rumo de referência 0 (o da grade) e, sem paredes, volta para ele; a curva
//    é ressincronizada em 0 e mira +-90, então corrige o erro herdado em vez de somá-lo.
//    As classes ControleReta e ControleCurva não mudam: é a entrada que elas já têm para
//    a IMU.
//  - Na largada (reiniciar()) o robô está alinhado à grade e o erro volta a 0.
//
// Giro (90 graus para cada lado; meia-volta = duas curvas seguidas para a direita):
//  - O ControleCurva é recriado a cada curva (uma curva abortada não deixa resto).
//  - CURVA_CONCLUIDA -> freio; CURVA_FALHOU (tempo) -> freio e FALHA_TEMPO.
//
// Avanço de uma célula:
//  - ControleReta com as distâncias laterais, rodado só quando chega amostra nova de um dos
//    sensores laterais (R11); entre amostras vale o último comando.
//  - Termina pela odometria quando a roda que andou MENOS atinge o alvo em pulsos (com uma
//    roda presa o avanço não conclui pela outra, e o monitor de travamento corta a tração),
//    ou pelo encosto frontal (parede à frente a DIST_ENCOSTO_FRONTAL_MM, depois de metade
//    do avanço).
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
    int32_t alvoAvancoPulsos;       // pulsos da roda mais lenta para avançar uma célula
    uint32_t tempoLimiteAvancoMs;   // desiste do avanço depois disso
    uint32_t tempoLimiteCurvaMs;    // desiste de cada curva de 90 graus depois disso
    uint32_t tempoFreioMs;          // freio e assentamento no fim de cada movimento
    uint16_t distEncostoFrontalMm;  // encosto frontal: fim do avanço com parede à frente
    uint16_t distMinimaSeguraMm;    // emergência frontal
    float fracaoEmergencia;         // a emergência só vale antes desta fração do alvo
    float fracaoMinimaEncosto;      // o encosto só vale depois desta fração do alvo
    // Estimativa do rumo pelas paredes
    uint16_t limiarParedeMm;        // as duas leituras laterais abaixo disso = duas paredes
    uint16_t saltoMaximoMm;         // variação entre amostras acima disso = amostra descartada
    uint8_t amostrasMinimas;        // amostras com as duas paredes para corrigir o rumo
    float trechoMinimoMm;           // extensão mínima do trecho com as duas paredes
};

// Rumo nominal de cada orientação de grade, no referencial do yaw (anti-horário positivo,
// leste = 0, como na largada). O executor trabalha com o erro em relação à grade e não
// precisa dele; fica para log e testes.
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
        p.limiarParedeMm = LIMIAR_PAREDE_MM;
        p.saltoMaximoMm = 10;
        p.amostrasMinimas = 5;
        p.trechoMinimoMm = 60.0f;
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
          sentido_(SENTIDO_ESQUERDA), curvasRestantes_(0), inicioMs_(0),
          encEsq0_(0), encDir0_(0), deslocamento0Mm_(0.0f), primeiroComando_(false),
          freioDesdeMs_(0), resultadoPendente_(MOV_OCIOSO), motivoPendente_(FALHA_NENHUMA),
          motivo_(FALHA_NENHUMA), erroRumoGraus_(0.0f), odoRumo0_(0.0f), erroRumo0_(0.0f),
          correcaoParedesGraus_(0.0f) {
        zerarParedes();
    }

    // Largada: o robô está parado e alinhado à grade.
    void reiniciar() override {
        abortar();
        erroRumoGraus_ = 0.0f;
        correcaoParedesGraus_ = 0.0f;
    }

    // orientacaoAtual não é usada: o executor trabalha com o erro em relação à grade.
    bool iniciarGiro(Giro giro, Direcao, uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO || giro == GIRO_NENHUM) return false;
        sentido_ = (giro == GIRO_ESQUERDA) ? SENTIDO_ESQUERDA : SENTIDO_DIREITA;
        curvasRestantes_ = (giro == GIRO_MEIA_VOLTA) ? 2 : 1;
        comecarCurva(agoraMs);
        fase_ = FASE_GIRANDO;
        return true;
    }

    bool iniciarAvanco(uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO) return false;
        encEsq0_ = odometria_.encEsq();
        encDir0_ = odometria_.encDir();
        deslocamento0Mm_ = odometria_.deslocamentoMm();
        marcarInicioRumo();
        zerarParedes();
        reta_.iniciar(0.0f, encEsq0_, encDir0_, agoraMs);  // referência: o rumo da grade
        inicioMs_ = agoraMs;
        primeiroComando_ = true;
        fase_ = FASE_AVANCANDO;
        return true;
    }

    ResultadoMovimento atualizar(uint32_t agoraMs) override {
        if (fase_ == FASE_OCIOSO) return MOV_OCIOSO;

        if ((fase_ == FASE_GIRANDO || fase_ == FASE_AVANCANDO) && motores_.bloqueado()) {
            erroRumoGraus_ = rumoAtual();
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
        erroRumoGraus_ = rumoAtual();
        motores_.frear();  // ignorado se o trinco estiver fechado, que já freou
        curva_ = ControleCurva(parametrosCurva_);
        fase_ = FASE_OCIOSO;
    }

    bool ocioso() const override { return fase_ == FASE_OCIOSO; }
    MotivoFalhaMovimento motivoUltimaFalha() const override { return motivo_; }

    // Erro de rumo estimado em relação à grade, em graus (positivo = girado à esquerda).
    float erroRumoEstimadoGraus() const { return fase_ == FASE_OCIOSO ? erroRumoGraus_ : rumoAtual(); }
    // Correção aplicada pelas paredes no último avanço (0 se o trecho não bastou).
    float ultimaCorrecaoParedesGraus() const { return correcaoParedesGraus_; }

    // Pulsos da roda que andou menos no avanço atual (ou no último).
    int32_t progressoAvancoPulsos() const {
        const int32_t esq = odometria_.encEsq() - encEsq0_;
        const int32_t dir = odometria_.encDir() - encDir0_;
        return esq < dir ? esq : dir;
    }
    const ParametrosExecutor& parametros() const { return p_; }

private:
    enum Fase : uint8_t { FASE_OCIOSO, FASE_GIRANDO, FASE_AVANCANDO, FASE_FREANDO };
    static constexpr float RAD_PARA_GRAUS = 57.29577951f;

    // ------------------------------------------------------------------ rumo estimado

    void marcarInicioRumo() {
        odoRumo0_ = odometria_.rumoGraus();
        erroRumo0_ = erroRumoGraus_;
    }

    // Erro de rumo agora: o do início do movimento mais o giro medido pelos encoders.
    float rumoAtual() const {
        return ControleCurva::normalizarGraus(erroRumo0_ + (odometria_.rumoGraus() - odoRumo0_));
    }

    void zerarParedes() {
        nParedes_ = 0;
        ultimaEsq_ = ultimaDir_ = 0;
        temUltimaLateral_ = false;
    }

    // Uma amostra nova dos laterais durante o avanço.
    void amostrarParedes(const LeituraToF& esq, const LeituraToF& dir, uint32_t agoraMs) {
        const bool validas = esq.valida(agoraMs) && dir.valida(agoraMs);
        const uint16_t de = esq.distanciaMm(), dd = dir.distanciaMm();
        const bool saltou = temUltimaLateral_ &&
                            (diferenca(de, ultimaEsq_) > p_.saltoMaximoMm ||
                             diferenca(dd, ultimaDir_) > p_.saltoMaximoMm);
        ultimaEsq_ = de;
        ultimaDir_ = dd;
        temUltimaLateral_ = validas;
        // Só com as duas paredes, e sem saltos (poste, fim de parede, leitura espúria).
        if (!validas || saltou || de > p_.limiarParedeMm || dd > p_.limiarParedeMm) return;

        if (nParedes_ >= MAX_AMOSTRAS_PAREDES) return;
        amostraS_[nParedes_] = odometria_.deslocamentoMm() - deslocamento0Mm_;
        amostraE_[nParedes_] = (float)de - (float)dd;
        amostraRumo_[nParedes_] = rumoAtual();
        nParedes_++;
    }

    // Ajuste por mínimos quadrados de (esq - dir) por s nas amostras marcadas. Devolve
    // false se não houver amostras ou extensão suficientes.
    bool ajustar(const bool* usar, float& inclinacao, float& intercepto, float& rumoMedio) const {
        float n = 0, somaS = 0, somaE = 0, somaSS = 0, somaSE = 0, somaRumo = 0;
        float sMin = 0, sMax = 0;
        for (uint16_t i = 0; i < nParedes_; i++) {
            if (!usar[i]) continue;
            const float s = amostraS_[i], e = amostraE_[i];
            if (n == 0) sMin = sMax = s;
            if (s < sMin) sMin = s;
            if (s > sMax) sMax = s;
            n += 1.0f;
            somaS += s;
            somaE += e;
            somaSS += s * s;
            somaSE += s * e;
            somaRumo += amostraRumo_[i];
        }
        if (n < (float)p_.amostrasMinimas || (sMax - sMin) < p_.trechoMinimoMm) return false;
        const float sMed = somaS / n, eMed = somaE / n;
        const float varS = somaSS / n - sMed * sMed;
        if (varS <= 1.0f) return false;
        inclinacao = (somaSE / n - sMed * eMed) / varS;
        intercepto = eMed - inclinacao * sMed;
        rumoMedio = somaRumo / n;
        return true;
    }

    // Fim do avanço: o erro é o dos encoders, corrigido pela média medida nas paredes.
    void fecharRumoDoAvanco() {
        const float rumoFim = rumoAtual();
        correcaoParedesGraus_ = 0.0f;
        bool usar[MAX_AMOSTRAS_PAREDES];
        for (uint16_t i = 0; i < nParedes_; i++) usar[i] = true;
        float inclinacao, intercepto, rumoMedio;
        if (ajustar(usar, inclinacao, intercepto, rumoMedio)) {
            // Segunda passada sem as amostras longe da reta (poste num lado aberto, por
            // exemplo, que passa pelo filtro de salto por duas ou três amostras).
            for (uint16_t i = 0; i < nParedes_; i++) {
                const float residuo = amostraE_[i] - (intercepto + inclinacao * amostraS_[i]);
                usar[i] = fabsf(residuo) <= RESIDUO_MAXIMO_MM;
            }
            if (ajustar(usar, inclinacao, intercepto, rumoMedio)) {
                if (inclinacao > 1.0f) inclinacao = 1.0f;
                if (inclinacao < -1.0f) inclinacao = -1.0f;
                const float rumoParedes = asinf(-inclinacao / 2.0f) * RAD_PARA_GRAUS;  // d(esq - dir)/ds = -2 sen
                correcaoParedesGraus_ = rumoParedes - rumoMedio;
            }
        }
        erroRumoGraus_ = ControleCurva::normalizarGraus(rumoFim + correcaoParedesGraus_);
    }

    static uint16_t diferenca(uint16_t a, uint16_t b) { return a > b ? (uint16_t)(a - b) : (uint16_t)(b - a); }

    // ------------------------------------------------------------------ movimentos

    // A curva parte do erro estimado e mira a grade (+-90): corrige o erro herdado.
    void comecarCurva(uint32_t agoraMs) {
        marcarInicioRumo();
        curva_ = ControleCurva(parametrosCurva_);  // sem resto de uma curva anterior ou abortada
        curva_.ressincronizar(0.0f);
        curva_.iniciar(sentido_, erroRumoGraus_, odometria_.encEsq(), odometria_.encDir(), agoraMs);
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
        e.yawGraus = rumoAtual();
        e.imuValido = true;  // rumo estimado (encoders + paredes) na entrada da IMU
        e.encEsq = odometria_.encEsq();
        e.encDir = odometria_.encDir();
        e.agoraMs = agoraMs;
        const SaidaControleCurva s = curva_.atualizar(e);

        if (s.estado == CURVA_CONCLUIDA) {
            // A grade girou 90 graus: o erro passa a ser em relação à nova direção.
            erroRumoGraus_ = ControleCurva::normalizarGraus(
                rumoAtual() - (sentido_ == SENTIDO_ESQUERDA ? 90.0f : -90.0f));
            if (--curvasRestantes_ > 0) {  // segunda metade da meia-volta
                comecarCurva(agoraMs);
                motores_.frear();
                return MOV_EM_ANDAMENTO;
            }
            marcarInicioRumo();
            return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        }
        if (s.estado == CURVA_FALHOU) {
            erroRumoGraus_ = rumoAtual();  // a grade não girou
            marcarInicioRumo();
            return terminar(agoraMs, MOV_FALHOU, FALHA_TEMPO);
        }

        comandar(s.velEsqMmS, s.velDirMmS);
        return MOV_EM_ANDAMENTO;
    }

    ResultadoMovimento terminarAvanco(uint32_t agoraMs, ResultadoMovimento resultado,
                                      MotivoFalhaMovimento motivo) {
        fecharRumoDoAvanco();
        marcarInicioRumo();
        return terminar(agoraMs, resultado, motivo);
    }

    ResultadoMovimento atualizarAvanco(uint32_t agoraMs) {
        const int32_t progresso = progressoAvancoPulsos();
        const float alvo = (float)p_.alvoAvancoPulsos;

        LeituraToF& frontal = tof_.frontal();
        if (frontal.valida(agoraMs)) {
            const uint16_t d = frontal.distanciaMm();
            if (progresso < alvo * p_.fracaoEmergencia && d <= p_.distMinimaSeguraMm) {
                return terminarAvanco(agoraMs, MOV_FALHOU, FALHA_EMERGENCIA_FRONTAL);
            }
            if (progresso >= alvo * p_.fracaoMinimaEncosto && d <= p_.distEncostoFrontalMm) {
                return terminarAvanco(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);  // encosto frontal
            }
        }
        if (progresso >= p_.alvoAvancoPulsos) return terminarAvanco(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        if ((uint32_t)(agoraMs - inicioMs_) > p_.tempoLimiteAvancoMs) {
            return terminarAvanco(agoraMs, MOV_FALHOU, FALHA_TEMPO);
        }

        LeituraToF& esq = tof_.esquerdo();
        LeituraToF& dir = tof_.direito();
        if (primeiroComando_ || esq.nova() || dir.nova()) {
            if (!primeiroComando_) amostrarParedes(esq, dir, agoraMs);
            esq.consumir();
            dir.consumir();
            primeiroComando_ = false;
            EntradaControleReta e;
            e.distEsqMm = esq.distanciaParaControle(agoraMs);
            e.distDirMm = dir.distanciaParaControle(agoraMs);
            e.yawGraus = rumoAtual();
            e.imuValido = true;  // rumo estimado (encoders + paredes) na entrada da IMU
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
        erroRumoGraus_ = rumoAtual();  // inclui o que o robô girou durante o freio
        marcarInicioRumo();
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
    uint32_t inicioMs_;
    int32_t encEsq0_, encDir0_;
    float deslocamento0Mm_;
    bool primeiroComando_;
    uint32_t freioDesdeMs_;
    ResultadoMovimento resultadoPendente_;
    MotivoFalhaMovimento motivoPendente_;
    MotivoFalhaMovimento motivo_;
    // Rumo estimado
    float erroRumoGraus_;
    float odoRumo0_, erroRumo0_;
    float correcaoParedesGraus_;
    // Trecho com as duas paredes no avanço atual (mínimos quadrados de esq - dir por s)
    static const uint16_t MAX_AMOSTRAS_PAREDES = 64;  // ~30 por célula a 30 Hz
    static constexpr float RESIDUO_MAXIMO_MM = 4.0f;
    uint16_t nParedes_;
    float amostraS_[MAX_AMOSTRAS_PAREDES];
    float amostraE_[MAX_AMOSTRAS_PAREDES];
    float amostraRumo_[MAX_AMOSTRAS_PAREDES];
    uint16_t ultimaEsq_, ultimaDir_;
    bool temUltimaLateral_;
};

#endif
