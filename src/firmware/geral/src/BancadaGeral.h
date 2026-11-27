#ifndef BANCADA_GERAL_H
#define BANCADA_GERAL_H

#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>
#include "TelemetriaCodec.h"
#include "TelemetriaConfig.h"
#include "TelemetriaPacket.h"
#include "TemporizadorPeriodico.h"
#include "app/CicloProva.h"
#include "drivers/Encoders.h"
#include "drivers/Motores.h"
#include "drivers/PonteHTb6612.h"
#include "drivers/SensoresToF.h"
#include "drivers/SinalizacaoLed.h"
#include "hardware/Pinos.h"
#include "movimento/ExecutorMovimentoRobo.h"
#include "odometria/Odometria.h"
#include "radio/RadioTelemetria.h"
#include "sensores/Bateria.h"
#include "sensores/BateriaConversao.h"

// Modos de bancada e comissionamento integrados ao firmware geral.
// Compartilham os mesmos drivers, pinagem e controladores da prova autonoma.
enum ModoBancada : uint8_t {
    BANCADA_INATIVA = 0,
    BANCADA_TOF,
    BANCADA_MOTOR_MANUAL,
    BANCADA_ENCODERS,
    BANCADA_TELEMETRIA,
    BANCADA_EXECUTOR,
    BANCADA_ANTICOLLISAO
};

class BancadaGeral {
public:
    BancadaGeral(PonteHTb6612& ponte, Motores& motores, Encoders& encoders,
                 SensoresToF& tof, Bateria& bateria, SinalizacaoLed& led,
                 Odometria& odometria, ExecutorMovimentoRobo& executor,
                 bool radioPronto)
        : ponte_(ponte), motores_(motores), encoders_(encoders), tof_(tof),
          bateria_(bateria), led_(led), odometria_(odometria), executor_(executor),
          radioPronto_(radioPronto), modo_(BANCADA_INATIVA),
          timerLog_(100), timerTelemetria_(TELEMETRIA_PERIODO_MS),
          pwmManual_(PWM_BASE_MOVIMENTO >= 160 ? PWM_BASE_MOVIMENTO : 160),
          dirManualEsq_(0), dirManualDir_(0),
          inicioManobraMs_(0), bateriaMv_(0) {}

    bool ativa() const { return modo_ != BANCADA_INATIVA; }

    void imprimirMenu(const CicloProva& ciclo) const {
        const GeometriaLabirinto geo = ciclo.mapa().getGeometria();
        const char* strGeo = (geo == LAB_4X4) ? "4x4" : (geo == LAB_8X4) ? "8x4" : "12x4";
        Serial.println("\n=======================================================");
        Serial.println("   MICROMOUSE PI1 - FIRMWARE GERAL (BANCADA + PROVA)   ");
        Serial.println("=======================================================");
        Serial.println("PROVA AUTONOMA:");
        Serial.println("  [k] - Iniciar labirinto (ou segure o botao por 2s)");
        Serial.printf("  [g] - Alternar geometria (Atual: %s -> 4x4/8x4/12x4)\n", strGeo);
        Serial.println("  [r] - Reiniciar para AGUARDANDO apos fim/falha");
        Serial.println("TESTES DE BANCADA:");
        Serial.println("  [1] - Sensores VL53L0X (mm)    | [2] - Motores manual");
        Serial.println("  [3] - Encoders manual          | [0] - Telemetria 10 Hz");
        Serial.println("  [4] - Avancar 1 celula (180mm) | [5] - Giro 90° Direita");
        Serial.println("  [6] - Giro 90° Esquerda        | [7] - Meia-volta 180°");
        Serial.println("  [8] - Avanco anti-colisao      | [9] - Bateria (ADC/mV)");
        Serial.println("  [i] - Analise I2C/XSHUT        | [d] - Diagnostico 1-linha");
        Serial.println("  [w] - Enviar telemetria agora  | [s] - Freio de emergencia");
        Serial.println("  [m] - Exibir menu              | [x] - Sair do teste");
        Serial.println("=======================================================");
    }

