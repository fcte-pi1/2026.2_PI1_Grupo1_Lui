#include "drivers/SensoresToF.h"
#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>
#include "hardware/Pinos.h"
#include "sensores/FiltroToF.h"

// Quantidade de VL53L0X no barramento I2C conforme CAL_MODO_SENSORES:
//   0: 3x ToF (Esq, Frontal, Dir)
//   1: 2x ToF laterais (Esq, Dir) + Ultrassonico frontal
//   2: 2x ToF apenas (Esq, Frontal)
#if CAL_MODO_SENSORES == 2
static const uint8_t QTD_TOF_I2C = 2;
static const uint8_t PINOS_XSHUT[QTD_TOF_I2C] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL};
static const uint8_t ENDERECOS[QTD_TOF_I2C] = {TOF_ENDERECO_ESQ, TOF_ENDERECO_FRONTAL};
static const int16_t OFFSETS_MM[QTD_TOF_I2C] = {TOF_OFFSET_ESQ_MM, TOF_OFFSET_FRONTAL_MM};
static const uint8_t IDX_INICIADO[QTD_TOF_I2C] = {0, 1};
#elif CAL_USA_ULTRASSONICO_FRONTAL
static const uint8_t QTD_TOF_I2C = 2;
static const uint8_t PINOS_XSHUT[QTD_TOF_I2C] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_DIR};
static const uint8_t ENDERECOS[QTD_TOF_I2C] = {TOF_ENDERECO_ESQ, TOF_ENDERECO_DIR};
static const int16_t OFFSETS_MM[QTD_TOF_I2C] = {TOF_OFFSET_ESQ_MM, TOF_OFFSET_DIR_MM};
static const uint8_t IDX_INICIADO[QTD_TOF_I2C] = {0, 2};
#else
static const uint8_t QTD_TOF_I2C = 3;
static const uint8_t PINOS_XSHUT[QTD_TOF_I2C] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR};
static const uint8_t ENDERECOS[QTD_TOF_I2C] = {TOF_ENDERECO_ESQ, TOF_ENDERECO_FRONTAL, TOF_ENDERECO_DIR};
static const int16_t OFFSETS_MM[QTD_TOF_I2C] = {TOF_OFFSET_ESQ_MM, TOF_OFFSET_FRONTAL_MM, TOF_OFFSET_DIR_MM};
static const uint8_t IDX_INICIADO[QTD_TOF_I2C] = {0, 1, 2};
#endif

static VL53L0X sensoresI2C[QTD_TOF_I2C];
static FiltroMedianaToF filtros[3];
static uint8_t proximoSensor = 0;

static const uint8_t DESLOCAMENTO_DISTANCIA = 10;

#if CAL_USA_ULTRASSONICO_FRONTAL
static volatile uint32_t usSubidaUs = 0;
static volatile uint32_t usDuracaoUs = 0;
static volatile bool usAguardandoDescida = false;
static volatile bool usViuSubida = false;
static volatile bool usAmostraPronta = false;

static bool usEmMedicao = false;
static uint32_t usInicioPingUs = 0;
static uint32_t usUltimoPingMs = 0;

static void IRAM_ATTR isrEchoUltrassonico() {
    const uint32_t agoraUs = (uint32_t)micros();
    if (digitalRead(PINO_ULTRASSONICO_ECHO) == HIGH) {
        usSubidaUs = agoraUs;
        usAguardandoDescida = true;
        usViuSubida = true;
    } else if (usAguardandoDescida) {
        usDuracaoUs = agoraUs - usSubidaUs;
        usAguardandoDescida = false;
        usAmostraPronta = true;
    }
}

static void dispararPulsoUltrassonico(uint32_t agoraMs) {
    noInterrupts();
    usAguardandoDescida = false;
    usViuSubida = false;
    usAmostraPronta = false;
    interrupts();

    usUltimoPingMs = agoraMs;
    usInicioPingUs = (uint32_t)micros();
    usEmMedicao = true;

    digitalWrite(PINO_ULTRASSONICO_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PINO_ULTRASSONICO_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PINO_ULTRASSONICO_TRIG, LOW);
}
#endif

