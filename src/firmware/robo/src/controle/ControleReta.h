#ifndef CONTROLE_RETA_H
#define CONTROLE_RETA_H

#include <math.h>
#include <stdint.h>
#include "../hardware/Pinos.h"

// Controle de movimento em reta (doc 4.4, seção 5.2; RF-SFT01, HU-01), sem dependência
// do Arduino nem de hardware: o laço de controle entrega as leituras já prontas e a
// classe devolve a velocidade desejada de cada roda. Quem converte velocidade em PWM é
// a camada de acionamento dos motores.
//
// Unidades: distâncias em mm, velocidades em mm/s, ângulos em graus, tempo em ms.
// Convenção de sinais: o yaw do IMU cresce no sentido anti-horário (visto de cima), ou
// seja, virar para a esquerda aumenta o yaw. O erro lateral é (distância esquerda -
// distância direita): positivo quando o robô está mais perto da parede direita.

struct ParametrosControleReta {
    float velocidadeBaseMmS;     // velocidade de avanço nas duas rodas com erro zero
    float kpParede;              // mm/s de correção por mm de erro lateral
    float kdParede;              // mm/s de correção por mm/s de variação do erro lateral
    float filtroDerivada;        // peso da amostra nova na derivada, de 0 (congela) a 1 (sem filtro)
    float kpRumo;                // mm/s de correção por grau de erro de rumo
    float limiteGiroMmS;         // saturação da correção, para cada roda
    float distMinParedeMm;       // abaixo disso (ou negativa) a leitura é inválida
    float distMaxParedeMm;       // acima disso considera-se que não há parede
    float mmPorPulso;            // odometria: deslocamento de uma roda por pulso de encoder
    float bitolaMm;              // distância entre os centros das rodas
    bool usarMonoparede;         // true: com apenas 1 parede lateral, centraliza pela referência
    float refParedeLateralMm;    // distância nominal à parede quando centrado (piso inicial)
    float pesoAprendizadoRef;    // peso EMA ao aprender a referência com duas paredes (0..1)
    float saltoMaxParedeMm;      // salto consecutivo acima disso (quina/borda) é ignorado (0 = desliga)
    float bandaMortaMm;          // |erroLateral| abaixo disso zera a ação de giro (0 = desliga)
};

enum ModoControleReta : uint8_t {
    MODO_PAREDES,  // referência: paredes laterais (duas ou monoparede habilitada)
    MODO_RUMO      // referência: rumo do início da reta (IMU ou encoders)
};

struct EntradaControleReta {
    float distEsqMm;     // ToF esquerdo; valor negativo indica leitura inválida (timeout)
    float distDirMm;     // ToF direito; valor negativo indica leitura inválida (timeout)
    float yawGraus;      // yaw do IMU, em graus
    bool imuValido;      // false se a leitura do IMU falhou neste ciclo
    int32_t encEsq;      // contagem acumulada de pulsos, roda esquerda (cresce ao avançar)
    int32_t encDir;      // contagem acumulada de pulsos, roda direita (cresce ao avançar)
    uint32_t agoraMs;    // millis() no instante da leitura
};

struct SaidaControleReta {
    float velEsqMmS;        // velocidade desejada da roda esquerda (nunca negativa)
    float velDirMmS;        // velocidade desejada da roda direita (nunca negativa)
    ModoControleReta modo;  // referência usada neste ciclo
    float erroLateralMm;    // esquerda - direita; 0 em MODO_RUMO (útil para log e telemetria)
    float erroRumoGraus;    // rumo atual - rumo do início da reta; 0 em MODO_PAREDES
};

class ControleReta {
public:
    static ParametrosControleReta parametrosPadrao() {
        ParametrosControleReta p;
        p.velocidadeBaseMmS = 150.0f;  // piso da faixa de exploração, 0,15 a 0,30 m/s (doc 4.3)
        p.kpParede = 1.0f;
        p.kdParede = 0.3f;
        p.filtroDerivada = 0.3f;
        p.kpRumo = 4.0f;
        p.limiteGiroMmS = 60.0f;
        p.distMinParedeMm = 1.0f;
        p.distMaxParedeMm = 100.0f;
        p.mmPorPulso = RODA_MM_POR_PULSO;
        p.bitolaMm = BITOLA_MM;
        p.usarMonoparede = false;  // compatível com testes legados; o executor do robô ativa
        p.refParedeLateralMm = DIST_IDEAL_PAREDE_LATERAL_MM;
        p.pesoAprendizadoRef = 0.15f;
        p.saltoMaxParedeMm = 0.0f;  // 0 = desliga; o executor ativa (18 mm) contra quinas
        p.bandaMortaMm = 0.0f;      // 0 = desliga; o executor ativa (1,5 mm) contra oscilação
        return p;
    }

    explicit ControleReta(const ParametrosControleReta& p = parametrosPadrao())
        : p_(p), refEsqAprendida_(-1.0f), refDirAprendida_(-1.0f) {
        iniciar(0.0f, 0, 0, 0);
    }