    void imprimirDiagnostico(const CicloProva& ciclo, uint32_t agoraMs) {
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        const uint16_t mv = lerBateriaSegura();
        const LeituraToF& te = tof_.esquerdo();
        const LeituraToF& tf = tof_.frontal();
        const LeituraToF& td = tof_.direito();
        Serial.printf("[DIAG] st=%u sub=%u pos=(L%u,C%u,ori=%u) ToF(E/F/D)=[%u/%u/%u mm ok=%d%d%d] Enc(E/D)=[%ld/%ld] Bat=%u mV (%u%%)\n",
                      (unsigned)ciclo.estado(), (unsigned)ciclo.subfase(),
                      (unsigned)ciclo.navegador().linha(), (unsigned)ciclo.navegador().coluna(),
                      (unsigned)ciclo.navegador().orientacao(),
                      te.valida(agoraMs) ? (unsigned)te.distanciaMm() : 0u,
                      tf.valida(agoraMs) ? (unsigned)tf.distanciaMm() : 0u,
                      td.valida(agoraMs) ? (unsigned)td.distanciaMm() : 0u,
                      tof_.esquerdoIniciado() ? 1 : 0,
                      tof_.frontalIniciado() ? 1 : 0,
                      tof_.direitoIniciado() ? 1 : 0,
                      (long)esq, (long)dir,
                      (unsigned)mv, (unsigned)porcentagemBateria(mv));
    }

    // Cancela qualquer teste de bancada em curso com seguranca.
    void abortar(uint32_t agoraMs) {
        executor_.abortar();
        motores_.liberar();
        dirManualEsq_ = 0;
        dirManualDir_ = 0;
        modo_ = BANCADA_INATIVA;
        led_.mostrarEstado(AGUARDANDO);
        (void)agoraMs;
    }

    void atualizar(uint32_t agoraMs, const CicloProva& ciclo) {
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        odometria_.atualizar(esq, dir, agoraMs);
        tof_.atualizar(agoraMs);
        led_.atualizar(agoraMs, false);

        if (timerTelemetria_.vencido(agoraMs)) {
            enviarTelemetriaBancada(ciclo, agoraMs);
        }

        switch (modo_) {
            case BANCADA_TOF:
                if (timerLog_.vencido(agoraMs)) imprimirToF(agoraMs);
                break;
            case BANCADA_ENCODERS:
                if (timerLog_.vencido(agoraMs)) imprimirEncoders();
                break;
            case BANCADA_TELEMETRIA:
                if (timerLog_.vencido(agoraMs)) imprimirTelemetriaGeral(agoraMs);
                break;
            case BANCADA_EXECUTOR:
                atualizarExecutor(agoraMs);
                break;
            case BANCADA_ANTICOLLISAO:
                atualizarAnticolisao(agoraMs);
                break;
            case BANCADA_MOTOR_MANUAL:
            case BANCADA_INATIVA:
                break;
        }
    }

