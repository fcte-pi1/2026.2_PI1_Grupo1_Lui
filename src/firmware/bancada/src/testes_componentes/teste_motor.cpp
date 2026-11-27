/**
 * TESTE DE UM MOTOR (TB6612FNG + N20), ISOLADO.
 *
 * Ambientes:
 *   env:teste_motor_esquerdo  -> -DTESTE_MOTOR_LADO=0 (AIN1, AIN2, PWMA)
 *   env:teste_motor_direito   -> -DTESTE_MOTOR_LADO=1 (BIN1, BIN2, PWMB)
 *
 * Com o robô erguido (rodas no ar), o motor escolhido repete o ciclo:
 *   frente 2 s -> parado 1 s -> ré 2 s -> freio 1 s.
 * O outro motor fica com as entradas em LOW e os sensores ToF em reset.
 * Digite 'p' no monitor serial para pausar/retomar.
 */

#include "comum_testes.h"

#ifndef TESTE_MOTOR_LADO
#error "Defina TESTE_MOTOR_LADO (0 = esquerdo, 1 = direito) no ambiente do platformio.ini"
#endif

#if TESTE_MOTOR_LADO == 0
static const char* NOME = "MOTOR ESQUERDO (Motor 1 / canal A da ponte H)";
static const uint8_t PINO_IN1 = PIN_AIN1;
static const uint8_t PINO_IN2 = PIN_AIN2;
static const uint8_t PINO_PWM = PIN_PWMA;
static const uint8_t CANAL_PWM = LEDC_CH_PWMA;
static const bool INVERTIDO = MOTOR_ESQ_INVERTIDO;
#else
static const char* NOME = "MOTOR DIREITO (Motor 2 / canal B da ponte H)";
static const uint8_t PINO_IN1 = PIN_BIN1;
static const uint8_t PINO_IN2 = PIN_BIN2;
static const uint8_t PINO_PWM = PIN_PWMB;
static const uint8_t CANAL_PWM = LEDC_CH_PWMB;
static const bool INVERTIDO = MOTOR_DIR_INVERTIDO;
#endif

static const uint8_t PWM_TESTE = (uint8_t)PWM_BASE_MOVIMENTO;

enum Fase { FRENTE, PARADO, RE, FREIO };
static Fase s_fase = FRENTE;
static uint32_t s_inicio_fase_ms = 0;
static bool s_pausado = false;

static void aplicar(int sentido, uint8_t duty, bool freio) {
    if (freio) {
        digitalWrite(PINO_IN1, HIGH);
        digitalWrite(PINO_IN2, HIGH);
        teste_pwm_escrever(PINO_PWM, CANAL_PWM, 255);
        return;
    }
    if (INVERTIDO) sentido = -sentido;
    digitalWrite(PINO_IN1, sentido > 0 ? HIGH : LOW);
    digitalWrite(PINO_IN2, sentido < 0 ? HIGH : LOW);
    teste_pwm_escrever(PINO_PWM, CANAL_PWM, sentido == 0 ? 0 : duty);
}

static void entrar_fase(Fase f) {
    s_fase = f;
    s_inicio_fase_ms = millis();
    switch (f) {
        case FRENTE: aplicar(+1, PWM_TESTE, false); break;
        case PARADO: aplicar(0, 0, false); break;
        case RE:     aplicar(-1, PWM_TESTE, false); break;
        case FREIO:  aplicar(0, 0, true); break;
    }
    const char* nomes[] = { "FRENTE", "PARADO", "RE", "FREIO" };
    Serial.printf("[%7lu ms] %-6s  IN1(GPIO%u)=%d IN2(GPIO%u)=%d PWM(GPIO%u)=%u STBY(GPIO%u)=%d\n",
                  (unsigned long)millis(), nomes[f],
                  PINO_IN1, digitalRead(PINO_IN1), PINO_IN2, digitalRead(PINO_IN2),
                  PINO_PWM, (f == FRENTE || f == RE) ? PWM_TESTE : (f == FREIO ? 255 : 0),
                  PIN_STBY, digitalRead(PIN_STBY));
}

void setup() {
    teste_motores_em_seguranca();
    teste_tof_todos_em_reset();
    teste_cabecalho(NOME,
        "  Erga o robo: so este motor deve girar.\n"
        "  Ciclo: frente 2 s, parado 1 s, re 2 s, freio 1 s.\n"
        "  Se 'FRENTE' girar para tras, troque o sentido em CalibracaoRobo.h.\n"
        "  Digite 'p' para pausar/retomar.");

    teste_pwm_configurar(PINO_PWM, CANAL_PWM);
    teste_pwm_escrever(PINO_PWM, CANAL_PWM, 0);
    digitalWrite(PIN_STBY, HIGH);  // habilita a ponte H só depois de tudo em LOW
    Serial.printf("  PWM de teste: %u/255 a %u Hz | invertido no CalibracaoRobo: %s\n",
                  PWM_TESTE, (unsigned)PWM_FREQ_HZ, INVERTIDO ? "sim" : "nao");
    entrar_fase(FRENTE);
}

void loop() {
    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'p' || c == 'P') {
            s_pausado = !s_pausado;
            if (s_pausado) {
                aplicar(0, 0, false);
                Serial.println("  >> PAUSADO (motor parado). 'p' retoma.");
            } else {
                Serial.println("  >> RETOMADO.");
                entrar_fase(FRENTE);
            }
        }
    }
    if (s_pausado) return;

    const uint32_t duracao[] = { 2000, 1000, 2000, 1000 };
    if (millis() - s_inicio_fase_ms >= duracao[s_fase]) {
        entrar_fase((Fase)((s_fase + 1) % 4));
    }
}
