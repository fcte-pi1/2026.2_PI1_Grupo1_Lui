#ifndef ODOMETRIA_H
#define ODOMETRIA_H

#include <math.h>
#include <stdint.h>
#include "../hardware/Pinos.h"
#include "../seguranca/MonitorTravamento.h"  // IFontePulsos

// Odometria pelos encoders, sem dependência do Arduino (docs/firmware/integracao-loop.md,
// achados R6 e B4, seção 3.8).
//
// O driver de encoders (drivers/Encoders.h) só conta pulsos na interrupção. A cada tick o
// laço entrega as duas contagens acumuladas e o instante; esta classe converte em
// milímetros, mede a velocidade e registra o instante em que cada contagem mudou. Esse
// instante é o que o MonitorTravamento precisa (IFontePulsos), sem tocar na interrupção.
//
// Convenções: as contagens crescem ao avançar (as inversões de montagem são aplicadas no
// driver, hardware/Pinos.h). Distâncias em mm desde reiniciar(), velocidades em mm/s, rumo
// em graus com o mesmo sinal do ControleReta e do ControleCurva (positivo = girou para a
// esquerda). Todas as conversões usam RODA_MM_POR_PULSO e BITOLA_MM de hardware/Pinos.h.
//
// Velocidade: diferença de deslocamento numa janela de JANELA_VELOCIDADE_MS (200 ms, como
// o hw/firm). O valor vale para a janela que acabou de fechar e é atualizado uma vez por
// janela; parado por uma janela inteira, a velocidade volta a 0.
//
// Rumo odométrico: (deslocamento direito - esquerdo) / bitola. Acumula derrapagem, por
// isso só é confiável em trechos curtos (uma célula ou uma curva).

class Odometria : public IFontePulsos {
public:
    static const uint32_t JANELA_VELOCIDADE_MS = 200;

    explicit Odometria(float mmPorPulso = RODA_MM_POR_PULSO, float bitolaMm = BITOLA_MM)
        : mmPorPulso_(mmPorPulso), bitolaMm_(bitolaMm) {
        reiniciar(0, 0, 0);
    }

    // Toma as contagens atuais como origem. Os instantes do último pulso passam a ser
    // agoraMs, para quem monitora travamento não ver pulsos antigos.
    void reiniciar(int32_t encEsq, int32_t encDir, uint32_t agoraMs) {
        encEsq0_ = encEsq;
        encDir0_ = encDir;
        encEsq_ = encEsq;
        encDir_ = encDir;
        ultimoPulsoEsqMs_ = agoraMs;
        ultimoPulsoDirMs_ = agoraMs;
        inicioJanelaMs_ = agoraMs;
        janelaEsq_ = encEsq;
        janelaDir_ = encDir;
        velEsqMmS_ = 0.0f;
        velDirMmS_ = 0.0f;
    }

    // Chamar a cada tick com as contagens acumuladas do driver.
    void atualizar(int32_t encEsq, int32_t encDir, uint32_t agoraMs) {
        if (encEsq != encEsq_) ultimoPulsoEsqMs_ = agoraMs;
        if (encDir != encDir_) ultimoPulsoDirMs_ = agoraMs;
        encEsq_ = encEsq;
        encDir_ = encDir;

        const uint32_t dt = agoraMs - inicioJanelaMs_;  // correto na volta do millis()
        if (dt >= JANELA_VELOCIDADE_MS) {
            const float segundos = (float)dt / 1000.0f;
            velEsqMmS_ = (float)(encEsq - janelaEsq_) * mmPorPulso_ / segundos;
            velDirMmS_ = (float)(encDir - janelaDir_) * mmPorPulso_ / segundos;
            janelaEsq_ = encEsq;
            janelaDir_ = encDir;
            inicioJanelaMs_ = agoraMs;
        }
    }

    // Contagens acumuladas como o driver entregou (entrada dos controles de reta e curva).
    int32_t encEsq() const { return encEsq_; }
    int32_t encDir() const { return encDir_; }

    // Deslocamento de cada roda e do centro do robô desde reiniciar().
    float deslocamentoEsqMm() const { return (float)(encEsq_ - encEsq0_) * mmPorPulso_; }
    float deslocamentoDirMm() const { return (float)(encDir_ - encDir0_) * mmPorPulso_; }
    float deslocamentoMm() const { return (deslocamentoEsqMm() + deslocamentoDirMm()) / 2.0f; }

    // Velocidade de cada roda e do centro do robô (média das rodas), na última janela.
    float velocidadeEsqMmS() const { return velEsqMmS_; }
    float velocidadeDirMmS() const { return velDirMmS_; }
    float velocidadeMmS() const { return (velEsqMmS_ + velDirMmS_) / 2.0f; }

    // Giro desde reiniciar(), só pelos encoders.
    float rumoGraus() const {
        return ((deslocamentoDirMm() - deslocamentoEsqMm()) / bitolaMm_) * RAD_PARA_GRAUS;
    }

    // Pulsos correspondentes a uma distância, arredondados: alvo do avanço de uma célula
    // (CELULA_MM) e de qualquer outro movimento medido pelos encoders.
    static int32_t pulsosParaMm(float mm, float mmPorPulso = RODA_MM_POR_PULSO) {
        const float p = mm / mmPorPulso;
        return (int32_t)(p >= 0.0f ? p + 0.5f : p - 0.5f);
    }

    // --- IFontePulsos (MonitorTravamento) ---
    uint32_t getUltimoPulsoMs() const override {
        // O mais recente dos dois, comparando pela diferença para valer na volta do millis().
        return (int32_t)(ultimoPulsoEsqMs_ - ultimoPulsoDirMs_) >= 0 ? ultimoPulsoEsqMs_
                                                                     : ultimoPulsoDirMs_;
    }
    uint32_t getUltimoPulsoEsqMs() const override { return ultimoPulsoEsqMs_; }
    uint32_t getUltimoPulsoDirMs() const override { return ultimoPulsoDirMs_; }

private:
    static constexpr float RAD_PARA_GRAUS = 57.29577951f;

    float mmPorPulso_;
    float bitolaMm_;
    int32_t encEsq0_, encDir0_;      // origem de reiniciar()
    int32_t encEsq_, encDir_;        // últimas contagens recebidas
    uint32_t ultimoPulsoEsqMs_, ultimoPulsoDirMs_;
    uint32_t inicioJanelaMs_;
    int32_t janelaEsq_, janelaDir_;  // contagens no início da janela de velocidade
    float velEsqMmS_, velDirMmS_;
};

#endif
