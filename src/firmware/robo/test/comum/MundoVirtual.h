#ifndef MUNDO_VIRTUAL_H
#define MUNDO_VIRTUAL_H

#include <math.h>
#include <stdint.h>
#include "../../src/app/InterfacesHardware.h"  // IEncoders, ISensoresToF
#include "../../src/drivers/Motores.h"         // IPonteH
#include "../../src/hardware/Pinos.h"
#include "../../src/sensores/LeituraToF.h"
#include "SimuladorLabirinto.h"

// Mundo físico do robô virtual (docs/firmware/integracao-loop.md, seção 3.10, nível 2).
//
// Faz o papel do hardware para o firmware real: implementa a ponte H (recebe o PWM que
// Motores aplica), os encoders e os três ToF. O firmware que roda sobre ele é o mesmo do
// robô (CicloProva, ExecutorMovimentoRobo, Motores, Odometria); nada aqui conhece a
// lógica da prova.
//
// Modelo, num plano com x para o leste e y para o norte, em mm:
//  - Labirinto: as paredes verdadeiras do SimuladorLabirinto, com 12 mm de espessura
//    centradas nas divisas das células de 180 mm, e postes de 12 x 12 mm em todos os
//    cantos. O perímetro é sempre fechado.
//  - Rodas: velocidade a partir do PWM, linear acima de uma zona morta, com ganho por
//    roda (motores diferentes) e atraso de primeira ordem; freio e roda livre com
//    constantes de tempo próprias.
//  - Pose: cinemática diferencial com a bitola de Pinos.h. Os encoders contam a rotação
//    real de cada roda (o firmware não vê a pose, só os pulsos).
//  - ToF: um raio de cada sensor até a primeira superfície sólida, com ruído
//    determinístico; sem alvo ao alcance devolvem 8190, como o VL53L0X. Posições dos
//    sensores escolhidas para, com o robô no centro da célula, as laterais lerem 40 mm e
//    a frontal 45 mm, os valores que o desenho supõe.
//  - Contato: se o corpo (um círculo) tocar uma superfície sólida, o movimento não
//    acontece e as rodas param, sem pulsos: é o travamento por encostar numa parede.
//
// Cenários de falha: roda presa (travaEsq, travaDir), sensores sem resposta, e um
// obstáculo que surge à frente sempre que o robô anda para a frente (maoNaFrente).

struct ParametrosMundo {
    float zonaMortaPwm;      // PWM abaixo disso não move a roda
    float mmSPorPwm;         // velocidade a mais por unidade de PWM acima da zona morta
    float ganhoEsq;          // diferença entre os motores (1 = nominal)
    float ganhoDir;
    float tauTracaoMs;       // atraso da roda acionada
    float tauFreioMs;        // freio ativo
    float tauLivreMs;        // roda livre
    float raioCorpoMm;       // corpo do robô para o contato com as paredes
    float sensorFrontalMm;   // posição do ToF frontal à frente do centro
    float sensorLateralMm;   // posição dos ToF laterais para cada lado do centro
    float alcanceToFMm;      // além disso, sem alvo (8190)
    float ruidoToFMm;        // ruído uniforme de +- isso em cada leitura
    uint32_t periodoToFMs;   // uma amostra nova de cada sensor a cada período
};

class MundoVirtual : public IPonteH, public IEncoders, public ISensoresToF {
public:
    static const uint16_t CELULA = 180;
    static constexpr float MEIA_PAREDE = 6.0f;

    static ParametrosMundo parametrosPadrao() {
        ParametrosMundo p;
        p.zonaMortaPwm = 35.0f;              // um pouco abaixo do PWM_MINIMO do firmware
        p.mmSPorPwm = 1.0f / PWM_POR_MM_S;   // a inclinação que o conversor supõe
        p.ganhoEsq = 1.0f;
        p.ganhoDir = 1.02f;                  // motor direito 2 % mais rápido
        p.tauTracaoMs = 25.0f;
        p.tauFreioMs = 10.0f;
        p.tauLivreMs = 80.0f;
        p.raioCorpoMm = 50.0f;
        p.sensorFrontalMm = 39.0f;           // 84 - 39 = 45 mm até a parede da frente
        p.sensorLateralMm = 44.0f;           // 84 - 44 = 40 mm até as paredes laterais
        p.alcanceToFMm = 1200.0f;
        p.ruidoToFMm = 2.0f;
        p.periodoToFMs = 33;                 // cerca de 30 Hz, como o orçamento de 33 ms
        return p;
    }

    explicit MundoVirtual(SimuladorLabirinto& sim, const ParametrosMundo& p = parametrosPadrao())
        : sim_(sim), p_(p) {
        reiniciar();
    }

    // Robô parado no centro de A1, virado para o leste; contagens zeradas.
    void reiniciar() {
        posicionarNaLargada();
        pulsosEsq_ = pulsosDir_ = 0.0;
        perdidosEsq_ = 0.0;
        pausadoEsq_ = false;
        pwmEsq_ = pwmDir_ = 0;
        modo_ = MODO_LIVRE;
        travaEsq = travaDir = false;
        sensoresMudos = false;
        maoNaFrente = false;
        encostos_ = 0;
        distanciaMm_ = 0.0f;
        semente_ = 12345u;
        primeiraAmostra_ = true;
        ultimaAmostraMs_ = 0;
    }

