#include "drivers/SensoresToF.h"
#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>
#include "hardware/Pinos.h"
#include "sensores/FiltroToF.h"

// Configuracao oficial: 3x sensores VL53L0X no barramento I2C (Esq, Frontal, Dir).
static const uint8_t QTD_TOF_I2C = 3;
static const uint8_t PINOS_XSHUT[QTD_TOF_I2C] = {
    PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR
};
static const uint8_t ENDERECOS[QTD_TOF_I2C] = {
    TOF_ENDERECO_ESQ, TOF_ENDERECO_FRONTAL, TOF_ENDERECO_DIR
};
static const int16_t OFFSETS_MM[QTD_TOF_I2C] = {
    TOF_OFFSET_ESQ_MM, TOF_OFFSET_FRONTAL_MM, TOF_OFFSET_DIR_MM
};

static VL53L0X sensoresI2C[QTD_TOF_I2C];
static FiltroMedianaToF filtros[QTD_TOF_I2C];
static uint8_t proximoSensor = 0;

static const uint8_t DESLOCAMENTO_DISTANCIA = 10;

bool SensoresToF::iniciar() {
    Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL, TOF_I2C_HZ);
    Wire.setTimeOut(TOF_I2C_TIMEOUT_MS);

    for (uint8_t i = 0; i < QTD_TOF_I2C; i++) {
        filtros[i].reiniciar();
        pinMode(PINOS_XSHUT[i], OUTPUT);
        digitalWrite(PINOS_XSHUT[i], LOW);
    }
    delay(15);

    for (uint8_t i = 0; i < QTD_TOF_I2C; i++) {
        digitalWrite(PINOS_XSHUT[i], HIGH);
        delay(15);
        VL53L0X& s = sensoresI2C[i];
        s.setBus(&Wire);
        s.setTimeout(TOF_TIMEOUT_BIBLIOTECA_MS);
        const bool ok = s.init();
        iniciado_[i] = ok;
        if (ok) {
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

    LeituraToF* leituras[QTD_TOF_I2C] = {&esq_, &frente_, &dir_};

    for (uint8_t tent = 0; tent < QTD_TOF_I2C; tent++) {
        const uint8_t i = proximoSensor;
        proximoSensor = (uint8_t)((proximoSensor + 1) % QTD_TOF_I2C);
        if (!iniciado_[i]) continue;
        if ((int32_t)(agoraMs - esperaAteMs_[i]) < 0) continue;
        VL53L0X& s = sensoresI2C[i];

        const uint8_t interrupcao = s.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
        if (s.last_status != 0) {
            esperaAteMs_[i] = agoraMs + TOF_ESPERA_APOS_ERRO_MS;
            break;
        }
        if ((interrupcao & 0x07) == 0) break;

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
