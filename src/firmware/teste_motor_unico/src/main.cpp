#include <Arduino.h>
#include <esp_arduino_version.h>
#include "PinosRobo.h"
#include "CalibracaoRobo.h"

// Firmware isolado para testar um motor de cada vez (TB6612FNG + encoders),
// sem interferencia de ToF, ESP-NOW ou controle em malha fechada.

static const uint32_t PWM_FREQ_HZ = 20000;
static const uint8_t PWM_BITS = 8;
static const uint8_t CANAL_PWM_ESQ = 0;
static const uint8_t CANAL_PWM_DIR = 1;
static const uint32_t DURACAO_PULSO_MS = 2000;

static volatile int32_t gPulsosEsq = 0;
static volatile int32_t gPulsosDir = 0;

static void IRAM_ATTR isrEncEsq() {
    const int b = digitalRead(PINO_ENCODER_ESQ_B);
    int delta = (b == LOW) ? 1 : -1;
    if (CAL_ENCODER_ESQ_INVERTIDO) delta = -delta;
    gPulsosEsq += delta;
}

static void IRAM_ATTR isrEncDir() {
    const int b = digitalRead(PINO_ENCODER_DIR_B);
    int delta = (b == LOW) ? 1 : -1;
    if (CAL_ENCODER_DIR_INVERTIDO) delta = -delta;
    gPulsosDir += delta;
}

static void lerEncoders(int32_t& esq, int32_t& dir) {
    noInterrupts();
    esq = gPulsosEsq;
    dir = gPulsosDir;
    interrupts();
}

static void configurarPwm(uint8_t pino, uint8_t canal) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    (void)canal;
    ledcAttach(pino, PWM_FREQ_HZ, PWM_BITS);
#else
    ledcSetup(canal, PWM_FREQ_HZ, PWM_BITS);
    ledcAttachPin(pino, canal);
#endif
}

static void escreverPwm(uint8_t pino, uint8_t canal, uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    (void)canal;
    ledcWrite(pino, duty);
#else
    (void)pino;
    ledcWrite(canal, duty);
#endif
}

static void aplicarRoda(uint8_t in1, uint8_t in2, uint8_t pinoPwm, uint8_t canal,
                        int8_t dir, uint8_t duty, bool invertido) {
    if (dir == 0 || duty == 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        escreverPwm(pinoPwm, canal, 0);
        return;
    }
    int8_t sentido = invertido ? (int8_t)-dir : dir;
    digitalWrite(in1, sentido > 0 ? HIGH : LOW);
    digitalWrite(in2, sentido < 0 ? HIGH : LOW);
    escreverPwm(pinoPwm, canal, duty);
}

static void frearTudo() {
    digitalWrite(PINO_PONTE_AIN1, HIGH);
    digitalWrite(PINO_PONTE_AIN2, HIGH);
    escreverPwm(PINO_PONTE_PWMA, CANAL_PWM_ESQ, 255);
    digitalWrite(PINO_PONTE_BIN1, HIGH);
    digitalWrite(PINO_PONTE_BIN2, HIGH);
    escreverPwm(PINO_PONTE_PWMB, CANAL_PWM_DIR, 255);
}

static void soltarTudo() {
    aplicarRoda(PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_PWMA, CANAL_PWM_ESQ, 0, 0, false);
    aplicarRoda(PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB, CANAL_PWM_DIR, 0, 0, false);
}

static int16_t sPwmAtual = 180;
static bool sRodando = false;
static uint32_t sInicioMs = 0;
static int32_t sEncIniEsq = 0;
static int32_t sEncIniDir = 0;

static void imprimirMenu() {
    Serial.println("\n=======================================================");
    Serial.println("   TESTE ISOLADO DE MOTOR UNICO (TB6612FNG + N20)      ");
    Serial.println("=======================================================");
    Serial.printf ("  PWM atual: %d/255 | InvE=%d InvD=%d | STBY=%d\n",
                   (int)sPwmAtual,
                   CAL_MOTOR_ESQ_INVERTIDO ? 1 : 0,
                   CAL_MOTOR_DIR_INVERTIDO ? 1 : 0,
                   digitalRead(PINO_PONTE_STBY));
    Serial.println("  [1] ou [e] - SOMENTE Motor Esquerdo FRENTE (2s)");
    Serial.println("  [2] ou [d] - SOMENTE Motor Direito  FRENTE (2s)");
    Serial.println("  [3] ou [a] - AMBOS os motores FRENTE (2s)");
    Serial.println("  [4] ou [t] - AMBOS os motores TRAS   (2s)");
    Serial.println("  [5]        - SOMENTE Motor Esquerdo TRAS   (2s)");
    Serial.println("  [6]        - SOMENTE Motor Direito  TRAS   (2s)");
    Serial.println("  [+] / [-]  - Ajustar potencia PWM (+/- 20)");
    Serial.println("  [s]        - Parar / freio imediato");
    Serial.println("  [m]        - Reexibir este menu");
    Serial.println("=======================================================");
}

