/**
 * TESTE DE UM SENSOR VL53L0X (ToF), ISOLADO NO BARRAMENTO I2C.
 *
 * Ambientes:
 *   env:teste_tof_esquerdo -> -DTESTE_TOF_ALVO=1 (PIN_XSHUT_1)
 *   env:teste_tof_frontal  -> -DTESTE_TOF_ALVO=2 (PIN_XSHUT_2)
 *   env:teste_tof_direito  -> -DTESTE_TOF_ALVO=3 (PIN_XSHUT_3)
 *
 * Os outros dois sensores ficam com XSHUT em LOW (desligados), então o teste
 * funciona com os três ligados na placa. O sensor testado fica no endereço de
 * fábrica 0x29, sem troca de endereço, para separar defeito do sensor/fiação
 * de defeito na sequência de endereçamento (essa fica no teste_tof_sequencia).
 */

#include "comum_testes.h"
#include <VL53L0X.h>

#ifndef TESTE_TOF_ALVO
#error "Defina TESTE_TOF_ALVO (1 = esquerdo, 2 = frontal, 3 = direito) no ambiente do platformio.ini"
#endif

#if TESTE_TOF_ALVO == 1
static const char* NOME = "SENSOR ToF ESQUERDO (VL53L0X #1)";
static const uint8_t PINO_XSHUT = PIN_XSHUT_1;
#elif TESTE_TOF_ALVO == 2
static const char* NOME = "SENSOR ToF FRONTAL (VL53L0X #2)";
static const uint8_t PINO_XSHUT = PIN_XSHUT_2;
#else
static const char* NOME = "SENSOR ToF DIREITO (VL53L0X #3)";
static const uint8_t PINO_XSHUT = PIN_XSHUT_3;
#endif

static VL53L0X s_sensor;
static bool s_pronto = false;
static uint32_t s_leituras = 0;
static uint32_t s_timeouts = 0;

static bool tentar_iniciar(void) {
    // Ciclo de reset do sensor alvo, com os outros sempre em LOW.
    teste_tof_todos_em_reset();
    delay(15);
    Serial.println("  [1/3] Todos os XSHUT em LOW (nenhum ToF deveria responder):");
    if (teste_varrer_i2c() > 0) {
        Serial.println("        ATENCAO: algo respondeu com os ToF desligados. Se for 0x29, ha um");
        Serial.println("        sensor cujo XSHUT nao esta ligado no GPIO esperado (ou esta solto).");
    }

    digitalWrite(PINO_XSHUT, HIGH);
    delay(15);
    Serial.printf("  [2/3] XSHUT GPIO%u em HIGH (so 0x29 deveria responder):\n", PINO_XSHUT);
    teste_varrer_i2c();

    s_sensor.setBus(&Wire);  // o objeto nunca troca de endereço: fica em 0x29
    s_sensor.setTimeout(500);
    Serial.print("  [3/3] init() do VL53L0X em 0x29... ");
    if (!s_sensor.init()) {
        Serial.println("FALHA (sensor nao respondeu ao init)");
        return false;
    }
    s_sensor.setMeasurementTimingBudget(33000);
    s_sensor.startContinuous();
    Serial.println("OK, medindo em modo continuo");
    return true;
}

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho(NOME,
        "  Os outros ToF ficam desligados por XSHUT. Aproxime e afaste a mao\n"
        "  do sensor: a distancia deve variar sem 'TIMEOUT'.\n"
        "  Digite 'r' para refazer a inicializacao.");

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);
    Serial.printf("  I2C: SDA=GPIO%u SCL=GPIO%u %lu Hz | XSHUT alvo=GPIO%u\n",
                  PIN_I2C_SDA, PIN_I2C_SCL, (unsigned long)I2C_CLOCK_SPEED, PINO_XSHUT);
    s_pronto = tentar_iniciar();
}

void loop() {
    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'r' || c == 'R') {
            s_pronto = tentar_iniciar();
            s_leituras = 0;
            s_timeouts = 0;
        }
    }

    if (!s_pronto) {
        static uint32_t ultima_tentativa = 0;
        if (millis() - ultima_tentativa > 3000) {
            ultima_tentativa = millis();
            Serial.println("  Nova tentativa de inicializacao...");
            s_pronto = tentar_iniciar();
        }
        return;
    }

    static uint32_t ultimo_ms = 0;
    if (millis() - ultimo_ms < 100) return;
    ultimo_ms = millis();

    uint16_t mm = s_sensor.readRangeContinuousMillimeters();
    bool timeout = s_sensor.timeoutOccurred();
    s_leituras++;
    if (timeout) s_timeouts++;
    Serial.printf("dist=%5u mm %s | leituras=%lu timeouts=%lu\n",
                  mm, timeout ? "TIMEOUT" : (mm >= 8000 ? "(fora de alcance)" : ""),
                  (unsigned long)s_leituras, (unsigned long)s_timeouts);
}
