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
static const uint8_t ENDERECO_PADRAO_VL53 = 0x29;
static const uint8_t ENDERECO_TEMP_SEMPRE_LIGADO = 0x3F;

// Destrava escravo I2C que tenha ficado segurando SDA em LOW apos reset em meio a leitura.
static void recuperarBarramentoI2C() {
    pinMode(PINO_I2C_SDA, INPUT_PULLUP);
    pinMode(PINO_I2C_SCL, OUTPUT);
    digitalWrite(PINO_I2C_SCL, HIGH);
    delayMicroseconds(10);
    for (uint8_t i = 0; i < 9 && digitalRead(PINO_I2C_SDA) == LOW; i++) {
        digitalWrite(PINO_I2C_SCL, LOW);
        delayMicroseconds(10);
        digitalWrite(PINO_I2C_SCL, HIGH);
        delayMicroseconds(10);
    }
    pinMode(PINO_I2C_SCL, INPUT_PULLUP);
}

static bool enderecoResponde(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

static bool inicializarSensorNoBarramento(VL53L0X& s, uint8_t enderecoNovo) {
    for (uint8_t tent = 0; tent < 2; tent++) {
        s.setBus(&Wire);
        s.setTimeout(TOF_TIMEOUT_BIBLIOTECA_MS);
        if (s.init()) {
            s.setAddress(enderecoNovo);
            s.setMeasurementTimingBudget(TOF_ORCAMENTO_US);
            s.startContinuous();
            return true;
        }
        delay(15);
    }
    return false;
}

bool SensoresToF::iniciar() {
    for (uint8_t i = 0; i < QTD_TOF_I2C; i++) {
        filtros[i].reiniciar();
        iniciado_[i] = false;
        esperaAteMs_[i] = 0;
        pinMode(PINOS_XSHUT[i], OUTPUT);
        digitalWrite(PINOS_XSHUT[i], LOW);
    }
    delay(25);

    recuperarBarramentoI2C();
    Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL, TOF_I2C_HZ);
    Wire.setTimeOut(TOF_I2C_TIMEOUT_MS);
    delay(5);

    // Resiliencia de hardware: se 1 sensor estiver com XSHUT desconectado (sempre ligado
    // em 0x29), estaciona-o em 0x3F para liberar 0x29 aos demais e o adota na vaga restante.
    VL53L0X sensorSempreLigado;
    bool haviaSempreLigado = false;
    if (enderecoResponde(ENDERECO_PADRAO_VL53)) {
        Serial.println("AVISO ToF: dispositivo respondeu em 0x29 com todos os XSHUT em LOW (XSHUT desconectado).");
        haviaSempreLigado = inicializarSensorNoBarramento(sensorSempreLigado, ENDERECO_TEMP_SEMPRE_LIGADO);
    }

    int8_t idxFaltante = -1;
    uint8_t qtdFaltantes = 0;
    for (uint8_t i = 0; i < QTD_TOF_I2C; i++) {
        digitalWrite(PINOS_XSHUT[i], HIGH);
        delay(20);
        const bool ok = enderecoResponde(ENDERECO_PADRAO_VL53) &&
                        inicializarSensorNoBarramento(sensoresI2C[i], ENDERECOS[i]);
        iniciado_[i] = ok;
        if (!ok) {
            idxFaltante = (int8_t)i;
            qtdFaltantes++;
        }
    }

    if (haviaSempreLigado && qtdFaltantes == 1 && idxFaltante >= 0) {
        sensorSempreLigado.setAddress(ENDERECOS[idxFaltante]);
        if (enderecoResponde(ENDERECOS[idxFaltante])) {
            sensoresI2C[idxFaltante] = sensorSempreLigado;
            iniciado_[idxFaltante] = true;
            Serial.printf("AVISO ToF: sensor sem XSHUT adotado no slot %d (0x%02X).\n",
                          (int)idxFaltante, (unsigned)ENDERECOS[idxFaltante]);
        }
    }

    proximoSensor = 0;
    consultou_ = false;
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
            continue;
        }
        // Se este sensor ainda nao tem amostra nova pronta, consulta o proximo no mesmo tick!
        if ((interrupcao & 0x07) == 0) continue;

        const uint8_t statusReg = s.readReg(VL53L0X::RESULT_RANGE_STATUS);
        const bool statusOk = (s.last_status == 0);
        const uint16_t mmBruto = s.readReg16Bit(VL53L0X::RESULT_RANGE_STATUS + DESLOCAMENTO_DISTANCIA);
        const bool lido = statusOk && (s.last_status == 0);
        s.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);
        if (lido) {
            const uint16_t classificada = classificarLeituraVL53(statusReg, mmBruto, OFFSETS_MM[i]);
            const uint16_t filtrada = filtros[i].atualizar(classificada);
            leituras[i]->registrar(filtrada, agoraMs);
        } else {
            esperaAteMs_[i] = agoraMs + TOF_ESPERA_APOS_ERRO_MS;
        }
        break;  // No maximo 1 leitura completa por consulta para manter tick < 3 ms
    }
}
