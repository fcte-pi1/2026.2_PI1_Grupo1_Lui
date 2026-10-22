#ifndef CONTROLE_CURVA_H
#define CONTROLE_CURVA_H

#include <math.h>
#include <stdint.h>

// Controle de curvas de 90 graus (doc 4.4, "Rotacoes ortogonais"; RF-SFT01, HU-02), sem
// dependencia do Arduino nem de hardware, no mesmo molde do ControleReta: o laco de
// controle entrega as leituras ja prontas e a classe devolve a velocidade desejada de
// cada roda. Quem converte velocidade em PWM e a camada de acionamento dos motores.
//
// Unidades: velocidades em mm/s, angulos em graus, tempo em ms. Convencao de sinais igual
// a do ControleReta: o yaw do IMU cresce no sentido anti-horario (virar para a esquerda
// aumenta o yaw) e o yaw entregue vai de -180 a 180 (a classe trata a virada).
//
// Como funciona:
//  - A curva e feita no proprio eixo: as rodas giram em sentidos opostos, por isso as
//    velocidades de saida podem ser negativas (diferente do ControleReta).
//  - O giro e medido pelo IMU (giroscopio integrado pelo firmware). Os encoders servem de
//    conferencia (giro = diferenca entre as rodas / bitola): a diferenca entre os dois
//    sai em discrepanciaGraus, para log e telemetria, e com o IMU invalido os encoders
//    passam a ser a medida.
//  - Controle proporcional sobre o erro de angulo, com velocidade minima (para vencer o
//    atrito perto do alvo) e maxima.
//  - O alvo e sempre ABSOLUTO: o rumo ideal e guardado e cada curva soma +-90 graus a ele,
//    e nao ao angulo medido. Assim o erro de uma curva nao se soma ao da seguinte, que e
//    o que o criterio de +-3 graus do RF-SFT01 pede.
//  - Ao entrar na tolerancia o robo para e a curva so e dada como feita depois do tempo
//    de assentamento: se a inercia dos motores o levar para fora da tolerancia, o
//    controle corrige antes de concluir (o doc pede verificar ao final da rotacao).
//  - A orientacao interna (N/L/S/O) so e atualizada quando a curva e concluida.
//
// Uso: chamar iniciar() quando a curva comeca e atualizar() a cada leitura do IMU, ate o
// estado sair de CURVA_GIRANDO. Depois da curva, o robo se realinha com os ToF (HU-01);
// nesse momento ele esta alinhado ao corredor, entao chamar ressincronizar() com o yaw
// atual para o rumo ideal absorver a deriva do giroscopio.
//
// Limitacoes conhecidas:
//  - Depois de CURVA_FALHOU a orientacao NAO foi atualizada e o rumo ideal ja foi
//    deslocado: quem chamou decide (por exemplo, realinhar e chamar ressincronizar()).
//  - Os valores padrao sao ponto de partida e devem ser ajustados no robo, em especial
//    a velocidade minima (zona morta dos motores) e o ganho.

// Os valores seguem os ids da telemetria (telemetriaNomeOrientacao: NORTE, SUL, LESTE,
// OESTE), para o firmware poder passar (uint8_t)orientacao() direto para o pacote. Por
// isso a ordem NAO e a de giro: a rotacao usa tabelas (ver girar()), nao aritmetica.
enum Orientacao : uint8_t {
    ORIENTACAO_NORTE = 0,
    ORIENTACAO_SUL = 1,
    ORIENTACAO_LESTE = 2,
    ORIENTACAO_OESTE = 3
};

enum SentidoCurva : uint8_t {
    SENTIDO_ESQUERDA,  // anti-horario, aumenta o yaw
    SENTIDO_DIREITA    // horario, diminui o yaw
};

enum EstadoControleCurva : uint8_t {
    CURVA_PARADA,      // nenhuma curva em andamento
    CURVA_GIRANDO,     // curva em andamento (inclui o assentamento)
    CURVA_CONCLUIDA,   // chegou aos 90 graus dentro da tolerancia; orientacao atualizada
    CURVA_FALHOU       // estourou o tempo limite sem chegar ao alvo
};

