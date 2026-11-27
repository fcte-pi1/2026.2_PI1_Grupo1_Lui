/**
 * TESTE DO BOTÃO DE LARGADA (IHM), ISOLADO.
 *
 * Ambiente: env:teste_botao
 *
 * PIN_BUTTON_START com pull-up interno: solto = HIGH, apertado = LOW.
 * Cada aperto e cada soltura aparecem no monitor serial, com o tempo apertado.
 */

#include "comum_testes.h"

static const uint32_t DEBOUNCE_MS = 30;

static int s_estado_estavel = HIGH;
static int s_ultima_leitura = HIGH;
static uint32_t s_mudanca_ms = 0;
static uint32_t s_apertou_ms = 0;
static uint32_t s_apertos = 0;

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho("BOTAO DE LARGADA",
        "  Aperte e solte o botao algumas vezes.\n"
        "  Se aparecer APERTADO sem tocar, o pino esta em curto com o GND.");

    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    s_estado_estavel = digitalRead(PIN_BUTTON_START);
    s_ultima_leitura = s_estado_estavel;
    Serial.printf("  Botao=GPIO%u | estado inicial: %s\n", PIN_BUTTON_START,
                  s_estado_estavel == LOW ? "APERTADO" : "solto");
}

void loop() {
    int leitura = digitalRead(PIN_BUTTON_START);
    if (leitura != s_ultima_leitura) {
        s_ultima_leitura = leitura;
        s_mudanca_ms = millis();
    }
    if (leitura != s_estado_estavel && millis() - s_mudanca_ms >= DEBOUNCE_MS) {
        s_estado_estavel = leitura;
        if (leitura == LOW) {
            s_apertos++;
            s_apertou_ms = millis();
            Serial.printf("[%7lu ms] APERTADO (aperto #%lu)\n",
                          (unsigned long)millis(), (unsigned long)s_apertos);
        } else {
            Serial.printf("[%7lu ms] solto depois de %lu ms\n",
                          (unsigned long)millis(), (unsigned long)(millis() - s_apertou_ms));
        }
    }
}
