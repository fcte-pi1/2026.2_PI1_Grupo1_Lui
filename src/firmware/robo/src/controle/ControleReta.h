#ifndef CONTROLE_RETA_H
#define CONTROLE_RETA_H

#include <math.h>
#include <stdint.h>

// Controle de movimento em reta (doc 4.4, seção 5.2; RF-SFT01, HU-01), sem dependência
// do Arduino nem de hardware: o laço de controle entrega as leituras já prontas e a
// classe devolve a velocidade desejada de cada roda. Quem converte velocidade em PWM é
// a camada de acionamento dos motores.
//
// Unidades: distâncias em mm, velocidades em mm/s, ângulos em graus, tempo em ms.
// Convenção de sinais: o yaw do IMU cresce no sentido anti-horário (visto de cima), ou
// seja, virar para a esquerda aumenta o yaw. Se o IMU entregar o contrário, inverta o
// sinal na leitura e não aqui. O erro lateral é (distância esquerda - distância
// direita): positivo quando o robô está mais perto da parede direita.
//
// Dois modos, como no fluxograma da seção 5.2:
//  - MODO_PAREDES: há parede válida nos dois lados. Controle PD sobre o erro lateral
//    (a correção é proporcional ao erro e à sua variação): reduz a velocidade da roda
//    do lado mais próximo da parede e aumenta a da roda oposta.
//  - MODO_RUMO: falta parede em pelo menos um lado (cruzamentos, entradas de corredor).
//    A comparação lateral deixa de valer e o robô mantém o rumo que tinha no início da
//    reta, pelo yaw do IMU. Se o IMU estiver inválido, usa a diferença de contagem dos
//    encoders.
//
// Uso: chamar iniciar() quando a reta começa e atualizar() a cada nova leitura dos
// sensores laterais (mínimo 20 Hz, seção 4.3 do doc 4.4). A derivada usa o intervalo
// entre chamadas, então chamar mais rápido do que os sensores atualizam gera derivada
// nula seguida de picos.
//
// Limitações conhecidas:
//  - O rumo de referência é o do instante de iniciar(). Se a reta começar desalinhada
//    (o RF-SFT01 admite até ±3° depois de uma curva) e logo à frente faltar parede, o
//    robô mantém o desalinhamento até as paredes voltarem.
//  - Com uma parede só, a classe não segue essa parede: usa o rumo, como no doc.
//  - Os valores padrão são ponto de partida e devem ser ajustados no robô (CT-01).

struct ParametrosControleReta {
    float velocidadeBaseMmS;  // velocidade de avanço nas duas rodas com erro zero
    float kpParede;           // mm/s de correção por mm de erro lateral
    float kdParede;           // mm/s de correção por mm/s de variação do erro lateral
    float filtroDerivada;     // peso da amostra nova na derivada, de 0 (congela) a 1 (sem filtro)
    float kpRumo;             // mm/s de correção por grau de erro de rumo
    float limiteGiroMmS;      // saturação da correção, para cada roda
    float distMinParedeMm;    // abaixo disso (ou negativa) a leitura é inválida; baixo de propósito:
                              // colado na parede a referência não pode sumir
    float distMaxParedeMm;    // acima disso considera-se que não há parede
    float mmPorPulso;         // odometria: deslocamento de uma roda por pulso de encoder
    float bitolaMm;           // distância entre os centros das rodas
};

enum ModoControleReta : uint8_t {
    MODO_PAREDES,  // referência: paredes laterais
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
        p.mmPorPulso = 0.1f;   // o doc 4.3 dá mais de 10 pulsos/mm: calibrar
        p.bitolaMm = 78.0f;    // doc 4.1: centros das rodas em X ±39 mm
        return p;
    }

    explicit ControleReta(const ParametrosControleReta& p = parametrosPadrao()) : p_(p) {
        iniciar(0.0f, 0, 0, 0);
    }

    // Começa uma reta: guarda o rumo e as contagens dos encoders como referência. Deve
    // ser chamado com o robô alinhado ao corredor.
    void iniciar(float yawGraus, int32_t encEsq, int32_t encDir, uint32_t agoraMs) {
        rumoImuGraus_ = yawGraus;
        encEsq0_ = encEsq;
        encDir0_ = encDir;
        ultimoMs_ = agoraMs;
        temAmostra_ = false;
        derivadaFiltrada_ = 0.0f;
        erroAnterior_ = 0.0f;
    }

    SaidaControleReta atualizar(const EntradaControleReta& e) {
        const bool esq = paredeValida(e.distEsqMm);
        const bool dir = paredeValida(e.distDirMm);

        SaidaControleReta s;
        float giro;  // positivo = virar para a esquerda (roda direita mais rápida)

        if (esq && dir) {
            s.modo = MODO_PAREDES;
            s.erroLateralMm = e.distEsqMm - e.distDirMm;
            s.erroRumoGraus = 0.0f;
            giro = p_.kpParede * s.erroLateralMm + p_.kdParede * derivada(s.erroLateralMm, e.agoraMs);
        } else {
            s.modo = MODO_RUMO;
            s.erroLateralMm = 0.0f;
            s.erroRumoGraus = erroDeRumo(e);
            giro = -p_.kpRumo * s.erroRumoGraus;
            temAmostra_ = false;  // a próxima volta às paredes não herda derivada antiga
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
    // com o IMU inválido, o rumo vem da diferença de deslocamento entre as rodas (roda
    // direita à frente = giro à esquerda = positivo).
    float erroDeRumo(const EntradaControleReta& e) const {
        if (e.imuValido) {
            return normalizarGraus(e.yawGraus - rumoImuGraus_);
        }
        const float dEsq = (float)(e.encEsq - encEsq0_) * p_.mmPorPulso;
        const float dDir = (float)(e.encDir - encDir0_) * p_.mmPorPulso;
        return ((dDir - dEsq) / p_.bitolaMm) * RAD_PARA_GRAUS;
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
};

#endif
