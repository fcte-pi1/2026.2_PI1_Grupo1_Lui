/**
 * TESTE DOS 3 SENSORES VL53L0X LIGADOS JUNTOS, PASSO A PASSO.
 *
 * Ambiente: env:teste_tof_sequencia
 *
 * Use depois que cada sensor passar no seu teste isolado (teste_tof_*).
 * Refaz a mesma sequência do vl53l0x_manager.cpp, mas varrendo o I2C a cada
 * passo, para mostrar em qual sensor a inicialização conjunta quebra:
 *   0. todos os XSHUT em LOW            -> nenhum endereço deve responder
 *   n. liga o XSHUT do sensor n         -> deve aparecer 0x29
 *      init() e troca para 0x30/31/32   -> 0x29 deve sumir e o novo aparecer
 * Ao final mede os três em modo contínuo e conta timeouts de cada um.
 * Digite 'r' para refazer a sequência.
 */

#include "comum_testes.h"
#include <VL53L0X.h>

struct CanalTeste {
    const char* nome;
    uint8_t xshut;
    uint8_t endereco;
    VL53L0X sensor;
    bool pronto;
    uint32_t timeouts;
};

static CanalTeste s_canais[3] = {
    { "ESQUERDO", PIN_XSHUT_1, ADDR_VL53_S1, VL53L0X(), false, 0 },
    { "FRONTAL",  PIN_XSHUT_2, ADDR_VL53_S2, VL53L0X(), false, 0 },
    { "DIREITO",  PIN_XSHUT_3, ADDR_VL53_S3, VL53L0X(), false, 0 },
};

static bool responde(uint8_t endereco) {
    Wire.beginTransmission(endereco);
    return Wire.endTransmission() == 0;
}

static void executar_sequencia(void) {
    teste_tof_todos_em_reset();
    delay(15);
    Serial.println();
    Serial.println("  Passo 0: todos os XSHUT em LOW");
    teste_varrer_i2c();
    if (responde(0x29)) {
        Serial.println("  FALHA no passo 0: 0x29 responde com todos desligados. Algum sensor");
        Serial.println("  nao obedece o XSHUT (fio solto, GPIO errado ou XSHUT sem ligar).");
    }

    for (uint8_t i = 0; i < 3; i++) {
        CanalTeste& c = s_canais[i];
        c.pronto = false;
        c.timeouts = 0;

        Serial.printf("\n  Passo %u: liga %s (XSHUT GPIO%u)\n", i + 1, c.nome, c.xshut);
        digitalWrite(c.xshut, HIGH);
        delay(15);
        teste_varrer_i2c();
        if (!responde(0x29)) {
            Serial.printf("  FALHA: %s nao apareceu em 0x29 depois de ligar o XSHUT.\n", c.nome);
            continue;
        }

        // Objeto novo a cada sequência, endereço de fábrica.
        c.sensor = VL53L0X();
        c.sensor.setBus(&Wire);
        c.sensor.setTimeout(500);
        if (!c.sensor.init()) {
            Serial.printf("  FALHA: init() do %s em 0x29.\n", c.nome);
            continue;
        }
        c.sensor.setAddress(c.endereco);
        delay(2);
        Serial.printf("  Endereco trocado para 0x%02X:\n", c.endereco);
        teste_varrer_i2c();
        if (!responde(c.endereco) || responde(0x29)) {
            Serial.printf("  FALHA: depois da troca, 0x%02X %s e 0x29 %s.\n",
                          c.endereco, responde(c.endereco) ? "responde" : "NAO responde",
                          responde(0x29) ? "AINDA responde (outro sensor ligado junto?)" : "sumiu");
            continue;
        }
        c.sensor.setMeasurementTimingBudget(33000);
        c.sensor.startContinuous();
        c.pronto = true;
        Serial.printf("  %s OK em 0x%02X\n", c.nome, c.endereco);
    }

    Serial.println("\n  Resumo da inicializacao:");
    for (uint8_t i = 0; i < 3; i++) {
        Serial.printf("    %-8s XSHUT GPIO%-2u 0x%02X -> %s\n", s_canais[i].nome,
                      s_canais[i].xshut, s_canais[i].endereco,
                      s_canais[i].pronto ? "OK" : "FALHA");
    }
    Serial.println("-------------------------------------------------------");
}

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho("3 SENSORES ToF JUNTOS (sequencia de enderecamento)",
        "  Rode depois dos testes isolados de cada ToF.\n"
        "  Mostra em qual passo a inicializacao conjunta falha.\n"
        "  Digite 'r' para refazer a sequencia.");

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);
    executar_sequencia();
}

void loop() {
    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'r' || c == 'R') executar_sequencia();
    }

    static uint32_t ultimo_ms = 0;
    if (millis() - ultimo_ms < 150) return;
    ultimo_ms = millis();

    for (uint8_t i = 0; i < 3; i++) {
        CanalTeste& c = s_canais[i];
        if (!c.pronto) {
            Serial.printf("%s=  ---- ", c.nome);
            continue;
        }
        uint16_t mm = c.sensor.readRangeContinuousMillimeters();
        bool timeout = c.sensor.timeoutOccurred();
        if (timeout) c.timeouts++;
        Serial.printf("%s=%5u%s ", c.nome, mm, timeout ? "T" : " ");
    }
    Serial.printf("| timeouts E/F/D = %lu/%lu/%lu\n",
                  (unsigned long)s_canais[0].timeouts, (unsigned long)s_canais[1].timeouts,
                  (unsigned long)s_canais[2].timeouts);
}