    // A equipe reposiciona o robô na largada (as contagens dos encoders continuam).
    void posicionarNaLargada() {
        x_ = CELULA / 2.0f;
        y_ = -CELULA / 2.0f;
        thetaGraus_ = 0.0f;
        vEsq_ = vDir_ = 0.0f;
    }

    // Avança a física em 1 ms. Chamar uma vez antes de cada tick do firmware.
    void passo1ms() {
        const float dt = 0.001f;
        atualizarRoda(vEsq_, pwmEsq_, p_.ganhoEsq, travaEsq);
        atualizarRoda(vDir_, pwmDir_, p_.ganhoDir, travaDir);

        const float dEsq = vEsq_ * dt;
        const float dDir = vDir_ * dt;
        const float d = (dEsq + dDir) / 2.0f;
        const float dTheta = (dDir - dEsq) / BITOLA_MM * RAD_PARA_GRAUS;
        const float thetaMeio = (thetaGraus_ + dTheta / 2.0f) / RAD_PARA_GRAUS;
        const float nx = x_ + d * cosf(thetaMeio);
        const float ny = y_ + d * sinf(thetaMeio);

        if (corpoColide(nx, ny)) {
            // Encostou: o corpo não entra na parede e as rodas param, sem pulsos.
            vEsq_ = vDir_ = 0.0f;
            encostos_++;
            return;
        }
        x_ = nx;
        y_ = ny;
        thetaGraus_ += dTheta;
        distanciaMm_ += fabsf(d);
        pulsosEsq_ += dEsq / RODA_MM_POR_PULSO;
        pulsosDir_ += dDir / RODA_MM_POR_PULSO;
        if (pausadoEsq_) perdidosEsq_ += dEsq / RODA_MM_POR_PULSO;
    }

    // --- IPonteH (o que Motores aplica) ---
    void aplicar(int16_t pwmEsq, int16_t pwmDir) override {
        pwmEsq_ = pwmEsq;
        pwmDir_ = pwmDir;
        modo_ = MODO_TRACAO;
    }
    void frear() override {
        pwmEsq_ = pwmDir_ = 0;
        modo_ = MODO_FREIO;
    }
    void soltar() override {
        pwmEsq_ = pwmDir_ = 0;
        modo_ = MODO_LIVRE;
    }

    // --- IEncoders ---
    void ler(int32_t& esq, int32_t& dir) override {
        esq = (int32_t)(pulsosEsq_ - perdidosEsq_);
        dir = (int32_t)pulsosDir_;
    }
    void pausarEsquerdo() override { pausadoEsq_ = true; }
    void retomarEsquerdo() override { pausadoEsq_ = false; }

    // --- ISensoresToF ---
    void atualizar(uint32_t agoraMs) override {
        if (sensoresMudos) return;
        if (!primeiraAmostra_ && (uint32_t)(agoraMs - ultimaAmostraMs_) < p_.periodoToFMs) return;
        primeiraAmostra_ = false;
        ultimaAmostraMs_ = agoraMs;
        esq_.registrar(medirSensor(0.0f, p_.sensorLateralMm, 90.0f), agoraMs);
        dir_.registrar(medirSensor(0.0f, -p_.sensorLateralMm, -90.0f), agoraMs);
        uint16_t frente = medirSensor(p_.sensorFrontalMm, 0.0f, 0.0f);
        if (maoNaFrente && vEsq_ > 20.0f && vDir_ > 20.0f) frente = 25;  // obstáculo surge
        frente_.registrar(frente, agoraMs);
    }
    LeituraToF& esquerdo() override { return esq_; }
    LeituraToF& frontal() override { return frente_; }
    LeituraToF& direito() override { return dir_; }

    // --- Cenários ---
    bool travaEsq;
    bool travaDir;
    bool sensoresMudos;
    bool maoNaFrente;

    // --- Estado físico, para os testes ---
    float x() const { return x_; }
    float y() const { return y_; }
    float thetaGraus() const { return thetaGraus_; }
    uint8_t linha() const { return (uint8_t)(-y_ / CELULA); }
    uint8_t coluna() const { return (uint8_t)(x_ / CELULA); }
    // Distância do centro do robô ao centro da célula em que ele está.
    float desvioDoCentroMm() const {
        const float cx = coluna() * CELULA + CELULA / 2.0f;
        const float cy = -(linha() * CELULA + CELULA / 2.0f);
        return sqrtf((x_ - cx) * (x_ - cx) + (y_ - cy) * (y_ - cy));
    }
    // Rumo em relação ao eixo mais próximo da grade, de -45 a 45 graus.
    float desalinhamentoGraus() const {
        float a = fmodf(thetaGraus_, 90.0f);
        if (a > 45.0f) a -= 90.0f;
        if (a < -45.0f) a += 90.0f;
        return a;
    }
    float velocidadeEsq() const { return vEsq_; }
    float velocidadeDir() const { return vDir_; }
    bool parado() const { return fabsf(vEsq_) < 0.5f && fabsf(vDir_) < 0.5f; }
    uint32_t encostos() const { return encostos_; }
    float distanciaPercorridaMm() const { return distanciaMm_; }
    int16_t pwmEsq() const { return pwmEsq_; }
    int16_t pwmDir() const { return pwmDir_; }

private:
    enum Modo : uint8_t { MODO_TRACAO, MODO_FREIO, MODO_LIVRE };
    static constexpr float RAD_PARA_GRAUS = 57.29577951f;

