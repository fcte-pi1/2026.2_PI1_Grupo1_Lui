#include "drivers/SensoresToF.h"
#include <Arduino.h>
#include <VL53L0X.h>
#include <Wire.h>
#include "hardware/Pinos.h"

// Os objetos da biblioteca ficam aqui, para o cabeçalho não depender dela.
static VL53L0X sensores[3];
static const uint8_t PINOS_XSHUT[3] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR};
static const uint8_t ENDERECOS[3] = {TOF_ENDERECO_ESQ, TOF_ENDERECO_FRONTAL, TOF_ENDERECO_DIR};

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

    esq_.reiniciar();
    frente_.reiniciar();
    dir_.reiniciar();
    return iniciado_[0] && iniciado_[1] && iniciado_[2];
}

void SensoresToF::atualizar(uint32_t agoraMs) {
    if (consultou_ && (uint32_t)(agoraMs - ultimaConsultaMs_) < TOF_PERIODO_CONSULTA_MS) return;
    consultou_ = true;
    ultimaConsultaMs_ = agoraMs;

    LeituraToF* leituras[3] = {&esq_, &frente_, &dir_};
    for (uint8_t i = 0; i < 3; i++) {
        if (!iniciado_[i]) continue;
        if ((int32_t)(agoraMs - esperaAteMs_[i]) < 0) continue;  // em espera depois de erro
        VL53L0X& s = sensores[i];

        // Amostra pronta? Sem esperar: se não, tenta de novo na próxima consulta.
        const uint8_t interrupcao = s.readReg(VL53L0X::RESULT_INTERRUPT_STATUS);
        if (s.last_status != 0) {
            // Erro no barramento (sensor solto ou travado): cada transação com erro pode
            // custar até TOF_I2C_TIMEOUT_MS. Este sensor fica TOF_ESPERA_APOS_ERRO_MS sem ser
            // consultado e a consulta acaba aqui, para o laço não acumular esperas.
            esperaAteMs_[i] = agoraMs + TOF_ESPERA_APOS_ERRO_MS;
            break;
        }
        if ((interrupcao & 0x07) == 0) continue;

        const uint16_t mm = s.readReg16Bit(VL53L0X::RESULT_RANGE_STATUS + DESLOCAMENTO_DISTANCIA);
        const bool lido = (s.last_status == 0);
        s.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR, 0x01);
        if (lido) leituras[i]->registrar(mm, agoraMs);
    }
}