    // Trata comandos de bancada quando nao ha corrida autonoma ativa.
    // Retorna true se o comando foi tratado pela bancada.
    bool tratarComando(char cmd, CicloProva& ciclo, uint32_t agoraMs) {
        if (modo_ == BANCADA_MOTOR_MANUAL) {
            return tratarMotorManual(cmd, agoraMs);
        }
        if ((modo_ == BANCADA_TOF || modo_ == BANCADA_ENCODERS || modo_ == BANCADA_TELEMETRIA) &&
            (cmd == 'x' || cmd == 'X' || cmd == 'm' || cmd == 'M')) {
            abortar(agoraMs);
            imprimirMenu(ciclo);
            return true;
        }
        if (modo_ == BANCADA_ENCODERS && (cmd == 'z' || cmd == 'Z')) {
            encoders_.zerar();
            Serial.println("[ENC] Encoders zerados.");
            return true;
        }

        switch (cmd) {
            case '1':
                abortar(agoraMs);
                modo_ = BANCADA_TOF;
                Serial.println("[TESTE 1] ToF continuo (mm). Envie 'x' para sair.");
                return true;
            case '2':
                abortar(agoraMs);
                motores_.liberar();
                modo_ = BANCADA_MOTOR_MANUAL;
                Serial.printf("[TESTE 2] Motores manual (PWM=%d): e/E, d/D, a/A, +/-, s/ESPACO, x=sair.\n",
                              (int)pwmManual_);
                return true;
            case '3':
                abortar(agoraMs);
                encoders_.zerar();
                modo_ = BANCADA_ENCODERS;
                Serial.println("[TESTE 3] Encoders manual. Envie 'z' para zerar ou 'x' para sair.");
                return true;
            case '4':
                iniciarAvancoBancada(agoraMs);
                return true;
            case '5':
                iniciarGiroBancada(GIRO_DIREITA, "DIREITA 90", agoraMs);
                return true;
            case '6':
                iniciarGiroBancada(GIRO_ESQUERDA, "ESQUERDA 90", agoraMs);
                return true;
            case '7':
                iniciarGiroBancada(GIRO_MEIA_VOLTA, "MEIA-VOLTA 180", agoraMs);
                return true;
            case '8':
                iniciarAnticolisao(agoraMs);
                return true;
            case '9':
                imprimirRelatorioBateria();
                return true;
            case '0':
                abortar(agoraMs);
                modo_ = BANCADA_TELEMETRIA;
                Serial.println("[TESTE 0] Telemetria geral 10 Hz. Envie 'x' para sair.");
                return true;
            case 'g':
            case 'G':
                ciclo.alternarGeometria(agoraMs);
                return true;
            case 'i':
            case 'I':
                abortar(agoraMs);
                executarAnaliseI2C();
                return true;
            case 'w':
            case 'W':
                enviarTelemetriaBancada(ciclo, agoraMs);
                Serial.println("[ESP-NOW] Pacote de telemetria transmitido.");
                return true;
            case 'r':
            case 'R':
                abortar(agoraMs);
                ciclo.reiniciarRemoto(agoraMs);
                return true;
            case 'm':
            case 'M':
            case 'h':
            case 'H':
                imprimirMenu(ciclo);
                return true;
            default:
                return false;
        }
    }

private:
    uint16_t lerBateriaSegura() {
        encoders_.pausarEsquerdo();
        const uint16_t mv = bateria_.lerMilivolts();
        encoders_.retomarEsquerdo();
        bateriaMv_ = mv;
        return mv;
    }

    void aplicarMotorManual() {
        motores_.comandar((int16_t)(dirManualEsq_ * pwmManual_),
                          (int16_t)(dirManualDir_ * pwmManual_));
    }

    bool tratarMotorManual(char cmd, uint32_t agoraMs) {
        switch (cmd) {
            case 'e': dirManualEsq_ = 1;  aplicarMotorManual(); return true;
            case 'E': dirManualEsq_ = -1; aplicarMotorManual(); return true;
            case 'd': dirManualDir_ = 1;  aplicarMotorManual(); return true;
            case 'D': dirManualDir_ = -1; aplicarMotorManual(); return true;
            case 'a': dirManualEsq_ = 1;  dirManualDir_ = 1;  aplicarMotorManual(); return true;
            case 'A': dirManualEsq_ = -1; dirManualDir_ = -1; aplicarMotorManual(); return true;
            case '+':
                pwmManual_ = (int16_t)min(255, (int)pwmManual_ + 20);
                aplicarMotorManual();
                Serial.printf("[MOTOR] PWM=%d\n", (int)pwmManual_);
                return true;
            case '-':
                pwmManual_ = (int16_t)max((int)PWM_MINIMO, (int)pwmManual_ - 20);
                aplicarMotorManual();
                Serial.printf("[MOTOR] PWM=%d\n", (int)pwmManual_);
                return true;
            case 's':
            case 'S':
            case ' ':
                dirManualEsq_ = 0;
                dirManualDir_ = 0;
                motores_.frear();
                Serial.println("[MOTOR] Freio aplicado.");
                return true;
            case 'x':
            case 'X':
            case 'm':
            case 'M':
                abortar(agoraMs);
                Serial.println("[MOTOR] Modo manual encerrado.");
                return true;
            default:
                return false;
        }
    }

    void iniciarAvancoBancada(uint32_t agoraMs) {
        abortar(agoraMs);
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        odometria_.reiniciar(esq, dir, agoraMs);
        executor_.reiniciar();
        motores_.liberar();
        if (executor_.iniciarAvanco(agoraMs)) {
            modo_ = BANCADA_EXECUTOR;
            inicioManobraMs_ = agoraMs;
            Serial.println("[TESTE 4] Avancando 1 celula com ExecutorMovimentoRobo...");
        }
    }

