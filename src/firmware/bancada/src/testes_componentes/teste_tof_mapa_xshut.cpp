/**
 * MAPA DOS XSHUT: DESCOBRE EM QUAL GPIO ESTÁ O XSHUT DE CADA VL53L0X.
 *
 * Ambiente: env:teste_tof_mapa_xshut
 *
 * Use quando os testes isolados (teste_tof_*) mostram 0x29 respondendo mesmo com
 * os XSHUT do pinout.h em LOW, isto é, a placa não segue o pinout.h ou algum
 * XSHUT não está ligado em pino nenhum.
 *
 *  [A] Põe todos os GPIO candidatos em LOW. Quem ainda responde em 0x29 está
 *      SEMPRE LIGADO (XSHUT sem controle); esses são movidos juntos para 0x40,
 *      liberando o 0x29 para o passo seguinte.
 *  [B] Liga um GPIO por vez (os outros em LOW): se 0x29 aparecer, o XSHUT de um
 *      sensor está naquele GPIO.
 *  [C] Resumo comparado com o pinout.h (GPIO 4, 16 e 17).
 *  [D] Mede os sensores encontrados ao mesmo tempo, cada um num endereço; ponha
 *      a mão em um sensor por vez para saber qual GPIO é qual sensor.
 *
 * Segurança: no passo [B] só um pino fica em HIGH por vez, então a ponte H nunca
 * recebe STBY + direção + PWM juntos e os motores não giram. No passo [D] pinos
 * que também são da ponte H não ficam em HIGH.
 * Digite 'r' para reiniciar o teste.
 */

#include "comum_testes.h"
#include <VL53L0X.h>

// Pinos de saída livres da ESP32 (sem I2C 21/22, UART 1/3, flash 6-11, entradas
// 34-39, GPIO 0 de boot, GPIO 2 do encoder e GPIO 15 do botão).
static const uint8_t CANDIDATOS[] = { 4, 5, 12, 13, 14, 16, 17, 18, 19, 23, 25, 26, 27, 32, 33 };
static const uint8_t N_CAND = sizeof(CANDIDATOS);
static const uint8_t PINOS_PONTE_H[] = { PIN_STBY, PIN_AIN1, PIN_AIN2, PIN_PWMA,
                                         PIN_BIN1, PIN_BIN2, PIN_PWMB };
static const uint8_t ENDERECO_FABRICA = 0x29;
static const uint8_t ENDERECO_GRUPO = 0x40;
static const uint8_t MAX_ACHADOS = 6;

static uint8_t s_achados[MAX_ACHADOS];
static uint8_t s_n_achados = 0;
static VL53L0X s_sensores[MAX_ACHADOS];
static bool s_medindo[MAX_ACHADOS];
static VL53L0X s_grupo;
static bool s_tem_grupo = false;
static bool s_grupo_medindo = false;

static bool responde(uint8_t endereco) {
    Wire.beginTransmission(endereco);
    return Wire.endTransmission() == 0;
}

static bool eh_pino_ponte_h(uint8_t pino) {
    for (uint8_t i = 0; i < sizeof(PINOS_PONTE_H); i++) {
        if (PINOS_PONTE_H[i] == pino) return true;
    }
    return false;
}

static void candidatos_em_low(void) {
    for (uint8_t i = 0; i < N_CAND; i++) {
        pinMode(CANDIDATOS[i], OUTPUT);
        digitalWrite(CANDIDATOS[i], LOW);
    }
}

static void passo_a(void) {
    Serial.println("\n[A] Todos os GPIO candidatos em LOW:");
    teste_varrer_i2c();
    s_grupo = VL53L0X();
    s_grupo.setBus(&Wire);
    if (responde(ENDERECO_FABRICA)) {
        s_tem_grupo = true;
        Serial.println("    0x29 responde com tudo em LOW: ha sensor(es) SEMPRE LIGADO(S),");
        Serial.println("    com o XSHUT sem controle (solto, sem ligacao ou preso em 3,3 V).");
        s_grupo.setAddress(ENDERECO_GRUPO);  // todos os sempre ligados vão juntos
        delay(5);
        Serial.print("    Movidos para 0x40.");
        teste_varrer_i2c();
        if (responde(ENDERECO_FABRICA)) {
            Serial.println("    ATENCAO: 0x29 ainda responde; o passo [B] fica pouco confiavel.");
        }
    } else if (responde(ENDERECO_GRUPO)) {
        // Sobrou de uma rodada anterior sem desligar a energia dos sensores.
        s_tem_grupo = true;
        s_grupo.setAddress(ENDERECO_GRUPO);  // ninguém em 0x29: só aponta o objeto
        Serial.println("    Sensor(es) sempre ligado(s) ja estavam em 0x40 (rodada anterior).");
    } else {
        Serial.println("    Nenhum ToF responde: todos obedecem algum XSHUT. Bom sinal.");
    }
}

