#include <Arduino.h>
#include "TemporizadorPeriodico.h"
#include "app/CicloProva.h"
#include "app/InterfacesHardware.h"
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

// Invólucro Arduino do firmware do robô (docs/firmware/integracao-loop.md, seção 3.2).
// Toda a lógica está em app/CicloProva.h, testada no ambiente nativo; aqui só se criam os
// drivers, se ligam as interfaces e se chama ciclo.tick() em todo loop(), sem delay().

// --- Invólucros mínimos das interfaces de hardware ---

class BotaoLargada : public IEntradaBotao {
public:
    bool pressionado() override { return digitalRead(PINO_BOTAO) == LOW; }  // pull-up interno
};

class BateriaRobo : public IBateria {
public:
    void iniciar() { bateria_.iniciar(); }
    uint16_t lerMilivolts() override { return bateria_.lerMilivolts(); }

private:
    Bateria bateria_;
};

class RadioEspNow : public IRadioTelemetria {
public:
    bool enviar(const TelemetriaPacket& pacote) override { return radioTelemetriaEnviar(pacote); }
};

class RegistroSerial : public IRegistro {
public:
    void registrar(const char* mensagem) override { Serial.println(mensagem); }
};

// --- Drivers e peças do laço (nenhum construtor mexe no hardware) ---

PonteHTb6612 ponte;
Motores motores(ponte);
Encoders encoders;
SensoresToF tof;
SinalizacaoLed led;
Odometria odometria;
ExecutorMovimentoRobo executor(motores, odometria, tof);
BotaoLargada botao;
BateriaRobo bateria;
RadioEspNow radio;
RegistroSerial registro;
CicloProva* ciclo = nullptr;  // criado no setup(), depois de saber se o rádio subiu

// Tempo de tick medido no robô (o desenho troca a preempção do FreeRTOS por esta medida;
// os valores vão para o relatório 7.5).
TemporizadorPeriodico temporizadorMedicao(5000);
uint32_t tickMaximoUs = 0;
uint32_t ticks = 0;

void setup() {
    Serial.begin(115200);
    pinMode(PINO_BOTAO, INPUT_PULLUP);

    ponte.iniciar();
    encoders.iniciar();
    led.iniciar();
    bateria.iniciar();
    if (!tof.iniciar()) {
        Serial.printf("ERRO: sensor ToF sem resposta (esq=%d frontal=%d dir=%d).\n",
                      tof.esquerdoIniciado(), tof.frontalIniciado(), tof.direitoIniciado());
    }

    const bool radioPronto = radioTelemetriaInit();
    Serial.println(radioPronto ? "ESP-NOW pronto (telemetria a 1 Hz)."
                               : "ERRO: falha ao iniciar ESP-NOW; a prova roda sem telemetria.");

    DependenciasProva d = {botao,    encoders,  tof,      bateria,
                           led,      motores,   odometria, executor,
                           radioPronto ? &radio : nullptr, &registro};
    static CicloProva instancia(d, CicloProva::ESPERA_LARGADA_HARDWARE_MS);
    ciclo = &instancia;
    ciclo->iniciar((uint32_t)millis());

    Serial.println("Robo ligado. Estado: AGUARDANDO");
    Serial.println("Toque 3 vezes no botao para trocar a pista; segure 2 s e solte para largar (1,2 s p/ afastar a mao).");
    Serial.println("Comandos Serial/ESP-NOW: [k] largada | [s] parada de emergencia | [d] diagnostico rapido.");
}

static void imprimirDiagnostico() {
    int32_t esq = 0, dir = 0;
    encoders.ler(esq, dir);
    const uint32_t agora = (uint32_t)millis();
    uint16_t mv = ciclo->ultimoPacote().bateria_mv;
    if (mv == 0 && ciclo->estado() == AGUARDANDO) {
        encoders.pausarEsquerdo();
        mv = bateria.lerMilivolts();
        encoders.retomarEsquerdo();
    }
    Serial.printf("[DIAG] estado=%u sub=%u pos=(L%u,C%u,ori=%u) ToF(E/F/D)=[%u/%u/%u mm ok=%d%d%d] Enc(E/D)=[%ld/%ld] Bat=%u mV rumoEst=%.1f deg\n",
                  (unsigned)ciclo->estado(),
                  (unsigned)ciclo->subfase(),
                  (unsigned)ciclo->navegador().linha(),
                  (unsigned)ciclo->navegador().coluna(),
                  (unsigned)ciclo->navegador().orientacao(),
                  (unsigned)tof.esquerdo().distanciaMm(),
                  (unsigned)tof.frontal().distanciaMm(),
                  (unsigned)tof.direito().distanciaMm(),
                  tof.esquerdoIniciado() ? 1 : 0,
                  tof.frontalIniciado() ? 1 : 0,
                  tof.direitoIniciado() ? 1 : 0,
                  (long)esq, (long)dir,
                  (unsigned)mv,
                  executor.erroRumoEstimadoGraus());
    (void)agora;
}

static void processarComando(char cmd) {
    if (cmd == 'k' || cmd == 'K') {
        ciclo->largadaRemota((uint32_t)millis());
    } else if (cmd == 's' || cmd == 'S' || cmd == ' ') {
        ciclo->paradaEmergencia((uint32_t)millis());
    } else if (cmd == 'd' || cmd == 'D') {
        imprimirDiagnostico();
    }
}

void loop() {
    const uint32_t inicioUs = micros();
    const uint32_t agora = (uint32_t)millis();

    while (Serial.available()) {
        processarComando((char)Serial.read());
    }
    char cmdRadio = 0;
    while (radioTelemetriaLerComando(cmdRadio)) {
        processarComando(cmdRadio);
    }

    ciclo->tick(agora);  // R10: millis() convertido para uint32_t
    const uint32_t duracaoUs = micros() - inicioUs;
    if (duracaoUs > tickMaximoUs) tickMaximoUs = duracaoUs;
    ticks++;

    if (temporizadorMedicao.vencido(agora)) {
        Serial.printf("[tick] %lu ticks em 5 s, maior tick %lu us\n",
                      (unsigned long)ticks, (unsigned long)tickMaximoUs);
        ticks = 0;
        tickMaximoUs = 0;
    }
}
