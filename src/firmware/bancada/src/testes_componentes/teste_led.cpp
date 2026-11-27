/**
 * TESTE DO LED DE STATUS, ISOLADO.
 *
 * Ambiente: env:teste_led
 *
 * PIN_STATUS_LED pisca a 1 Hz (500 ms aceso, 500 ms apagado).
 * Atenção: no pinout.h o mesmo GPIO também é PIN_XSHUT_4_EXTRA (reserva para
 * um 4º ToF); use um ou outro, não os dois.
 */

#include "comum_testes.h"

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho("LED DE STATUS",
        "  O LED deve piscar uma vez por segundo.");

    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
    Serial.printf("  LED=GPIO%u\n", PIN_STATUS_LED);
}

void loop() {
    static uint32_t ultimo_ms = 0;
    if (millis() - ultimo_ms < 500) return;
    ultimo_ms = millis();

    int novo = !digitalRead(PIN_STATUS_LED);
    digitalWrite(PIN_STATUS_LED, novo);
    Serial.printf("[%7lu ms] LED %s\n", (unsigned long)millis(), novo ? "ACESO" : "apagado");
}
