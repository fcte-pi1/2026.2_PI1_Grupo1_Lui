#ifndef INTERFACES_HARDWARE_H
#define INTERFACES_HARDWARE_H

#include <stdint.h>
#include "TelemetriaPacket.h"                 // TelemetriaPacket, EstadoRobo
#include "../seguranca/MonitorTravamento.h"   // ISinalizacaoEstado
#include "../sensores/LeituraToF.h"
#include "../sinalizacao/PadraoLed.h"         // CorLed

// O que o laço da prova (app/CicloProva.h) e o executor de movimento
// (movimento/ExecutorMovimentoRobo.h) precisam do hardware, sem dependência do Arduino
// (docs/firmware/integracao-loop.md, seção 3.2). No robô, quem implementa são os drivers
// de src/drivers e pequenos invólucros no main.cpp; nos testes nativos e no robô virtual,
// dublês. Os motores não aparecem aqui: Motores (drivers/Motores.h) já é puro e fala com
// o hardware pela IPonteH.

// Botão de largada: nível lido no pino, já invertido (true = pressionado).
class IEntradaBotao {
public:
    virtual ~IEntradaBotao() {}
    virtual bool pressionado() = 0;
};

// Contagens acumuladas dos encoders (crescem ao avançar).
class IEncoders {
public:
    virtual ~IEncoders() {}
    virtual void ler(int32_t& esq, int32_t& dir) = 0;

    // Pausa e retoma a interrupção do encoder esquerdo. Os canais dele estão nos GPIO 36 e
    // 39, que a errata do ESP32 (item 3.11) puxa para baixo por cerca de 80 ns quando o
    // ADC1 liga: sem a pausa, cada leitura da bateria pode contar pulsos falsos
    // (pendência 9 do desenho). O laço pausa só durante a leitura da bateria.
    virtual void pausarEsquerdo() = 0;
    virtual void retomarEsquerdo() = 0;
};

// Três sensores ToF lidos sem espera (B10). atualizar() consulta os sensores; as leituras
// dizem se ainda valem.
class ISensoresToF {
public:
    virtual ~ISensoresToF() {}
    virtual void atualizar(uint32_t agoraMs) = 0;
    virtual LeituraToF& esquerdo() = 0;
    virtual LeituraToF& frontal() = 0;
    virtual LeituraToF& direito() = 0;
};

// Tensão da bateria em mV (a média de amostras do ADC leva cerca de 2 ms).
class IBateria {
public:
    virtual ~IBateria() {}
    virtual uint16_t lerMilivolts() = 0;
};

// Envio do pacote de 18 bytes (ESP-NOW no robô). Retorna false se o envio falhar.
class IRadioTelemetria {
public:
    virtual ~IRadioTelemetria() {}
    virtual bool enviar(const TelemetriaPacket& pacote) = 0;
};

// LED de estado: o padrão do estado (mostrarEstado, herdado do MonitorTravamento) e
// piscadas de aviso que passam por cima dele por um tempo (geometria escolhida, largada
// recusada). Tudo por relógio, sem delay() (R8).
class ILedEstado : public ISinalizacaoEstado {
public:
    virtual ~ILedEstado() {}
    virtual void piscar(CorLed cor, uint8_t vezes, uint32_t agoraMs) = 0;
    virtual void atualizar(uint32_t agoraMs, bool largadaArmada) = 0;
};

// Registro de eventos para o monitor serial (opcional).
class IRegistro {
public:
    virtual ~IRegistro() {}
    virtual void registrar(const char* mensagem) = 0;
};

#endif