struct ParametrosControleCurva {
    float kpGiro;             // mm/s de velocidade de roda por grau de erro
    float velocidadeMinMmS;   // piso da velocidade de cada roda enquanto gira
    float velocidadeMaxMmS;   // teto da velocidade de cada roda
    float toleranciaGraus;    // |erro| <= isso para dar a curva como feita (menor que os 3 do RF-SFT01)
    uint32_t assentamentoMs;  // tempo parado dentro da tolerancia antes de concluir
    uint32_t tempoLimiteMs;   // desiste da curva depois disso
    float mmPorPulso;         // odometria: deslocamento de uma roda por pulso de encoder
    float bitolaMm;           // distancia entre os centros das rodas
};

struct EntradaControleCurva {
    float yawGraus;      // yaw do IMU, de -180 a 180
    bool imuValido;      // false se a leitura do IMU falhou neste ciclo
    int32_t encEsq;      // contagem acumulada de pulsos, roda esquerda (cresce ao avancar)
    int32_t encDir;      // contagem acumulada de pulsos, roda direita (cresce ao avancar)
    uint32_t agoraMs;    // millis() no instante da leitura
};

struct SaidaControleCurva {
    float velEsqMmS;              // velocidade desejada da roda esquerda (pode ser negativa)
    float velDirMmS;              // velocidade desejada da roda direita (pode ser negativa)
    EstadoControleCurva estado;   // estado depois deste ciclo
    float erroGraus;              // rumo ideal - rumo atual, de -180 a 180 (0 fora de uma curva)
    float giroEncodersGraus;      // giro desde iniciar() medido so pelos encoders
    float discrepanciaGraus;      // giro pelo IMU - giro pelos encoders; 0 com o IMU invalido
};

class ControleCurva {
public:
    static ParametrosControleCurva parametrosPadrao() {
        ParametrosControleCurva p;
        p.kpGiro = 3.0f;            // satura a velocidade maxima a partir de ~33 graus de erro
        p.velocidadeMinMmS = 12.0f; // a calibrar no robo: depende da zona morta dos motores
        p.velocidadeMaxMmS = 100.0f;
        p.toleranciaGraus = 1.0f;
        p.assentamentoMs = 100;
        p.tempoLimiteMs = 3000;
        p.mmPorPulso = 0.1f;        // mesmo valor provisorio do ControleReta: calibrar
        p.bitolaMm = 78.0f;         // doc 4.1: centros das rodas em X +-39 mm
        return p;
    }

    // A partida do labirinto e a celula A1 com o robo virado para o leste.
    explicit ControleCurva(const ParametrosControleCurva& p = parametrosPadrao(),
                           Orientacao inicial = ORIENTACAO_LESTE)
        : p_(p), orientacao_(inicial) {}

    // Comeca uma curva de 90 graus. Ignorado se ja houver uma em andamento. Na primeira
    // curva o yaw recebido vira a referencia (robo alinhado ao corredor).
    void iniciar(SentidoCurva sentido, float yawGraus, int32_t encEsq, int32_t encDir,
                 uint32_t agoraMs) {
        if (estado_ == CURVA_GIRANDO) return;
        if (!temReferencia_) {
            rumoIdealGraus_ = normalizarGraus(yawGraus);
            temReferencia_ = true;
        }
        rumoIdealGraus_ = normalizarGraus(rumoIdealGraus_ + (sentido == SENTIDO_ESQUERDA ? 90.0f : -90.0f));
        sentido_ = sentido;
        yawInicioGraus_ = yawGraus;
        encEsq0_ = encEsq;
        encDir0_ = encDir;
        inicioMs_ = agoraMs;
        assentando_ = false;
        erro_ = 0.0f;
        estado_ = CURVA_GIRANDO;
    }

    // Chamar com o robo alinhado ao corredor (por exemplo, depois do realinhamento pelos
    // ToF): o yaw atual passa a ser o rumo ideal, absorvendo a deriva do giroscopio.
    void ressincronizar(float yawGraus) {
        rumoIdealGraus_ = normalizarGraus(yawGraus);
        temReferencia_ = true;
    }

    void definirOrientacao(Orientacao o) { orientacao_ = o; }
    Orientacao orientacao() const { return orientacao_; }
    EstadoControleCurva estado() const { return estado_; }