    // Esquece a referência de corredor aprendida em corridas anteriores (chamar na largada).
    void limparReferenciaAprendida() {
        refEsqAprendida_ = -1.0f;
        refDirAprendida_ = -1.0f;
    }

    float referenciaEsqMm() const {
        return refEsqAprendida_ > 0.0f ? refEsqAprendida_ : p_.refParedeLateralMm;
    }

    float referenciaDirMm() const {
        return refDirAprendida_ > 0.0f ? refDirAprendida_ : p_.refParedeLateralMm;
    }

    float velocidadeBaseMmS() const { return p_.velocidadeBaseMmS; }

    // Começa uma reta: guarda o rumo e as contagens dos encoders como referência. Deve
    // ser chamado com o robô alinhado ao corredor. Mantém a referência lateral aprendida.
    void iniciar(float yawGraus, int32_t encEsq, int32_t encDir, uint32_t agoraMs) {
        rumoImuGraus_ = yawGraus;
        encEsq0_ = encEsq;
        encDir0_ = encDir;
        ultimoMs_ = agoraMs;
        temAmostra_ = false;
        derivadaFiltrada_ = 0.0f;
        erroAnterior_ = 0.0f;
        ultimoRegime_ = REGIME_RUMO;
        temUltimaEsq_ = false;
        temUltimaDir_ = false;
        ultimaEsqMm_ = 0.0f;
        ultimaDirMm_ = 0.0f;
        emSaltoEsq_ = false;
        emSaltoDir_ = false;
        candidataEsqMm_ = 0.0f;
        candidataDirMm_ = 0.0f;
    }

    SaidaControleReta atualizar(const EntradaControleReta& e) {
        const bool esq = filtrarLateral(e.distEsqMm, temUltimaEsq_, ultimaEsqMm_,
                                        emSaltoEsq_, candidataEsqMm_);
        const bool dir = filtrarLateral(e.distDirMm, temUltimaDir_, ultimaDirMm_,
                                        emSaltoDir_, candidataDirMm_);

        SaidaControleReta s;
        float giro;  // positivo = virar para a esquerda (roda direita mais rápida)

        if (esq && dir) {
            s.modo = MODO_PAREDES;
            s.erroLateralMm = e.distEsqMm - e.distDirMm;
            s.erroRumoGraus = 0.0f;
            if (ultimoRegime_ != REGIME_DUAS) temAmostra_ = false;
            ultimoRegime_ = REGIME_DUAS;
            aprenderReferencias(e.distEsqMm, e.distDirMm, s.erroLateralMm);
            const float ef = aplicarBandaMorta(s.erroLateralMm);
            giro = p_.kpParede * ef + p_.kdParede * derivada(ef, e.agoraMs);
        } else if (p_.usarMonoparede && esq) {
            // Modo monoparede esquerda: escala x2 para equivaler a (distEsq - distDir).
            s.modo = MODO_PAREDES;
            s.erroLateralMm = 2.0f * (e.distEsqMm - referenciaEsqMm());
            s.erroRumoGraus = 0.0f;
            if (ultimoRegime_ != REGIME_ESQ) temAmostra_ = false;
            ultimoRegime_ = REGIME_ESQ;
            const float ef = aplicarBandaMorta(s.erroLateralMm);
            giro = p_.kpParede * ef + p_.kdParede * derivada(ef, e.agoraMs);
        } else if (p_.usarMonoparede && dir) {
            // Modo monoparede direita: mais perto da parede direita -> erro positivo -> vira à esquerda.
            s.modo = MODO_PAREDES;
            s.erroLateralMm = 2.0f * (referenciaDirMm() - e.distDirMm);
            s.erroRumoGraus = 0.0f;
            if (ultimoRegime_ != REGIME_DIR) temAmostra_ = false;
            ultimoRegime_ = REGIME_DIR;
            const float ef = aplicarBandaMorta(s.erroLateralMm);
            giro = p_.kpParede * ef + p_.kdParede * derivada(ef, e.agoraMs);
        } else {
            s.modo = MODO_RUMO;
            s.erroLateralMm = 0.0f;
            s.erroRumoGraus = erroDeRumo(e);
            giro = -p_.kpRumo * s.erroRumoGraus;
            temAmostra_ = false;  // a próxima volta às paredes não herda derivada antiga
            ultimoRegime_ = REGIME_RUMO;
        }
        ultimoMs_ = e.agoraMs;

        if (giro > p_.limiteGiroMmS) giro = p_.limiteGiroMmS;
        if (giro < -p_.limiteGiroMmS) giro = -p_.limiteGiroMmS;

        s.velEsqMmS = limitarNaoNegativa(p_.velocidadeBaseMmS - giro);
        s.velDirMmS = limitarNaoNegativa(p_.velocidadeBaseMmS + giro);
        return s;
    }

    // Leitura entre os limites da faixa válida; negativa ou fora da faixa = sem parede.
    bool paredeValida(float distMm) const {
        return distMm >= p_.distMinParedeMm && distMm <= p_.distMaxParedeMm;
    }

