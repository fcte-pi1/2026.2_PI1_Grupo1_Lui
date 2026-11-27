/**
 * TESTE DE UM ENCODER (N20, quadratura), ISOLADO.
 *
 * Ambientes:
 *   env:teste_encoder_esquerdo -> -DTESTE_ENCODER_LADO=0 (PIN_ENC1_A / PIN_ENC1_B)
 *   env:teste_encoder_direito  -> -DTESTE_ENCODER_LADO=1 (PIN_ENC2_A / PIN_ENC2_B)
 *
 * Motores em standby: gire a roda com a mão. A contagem segue a mesma regra do
 * encoder_driver.cpp (borda de subida do canal A, sentido pelo nível do canal B).
 * O teste também conta as bordas de cada canal em separado, para mostrar se um
 * dos fios (A ou B) está sem sinal.
 * Digite 'z' para zerar as contagens.
 */

#include "comum_testes.h"

#ifndef TESTE_ENCODER_LADO
#error "Defina TESTE_ENCODER_LADO (0 = esquerdo, 1 = direito) no ambiente do platformio.ini"
#endif

#if TESTE_ENCODER_LADO == 0
static const char* NOME = "ENCODER ESQUERDO (Motor 1)";
static const uint8_t PINO_A = PIN_ENC1_A;
static const uint8_t PINO_B = PIN_ENC1_B;
static const int SINAL_B_ALTO = -1;   // mesma convenção do encoder_driver.cpp
static const bool INVERTIDO = ENCODER_ESQ_INVERTIDO;
#else
static const char* NOME = "ENCODER DIREITO (Motor 2)";
static const uint8_t PINO_A = PIN_ENC2_A;
static const uint8_t PINO_B = PIN_ENC2_B;
static const int SINAL_B_ALTO = +1;
static const bool INVERTIDO = ENCODER_DIR_INVERTIDO;
#endif

static volatile int32_t s_ticks = 0;
static volatile uint32_t s_bordas_a = 0;
static volatile uint32_t s_bordas_b = 0;

static void IRAM_ATTR isr_canal_a(void) {
    int passo = digitalRead(PINO_B) ? SINAL_B_ALTO : -SINAL_B_ALTO;
    if (INVERTIDO) passo = -passo;
    s_ticks += passo;
    s_bordas_a++;
}

static void IRAM_ATTR isr_canal_b(void) {
    s_bordas_b++;
}

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho(NOME,
        "  Motores desligados. Gire a roda deste encoder com a mao.\n"
        "  Uma volta da RODA deve dar perto do valor de pulsos por volta abaixo.\n"
        "  Girar para frente deve aumentar 'ticks'. Digite 'z' para zerar.\n"
        "  Com os dois canais bons, 'bordas B' fica perto do dobro de 'subidas A'.\n"
        "  Se um dos dois contadores nao sai do zero, aquele fio esta sem sinal.");

    // GPIO 34, 35, 36 e 39 são só de entrada e não têm pull-up interno.
    pinMode(PINO_A, (PINO_A >= 34) ? INPUT : INPUT_PULLUP);
    pinMode(PINO_B, (PINO_B >= 34) ? INPUT : INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PINO_A), isr_canal_a, RISING);
    attachInterrupt(digitalPinToInterrupt(PINO_B), isr_canal_b, CHANGE);

    Serial.printf("  Canal A = GPIO%u | Canal B = GPIO%u | pulsos/volta (calibracao) = %u\n",
                  PINO_A, PINO_B, (unsigned)ENCODER_TICKS_PER_REV);
}

void loop() {
    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'z' || c == 'Z') {
            noInterrupts();
            s_ticks = 0;
            s_bordas_a = 0;
            s_bordas_b = 0;
            interrupts();
            Serial.println("  >> contagens zeradas");
        }
    }

    static uint32_t ultimo_ms = 0;
    if (millis() - ultimo_ms < 200) return;
    ultimo_ms = millis();

    noInterrupts();
    int32_t ticks = s_ticks;
    uint32_t ba = s_bordas_a;
    uint32_t bb = s_bordas_b;
    interrupts();

    float voltas = (float)ticks / (float)ENCODER_TICKS_PER_REV;
    Serial.printf("ticks=%7ld  voltas=%6.2f  dist=%7.1f mm | subidas A=%6lu  bordas B=%6lu | nivel A=%d B=%d\n",
                  (long)ticks, voltas, voltas * WHEEL_CIRCUMFERENCE_MM,
                  (unsigned long)ba, (unsigned long)bb,
                  digitalRead(PINO_A), digitalRead(PINO_B));
}
