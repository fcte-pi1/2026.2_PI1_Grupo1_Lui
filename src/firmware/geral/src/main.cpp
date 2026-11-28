#include <Arduino.h>
#include "BancadaGeral.h"
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

// Firmware geral unificado: combina todos os testes/comissionamento de bancada
// com a prova autonoma completa (CicloProva + Navegador Flood Fill), usando os
// mesmos drivers e parametros de src/firmware/robo e src/firmware/lib/calibracao.

// Nao ha botao de largada conectado a GPIO (chave fisica apenas liga/desliga geral);
// a largada e disparada por software via Serial/ESP-NOW ('k').
class BotaoGeral : public IEntradaBotao {
public:
    bool pressionado() override { return false; }
};

class BateriaGeral : public IBateria {
public:
    void iniciar() { driver.iniciar(); }
    uint16_t lerMilivolts() override { return driver.lerMilivolts(); }
    Bateria driver;
};

class RadioGeral : public IRadioTelemetria {
public:
    bool enviar(const TelemetriaPacket& pacote) override { return radioTelemetriaEnviar(pacote); }
};

class RegistroGeral : public IRegistro {
public:
    void registrar(const char* mensagem) override { Serial.println(mensagem); }
};

PonteHTb6612 ponte;
Motores motores(ponte);
Encoders encoders;
SensoresToF tof;
SinalizacaoLed led;
Odometria odometria;
ExecutorMovimentoRobo executor(motores, odometria, tof);
BotaoGeral botao;
BateriaGeral bateria;
RadioGeral radio;
RegistroGeral registro;
CicloProva* ciclo = nullptr;
BancadaGeral* bancada = nullptr;

static bool emProvaAtiva() {
    if (ciclo == nullptr) return false;
    const EstadoRobo st = ciclo->estado();
    return st == MAPEANDO || st == SPEED_RUN;
}

static void processarComando(char cmd, uint32_t agoraMs) {
    if (cmd == '\r' || cmd == '\n') return;

    if (cmd == 's' || cmd == 'S' || cmd == ' ') {
        if (emProvaAtiva()) {
            ciclo->paradaEmergencia(agoraMs);
        } else {
            bancada->abortar(agoraMs);
            motores.frear();
            Serial.println("[EMERGENCIA] Motores freados e teste cancelado.");
        }
        return;
    }

    if (cmd == 'd' || cmd == 'D') {
        if (!bancada->ativa() || emProvaAtiva()) {
            bancada->imprimirDiagnostico(*ciclo, agoraMs);
            return;
        }
    }

    // Durante a corrida autonoma, comandos de bancada ficam bloqueados por seguranca.
    if (emProvaAtiva()) return;

    if (cmd == 'k' || cmd == 'K') {
        if (!bancada->ativa() && ciclo->estado() == AGUARDANDO) {
            ciclo->largadaRemota(agoraMs);
            return;
        }
        if (bancada->ativa()) {
            bancada->abortar(agoraMs);
            if (ciclo->estado() == AGUARDANDO) {
                ciclo->largadaRemota(agoraMs);
                return;
            }
        }
    }

    bancada->tratarComando(cmd, *ciclo, agoraMs);
}

void setup() {
    Serial.begin(115200);

    ponte.iniciar();
    encoders.iniciar();
    led.iniciar();
    bateria.iniciar();
    if (!tof.iniciar()) {
        Serial.printf("AVISO: ToF sem resposta (esq=%d frontal=%d dir=%d).\n",
                      tof.esquerdoIniciado(), tof.frontalIniciado(), tof.direitoIniciado());
    }

    const bool radioPronto = radioTelemetriaInit();
    Serial.println(radioPronto ? "ESP-NOW pronto (telemetria a 1 Hz)."
                               : "AVISO: falha ao iniciar ESP-NOW.");

    DependenciasProva dep = {botao, encoders, tof, bateria, led, motores,
                             odometria, executor, radioPronto ? &radio : nullptr, &registro};
    static CicloProva instanciaCiclo(dep, CicloProva::ESPERA_LARGADA_HARDWARE_MS);
    static BancadaGeral instanciaBancada(ponte, motores, encoders, tof, bateria.driver,
                                         led, odometria, executor, radioPronto);
    ciclo = &instanciaCiclo;
    bancada = &instanciaBancada;
    ciclo->iniciar((uint32_t)millis());
    bancada->imprimirMenu(*ciclo);
}

void loop() {
    const uint32_t agora = (uint32_t)millis();

    while (Serial.available()) {
        processarComando((char)Serial.read(), agora);
    }
    char cmdRadio = 0;
    while (radioTelemetriaLerComando(cmdRadio)) {
        processarComando(cmdRadio, agora);
    }

    if (bancada->ativa() && !emProvaAtiva()) {
        bancada->atualizar(agora, *ciclo);
    } else {
        ciclo->tick(agora);
    }
}
