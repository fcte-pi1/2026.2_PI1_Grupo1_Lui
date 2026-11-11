#ifndef MOTORES_H
#define MOTORES_H

#include <stdint.h>
#include "../hardware/Pinos.h"
#include "../seguranca/MonitorTravamento.h"  // IFonteTracao (e ITracao)

// Motores do robô: trinco de parada, tração por roda e inversão de montagem, sem
// dependência do Arduino (docs/firmware/integracao-loop.md, achados B7, R1 e R7).
//
// A ponte H real (TB6612FNG, drivers/PonteHTb6612.cpp) entra pela interface IPonteH; nos
// testes nativos entra um dublê. Assim a regra de segurança abaixo é testada sem hardware.
//
// Trinco (B7): parar() é o corte de segurança, chamado pelo ParadaObjetivo (chegada) ou
// pelo MonitorTravamento (travamento). Ele freia e trava os motores: todo comandar()
// seguinte é ignorado até liberar(), na próxima largada. Sem isso, o controle reaplicaria
// PWM no tick seguinte e desfaria o corte. Quem comanda os motores consulta bloqueado()
// para encerrar o movimento.
//
// Tração por roda (R1): uma roda tem tração quando o último PWM aplicado a ela é diferente
// de zero. É o que o MonitorTravamento consulta, por roda.
//
// Inversão (R7): o comando lógico é sempre "positivo = para a frente"; a troca de sinal
// para a montagem física acontece só aqui, ao falar com a ponte H.

// Ponte H vista pelo firmware. PWM de -PWM_MAXIMO a PWM_MAXIMO, positivo = sentido físico
// "frente" da ponte; 0 = roda livre naquela roda.
class IPonteH {
public:
    virtual ~IPonteH() {}
    virtual void aplicar(int16_t pwmEsq, int16_t pwmDir) = 0;
    virtual void frear() = 0;   // freio ativo nas duas rodas (terminais em curto)
    virtual void soltar() = 0;  // roda livre nas duas rodas
};

class Motores : public IFonteTracao {
public:
    explicit Motores(IPonteH& ponte,
                     bool esqInvertido = MOTOR_ESQ_INVERTIDO,
                     bool dirInvertido = MOTOR_DIR_INVERTIDO)
        : ponte_(ponte), esqInvertido_(esqInvertido), dirInvertido_(dirInvertido),
          pwmEsq_(0), pwmDir_(0), bloqueado_(false), freiando_(false) {}

    // Aplica o PWM de cada roda (positivo = frente do robô), saturado em +-PWM_MAXIMO.
    // Retorna false, sem tocar na ponte, se o trinco estiver fechado.
    bool comandar(int16_t pwmEsq, int16_t pwmDir) {
        if (bloqueado_) return false;
        pwmEsq_ = saturar(pwmEsq);
        pwmDir_ = saturar(pwmDir);
        freiando_ = false;
        ponte_.aplicar(esqInvertido_ ? (int16_t)-pwmEsq_ : pwmEsq_,
                       dirInvertido_ ? (int16_t)-pwmDir_ : pwmDir_);
        return true;
    }

    // Freio ativo sem fechar o trinco: fim normal de um movimento (o executor freia e
    // espera o assentamento). Ignorado com o trinco fechado, que já está freando.
    void frear() {
        if (bloqueado_) return;
        aplicarFreio();
    }

    // Roda livre sem abrir o trinco: estados terminais, depois do tempo de freio.
    void soltar() {
        pwmEsq_ = 0;
        pwmDir_ = 0;
        freiando_ = false;
        ponte_.soltar();
    }

    // --- ITracao / IFonteTracao ---
    // Corte de segurança: freia e fecha o trinco. Chamadas repetidas são inócuas (o
    // primeiro parar() vence, seção 3.7 do desenho).
    void parar() override {
        if (bloqueado_) return;
        bloqueado_ = true;
        aplicarFreio();
    }

    bool tracaoAtiva() const override { return pwmEsq_ != 0 || pwmDir_ != 0; }
    bool tracaoEsqAtiva() const override { return pwmEsq_ != 0; }
    bool tracaoDirAtiva() const override { return pwmDir_ != 0; }

    // Abre o trinco na largada. Os motores ficam em roda livre até o próximo comandar().
    void liberar() {
        bloqueado_ = false;
        soltar();
    }

    bool bloqueado() const { return bloqueado_; }
    bool freiando() const { return freiando_; }
    // Último PWM lógico (positivo = frente do robô), antes da inversão de montagem.
    int16_t pwmEsq() const { return pwmEsq_; }
    int16_t pwmDir() const { return pwmDir_; }

private:
    static int16_t saturar(int16_t pwm) {
        if (pwm > (int16_t)PWM_MAXIMO) return (int16_t)PWM_MAXIMO;
        if (pwm < -(int16_t)PWM_MAXIMO) return (int16_t)-(int16_t)PWM_MAXIMO;
        return pwm;
    }

    void aplicarFreio() {
        pwmEsq_ = 0;
        pwmDir_ = 0;
        freiando_ = true;
        ponte_.frear();
    }

    IPonteH& ponte_;
    bool esqInvertido_;
    bool dirInvertido_;
    int16_t pwmEsq_;
    int16_t pwmDir_;
    bool bloqueado_;
    bool freiando_;
};

#endif
