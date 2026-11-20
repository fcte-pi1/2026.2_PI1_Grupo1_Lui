#include "drivers/SensoresToF.h"
#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>
#include "hardware/Pinos.h"
#include "sensores/FiltroToF.h"

// Os objetos da biblioteca ficam aqui, para o cabeçalho não depender dela.
static VL53L0X sensores[3];
static FiltroMedianaToF filtros[3];
static const uint8_t PINOS_XSHUT[3] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR};
static const uint8_t ENDERECOS[3] = {TOF_ENDERECO_ESQ, TOF_ENDERECO_FRONTAL, TOF_ENDERECO_DIR};
static const int16_t OFFSETS_MM[3] = {TOF_OFFSET_ESQ_MM, TOF_OFFSET_FRONTAL_MM, TOF_OFFSET_DIR_MM};
static uint8_t proximoSensor = 0;

// Registrador de resultado: a distância fica 10 bytes depois de RESULT_RANGE_STATUS (é o
// que a biblioteca lê em readRangeContinuousMillimeters()).
static const uint8_t DESLOCAMENTO_DISTANCIA = 10;

bool SensoresToF::iniciar() {
    Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL, TOF_I2C_HZ);
    // Um sensor que trave o barramento não pode segurar o laço pelos 50 ms padrão do Wire.
    Wire.setTimeOut(TOF_I2C_TIMEOUT_MS);

    // Todos desligados; depois liga um por vez e troca o endereço (todos nascem em 0x29).
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(PINOS_XSHUT[i], OUTPUT);
        digitalWrite(PINOS_XSHUT[i], LOW);
        filtros[i].reiniciar();
    }
    delay(15);  // só no setup(): reset dos sensores

    for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(PINOS_XSHUT[i], HIGH);
        delay(15);
        VL53L0X& s = sensores[i];
        s.setBus(&Wire);
        s.setTimeout(TOF_TIMEOUT_BIBLIOTECA_MS);  // segunda barreira (o hw/firm usa 500 ms)
        iniciado_[i] = s.init();
        if (iniciado_[i]) {
            s.setAddress(ENDERECOS[i]);
            s.setMeasurementTimingBudget(TOF_ORCAMENTO_US);
            s.startContinuous();
        }
    }

    proximoSensor = 0;
    esq_.reiniciar();
    frente_.reiniciar();
    dir_.reiniciar();
    return iniciado_[0] && iniciado_[1] && iniciado_[2];
}

void SensoresToF::atualizar(uint32_t agoraMs) {
    if (consultou_ && (uint32_t)(agoraMs - ultimaConsultaMs_) < TOF_PERIODO_CONSULTA_MS) return;
    consultou_ = true;
    ultimaConsultaMs_ = agoraMs;

    // Consulta um sensor por vez em round-robin (15 ms por ciclo completo vs 33 ms de
    // orçamento), reduzindo picos de latência no barramento I2C e no loop().
    LeituraToF* leituras[3] = {&esq_, &frente_, &dir_};
    for (uint8_t tent = 0; tent < 3; tent++) {
        const uint8_t i = proximoSensor;
        proximoSensor = (uint8_t)((proximoSensor + 1) % 3);
        if (!iniciado_[i]) continue;
        if ((int32_t)(agoraMs - esperaAteMs_[i]) < 0) continue;  // em espera depois de erro
        VL53L0X& s = sensores[i];

        // Amostra pronta? Sem esperar: se não, passa a vez ao próximo sensor no ciclo seguinte.
        const uint8_t interrupcao = s.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
        if (s.last_status != 0) {
            esperaAteMs_[i] = agoraMs + TOF_ESPERA_APOS_ERRO_MS;
            break;
        }
        if ((interrupcao & 0x07) == 0) break;

        // Lê status óptico (RESULT_RANGE_STATUS) + distância; aplica offset e mediana(3).
        const uint8_t statusReg = s.readReg(VL53L0X::RESULT_RANGE_STATUS);
        const bool statusOk = (s.last_status == 0);
        const uint16_t mmBruto = s.readReg16Bit(VL53L0X::RESULT_RANGE_STATUS + DESLOCAMENTO_DISTANCIA);
        const bool lido = statusOk && (s.last_status == 0);
        s.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);
        if (lido) {
            const uint16_t classificada = classificarLeituraVL53(statusReg, mmBruto, OFFSETS_MM[i]);
            const uint16_t filtrada = filtros[i].atualizar(classificada);
            leituras[i]->registrar(filtrada, agoraMs);
        }
        break;
    }
}
