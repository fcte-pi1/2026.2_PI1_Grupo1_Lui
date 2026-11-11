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
    static CicloProva instancia(d);
    ciclo = &instancia;
    ciclo->iniciar((uint32_t)millis());

    Serial.println("Robo ligado. Estado: AGUARDANDO");
    Serial.println("Toque 3 vezes no botao para trocar a pista; segure 2 s (LED branco) e solte para largar.");
}

void loop() {
    const uint32_t inicioUs = micros();
    ciclo->tick((uint32_t)millis());  // R10: millis() convertido para uint32_t
    const uint32_t duracaoUs = micros() - inicioUs;
    if (duracaoUs > tickMaximoUs) tickMaximoUs = duracaoUs;
    ticks++;

    if (temporizadorMedicao.vencido((uint32_t)millis())) {
        Serial.printf("[tick] %lu ticks em 5 s, maior tick %lu us\n",
                      (unsigned long)ticks, (unsigned long)tickMaximoUs);
        ticks = 0;
        tickMaximoUs = 0;
    }
}