    void atualizarRoda(float& v, int16_t pwm, float ganho, bool presa) {
        if (presa) {
            v = 0.0f;
            return;
        }
        float alvo = 0.0f;
        float tau = p_.tauLivreMs;
        if (modo_ == MODO_TRACAO) {
            const float m = (float)(pwm < 0 ? -pwm : pwm);
            const float vel = (m > p_.zonaMortaPwm) ? (m - p_.zonaMortaPwm) * p_.mmSPorPwm * ganho : 0.0f;
            alvo = pwm < 0 ? -vel : vel;
            tau = p_.tauTracaoMs;
        } else if (modo_ == MODO_FREIO) {
            tau = p_.tauFreioMs;
        }
        const float k = 1.0f / tau;  // dt = 1 ms
        v += (alvo - v) * (k > 1.0f ? 1.0f : k);
    }

    // Ponto dentro de parede, poste ou fora do labirinto.
    bool solido(float px, float py) const {
        const float largura = 4.0f * CELULA;
        const float altura = (float)sim_.linhas * CELULA;
        const float sx = px;
        const float sy = -py;  // para o sul
        if (sx < MEIA_PAREDE || sx > largura - MEIA_PAREDE || sy < MEIA_PAREDE || sy > altura - MEIA_PAREDE) {
            return true;  // perímetro (e fora dele)
        }
        int c = (int)(sx / CELULA);
        int l = (int)(sy / CELULA);
        if (c > 3) c = 3;
        if (l > sim_.linhas - 1) l = sim_.linhas - 1;
        const float xl = sx - c * CELULA;
        const float yl = sy - l * CELULA;
        const bool oeste = xl < MEIA_PAREDE, leste = xl > CELULA - MEIA_PAREDE;
        const bool norte = yl < MEIA_PAREDE, sul = yl > CELULA - MEIA_PAREDE;
        if ((oeste || leste) && (norte || sul)) return true;  // poste
        if (oeste && sim_.temParedeReal((uint8_t)l, (uint8_t)c, DIR_OESTE)) return true;
        if (leste && sim_.temParedeReal((uint8_t)l, (uint8_t)c, DIR_LESTE)) return true;
        if (norte && sim_.temParedeReal((uint8_t)l, (uint8_t)c, DIR_NORTE)) return true;
        if (sul && sim_.temParedeReal((uint8_t)l, (uint8_t)c, DIR_SUL)) return true;
        return false;
    }

    bool corpoColide(float cx, float cy) const {
        for (int i = 0; i < 16; i++) {
            const float a = (float)i * (360.0f / 16.0f) / RAD_PARA_GRAUS;
            if (solido(cx + p_.raioCorpoMm * cosf(a), cy + p_.raioCorpoMm * sinf(a))) return true;
        }
        return false;
    }

    // Sensor a (frente, esquerda) mm do centro, apontando 'anguloGraus' em relação à frente.
    uint16_t medirSensor(float frente, float esquerda, float anguloGraus) {
        const float t = thetaGraus_ / RAD_PARA_GRAUS;
        const float sx = x_ + frente * cosf(t) - esquerda * sinf(t);
        const float sy = y_ + frente * sinf(t) + esquerda * cosf(t);
        const float a = (thetaGraus_ + anguloGraus) / RAD_PARA_GRAUS;
        const float dx = cosf(a), dy = sinf(a);
        for (float d = 0.0f; d <= p_.alcanceToFMm; d += 1.0f) {
            if (solido(sx + d * dx, sy + d * dy)) {
                const float medido = d + ruido();
                return (uint16_t)(medido < 1.0f ? 1.0f : medido + 0.5f);
            }
        }
        return TOF_LEITURA_INVALIDA_MM;
    }

    // Uniforme em [-ruido, ruido], determinístico.
    float ruido() {
        semente_ = semente_ * 1664525u + 1013904223u;
        const float u = (float)((semente_ >> 8) & 0xFFFF) / 65535.0f;
        return (2.0f * u - 1.0f) * p_.ruidoToFMm;
    }

    SimuladorLabirinto& sim_;
    ParametrosMundo p_;
    float x_, y_, thetaGraus_;
    float vEsq_, vDir_;
    double pulsosEsq_, pulsosDir_, perdidosEsq_;
    bool pausadoEsq_;
    int16_t pwmEsq_, pwmDir_;
    Modo modo_;
    uint32_t encostos_;
    float distanciaMm_;
    uint32_t semente_;
    bool primeiraAmostra_;
    uint32_t ultimaAmostraMs_;
    LeituraToF esq_, frente_, dir_;
};

#endif