static void passo_b(void) {
    Serial.println("\n[B] Ligando um GPIO por vez (os outros em LOW):");
    s_n_achados = 0;
    for (uint8_t i = 0; i < N_CAND; i++) {
        const uint8_t pino = CANDIDATOS[i];
        digitalWrite(pino, HIGH);
        delay(15);
        const bool apareceu = responde(ENDERECO_FABRICA);
        digitalWrite(pino, LOW);
        delay(15);
        Serial.printf("    GPIO %2u -> %s\n", pino,
                      apareceu ? "0x29 APARECEU: XSHUT de um sensor esta aqui" : "nada");
        if (apareceu && s_n_achados < MAX_ACHADOS) s_achados[s_n_achados++] = pino;
    }
}

static void passo_c(void) {
    Serial.println("\n[C] Resumo:");
    Serial.printf("    Sensores com XSHUT controlado: %u ->", s_n_achados);
    for (uint8_t i = 0; i < s_n_achados; i++) Serial.printf(" GPIO %u", s_achados[i]);
    Serial.println();
    Serial.printf("    pinout.h espera: GPIO %u (esq), GPIO %u (frontal), GPIO %u (dir)\n",
                  PIN_XSHUT_1, PIN_XSHUT_2, PIN_XSHUT_3);
    if (s_tem_grupo) {
        const int sem_controle = 3 - (int)s_n_achados;
        Serial.printf("    Sempre ligados (sem controle de XSHUT): %d (contando 3 sensores no robo)\n",
                      sem_controle > 0 ? sem_controle : 1);
    } else {
        Serial.println("    Nenhum sensor sempre ligado.");
    }
}

static void passo_d(void) {
    Serial.println("\n[D] Preparando a medicao de cada sensor encontrado:");
    for (uint8_t i = 0; i < s_n_achados; i++) {
        const uint8_t pino = s_achados[i];
        s_medindo[i] = false;
        if (eh_pino_ponte_h(pino)) {
            Serial.printf("    GPIO %u tambem e pino da ponte H: nao fica em HIGH, sem medicao.\n", pino);
            continue;
        }
        digitalWrite(pino, HIGH);
        delay(15);
        s_sensores[i] = VL53L0X();
        s_sensores[i].setBus(&Wire);
        s_sensores[i].setTimeout(300);
        if (s_sensores[i].init()) {
            s_sensores[i].setAddress(0x30 + i);
            s_sensores[i].setMeasurementTimingBudget(33000);
            s_sensores[i].startContinuous();
            s_medindo[i] = true;
            Serial.printf("    GPIO %u: init OK, endereco 0x%02X\n", pino, 0x30 + i);
        } else {
            Serial.printf("    GPIO %u: init FALHOU\n", pino);
            digitalWrite(pino, LOW);
        }
    }
    if (s_tem_grupo) {
        s_grupo.setTimeout(300);
        s_grupo_medindo = s_grupo.init();
        if (s_grupo_medindo) {
            s_grupo.setMeasurementTimingBudget(33000);
            s_grupo.startContinuous();
        }
        Serial.printf("    Sempre ligado(s) em 0x40: init %s\n",
                      s_grupo_medindo ? "OK (provavelmente um so sensor)"
                                      : "FALHOU (provavelmente 2 ou mais no mesmo endereco)");
    }
    Serial.println("\n    Ponha a mao a ~5 cm de UM sensor por vez: a coluna que cair e a dele.");
    Serial.println("-------------------------------------------------------");
}

static void executar(void) {
    candidatos_em_low();
    delay(20);
    passo_a();
    passo_b();
    passo_c();
    passo_d();
}

void setup() {
    candidatos_em_low();  // inclui a ponte H: motores parados
    teste_cabecalho("MAPA DOS XSHUT DOS SENSORES ToF",
        "  Descobre em qual GPIO esta o XSHUT de cada VL53L0X e quais sensores\n"
        "  ficam sempre ligados. Os motores nao giram. Digite 'r' para refazer.");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);
    executar();
}

void loop() {
    if (Serial.available()) {
        const char c = (char)Serial.read();
        if (c == 'r' || c == 'R') ESP.restart();
    }

    static uint32_t ultimo_ms = 0;
    if (millis() - ultimo_ms < 400) return;
    ultimo_ms = millis();

    bool algum = false;
    for (uint8_t i = 0; i < s_n_achados; i++) {
        if (!s_medindo[i]) continue;
        const uint16_t mm = s_sensores[i].readRangeContinuousMillimeters();
        const bool t = s_sensores[i].timeoutOccurred();
        Serial.printf("GPIO%u: %s%5u mm | ", s_achados[i], t ? "TIMEOUT " : "", t ? 0 : mm);
        algum = true;
    }
    if (s_grupo_medindo) {
        const uint16_t mm = s_grupo.readRangeContinuousMillimeters();
        const bool t = s_grupo.timeoutOccurred();
        Serial.printf("sempre ligado(0x40): %s%5u mm", t ? "TIMEOUT " : "", t ? 0 : mm);
        algum = true;
    }
    if (algum) Serial.println();
}