    void iniciarGiroBancada(Giro giro, const char* nome, uint32_t agoraMs) {
        abortar(agoraMs);
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        odometria_.reiniciar(esq, dir, agoraMs);
        executor_.reiniciar();
        motores_.liberar();
        if (executor_.iniciarGiro(giro, DIR_LESTE, agoraMs)) {
            modo_ = BANCADA_EXECUTOR;
            inicioManobraMs_ = agoraMs;
            Serial.printf("[TESTE] Giro %s iniciado...\n", nome);
        }
    }

    void atualizarExecutor(uint32_t agoraMs) {
        const ResultadoMovimento res = executor_.atualizar(agoraMs);
        if (res == MOV_EM_ANDAMENTO) return;
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        Serial.printf("[TESTE] Manobra %s (%lu ms) | Enc(E/D)=[%ld/%ld]\n",
                      (res == MOV_CONCLUIDO) ? "CONCLUIDA" : "FALHOU",
                      (unsigned long)(agoraMs - inicioManobraMs_), (long)esq, (long)dir);
        abortar(agoraMs);
    }

    void iniciarAnticolisao(uint32_t agoraMs) {
        abortar(agoraMs);
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        odometria_.reiniciar(esq, dir, agoraMs);
        motores_.liberar();
        const int16_t pwm = (int16_t)max((int)PWM_MINIMO, (int)PWM_BASE_MOVIMENTO - 15);
        motores_.comandar(pwm, pwm);
        inicioManobraMs_ = agoraMs;
        modo_ = BANCADA_ANTICOLLISAO;
        Serial.println("[TESTE 8] Avanco anti-colisao (< 100 mm)...");
    }

    void atualizarAnticolisao(uint32_t agoraMs) {
        const LeituraToF& tf = tof_.frontal();
        if (tf.valida(agoraMs) && tf.distanciaMm() <= 100) {
            motores_.frear();
            Serial.printf("[TESTE 8] Obstaculo a %u mm! Freio ativo.\n", (unsigned)tf.distanciaMm());
            modo_ = BANCADA_INATIVA;
            return;
        }
        if ((uint32_t)(agoraMs - inicioManobraMs_) >= 5000) {
            motores_.frear();
            Serial.println("[TESTE 8] Timeout 5s sem obstaculo.");
            modo_ = BANCADA_INATIVA;
        }
    }

    void imprimirToF(uint32_t agoraMs) const {
        const LeituraToF& te = tof_.esquerdo();
        const LeituraToF& tf = tof_.frontal();
        const LeituraToF& td = tof_.direito();
        Serial.printf("ToF | E=%4u (%s) | F=%4u (%s) | D=%4u (%s)\n",
                      te.valida(agoraMs) ? (unsigned)te.distanciaMm() : 8190u,
                      (te.valida(agoraMs) && te.distanciaMm() <= LIMIAR_PAREDE_MM) ? "PAREDE" : "livre ",
                      tf.valida(agoraMs) ? (unsigned)tf.distanciaMm() : 8190u,
                      (tf.valida(agoraMs) && tf.distanciaMm() <= LIMIAR_PAREDE_MM) ? "PAREDE" : "livre ",
                      td.valida(agoraMs) ? (unsigned)td.distanciaMm() : 8190u,
                      (td.valida(agoraMs) && td.distanciaMm() <= LIMIAR_PAREDE_MM) ? "PAREDE" : "livre ");
    }

    void imprimirEncoders() {
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        Serial.printf("ENC | Esq=%6ld (%.1f mm) | Dir=%6ld (%.1f mm)\n",
                      (long)esq, (double)esq * RODA_MM_POR_PULSO,
                      (long)dir, (double)dir * RODA_MM_POR_PULSO);
    }

    void imprimirTelemetriaGeral(uint32_t agoraMs) {
        int32_t esq = 0, dir = 0;
        encoders_.ler(esq, dir);
        const uint16_t mv = lerBateriaSegura();
        Serial.printf("TEL | ToF(E/F/D)=[%u/%u/%u] | Enc=[%ld/%ld] | Bat=%.2f V (%u%%)\n",
                      tof_.esquerdo().valida(agoraMs) ? (unsigned)tof_.esquerdo().distanciaMm() : 8190u,
                      tof_.frontal().valida(agoraMs) ? (unsigned)tof_.frontal().distanciaMm() : 8190u,
                      tof_.direito().valida(agoraMs) ? (unsigned)tof_.direito().distanciaMm() : 8190u,
                      (long)esq, (long)dir, mv / 1000.0, (unsigned)porcentagemBateria(mv));
    }