bool SensoresToF::iniciar() {
    Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL, TOF_I2C_HZ);
    Wire.setTimeOut(TOF_I2C_TIMEOUT_MS);

    for (uint8_t i = 0; i < 3; i++) filtros[i].reiniciar();
    for (uint8_t i = 0; i < QTD_TOF_I2C; i++) {
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
        iniciado_[IDX_INICIADO[i]] = ok;
        if (ok) {
            s.setAddress(ENDERECOS[i]);
            s.setMeasurementTimingBudget(TOF_ORCAMENTO_US);
            s.startContinuous();
        }
    }

#if CAL_USA_ULTRASSONICO_FRONTAL
    pinMode(PINO_ULTRASSONICO_TRIG, OUTPUT);
    digitalWrite(PINO_ULTRASSONICO_TRIG, LOW);
    pinMode(PINO_ULTRASSONICO_ECHO, INPUT);
    attachInterrupt(digitalPinToInterrupt(PINO_ULTRASSONICO_ECHO), isrEchoUltrassonico, CHANGE);

    dispararPulsoUltrassonico((uint32_t)millis());
    const uint32_t t0 = (uint32_t)millis();
    while (!usViuSubida && (uint32_t)((uint32_t)millis() - t0) < 5) {
        delayMicroseconds(100);
    }
    iniciado_[1] = usViuSubida;
#endif

    proximoSensor = 0;
    esq_.reiniciar();
    frente_.reiniciar();
    dir_.reiniciar();
#if CAL_TEM_TOF_DIREITO
    return iniciado_[0] && iniciado_[1] && iniciado_[2];
#else
    return iniciado_[0] && iniciado_[1];
#endif
}

void SensoresToF::atualizar(uint32_t agoraMs) {
#if CAL_USA_ULTRASSONICO_FRONTAL
    bool pronta = false;
    bool viuSubida = false;
    uint32_t duracaoUs = 0;
    noInterrupts();
    if (usAmostraPronta) {
        pronta = true;
        duracaoUs = usDuracaoUs;
        usAmostraPronta = false;
    }
    viuSubida = usViuSubida;
    interrupts();

    if (pronta) {
        usEmMedicao = false;
        const uint16_t classificada = classificarLeituraUltrassonico(duracaoUs, TOF_OFFSET_FRONTAL_MM);
        frente_.registrar(filtros[2].atualizar(classificada), agoraMs);
    } else if (usEmMedicao &&
               (uint32_t)((uint32_t)micros() - usInicioPingUs) > ULTRASSONICO_TIMEOUT_US) {
        usEmMedicao = false;
        if (viuSubida) {
            frente_.registrar(filtros[2].atualizar(TOF_LEITURA_INVALIDA_MM), agoraMs);
        }
    }

    if (!usEmMedicao && (uint32_t)(agoraMs - usUltimoPingMs) >= ULTRASSONICO_PERIODO_MS) {
        dispararPulsoUltrassonico(agoraMs);
    }
#endif

    if (consultou_ && (uint32_t)(agoraMs - ultimaConsultaMs_) < TOF_PERIODO_CONSULTA_MS) return;
    consultou_ = true;
    ultimaConsultaMs_ = agoraMs;

#if CAL_MODO_SENSORES == 2
    LeituraToF* leituras[QTD_TOF_I2C] = {&esq_, &frente_};
#elif CAL_USA_ULTRASSONICO_FRONTAL
    LeituraToF* leituras[QTD_TOF_I2C] = {&esq_, &dir_};
#else
    LeituraToF* leituras[QTD_TOF_I2C] = {&esq_, &frente_, &dir_};
#endif

    for (uint8_t tent = 0; tent < QTD_TOF_I2C; tent++) {
        const uint8_t i = proximoSensor;
        proximoSensor = (uint8_t)((proximoSensor + 1) % QTD_TOF_I2C);
        const uint8_t idx = IDX_INICIADO[i];
        if (!iniciado_[idx]) continue;
        if ((int32_t)(agoraMs - esperaAteMs_[idx]) < 0) continue;
        VL53L0X& s = sensoresI2C[i];

        const uint8_t interrupcao = s.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
        if (s.last_status != 0) {
            esperaAteMs_[idx] = agoraMs + TOF_ESPERA_APOS_ERRO_MS;
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