    SaidaControleCurva atualizar(const EntradaControleCurva& e) {
        SaidaControleCurva s;
        s.velEsqMmS = 0.0f;
        s.velDirMmS = 0.0f;
        s.erroGraus = erro_;
        s.giroEncodersGraus = 0.0f;
        s.discrepanciaGraus = 0.0f;

        if (estado_ != CURVA_GIRANDO) {
            s.estado = estado_;
            return s;
        }

        s.giroEncodersGraus = giroPorEncoders(e);
        float atual;
        if (e.imuValido) {
            atual = e.yawGraus;
            s.discrepanciaGraus = normalizarGraus(e.yawGraus - yawInicioGraus_) - s.giroEncodersGraus;
        } else {
            atual = yawInicioGraus_ + s.giroEncodersGraus;
        }
        erro_ = normalizarGraus(rumoIdealGraus_ - atual);
        s.erroGraus = erro_;

        if ((uint32_t)(e.agoraMs - inicioMs_) > p_.tempoLimiteMs) {
            estado_ = CURVA_FALHOU;  // a orientacao nao e atualizada
        } else if (fabsf(erro_) <= p_.toleranciaGraus) {
            if (!assentando_) {
                assentando_ = true;
                assentandoDesdeMs_ = e.agoraMs;
            }
            if ((uint32_t)(e.agoraMs - assentandoDesdeMs_) >= p_.assentamentoMs) {
                orientacao_ = girar(orientacao_, sentido_);  // atualiza a orientacao no fim
                estado_ = CURVA_CONCLUIDA;
            }
        } else {
            assentando_ = false;
            float v = p_.kpGiro * fabsf(erro_);
            if (v < p_.velocidadeMinMmS) v = p_.velocidadeMinMmS;
            if (v > p_.velocidadeMaxMmS) v = p_.velocidadeMaxMmS;
            // erro > 0: falta girar para a esquerda (roda esquerda para tras, direita para
            // frente). erro < 0: o inverso, o que tambem corrige uma ultrapassagem.
            const float sinal = (erro_ > 0.0f) ? 1.0f : -1.0f;
            s.velEsqMmS = -sinal * v;
            s.velDirMmS = sinal * v;
        }
        s.estado = estado_;
        return s;
    }

    // Leva o angulo para o intervalo [-180, 180).
    static float normalizarGraus(float graus) {
        graus = fmodf(graus + 180.0f, 360.0f);
        if (graus < 0.0f) graus += 360.0f;
        return graus - 180.0f;
    }

private:
    // Giro desde iniciar() pelos encoders (roda direita a frente = giro a esquerda = positivo).
    float giroPorEncoders(const EntradaControleCurva& e) const {
        const float dEsq = (float)(e.encEsq - encEsq0_) * p_.mmPorPulso;
        const float dDir = (float)(e.encDir - encDir0_) * p_.mmPorPulso;
        return ((dDir - dEsq) / p_.bitolaMm) * RAD_PARA_GRAUS;
    }

    // Indexadas pelo id da orientacao (NORTE, SUL, LESTE, OESTE).
    static Orientacao girar(Orientacao o, SentidoCurva s) {
        static const Orientacao aDireita[4] = {ORIENTACAO_LESTE, ORIENTACAO_OESTE, ORIENTACAO_SUL, ORIENTACAO_NORTE};
        static const Orientacao aEsquerda[4] = {ORIENTACAO_OESTE, ORIENTACAO_LESTE, ORIENTACAO_NORTE, ORIENTACAO_SUL};
        return (s == SENTIDO_DIREITA) ? aDireita[o] : aEsquerda[o];
    }

    static constexpr float RAD_PARA_GRAUS = 57.29577951f;

    ParametrosControleCurva p_;
    Orientacao orientacao_;
    SentidoCurva sentido_ = SENTIDO_ESQUERDA;
    EstadoControleCurva estado_ = CURVA_PARADA;
    float rumoIdealGraus_ = 0.0f;   // rumo ideal acumulado, no referencial do IMU
    bool temReferencia_ = false;
    float yawInicioGraus_ = 0.0f;
    int32_t encEsq0_ = 0;
    int32_t encDir0_ = 0;
    uint32_t inicioMs_ = 0;
    bool assentando_ = false;
    uint32_t assentandoDesdeMs_ = 0;
    float erro_ = 0.0f;
};

#endif