    void imprimirRelatorioBateria() {
        const uint16_t mv = lerBateriaSegura();
        Serial.printf("[TESTE 9] Bateria = %u mV (%.2f V) | %u%% | %s\n",
                      (unsigned)mv, mv / 1000.0, (unsigned)porcentagemBateria(mv),
                      bateriaPermiteLargada(mv) ? "OK para largada" : "ABAIXO DO MINIMO (<6.8V)");
    }

    void executarAnaliseI2C() {
        Serial.println("[TESTE i] Escaneando XSHUT individuais (Esq/Fre/Dir)...");
        Wire.end();
        ponte_.standby(true);
        const uint8_t xshuts[3] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR};
        const char* nomes[3] = {"Esq(GPIO4)", "Fre(GPIO16)", "Dir(GPIO17)"};
        Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL, TOF_I2C_HZ);
        for (uint8_t idx = 0; idx < 3; idx++) {
            for (uint8_t j = 0; j < 3; j++) {
                pinMode(xshuts[j], OUTPUT);
                digitalWrite(xshuts[j], (j == idx) ? HIGH : LOW);
            }
            delay(20);
            VL53L0X probe;
            probe.setBus(&Wire);
            probe.setTimeout(200);
            const bool ok = probe.init();
            Serial.printf("  * %s: init=%s\n", nomes[idx], ok ? "OK" : "FALHA");
        }
        ponte_.standby(false);
        const bool todosOk = tof_.iniciar();
        Serial.printf("[TESTE i] ToF reinicializados: %s\n", todosOk ? "3/3 OK" : "PARCIAL/FALHA");
    }

    void enviarTelemetriaBancada(const CicloProva& ciclo, uint32_t agoraMs) {
        if (!radioPronto_) return;
        TelemetriaPacket p;
        memset(&p, 0, sizeof(p));
        p.timestamp_ms = 0;
        p.labirinto_id = (uint8_t)ciclo.mapa().getGeometria();
        p.status_id = (uint8_t)AGUARDANDO;
        const uint16_t mv = (bateriaMv_ == 0 || executor_.ocioso()) ? lerBateriaSegura() : bateriaMv_;
        p.bateria_mv = mv;
        p.bateria_pct = porcentagemBateria(mv);
        const float v = odometria_.velocidadeMmS();
        p.velocidade_atual = (int16_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
        p.orientacao = (uint8_t)DIR_LESTE;
        uint8_t mask = 0;
        const LeituraToF& tf = tof_.frontal();
        const LeituraToF& td = tof_.direito();
        const LeituraToF& te = tof_.esquerdo();
        if (tf.valida(agoraMs) && tf.distanciaMm() <= LIMIAR_PAREDE_MM) mask |= PAREDE_NORTE;
        if (td.valida(agoraMs) && td.distanciaMm() <= LIMIAR_PAREDE_MM) mask |= PAREDE_LESTE;
        if (te.valida(agoraMs) && te.distanciaMm() <= LIMIAR_PAREDE_MM) mask |= PAREDE_OESTE;
        p.paredes = mask;
        if (telemetriaValidar(p) == TELEMETRIA_OK) radioTelemetriaEnviar(p);
    }

    PonteHTb6612& ponte_;
    Motores& motores_;
    Encoders& encoders_;
    SensoresToF& tof_;
    Bateria& bateria_;
    SinalizacaoLed& led_;
    Odometria& odometria_;
    ExecutorMovimentoRobo& executor_;
    bool radioPronto_;
    ModoBancada modo_;
    TemporizadorPeriodico timerLog_;
    TemporizadorPeriodico timerTelemetria_;
    int16_t pwmManual_;
    int8_t dirManualEsq_;
    int8_t dirManualDir_;
    uint32_t inicioManobraMs_;
    uint16_t bateriaMv_;
};

#endif
