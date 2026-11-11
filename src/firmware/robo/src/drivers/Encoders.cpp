#include "drivers/Encoders.h"
#include <Arduino.h>
#include "hardware/Pinos.h"

// Contagens escritas pelas interrupções. A seção crítica protege a leitura das duas
// contagens juntas e funciona com interrupções nos dois núcleos do ESP32.
static volatile int32_t contagemEsq = 0;
static volatile int32_t contagemDir = 0;
static portMUX_TYPE travaContagens = portMUX_INITIALIZER_UNLOCKED;

// Sinais iguais aos do hw/firm (os dois motores são montados espelhados), com a inversão
// de montagem de hardware/Pinos.h aplicada por cima.
static void IRAM_ATTR aoPulsoEsq() {
    int32_t passo = digitalRead(PINO_ENCODER_ESQ_B) ? -1 : 1;
    if (ENCODER_ESQ_INVERTIDO) passo = -passo;
    portENTER_CRITICAL_ISR(&travaContagens);
    contagemEsq += passo;
    portEXIT_CRITICAL_ISR(&travaContagens);
}

static void IRAM_ATTR aoPulsoDir() {
    int32_t passo = digitalRead(PINO_ENCODER_DIR_B) ? 1 : -1;
    if (ENCODER_DIR_INVERTIDO) passo = -passo;
    portENTER_CRITICAL_ISR(&travaContagens);
    contagemDir += passo;
    portEXIT_CRITICAL_ISR(&travaContagens);
}

void Encoders::iniciar() {
    // GPIO 36, 39 e 35 só funcionam como entrada e não têm pull-up interno.
    pinMode(PINO_ENCODER_ESQ_A, INPUT);
    pinMode(PINO_ENCODER_ESQ_B, INPUT);
    pinMode(PINO_ENCODER_DIR_A, INPUT);
    pinMode(PINO_ENCODER_DIR_B, INPUT_PULLUP);  // GPIO 2 tem pull-up interno

    zerar();
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_ESQ_A), aoPulsoEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_DIR_A), aoPulsoDir, RISING);
}

int32_t Encoders::esq() const {
    portENTER_CRITICAL(&travaContagens);
    const int32_t v = contagemEsq;
    portEXIT_CRITICAL(&travaContagens);
    return v;
}

int32_t Encoders::dir() const {
    portENTER_CRITICAL(&travaContagens);
    const int32_t v = contagemDir;
    portEXIT_CRITICAL(&travaContagens);
    return v;
}

void Encoders::ler(int32_t& esq, int32_t& dir) const {
    portENTER_CRITICAL(&travaContagens);
    esq = contagemEsq;
    dir = contagemDir;
    portEXIT_CRITICAL(&travaContagens);
}

void Encoders::zerar() {
    portENTER_CRITICAL(&travaContagens);
    contagemEsq = 0;
    contagemDir = 0;
    portEXIT_CRITICAL(&travaContagens);
}
