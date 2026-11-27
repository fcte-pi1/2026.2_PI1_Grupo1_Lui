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
// Inclui:
//  - Rumo estimado pelas paredes (mínimos quadrados sobre esq - dir) para operar sem IMU;
//  - Giro no próprio eixo com piso de velocidade firme acima da zona morta e meia-volta contínua;
//  - Wall-squaring (iniciarAlinhamentoTraseiro): ré curta em malha aberta contra parede traseira
//    conhecida para zerar o erro angular acumulado e compensar o deslocamento longitudinal;
//  - Recuo de desatolamento (iniciarRecuoDesatolamento): ré reta sincronizada por encoders para
//    desencostar de quina/obstáculo antes da nova tentativa;
//  - Saída suave pós-curva e fallback de comando se os sensores laterais atrasarem.

struct ParametrosExecutor {
    int32_t alvoAvancoPulsos;         // pulsos da roda mais lenta para avançar uma célula
    int32_t compensacaoLargadaPulsos; // pulsos extras só na 1ª reta (largada encostada atrás)
    uint32_t tempoLimiteAvancoMs;     // desiste do avanço depois disso
    uint32_t tempoLimiteCurvaMs;      // desiste de cada curva de 90 graus depois disso
    uint32_t tempoFreioMs;            // freio e assentamento no fim de cada movimento
    uint16_t distEncostoFrontalMm;    // encosto frontal: fim do avanço com parede à frente
    uint16_t distMinimaSeguraMm;      // emergência frontal
    float fracaoEmergencia;           // a emergência só vale antes desta fração do alvo
    float fracaoMinimaEncosto;        // o encosto só vale depois desta fração do alvo
    // Estimativa do rumo pelas paredes (PR #202)
    uint16_t limiarParedeMm;          // as duas leituras laterais abaixo disso = duas paredes
    uint16_t saltoMaximoMm;           // variação entre amostras acima disso = amostra descartada
    uint8_t amostrasMinimas;          // amostras com as duas paredes para corrigir o rumo
    float trechoMinimoMm;             // extensão mínima do trecho com as duas paredes
    // Rampas, saída pós-curva, malha fechada de velocidade e anti-travamento
    uint32_t tempoRampaPartidaMs;     // rampa suave de aceleração na partida (0 = desliga)
    float distDesaceleracaoMm;        // distância final de desaceleração suave (0 = desliga)
    float velMinAproximacaoMmS;       // velocidade mínima na desaceleração da reta
    float distPosCurvaMm;             // trecho inicial mais suave logo após sair de uma curva
    float fatorVelPosCurva;           // fator de velocidade no trecho pós-curva (0..1)
    float kpVelPwm;                   // ganho P do trim de velocidade por roda (PWM por mm/s)
    float kiVelPwm;                   // ganho I do trim de velocidade por roda (PWM por mm/s·s)
    uint32_t ajudaStallInicioMs;      // sem pulsos por este tempo -> sobe PWM de arranque
    uint32_t ajudaStallRampaMs;       // duração da rampa até o reforço máximo
    uint8_t ajudaStallMaxPwm;         // reforço máximo de PWM contra atrito estático
    // Wall-squaring (alinhamento traseiro) e recuo de desatolamento
    int16_t pwmAlinhamentoTras;       // PWM de ré em malha aberta para encostar na parede de trás
    uint32_t tempoMinAlinhamentoTrasMs;
    uint32_t janelaParadaAlinhamentoMs;
    uint32_t tempoMaxAlinhamentoTrasMs; // sempre < TEMPO_TRAVAMENTO_MS (1000 ms)
    int32_t maxPulsosAlinhamentoTras;
    int32_t maxCompensacaoTrasPulsos;
    int16_t pwmRecuoDesatolar;        // PWM base do recuo reto de desatolamento
    float kpSyncRecuo;                // ganho de sincronismo entre encoders na ré
    int32_t alvoRecuoPulsos;          // distância nominal do recuo de desatolamento
    uint32_t tempoMaxRecuoMs;         // sempre < TEMPO_TRAVAMENTO_MS (1000 ms)
};