    // Leva o ângulo para o intervalo [-180, 180), para o erro de rumo não saltar quando
    // o yaw passa de +180 para -180.
    static float normalizarGraus(float graus) {
        graus = fmodf(graus + 180.0f, 360.0f);
        if (graus < 0.0f) graus += 360.0f;
        return graus - 180.0f;
    }

private:
    float aplicarBandaMorta(float erroMm) const {
        if (p_.bandaMortaMm > 0.0f && fabsf(erroMm) < p_.bandaMortaMm) return 0.0f;
        return erroMm;
    }

    // Derivada do erro lateral, filtrada. Sem amostra anterior (início da reta ou volta
    // das paredes depois de um trecho em MODO_RUMO) a derivada é zero, para não gerar pico.
    float derivada(float erro, uint32_t agoraMs) {
        const uint32_t dtMs = (uint32_t)(agoraMs - ultimoMs_);  // correto na volta do millis()
        if (!temAmostra_ || dtMs > DT_MAX_MS) {
            derivadaFiltrada_ = 0.0f;
        } else if (dtMs > 0) {
            const float bruta = (erro - erroAnterior_) / ((float)dtMs / 1000.0f);
            derivadaFiltrada_ += p_.filtroDerivada * (bruta - derivadaFiltrada_);
        }
        erroAnterior_ = erro;
        temAmostra_ = true;
        return derivadaFiltrada_;
    }

    // Rumo atual menos rumo do início da reta, em graus. O IMU é a referência principal;
    // com o IMU inválido, o rumo vem da diferença de deslocamento entre as rodas.
    float erroDeRumo(const EntradaControleReta& e) const {
        if (e.imuValido) {
            return normalizarGraus(e.yawGraus - rumoImuGraus_);
        }
        const float dEsq = (float)(e.encEsq - encEsq0_) * p_.mmPorPulso;
        const float dDir = (float)(e.encDir - encDir0_) * p_.mmPorPulso;
        return ((dDir - dEsq) / p_.bitolaMm) * RAD_PARA_GRAUS;
    }

    enum RegimeLateral : uint8_t { REGIME_RUMO, REGIME_DUAS, REGIME_ESQ, REGIME_DIR };

    // Valida a parede lateral: aceita a primeira amostra válida imediatamente e rejeita
    // picos isolados de quina/borda sem contaminar a referência da amostra seguinte;
    // se a mudança se confirmar na 2ª amostra consecutiva, adota o novo patamar.
    bool filtrarLateral(float distMm, bool& temUltima, float& ultimaMm,
                        bool& emSalto, float& candidataMm) const {
        if (!paredeValida(distMm)) {
            temUltima = false;
            emSalto = false;
            return false;
        }
        if (p_.saltoMaxParedeMm <= 0.0f || !temUltima) {
            ultimaMm = distMm;
            temUltima = true;
            emSalto = false;
            return true;
        }
        if (fabsf(distMm - ultimaMm) <= p_.saltoMaxParedeMm) {
            ultimaMm = distMm;
            emSalto = false;
            return true;
        }
        if (emSalto && fabsf(distMm - candidataMm) <= p_.saltoMaxParedeMm) {
            ultimaMm = distMm;
            emSalto = false;
            return true;
        }
        candidataMm = distMm;
        emSalto = true;
        return false;
    }

    // Aprende a distância de cada parede quando o robô está aproximadamente centrado
    // entre duas paredes (|erro| <= 8 mm), sem contaminar por leitura em uma parede só.
    void aprenderReferencias(float distEsqMm, float distDirMm, float erroMm) {
        if (p_.pesoAprendizadoRef <= 0.0f || fabsf(erroMm) > 8.0f) return;
        const float a = p_.pesoAprendizadoRef;
        refEsqAprendida_ = (refEsqAprendida_ < 0.0f) ? distEsqMm
                                                     : refEsqAprendida_ + a * (distEsqMm - refEsqAprendida_);
        refDirAprendida_ = (refDirAprendida_ < 0.0f) ? distDirMm
                                                     : refDirAprendida_ + a * (distDirMm - refDirAprendida_);
    }

    static float limitarNaoNegativa(float v) { return v < 0.0f ? 0.0f : v; }

    static const uint32_t DT_MAX_MS = 250;  // acima disso a amostra anterior é velha demais
    static constexpr float RAD_PARA_GRAUS = 57.29577951f;

    ParametrosControleReta p_;
    float rumoImuGraus_;
    int32_t encEsq0_;
    int32_t encDir0_;
    uint32_t ultimoMs_;
    bool temAmostra_;
    float erroAnterior_;
    float derivadaFiltrada_;
    RegimeLateral ultimoRegime_;
    bool temUltimaEsq_;
    bool temUltimaDir_;
    float ultimaEsqMm_;
    float ultimaDirMm_;
    bool emSaltoEsq_;
    bool emSaltoDir_;
    float candidataEsqMm_;
    float candidataDirMm_;
    float refEsqAprendida_;
    float refDirAprendida_;
};

#endif