static void dispararTeste(int8_t dirEsq, int8_t dirDir, const char* rotulo) {
    soltarTudo();
    digitalWrite(PINO_PONTE_STBY, HIGH);
    lerEncoders(sEncIniEsq, sEncIniDir);
    sInicioMs = millis();
    sRodando = true;

    const uint8_t duty = (uint8_t)sPwmAtual;
    aplicarRoda(PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_PWMA, CANAL_PWM_ESQ,
                dirEsq, duty, CAL_MOTOR_ESQ_INVERTIDO);
    aplicarRoda(PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB, CANAL_PWM_DIR,
                dirDir, duty, CAL_MOTOR_DIR_INVERTIDO);

    Serial.printf("\n[RUN] %s | PWM=%u/255\n", rotulo, (unsigned)duty);
    Serial.printf("  Pinos ESQ: AIN1(%u)=%d AIN2(%u)=%d PWMA(%u)=%u\n",
                  PINO_PONTE_AIN1, digitalRead(PINO_PONTE_AIN1),
                  PINO_PONTE_AIN2, digitalRead(PINO_PONTE_AIN2),
                  PINO_PONTE_PWMA, (dirEsq != 0) ? (unsigned)duty : 0u);
    Serial.printf("  Pinos DIR: BIN1(%u)=%d BIN2(%u)=%d PWMB(%u)=%u | STBY(%u)=%d\n",
                  PINO_PONTE_BIN1, digitalRead(PINO_PONTE_BIN1),
                  PINO_PONTE_BIN2, digitalRead(PINO_PONTE_BIN2),
                  PINO_PONTE_PWMB, (dirDir != 0) ? (unsigned)duty : 0u,
                  PINO_PONTE_STBY, digitalRead(PINO_PONTE_STBY));
}

static void tratarComando(char c) {
    switch (c) {
        case '\r':
        case '\n':
            return;
        case '1':
        case 'e':
        case 'E':
            dispararTeste(1, 0, "SOMENTE Motor Esquerdo FRENTE");
            break;
        case '2':
        case 'd':
        case 'D':
            dispararTeste(0, 1, "SOMENTE Motor Direito FRENTE");
            break;
        case '3':
        case 'a':
        case 'A':
            dispararTeste(1, 1, "AMBOS os motores FRENTE");
            break;
        case '4':
        case 't':
        case 'T':
            dispararTeste(-1, -1, "AMBOS os motores TRAS");
            break;
        case '5':
            dispararTeste(-1, 0, "SOMENTE Motor Esquerdo TRAS");
            break;
        case '6':
            dispararTeste(0, -1, "SOMENTE Motor Direito TRAS");
            break;
        case '+':
            sPwmAtual = (int16_t)min(255, (int)sPwmAtual + 20);
            Serial.printf("[PWM] Ajustado para %d/255\n", (int)sPwmAtual);
            break;
        case '-':
            sPwmAtual = (int16_t)max(40, (int)sPwmAtual - 20);
            Serial.printf("[PWM] Ajustado para %d/255\n", (int)sPwmAtual);
            break;
        case 's':
        case 'S':
        case ' ':
            frearTudo();
            delay(50);
            soltarTudo();
            sRodando = false;
            Serial.println("[PARADO] Motores freados e soltos.");
            break;
        case 'm':
        case 'M':
            imprimirMenu();
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(400);

    // Mantem ToF em reset para isolar o barramento e economizar corrente.
    const uint8_t xshuts[] = { PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR };
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(xshuts[i], OUTPUT);
        digitalWrite(xshuts[i], LOW);
    }

    pinMode(PINO_PONTE_AIN1, OUTPUT);
    pinMode(PINO_PONTE_AIN2, OUTPUT);
    pinMode(PINO_PONTE_BIN1, OUTPUT);
    pinMode(PINO_PONTE_BIN2, OUTPUT);
    pinMode(PINO_PONTE_STBY, OUTPUT);

    configurarPwm(PINO_PONTE_PWMA, CANAL_PWM_ESQ);
    configurarPwm(PINO_PONTE_PWMB, CANAL_PWM_DIR);
    soltarTudo();
    digitalWrite(PINO_PONTE_STBY, HIGH);

    pinMode(PINO_ENCODER_ESQ_A, INPUT);
    pinMode(PINO_ENCODER_ESQ_B, INPUT);
    pinMode(PINO_ENCODER_DIR_A, INPUT);
    pinMode(PINO_ENCODER_DIR_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_ESQ_A), isrEncEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_DIR_A), isrEncDir, RISING);

    imprimirMenu();
}

void loop() {
    while (Serial.available()) {
        tratarComando((char)Serial.read());
    }

    if (sRodando && (millis() - sInicioMs) >= DURACAO_PULSO_MS) {
        frearTudo();
        delay(50);
        soltarTudo();
        sRodando = false;
        int32_t esq = 0, dir = 0;
        lerEncoders(esq, dir);
        Serial.printf("[FIM] Pulsos em 2s -> dE=%ld | dD=%ld\n",
                      (long)(esq - sEncIniEsq), (long)(dir - sEncIniDir));
    }
}