// Rumo nominal de cada orientação de grade, no referencial do yaw (anti-horário positivo,
// leste = 0, como na largada). Fica para log e testes.
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
    static ParametrosExecutor parametrosPadrao() {
        ParametrosExecutor p;
        p.alvoAvancoPulsos = Odometria::pulsosParaMm((float)CELULA_MM * ODOM_ESCALA_RETO);
        p.compensacaoLargadaPulsos = Odometria::pulsosParaMm(COMPENSACAO_LARGADA_MM * ODOM_ESCALA_RETO);
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
        p.tempoRampaPartidaMs = TEMPO_RAMPA_PARTIDA_MS;
        p.distDesaceleracaoMm = DIST_DESACELERACAO_MM;
        p.velMinAproximacaoMmS = VEL_MIN_APROXIMACAO_MM_S;
        p.distPosCurvaMm = 55.0f;
        p.fatorVelPosCurva = 0.80f;
        p.kpVelPwm = 0.08f;
        p.kiVelPwm = 0.25f;
        p.ajudaStallInicioMs = 150;
        p.ajudaStallRampaMs = 550;
        p.ajudaStallMaxPwm = 40;
        p.pwmAlinhamentoTras = (int16_t)(PWM_BASE_MOVIMENTO > 15 ? PWM_BASE_MOVIMENTO - 15 : PWM_MINIMO);
        p.tempoMinAlinhamentoTrasMs = 140;
        p.janelaParadaAlinhamentoMs = 80;
        p.tempoMaxAlinhamentoTrasMs = 400;
        p.maxPulsosAlinhamentoTras = Odometria::pulsosParaMm(55.0f);
        p.maxCompensacaoTrasPulsos = Odometria::pulsosParaMm(45.0f);
        p.pwmRecuoDesatolar = (int16_t)(PWM_BASE_MOVIMENTO > 10 ? PWM_BASE_MOVIMENTO - 10 : PWM_MINIMO);
        p.kpSyncRecuo = 0.35f;
        p.alvoRecuoPulsos = Odometria::pulsosParaMm(30.0f);
        p.tempoMaxRecuoMs = 450;
        return p;
    }

    // Controle de reta com autoridade lateral reforçada, banda morta contra micro-oscilação,
    // monoparede com aprendizado dinâmico e rejeição de quina.
    static ParametrosControleReta parametrosReta() {
        ParametrosControleReta r = ControleReta::parametrosPadrao();
        r.kpParede = 2.2f;
        r.kdParede = 0.50f;
        r.limiteGiroMmS = 85.0f;
        r.distMaxParedeMm = (float)LIMIAR_PAREDE_MM;
        r.usarMonoparede = true;
        r.saltoMaxParedeMm = 18.0f;
        r.bandaMortaMm = 1.5f;
        return r;
    }

    // Sem IMU física: piso de velocidade firme acima da zona morta dos motores N20 no chão,
    // sincronismo entre os arcos das duas rodas e corte unidirecional sem reversão.
    static ParametrosControleCurva parametrosCurva(const ParametrosExecutor& p) {
        ParametrosControleCurva c = ControleCurva::parametrosPadrao();
        c.kpGiro = 3.5f;
        c.velocidadeMinMmS = 55.0f;
        c.velocidadeMaxMmS = 150.0f;
        c.tempoLimiteMs = p.tempoLimiteCurvaMs;
        c.kpSyncRodas = 1.5f;
        c.permitirReversao = false;
        c.toleranciaGraus = 0.8f;
        c.antecipacaoFreioGraus = CURVA_ANTECIPACAO_FREIO_GRAUS;
        return c;
    }

    ExecutorMovimentoRobo(Motores& motores, Odometria& odometria, ISensoresToF& tof,
                          const ParametrosExecutor& p = parametrosPadrao(),
                          const ConversorVelocidadePwm& conversor = ConversorVelocidadePwm())
        : motores_(motores), odometria_(odometria), tof_(tof), p_(p),
          parametrosCurva_(parametrosCurva(p)), conversor_(conversor),
          reta_(parametrosReta()), curva_(parametrosCurva_), fase_(FASE_OCIOSO),
          sentido_(SENTIDO_ESQUERDA), curvasRestantes_(0), inicioMs_(0),
          encEsq0_(0), encDir0_(0), deslocamento0Mm_(0.0f),
          alvoAtualPulsos_(p.alvoAvancoPulsos), compensacaoExtraPulsos_(0),
          ultimoAvancoParcialPulsos_(0), primeiroAvanco_(true), aposGiro_(false),
          primeiroComando_(false), ultimoComandoAvancoMs_(0),
          ultimaVelDesejadaEsqMmS_(0.0f), ultimaVelDesejadaDirMmS_(0.0f),
          encEstavelEsq_(0), encEstavelDir_(0), ultimoMovTrasMs_(0),
          freioDesdeMs_(0), resultadoPendente_(MOV_OCIOSO),
          motivoPendente_(FALHA_NENHUMA), motivo_(FALHA_NENHUMA),
          erroRumoGraus_(0.0f), odoRumo0_(0.0f), erroRumo0_(0.0f),
          correcaoParedesGraus_(0.0f), intVelEsq_(0.0f), intVelDir_(0.0f),
          ultimoPiMs_(0), encPiEsq_(0), encPiDir_(0), temPi_(false) {
        zerarParedes();
    }

    // Largada ou nova tentativa: robô parado e alinhado à grade.
    void reiniciar() override {
        abortar();
        erroRumoGraus_ = 0.0f;
        correcaoParedesGraus_ = 0.0f;
        reta_.limparReferenciaAprendida();
        primeiroAvanco_ = true;
        aposGiro_ = false;
        compensacaoExtraPulsos_ = 0;
        ultimoAvancoParcialPulsos_ = 0;
    }

    bool iniciarGiro(Giro giro, Direcao, uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO || giro == GIRO_NENHUM) return false;
        primeiroAvanco_ = false;  // ao girar em A1, a traseira deixa de estar no encosto de largada
        sentido_ = (giro == GIRO_ESQUERDA) ? SENTIDO_ESQUERDA : SENTIDO_DIREITA;
        curvasRestantes_ = (giro == GIRO_MEIA_VOLTA) ? 2 : 1;
        reiniciarPiVelocidade(agoraMs);
        comecarCurva(agoraMs);
        fase_ = FASE_GIRANDO;
        return true;
    }

    bool iniciarAvanco(uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO) return false;
        encEsq0_ = odometria_.encEsq();
        encDir0_ = odometria_.encDir();
        deslocamento0Mm_ = odometria_.deslocamentoMm();
        alvoAtualPulsos_ = p_.alvoAvancoPulsos +
                           (primeiroAvanco_ ? p_.compensacaoLargadaPulsos : 0) +
                           compensacaoExtraPulsos_;
        compensacaoExtraPulsos_ = 0;
        const int32_t minAlvo = p_.alvoAvancoPulsos / 3;
        if (alvoAtualPulsos_ < minAlvo) alvoAtualPulsos_ = minAlvo > 1 ? minAlvo : 1;
        ultimoAvancoParcialPulsos_ = 0;
        marcarInicioRumo();
        zerarParedes();
        reta_.iniciar(0.0f, encEsq0_, encDir0_, agoraMs);  // referência: o rumo da grade
        reiniciarPiVelocidade(agoraMs);
        inicioMs_ = agoraMs;
        primeiroComando_ = true;
        ultimoComandoAvancoMs_ = agoraMs;
        ultimaVelDesejadaEsqMmS_ = reta_.velocidadeBaseMmS();
        ultimaVelDesejadaDirMmS_ = reta_.velocidadeBaseMmS();
        fase_ = FASE_AVANCANDO;
        return true;
    }

    bool iniciarAlinhamentoTraseiro(uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO) return false;
        primeiroAvanco_ = false;
        encEsq0_ = odometria_.encEsq();
        encDir0_ = odometria_.encDir();
        encEstavelEsq_ = encEsq0_;
        encEstavelDir_ = encDir0_;
        ultimoMovTrasMs_ = agoraMs;
        marcarInicioRumo();
        inicioMs_ = agoraMs;
        fase_ = FASE_ALINHANDO_TRAS;
        return true;
    }

    bool iniciarRecuoDesatolamento(uint32_t agoraMs) override {
        if (fase_ != FASE_OCIOSO) return false;
        encEsq0_ = odometria_.encEsq();
        encDir0_ = odometria_.encDir();
        encEstavelEsq_ = encEsq0_;
        encEstavelDir_ = encDir0_;
        ultimoMovTrasMs_ = agoraMs;
        marcarInicioRumo();
        inicioMs_ = agoraMs;
        fase_ = FASE_RECUANDO;
        return true;
    }

    ResultadoMovimento atualizar(uint32_t agoraMs) override {
        if (fase_ == FASE_OCIOSO) return MOV_OCIOSO;

        if (fase_ != FASE_FREANDO && motores_.bloqueado()) {
            erroRumoGraus_ = rumoAtual();
            fase_ = FASE_OCIOSO;  // corte de segurança por fora: nenhum comando novo
            motivo_ = FALHA_TRACAO_CORTADA;
            return MOV_FALHOU;
        }

        switch (fase_) {
            case FASE_GIRANDO:        return atualizarGiro(agoraMs);
            case FASE_AVANCANDO:      return atualizarAvanco(agoraMs);
            case FASE_ALINHANDO_TRAS: return atualizarAlinhamentoTras(agoraMs);
            case FASE_RECUANDO:       return atualizarRecuo(agoraMs);
            case FASE_FREANDO:        return atualizarFreio(agoraMs);
            case FASE_OCIOSO:         break;
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

    float erroRumoEstimadoGraus() const { return fase_ == FASE_OCIOSO ? erroRumoGraus_ : rumoAtual(); }
    float ultimaCorrecaoParedesGraus() const { return correcaoParedesGraus_; }

    // Pulsos da roda que andou menos no avanço atual (ou no último).
    int32_t progressoAvancoPulsos() const {
        const int32_t esq = odometria_.encEsq() - encEsq0_;
        const int32_t dir = odometria_.encDir() - encDir0_;
        return esq < dir ? esq : dir;
    }
    int32_t compensacaoExtraPendentePulsos() const { return compensacaoExtraPulsos_; }
    const ParametrosExecutor& parametros() const { return p_; }

private:
    enum Fase : uint8_t {
        FASE_OCIOSO,
        FASE_GIRANDO,
        FASE_AVANCANDO,
        FASE_ALINHANDO_TRAS,
        FASE_RECUANDO,
        FASE_FREANDO
    };
    static constexpr float RAD_PARA_GRAUS = 57.29577951f;
    static const uint32_t INTERVALO_FALLBACK_COMANDO_MS = 50;

    // ------------------------------------------------------------------ rumo estimado

    void marcarInicioRumo() {
        odoRumo0_ = odometria_.rumoGraus();
        erroRumo0_ = erroRumoGraus_;
    }

    float rumoAtual() const {
        return ControleCurva::normalizarGraus(erroRumo0_ + (odometria_.rumoGraus() - odoRumo0_));
    }

    void zerarParedes() {
        nParedes_ = 0;
        ultimaEsq_ = ultimaDir_ = 0;
        candEsq_ = candDir_ = 0;
        temUltimaLateral_ = false;
        emSaltoLateral_ = false;
    }

    void amostrarParedes(const LeituraToF& esq, const LeituraToF& dir, uint32_t agoraMs) {
        const bool validas = esq.valida(agoraMs) && dir.valida(agoraMs);
        const uint16_t de = esq.distanciaMm(), dd = dir.distanciaMm();
        if (!validas || de > p_.limiarParedeMm || dd > p_.limiarParedeMm) {
            temUltimaLateral_ = false;
            emSaltoLateral_ = false;
            return;
        }
        if (!temUltimaLateral_) {
            ultimaEsq_ = de;
            ultimaDir_ = dd;
            temUltimaLateral_ = true;
            emSaltoLateral_ = false;
        } else if (diferenca(de, ultimaEsq_) <= p_.saltoMaximoMm &&
                   diferenca(dd, ultimaDir_) <= p_.saltoMaximoMm) {
            ultimaEsq_ = de;
            ultimaDir_ = dd;
            emSaltoLateral_ = false;
        } else if (emSaltoLateral_ &&
                   diferenca(de, candEsq_) <= p_.saltoMaximoMm &&
                   diferenca(dd, candDir_) <= p_.saltoMaximoMm) {
            ultimaEsq_ = de;
            ultimaDir_ = dd;
            emSaltoLateral_ = false;
        } else {
            candEsq_ = de;
            candDir_ = dd;
            emSaltoLateral_ = true;
            return;
        }

        if (nParedes_ >= MAX_AMOSTRAS_PAREDES) return;
        amostraS_[nParedes_] = odometria_.deslocamentoMm() - deslocamento0Mm_;
        amostraE_[nParedes_] = (float)de - (float)dd;
        amostraRumo_[nParedes_] = rumoAtual();
        nParedes_++;
    }

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

    void fecharRumoDoAvanco() {
        const float rumoFim = rumoAtual();
        correcaoParedesGraus_ = 0.0f;
        bool usar[MAX_AMOSTRAS_PAREDES];
        for (uint16_t i = 0; i < nParedes_; i++) usar[i] = true;
        float inclinacao, intercepto, rumoMedio;
        if (ajustar(usar, inclinacao, intercepto, rumoMedio)) {
            for (uint16_t i = 0; i < nParedes_; i++) {
                const float residuo = amostraE_[i] - (intercepto + inclinacao * amostraS_[i]);
                usar[i] = fabsf(residuo) <= RESIDUO_MAXIMO_MM;
            }
            if (ajustar(usar, inclinacao, intercepto, rumoMedio)) {
                if (inclinacao > 1.0f) inclinacao = 1.0f;
                if (inclinacao < -1.0f) inclinacao = -1.0f;
                const float rumoParedes = asinf(-inclinacao / 2.0f) * RAD_PARA_GRAUS;
                correcaoParedesGraus_ = rumoParedes - rumoMedio;
            }
        }
        erroRumoGraus_ = ControleCurva::normalizarGraus(rumoFim + correcaoParedesGraus_);
    }

    static uint16_t diferenca(uint16_t a, uint16_t b) { return a > b ? (uint16_t)(a - b) : (uint16_t)(b - a); }
    static int32_t modulo32(int32_t v) { return v < 0 ? -v : v; }

    // ------------------------------------------------------------------ velocidade / PWM

    void reiniciarPiVelocidade(uint32_t agoraMs) {
        intVelEsq_ = 0.0f;
        intVelDir_ = 0.0f;
        ultimoPiMs_ = agoraMs;
        encPiEsq_ = odometria_.encEsq();
        encPiDir_ = odometria_.encDir();
        temPi_ = false;
    }

    void comecarCurva(uint32_t agoraMs) {
        marcarInicioRumo();
        curva_ = ControleCurva(parametrosCurva_);
        curva_.ressincronizar(0.0f);
        curva_.iniciar(sentido_, erroRumoGraus_, odometria_.encEsq(), odometria_.encDir(), agoraMs);
        inicioMs_ = agoraMs;
    }

    int16_t reforcoStall(uint32_t ultimoPulsoMs, uint32_t agoraMs) const {
        if (p_.ajudaStallMaxPwm == 0 || p_.ajudaStallInicioMs == 0) return 0;
        const uint32_t semPulso = (uint32_t)(agoraMs - ultimoPulsoMs);
        const uint32_t desdeInicio = (uint32_t)(agoraMs - inicioMs_);
        const uint32_t inativo = semPulso < desdeInicio ? semPulso : desdeInicio;
        if (inativo < p_.ajudaStallInicioMs) return 0;
        if (p_.ajudaStallRampaMs == 0 || inativo >= p_.ajudaStallInicioMs + p_.ajudaStallRampaMs) {
            return (int16_t)p_.ajudaStallMaxPwm;
        }
        const float frac = (float)(inativo - p_.ajudaStallInicioMs) / (float)p_.ajudaStallRampaMs;
        return (int16_t)(frac * (float)p_.ajudaStallMaxPwm + 0.5f);
    }

    int16_t pwmRodaComMalha(float velDesejadaMmS, float velMedidaMmS, float dtS,
                            float& integrador, uint32_t ultimoPulsoMs, uint32_t agoraMs) const {
        int16_t base = conversor_.converter(velDesejadaMmS);
        if (base == 0) {
            integrador = 0.0f;
            return 0;
        }
        const int16_t sinal = base > 0 ? 1 : -1;
        const float magDesejada = velDesejadaMmS < 0.0f ? -velDesejadaMmS : velDesejadaMmS;
        const float magMedida = (float)sinal * velMedidaMmS;
        const float erro = magDesejada - magMedida;

        float trim = 0.0f;
        if (temPi_ && dtS > 0.0f && (p_.kpVelPwm > 0.0f || p_.kiVelPwm > 0.0f)) {
            const float prop = p_.kpVelPwm * erro;
            const float tentativa = (float)(sinal * base) + prop + integrador;
            const float teto = (float)conversor_.parametros().pwmMaximo;
            const float piso = (float)conversor_.parametros().pwmMinimo;
            if ((tentativa > piso && tentativa < teto) ||
                (tentativa >= teto && erro < 0.0f) ||
                (tentativa <= piso && erro > 0.0f)) {
                integrador += p_.kiVelPwm * erro * dtS;
                if (integrador > 25.0f) integrador = 25.0f;
                if (integrador < -25.0f) integrador = -25.0f;
            }
            trim = prop + integrador;
        }

        const int16_t reforco = reforcoStall(ultimoPulsoMs, agoraMs);
        int16_t mag = (int16_t)((float)(sinal * base) + trim + (float)reforco + 0.5f);
        const int16_t piso = (int16_t)conversor_.parametros().pwmMinimo;
        const int16_t teto = (int16_t)conversor_.parametros().pwmMaximo;
        if (mag < piso) mag = piso;
        if (mag > teto) mag = teto;
        return (int16_t)(sinal * mag);
    }

    void comandar(float velEsqMmS, float velDirMmS, uint32_t agoraMs) {
        const int32_t eEsq = odometria_.encEsq();
        const int32_t eDir = odometria_.encDir();
        const uint32_t dtMs = (uint32_t)(agoraMs - ultimoPiMs_);
        float dtS = 0.0f;
        float vMedEsq = 0.0f;
        float vMedDir = 0.0f;
        if (temPi_ && dtMs > 0 && dtMs <= 250) {
            dtS = (float)dtMs / 1000.0f;
            vMedEsq = (float)(eEsq - encPiEsq_) * RODA_MM_POR_PULSO / dtS;
            vMedDir = (float)(eDir - encPiDir_) * RODA_MM_POR_PULSO / dtS;
        }
        const int16_t pwmEsq = pwmRodaComMalha(velEsqMmS, vMedEsq, dtS, intVelEsq_,
                                               odometria_.getUltimoPulsoEsqMs(), agoraMs);
        const int16_t pwmDir = pwmRodaComMalha(velDirMmS, vMedDir, dtS, intVelDir_,
                                               odometria_.getUltimoPulsoDirMs(), agoraMs);
        encPiEsq_ = eEsq;
        encPiDir_ = eDir;
        ultimoPiMs_ = agoraMs;
        temPi_ = true;

        if (pwmEsq == 0 && pwmDir == 0) {
            motores_.frear();
        } else {
            motores_.comandar(pwmEsq, pwmDir);
        }
    }

    float fatorVelocidadeAvanco(int32_t progressoPulsos, uint32_t agoraMs,
                                bool frontalValida, uint16_t distFrontalMm) const {
        float fator = 1.0f;
        if (p_.tempoRampaPartidaMs > 0) {
            const uint32_t decorrido = (uint32_t)(agoraMs - inicioMs_);
            if (decorrido < p_.tempoRampaPartidaMs) {
                const float rampa = 0.55f + 0.45f * ((float)decorrido / (float)p_.tempoRampaPartidaMs);
                if (rampa < fator) fator = rampa;
            }
        }
        if (aposGiro_ && p_.distPosCurvaMm > 0.0f && p_.fatorVelPosCurva > 0.0f) {
            const float andadoMm = (progressoPulsos > 0 ? (float)progressoPulsos : 0.0f) * RODA_MM_POR_PULSO;
            if (andadoMm < p_.distPosCurvaMm && p_.fatorVelPosCurva < fator) {
                fator = p_.fatorVelPosCurva;
            }
        }
        if (p_.distDesaceleracaoMm > 0.0f) {
            const float restanteOdomMm = (float)(alvoAtualPulsos_ - progressoPulsos) * RODA_MM_POR_PULSO;
            float restanteMm = restanteOdomMm;
            if (frontalValida && distFrontalMm > p_.distEncostoFrontalMm) {
                const float restanteParedeMm = (float)(distFrontalMm - p_.distEncostoFrontalMm);
                if (restanteParedeMm < restanteMm) restanteMm = restanteParedeMm;
            }
            if (restanteMm < p_.distDesaceleracaoMm) {
                const float base = reta_.velocidadeBaseMmS();
                const float piso = base > 0.0f ? (p_.velMinAproximacaoMmS / base) : 0.45f;
                const float frac = restanteMm <= 0.0f ? 0.0f : (restanteMm / p_.distDesaceleracaoMm);
                const float desacel = piso + (1.0f - piso) * frac;
                if (desacel < fator) fator = desacel;
            }
        }
        return fator;
    }

    // ------------------------------------------------------------------ movimentos

    ResultadoMovimento terminar(uint32_t agoraMs, ResultadoMovimento resultado,
                                MotivoFalhaMovimento motivo) {
        if (fase_ == FASE_AVANCANDO && resultado == MOV_CONCLUIDO) {
            primeiroAvanco_ = false;
            aposGiro_ = false;
        }
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
        e.imuValido = true;
        e.encEsq = odometria_.encEsq();
        e.encDir = odometria_.encDir();
        e.agoraMs = agoraMs;
        const SaidaControleCurva s = curva_.atualizar(e);

        if (s.estado == CURVA_CONCLUIDA) {
            erroRumoGraus_ = ControleCurva::normalizarGraus(
                rumoAtual() - (sentido_ == SENTIDO_ESQUERDA ? 90.0f : -90.0f));
            if (--curvasRestantes_ > 0) {
                // Segunda metade da meia-volta: segue girando sem tranco de freio no meio.
                comecarCurva(agoraMs);
                EntradaControleCurva e2 = e;
                e2.yawGraus = rumoAtual();
                const SaidaControleCurva s2 = curva_.atualizar(e2);
                comandar(s2.velEsqMmS, s2.velDirMmS, agoraMs);
                return MOV_EM_ANDAMENTO;
            }
            aposGiro_ = true;
            marcarInicioRumo();
            return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        }
        if (s.estado == CURVA_FALHOU) {
            erroRumoGraus_ = rumoAtual();
            marcarInicioRumo();
            return terminar(agoraMs, MOV_FALHOU, FALHA_TEMPO);
        }

        comandar(s.velEsqMmS, s.velDirMmS, agoraMs);
        return MOV_EM_ANDAMENTO;
    }

    ResultadoMovimento terminarAvanco(uint32_t agoraMs, ResultadoMovimento resultado,
                                      MotivoFalhaMovimento motivo) {
        if (resultado == MOV_FALHOU) {
            const int32_t dE = odometria_.encEsq() - encEsq0_;
            const int32_t dD = odometria_.encDir() - encDir0_;
            const int32_t media = ((dE > 0 ? dE : 0) + (dD > 0 ? dD : 0)) / 2;
            ultimoAvancoParcialPulsos_ = media;
        }
        fecharRumoDoAvanco();
        marcarInicioRumo();
        return terminar(agoraMs, resultado, motivo);
    }

    ResultadoMovimento atualizarAvanco(uint32_t agoraMs) {
        const int32_t progresso = progressoAvancoPulsos();
        const float alvo = (float)alvoAtualPulsos_;

        LeituraToF& frontal = tof_.frontal();
        const bool frontalValida = frontal.valida(agoraMs);
        const uint16_t distFrontal = frontal.distanciaMm();
        if (frontalValida) {
            if (progresso < alvo * p_.fracaoEmergencia && distFrontal <= p_.distMinimaSeguraMm) {
                return terminarAvanco(agoraMs, MOV_FALHOU, FALHA_EMERGENCIA_FRONTAL);
            }
            if (progresso >= alvo * p_.fracaoMinimaEncosto && distFrontal <= p_.distEncostoFrontalMm) {
                return terminarAvanco(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
            }
        }
        if (progresso >= alvoAtualPulsos_) return terminarAvanco(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        if ((uint32_t)(agoraMs - inicioMs_) > p_.tempoLimiteAvancoMs) {
            return terminarAvanco(agoraMs, MOV_FALHOU, FALHA_TEMPO);
        }

        LeituraToF& esq = tof_.esquerdo();
        LeituraToF& dir = tof_.direito();
        const bool novaAmostra = esq.nova() || dir.nova();
        const bool fallbackTempo = !primeiroComando_ &&
                                   (uint32_t)(agoraMs - ultimoComandoAvancoMs_) >= INTERVALO_FALLBACK_COMANDO_MS;
        if (primeiroComando_ || novaAmostra || fallbackTempo) {
            if (primeiroComando_ || novaAmostra) {
                if (!primeiroComando_) amostrarParedes(esq, dir, agoraMs);
                esq.consumir();
                dir.consumir();
                primeiroComando_ = false;
                EntradaControleReta e;
                e.distEsqMm = esq.distanciaParaControle(agoraMs);
                e.distDirMm = dir.distanciaParaControle(agoraMs);
                e.yawGraus = rumoAtual();
                e.imuValido = true;
                e.encEsq = odometria_.encEsq();
                e.encDir = odometria_.encDir();
                e.agoraMs = agoraMs;
                const SaidaControleReta s = reta_.atualizar(e);
                ultimaVelDesejadaEsqMmS_ = s.velEsqMmS;
                ultimaVelDesejadaDirMmS_ = s.velDirMmS;
            }
            ultimoComandoAvancoMs_ = agoraMs;
            const float fator = fatorVelocidadeAvanco(progresso, agoraMs, frontalValida, distFrontal);
            comandar(ultimaVelDesejadaEsqMmS_ * fator, ultimaVelDesejadaDirMmS_ * fator, agoraMs);
        }
        return MOV_EM_ANDAMENTO;
    }

    // Wall-squaring: recua em malha aberta sem KP_SYNC para encostar a traseira na parede
    // conhecida atrás do robô, zerando o erro angular acumulado.
    ResultadoMovimento atualizarAlinhamentoTras(uint32_t agoraMs) {
        const int32_t eEsq = odometria_.encEsq();
        const int32_t eDir = odometria_.encDir();
        if (modulo32(eEsq - encEstavelEsq_) > 1 || modulo32(eDir - encEstavelDir_) > 1) {
            encEstavelEsq_ = eEsq;
            encEstavelDir_ = eDir;
            ultimoMovTrasMs_ = agoraMs;
        }
        const int32_t dEsq = encEsq0_ - eEsq;
        const int32_t dDir = encDir0_ - eDir;
        const int32_t mediaRecuo = ((dEsq > 0 ? dEsq : 0) + (dDir > 0 ? dDir : 0)) / 2;
        const uint32_t decorrido = (uint32_t)(agoraMs - inicioMs_);
        const bool encostou = decorrido >= p_.tempoMinAlinhamentoTrasMs &&
                              (uint32_t)(agoraMs - ultimoMovTrasMs_) >= p_.janelaParadaAlinhamentoMs;

        if (encostou || mediaRecuo >= p_.maxPulsosAlinhamentoTras || decorrido >= p_.tempoMaxAlinhamentoTrasMs) {
            erroRumoGraus_ = encostou ? 0.0f : rumoAtual();
            marcarInicioRumo();
            int32_t comp = mediaRecuo;
            if (comp > p_.maxCompensacaoTrasPulsos) comp = p_.maxCompensacaoTrasPulsos;
            if (comp > 0) compensacaoExtraPulsos_ = comp;
            return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        }

        motores_.comandar((int16_t)-p_.pwmAlinhamentoTras, (int16_t)-p_.pwmAlinhamentoTras);
        return MOV_EM_ANDAMENTO;
    }

    // Recuo reto sincronizado por encoders para desencostar após falha de avanço.
    ResultadoMovimento atualizarRecuo(uint32_t agoraMs) {
        const int32_t eEsq = odometria_.encEsq();
        const int32_t eDir = odometria_.encDir();
        if (modulo32(eEsq - encEstavelEsq_) > 1 || modulo32(eDir - encEstavelDir_) > 1) {
            encEstavelEsq_ = eEsq;
            encEstavelDir_ = eDir;
            ultimoMovTrasMs_ = agoraMs;
        }
        const int32_t dEsq = encEsq0_ - eEsq;
        const int32_t dDir = encDir0_ - eDir;
        const int32_t recEsq = dEsq > 0 ? dEsq : 0;
        const int32_t recDir = dDir > 0 ? dDir : 0;
        const int32_t mediaRecuo = (recEsq + recDir) / 2;
        const uint32_t decorrido = (uint32_t)(agoraMs - inicioMs_);
        const bool bloqueadoAtras = decorrido >= p_.tempoMinAlinhamentoTrasMs &&
                                    (uint32_t)(agoraMs - ultimoMovTrasMs_) >= p_.janelaParadaAlinhamentoMs;

        if (mediaRecuo >= p_.alvoRecuoPulsos || bloqueadoAtras || decorrido >= p_.tempoMaxRecuoMs) {
            // Deslocamento líquido em relação à origem da célula antes da falha:
            // se recuou mais do que tinha avançado, soma a falta; se ainda ficou adiante, subtrai.
            int32_t comp = mediaRecuo - ultimoAvancoParcialPulsos_;
            const int32_t minComp = -(p_.alvoAvancoPulsos / 2);
            if (comp < minComp) comp = minComp;
            if (comp > p_.maxCompensacaoTrasPulsos) comp = p_.maxCompensacaoTrasPulsos;
            compensacaoExtraPulsos_ = comp;
            ultimoAvancoParcialPulsos_ = 0;
            erroRumoGraus_ = rumoAtual();
            marcarInicioRumo();
            return terminar(agoraMs, MOV_CONCLUIDO, FALHA_NENHUMA);
        }

        int16_t corr = (int16_t)((float)(recEsq - recDir) * p_.kpSyncRecuo);
        if (corr > 20) corr = 20;
        if (corr < -20) corr = -20;
        int16_t pwmE = (int16_t)(p_.pwmRecuoDesatolar - corr);
        int16_t pwmD = (int16_t)(p_.pwmRecuoDesatolar + corr);
        if (pwmE < (int16_t)PWM_MINIMO) pwmE = (int16_t)PWM_MINIMO;
        if (pwmD < (int16_t)PWM_MINIMO) pwmD = (int16_t)PWM_MINIMO;
        if (pwmE > (int16_t)PWM_MAXIMO_MOVIMENTO) pwmE = (int16_t)PWM_MAXIMO_MOVIMENTO;
        if (pwmD > (int16_t)PWM_MAXIMO_MOVIMENTO) pwmD = (int16_t)PWM_MAXIMO_MOVIMENTO;
        motores_.comandar((int16_t)-pwmE, (int16_t)-pwmD);
        return MOV_EM_ANDAMENTO;
    }

    ResultadoMovimento atualizarFreio(uint32_t agoraMs) {
        if ((uint32_t)(agoraMs - freioDesdeMs_) < p_.tempoFreioMs) return MOV_EM_ANDAMENTO;
        motores_.soltar();
        erroRumoGraus_ = rumoAtual();
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
    int32_t alvoAtualPulsos_;
    int32_t compensacaoExtraPulsos_;
    int32_t ultimoAvancoParcialPulsos_;
    bool primeiroAvanco_;
    bool aposGiro_;
    bool primeiroComando_;
    uint32_t ultimoComandoAvancoMs_;
    float ultimaVelDesejadaEsqMmS_;
    float ultimaVelDesejadaDirMmS_;
    int32_t encEstavelEsq_, encEstavelDir_;
    uint32_t ultimoMovTrasMs_;
    uint32_t freioDesdeMs_;
    ResultadoMovimento resultadoPendente_;
    MotivoFalhaMovimento motivoPendente_;
    MotivoFalhaMovimento motivo_;
    // Rumo estimado (PR #202)
    float erroRumoGraus_;
    float odoRumo0_, erroRumo0_;
    float correcaoParedesGraus_;
    static const uint16_t MAX_AMOSTRAS_PAREDES = 64;
    static constexpr float RESIDUO_MAXIMO_MM = 4.0f;
    uint16_t nParedes_;
    float amostraS_[MAX_AMOSTRAS_PAREDES];
    float amostraE_[MAX_AMOSTRAS_PAREDES];
    float amostraRumo_[MAX_AMOSTRAS_PAREDES];
    uint16_t ultimaEsq_, ultimaDir_;
    uint16_t candEsq_, candDir_;
    bool temUltimaLateral_;
    bool emSaltoLateral_;
    // Malha fechada de velocidade por roda
    float intVelEsq_, intVelDir_;
    uint32_t ultimoPiMs_;
    int32_t encPiEsq_, encPiDir_;
    bool temPi_;
};

#endif
