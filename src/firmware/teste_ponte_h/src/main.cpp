/**
 * ==============================================================================
 * TESTE ISOLADO: PONTE H TB6612FNG + 2 MOTORES N20 + 2 ENCODERS + ESP-NOW
 * Micromouse PI1 2026.2 (ESP32 DevKit V1 + Gateway ESP8266)
 * ==============================================================================
 * Pinagem validada a partir de PinosRobo.h:
 *   Ponte H: PWMA=25, AIN1=26, AIN2=27, STBY=13, BIN1=18, BIN2=19, PWMB=23
 *   Encoders: EEA=34, EEB=35 (Esquerdo) | EDA=32, EDB=33 (Direito)
 *
 * Aceita comandos tanto pela Serial USB da ESP32 (115200 bps) quanto sem fio
 * via ESP-NOW (Canal 1) atraves do Gateway ESP8266. Compativel com o script
 * scripts/teste_motores_remoto.py e com envio direto pela serial.
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "CalibracaoRobo.h"
#include "PinosRobo.h"
#include "TelemetriaCodec.h"
#include "TelemetriaConfig.h"
#include "TelemetriaPacket.h"

// Conferencia em tempo de compilacao da pinagem solicitada
static_assert(PINO_PONTE_PWMA == 25, "PWMA deve ser GPIO 25");
static_assert(PINO_PONTE_AIN1 == 26, "AIN1 deve ser GPIO 26");
static_assert(PINO_PONTE_AIN2 == 27, "AIN2 deve ser GPIO 27");
static_assert(PINO_PONTE_BIN1 == 18, "BIN1 deve ser GPIO 18");
static_assert(PINO_PONTE_BIN2 == 19, "BIN2 deve ser GPIO 19");
static_assert(PINO_PONTE_PWMB == 23, "PWMB deve ser GPIO 23");
static_assert(PINO_ENCODER_ESQ_A == 34, "EEA deve ser GPIO 34");
static_assert(PINO_ENCODER_ESQ_B == 35, "EEB deve ser GPIO 35");
static_assert(PINO_ENCODER_DIR_A == 32, "EDA deve ser GPIO 32");
static_assert(PINO_ENCODER_DIR_B == 33, "EDB deve ser GPIO 33");

static constexpr uint32_t PWM_FREQ_HZ = 20000;
static constexpr uint8_t PWM_BITS = 8;
static constexpr uint8_t CANAL_PWM_ESQ = 0;
static constexpr uint8_t CANAL_PWM_DIR = 1;
static constexpr float CIRCUNFERENCIA_RODA_MM = 3.14159265f * CAL_RODA_DIAMETRO_MM;

// ------------------------------------------------------------------------------
// ENCODERS (QUADRATURA + CONTAGEM DE BORDAS A/B PARA DIAGNOSTICO)
// ------------------------------------------------------------------------------
struct DadosEncoders {
    int32_t ticksEsq;
    int32_t ticksDir;
    uint32_t subidasEsqA;
    uint32_t bordasEsqB;
    uint32_t subidasDirA;
    uint32_t bordasDirB;
};

static volatile int32_t gTicksEsq = 0;
static volatile int32_t gTicksDir = 0;
static volatile uint32_t gSubidasEsqA = 0;
static volatile uint32_t gBordasEsqB = 0;
static volatile uint32_t gSubidasDirA = 0;
static volatile uint32_t gBordasDirB = 0;
static portMUX_TYPE gTravaEnc = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR isrEsqA() {
    int32_t passo = digitalRead(PINO_ENCODER_ESQ_B) ? -1 : 1;
    if (CAL_ENCODER_ESQ_INVERTIDO) passo = -passo;
    portENTER_CRITICAL_ISR(&gTravaEnc);
    gTicksEsq += passo;
    gSubidasEsqA++;
    portEXIT_CRITICAL_ISR(&gTravaEnc);
}

static void IRAM_ATTR isrEsqB() {
    portENTER_CRITICAL_ISR(&gTravaEnc);
    gBordasEsqB++;
    portEXIT_CRITICAL_ISR(&gTravaEnc);
}

static void IRAM_ATTR isrDirA() {
    int32_t passo = digitalRead(PINO_ENCODER_DIR_B) ? 1 : -1;
    if (CAL_ENCODER_DIR_INVERTIDO) passo = -passo;
    portENTER_CRITICAL_ISR(&gTravaEnc);
    gTicksDir += passo;
    gSubidasDirA++;
    portEXIT_CRITICAL_ISR(&gTravaEnc);
}

static void IRAM_ATTR isrDirB() {
    portENTER_CRITICAL_ISR(&gTravaEnc);
    gBordasDirB++;
    portEXIT_CRITICAL_ISR(&gTravaEnc);
}

static DadosEncoders lerEncoders() {
    DadosEncoders d;
    portENTER_CRITICAL(&gTravaEnc);
    d.ticksEsq = gTicksEsq;
    d.ticksDir = gTicksDir;
    d.subidasEsqA = gSubidasEsqA;
    d.bordasEsqB = gBordasEsqB;
    d.subidasDirA = gSubidasDirA;
    d.bordasDirB = gBordasDirB;
    portEXIT_CRITICAL(&gTravaEnc);
    return d;
}

static void zerarEncoders() {
    portENTER_CRITICAL(&gTravaEnc);
    gTicksEsq = 0;
    gTicksDir = 0;
    gSubidasEsqA = 0;
    gBordasEsqB = 0;
    gSubidasDirA = 0;
    gBordasDirB = 0;
    portEXIT_CRITICAL(&gTravaEnc);
}

// ------------------------------------------------------------------------------
// PONTE H TB6612FNG
// ------------------------------------------------------------------------------
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
    const int8_t sentido = invertido ? (int8_t)-dir : dir;
    digitalWrite(in1, sentido > 0 ? HIGH : LOW);
    digitalWrite(in2, sentido < 0 ? HIGH : LOW);
    escreverPwm(pinoPwm, canal, duty);
}

static int16_t sPwmAtual = 140;
static int8_t sDirEsq = 0;
static int8_t sDirDir = 0;
static bool sFreioAtivo = false;
static uint32_t sInicioMovMs = 0;

static void aplicarMotores(int8_t dirEsq, int8_t dirDir) {
    digitalWrite(PINO_PONTE_STBY, HIGH);
    const bool estavaParado = (sDirEsq == 0 && sDirDir == 0);
    sDirEsq = dirEsq;
    sDirDir = dirDir;
    sFreioAtivo = false;
    if (estavaParado && (dirEsq != 0 || dirDir != 0)) {
        sInicioMovMs = millis();
    }
    const uint8_t duty = (uint8_t)sPwmAtual;
    aplicarRoda(PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_PWMA, CANAL_PWM_ESQ,
                dirEsq, duty, CAL_MOTOR_ESQ_INVERTIDO);
    aplicarRoda(PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB, CANAL_PWM_DIR,
                dirDir, duty, CAL_MOTOR_DIR_INVERTIDO);
}

static void frearTudo() {
    sDirEsq = 0;
    sDirDir = 0;
    sFreioAtivo = true;
    digitalWrite(PINO_PONTE_STBY, HIGH);
    digitalWrite(PINO_PONTE_AIN1, HIGH);
    digitalWrite(PINO_PONTE_AIN2, HIGH);
    escreverPwm(PINO_PONTE_PWMA, CANAL_PWM_ESQ, 255);
    digitalWrite(PINO_PONTE_BIN1, HIGH);
    digitalWrite(PINO_PONTE_BIN2, HIGH);
    escreverPwm(PINO_PONTE_PWMB, CANAL_PWM_DIR, 255);
}

static void soltarTudo() {
    sDirEsq = 0;
    sDirDir = 0;
    sFreioAtivo = false;
    aplicarRoda(PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_PWMA, CANAL_PWM_ESQ, 0, 0, false);
    aplicarRoda(PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB, CANAL_PWM_DIR, 0, 0, false);
}

// ------------------------------------------------------------------------------
// ESP-NOW (TELEMETRIA 1 Hz + COMANDOS REMOTOS VIA GATEWAY ESP8266)
// ------------------------------------------------------------------------------
static const uint8_t ENDERECO_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool sRadioOk = false;

static constexpr uint8_t TAM_FILA_CMD = 32;
static volatile char sFilaCmd[TAM_FILA_CMD];
static volatile uint8_t sHeadCmd = 0;
static volatile uint8_t sTailCmd = 0;

static void enfileirarComandos(const uint8_t* dados, int tamanho) {
    if (dados == nullptr || tamanho <= 0) return;
    for (int i = 0; i < tamanho; i++) {
        const char c = (char)dados[i];
        if (c == '\r' || c == '\n') continue;
        const uint8_t prox = (uint8_t)((sHeadCmd + 1) % TAM_FILA_CMD);
        if (prox != sTailCmd) {
            sFilaCmd[sHeadCmd] = c;
            sHeadCmd = prox;
        }
    }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void aoReceberEspNow(const esp_now_recv_info_t* info, const uint8_t* dados, int tamanho) {
    (void)info;
    enfileirarComandos(dados, tamanho);
}
#else
static void aoReceberEspNow(const uint8_t* mac, const uint8_t* dados, int tamanho) {
    (void)mac;
    enfileirarComandos(dados, tamanho);
}
#endif

static bool iniciarEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_wifi_set_channel(TELEMETRIA_CANAL_WIFI, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        return false;
    }
    if (esp_now_init() != ESP_OK) {
        return false;
    }
    esp_now_register_recv_cb(aoReceberEspNow);

    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, ENDERECO_BROADCAST, 6);
    peer.channel = TELEMETRIA_CANAL_WIFI;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        return false;
    }
    return true;
}

static bool lerComandoEspNow(char& cmd) {
    if (sHeadCmd == sTailCmd) return false;
    cmd = sFilaCmd[sTailCmd];
    sTailCmd = (uint8_t)((sTailCmd + 1) % TAM_FILA_CMD);
    return true;
}

static int32_t sUltimosTicksMedia = 0;
static uint32_t sUltimoCalcVelMs = 0;

static void enviarTelemetriaEspNow(bool emMovimento) {
    if (!sRadioOk) return;

    const DadosEncoders enc = lerEncoders();
    const uint32_t agora = millis();
    const int32_t mediaTicks = (enc.ticksEsq + enc.ticksDir) / 2;

    int16_t velAtual = 0;
    if (sUltimoCalcVelMs > 0 && agora > sUltimoCalcVelMs) {
        const uint32_t dt = agora - sUltimoCalcVelMs;
        const int32_t delta = mediaTicks - sUltimosTicksMedia;
        const float deltaMm = ((float)delta / (float)CAL_ENCODER_PULSOS_POR_VOLTA) * CIRCUNFERENCIA_RODA_MM;
        velAtual = (int16_t)((deltaMm / (float)dt) * 1000.0f);
    }
    sUltimosTicksMedia = mediaTicks;
    sUltimoCalcVelMs = agora;

    TelemetriaPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.labirinto_id = LAB_8X4;
    pkt.status_id = emMovimento ? (uint8_t)MAPEANDO : (uint8_t)AGUARDANDO;
    if (emMovimento) {
        const uint32_t dec = agora - sInicioMovMs;
        pkt.timestamp_ms = (dec > TELEMETRIA_TIMESTAMP_MAX_MS) ? TELEMETRIA_TIMESTAMP_MAX_MS : dec;
    } else {
        pkt.timestamp_ms = 0;
    }
    pkt.desafio_cumprido = 0;
    pkt.bateria_pct = 0;
    pkt.bateria_mv = 0;
    pkt.velocidade_atual = velAtual;
    pkt.velocidade_media = velAtual;
    pkt.coluna = 0;
    pkt.linha = 0;
    pkt.orientacao = 0;
    pkt.paredes = 0;

    if (telemetriaValidar(pkt) != TELEMETRIA_OK) return;
    uint8_t buf[TELEMETRIA_TAMANHO_BYTES];
    if (telemetriaCodificar(pkt, buf, sizeof(buf)) == TELEMETRIA_TAMANHO_BYTES) {
        esp_now_send(ENDERECO_BROADCAST, buf, TELEMETRIA_TAMANHO_BYTES);
    }
}

// ------------------------------------------------------------------------------
// SEQUENCIA AUTOMATICA COMPLETA (NAO-BLOQUEANTE)
// ------------------------------------------------------------------------------
struct PassoAuto {
    const char* nome;
    int8_t dirEsq;
    int8_t dirDir;
    bool freio;
    uint32_t duracaoMs;
    bool avaliar;
};

static const PassoAuto PASSOS_AUTO[] = {
    {"1/6 Motor ESQUERDO FRENTE",  1,  0, false, 2000, true},
    {"    Pausa",                  0,  0, false,  700, false},
    {"2/6 Motor ESQUERDO RE",     -1,  0, false, 2000, true},
    {"    Pausa",                  0,  0, false,  700, false},
    {"3/6 Motor DIREITO FRENTE",   0,  1, false, 2000, true},
    {"    Pausa",                  0,  0, false,  700, false},
    {"4/6 Motor DIREITO RE",       0, -1, false, 2000, true},
    {"    Pausa",                  0,  0, false,  700, false},
    {"5/6 AMBOS FRENTE",           1,  1, false, 2000, true},
    {"    Pausa",                  0,  0, false,  700, false},
    {"6/6 AMBOS RE",              -1, -1, false, 2000, true},
    {"    Freio final",            0,  0, true,   400, false},
};
static constexpr size_t QTD_PASSOS_AUTO = sizeof(PASSOS_AUTO) / sizeof(PASSOS_AUTO[0]);

static bool sAutoAtivo = false;
static size_t sAutoIdx = 0;
static uint32_t sAutoPassoInicioMs = 0;
static DadosEncoders sAutoEncInicio{};

static void iniciarPassoAuto(size_t idx) {
    sAutoIdx = idx;
    sAutoPassoInicioMs = millis();
    sAutoEncInicio = lerEncoders();

    const PassoAuto& p = PASSOS_AUTO[idx];
    if (p.freio) {
        frearTudo();
    } else if (p.dirEsq == 0 && p.dirDir == 0) {
        soltarTudo();
    } else {
        aplicarMotores(p.dirEsq, p.dirDir);
    }

    if (p.avaliar || p.freio) {
        Serial.printf("\n[AUTO] %s (PWM=%d, %lu ms)\n",
                      p.nome, (int)sPwmAtual, (unsigned long)p.duracaoMs);
    }
}

static void iniciarSequenciaAuto() {
    sAutoAtivo = true;
    Serial.println("\n=======================================================");
    Serial.println("  INICIANDO SEQUENCIA AUTOMATICA (ERGA AS RODAS!)");
    Serial.println("  Envie 's' ou ESPACO a qualquer momento para parar.");
    Serial.println("=======================================================");
    iniciarPassoAuto(0);
}

static void atualizarSequenciaAuto() {
    if (!sAutoAtivo) return;
    const PassoAuto& p = PASSOS_AUTO[sAutoIdx];
    if (millis() - sAutoPassoInicioMs < p.duracaoMs) return;

    if (p.avaliar) {
        const DadosEncoders fim = lerEncoders();
        const int32_t dE = fim.ticksEsq - sAutoEncInicio.ticksEsq;
        const int32_t dD = fim.ticksDir - sAutoEncInicio.ticksDir;
        const uint32_t dEA = fim.subidasEsqA - sAutoEncInicio.subidasEsqA;
        const uint32_t dEB = fim.bordasEsqB - sAutoEncInicio.bordasEsqB;
        const uint32_t dDA = fim.subidasDirA - sAutoEncInicio.subidasDirA;
        const uint32_t dDB = fim.bordasDirB - sAutoEncInicio.bordasDirB;
        Serial.printf("  -> Delta ESQ: ticks=%+ld (subA=%lu, bordB=%lu) | DIR: ticks=%+ld (subA=%lu, bordB=%lu)\n",
                      (long)dE, (unsigned long)dEA, (unsigned long)dEB,
                      (long)dD, (unsigned long)dDA, (unsigned long)dDB);
    }

    if (sAutoIdx + 1 < QTD_PASSOS_AUTO) {
        iniciarPassoAuto(sAutoIdx + 1);
    } else {
        soltarTudo();
        sAutoAtivo = false;
        Serial.println("\n[AUTO] Sequencia concluida! Motores soltos.");
        Serial.println("=======================================================\n");
    }
}

// ------------------------------------------------------------------------------
// INTERFACE SERIAL E RELATORIO
// ------------------------------------------------------------------------------
static void imprimirRelatorio() {
    const DadosEncoders enc = lerEncoders();
    Serial.println("\n--- [STATUS PONTE H + ENCODERS + ESP-NOW] ---");
    Serial.printf("  ESP-NOW Canal %d : %s | MAC: %s\n",
                  TELEMETRIA_CANAL_WIFI, sRadioOk ? "OK (1 Hz)" : "ERRO",
                  WiFi.macAddress().c_str());
    Serial.printf("  PWM atual       : %d/255 | Dir(E/D)=%d/%d | Freio=%s | Auto=%s\n",
                  (int)sPwmAtual, (int)sDirEsq, (int)sDirDir,
                  sFreioAtivo ? "SIM" : "NAO", sAutoAtivo ? "RODANDO" : "NAO");
    Serial.printf("  Ponte H ESQ     : PWMA(25) AIN1(26)=%d AIN2(27)=%d | STBY(13)=%d\n",
                  digitalRead(PINO_PONTE_AIN1), digitalRead(PINO_PONTE_AIN2),
                  digitalRead(PINO_PONTE_STBY));
    Serial.printf("  Ponte H DIR     : PWMB(23) BIN1(18)=%d BIN2(19)=%d\n",
                  digitalRead(PINO_PONTE_BIN1), digitalRead(PINO_PONTE_BIN2));
    Serial.printf("  Encoder ESQ     : ticks=%ld | subA(34)=%lu bordB(35)=%lu | nivel A=%d B=%d\n",
                  (long)enc.ticksEsq, (unsigned long)enc.subidasEsqA,
                  (unsigned long)enc.bordasEsqB,
                  digitalRead(PINO_ENCODER_ESQ_A), digitalRead(PINO_ENCODER_ESQ_B));
    Serial.printf("  Encoder DIR     : ticks=%ld | subA(32)=%lu bordB(33)=%lu | nivel A=%d B=%d\n",
                  (long)enc.ticksDir, (unsigned long)enc.subidasDirA,
                  (unsigned long)enc.bordasDirB,
                  digitalRead(PINO_ENCODER_DIR_A), digitalRead(PINO_ENCODER_DIR_B));
    Serial.println("---------------------------------------------\n");
}

static void imprimirMenu() {
    Serial.println("\n=======================================================");
    Serial.println("  TESTE SEPARADO: PONTE H + MOTORES + ENCODERS + ESP-NOW");
    Serial.println("=======================================================");
    Serial.println("  Ponte H : PWMA=25 AIN1=26 AIN2=27 | BIN1=18 BIN2=19 PWMB=23 | STBY=13");
    Serial.println("  Encoders: EEA=34 EEB=35 | EDA=32 EDB=33");
    Serial.printf ("  ESP-NOW : %s (Canal %d) | PWM atual: %d/255\n",
                   sRadioOk ? "PRONTO" : "FALHA", TELEMETRIA_CANAL_WIFI, (int)sPwmAtual);
    Serial.println("-------------------------------------------------------");
    Serial.println("  [a]     - Sequencia automatica completa (Esq, Dir, Ambos)");
    Serial.println("  [e]/[E] - Motor ESQUERDO frente / re (continuo)");
    Serial.println("  [d]/[D] - Motor DIREITO  frente / re (continuo)");
    Serial.println("  [f]/[t] - AMBOS os motores frente / re (continuo)");
    Serial.println("  [+]/[-] - Ajustar PWM (+/- 20, faixa 40..255)");
    Serial.println("  [s]/[ ] - FREIO ativo (para tudo imediatamente)");
    Serial.println("  [x]     - Parar sem freio (soltar motores)");
    Serial.println("  [r]     - Relatorio de pinos, encoders e ESP-NOW");
    Serial.println("  [z]     - Zerar contadores de encoders");
    Serial.println("  [m]     - Exibir este menu");
    Serial.println("=======================================================");
}

static void processarComando(char c, const char* origem) {
    if (c == '\r' || c == '\n') return;

    // '2' e enviado pelo scripts/teste_motores_remoto.py antes de cada comando
    if (c == '2') return;

    switch (c) {
        case 'a':
        case 'A':
            iniciarSequenciaAuto();
            break;

        case 'e':
            sAutoAtivo = false;
            if (sDirEsq != 1 || sDirDir != 0) {
                aplicarMotores(1, 0);
                Serial.printf("[%s] Motor ESQUERDO FRENTE (PWM=%d)\n", origem, (int)sPwmAtual);
            }
            break;

        case 'E':
            sAutoAtivo = false;
            if (sDirEsq != -1 || sDirDir != 0) {
                aplicarMotores(-1, 0);
                Serial.printf("[%s] Motor ESQUERDO RE (PWM=%d)\n", origem, (int)sPwmAtual);
            }
            break;

        case 'd':
            sAutoAtivo = false;
            if (sDirEsq != 0 || sDirDir != 1) {
                aplicarMotores(0, 1);
                Serial.printf("[%s] Motor DIREITO FRENTE (PWM=%d)\n", origem, (int)sPwmAtual);
            }
            break;

        case 'D':
            sAutoAtivo = false;
            if (sDirEsq != 0 || sDirDir != -1) {
                aplicarMotores(0, -1);
                Serial.printf("[%s] Motor DIREITO RE (PWM=%d)\n", origem, (int)sPwmAtual);
            }
            break;

        case 'f':
        case 'F':
            sAutoAtivo = false;
            if (sDirEsq != 1 || sDirDir != 1) {
                aplicarMotores(1, 1);
                Serial.printf("[%s] AMBOS FRENTE (PWM=%d)\n", origem, (int)sPwmAtual);
            }
            break;

        case 't':
        case 'T':
            sAutoAtivo = false;
            if (sDirEsq != -1 || sDirDir != -1) {
                aplicarMotores(-1, -1);
                Serial.printf("[%s] AMBOS RE (PWM=%d)\n", origem, (int)sPwmAtual);
            }
            break;

        case '+':
        case '=':
            sPwmAtual = (int16_t)constrain((int)sPwmAtual + 20, 40, 255);
            if (sDirEsq != 0 || sDirDir != 0) aplicarMotores(sDirEsq, sDirDir);
            Serial.printf("[%s] PWM=%d/255\n", origem, (int)sPwmAtual);
            break;

        case '-':
            sPwmAtual = (int16_t)constrain((int)sPwmAtual - 20, 40, 255);
            if (sDirEsq != 0 || sDirDir != 0) aplicarMotores(sDirEsq, sDirDir);
            Serial.printf("[%s] PWM=%d/255\n", origem, (int)sPwmAtual);
            break;

        case 's':
        case 'S':
        case ' ':
            if (sAutoAtivo || sDirEsq != 0 || sDirDir != 0 || !sFreioAtivo) {
                sAutoAtivo = false;
                frearTudo();
                const DadosEncoders enc = lerEncoders();
                Serial.printf("[%s] FREIO! Ticks: ESQ=%ld | DIR=%ld\n",
                              origem, (long)enc.ticksEsq, (long)enc.ticksDir);
            }
            break;

        case 'x':
        case 'X':
            if (sAutoAtivo || sDirEsq != 0 || sDirDir != 0 || sFreioAtivo) {
                sAutoAtivo = false;
                soltarTudo();
                Serial.printf("[%s] Motores soltos (parados sem freio).\n", origem);
            }
            break;

        case 'r':
        case 'R':
            imprimirRelatorio();
            break;

        case 'z':
        case 'Z':
            zerarEncoders();
            Serial.printf("[%s] Encoders zerados.\n", origem);
            break;

        case 'm':
        case 'M':
        case 'h':
        case 'H':
            imprimirMenu();
            break;

        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(400);

    // Mantem sensores ToF em reset caso estejam na placa
    const uint8_t xshuts[] = {PINO_TOF_XSHUT_ESQ, PINO_TOF_XSHUT_FRONTAL, PINO_TOF_XSHUT_DIR};
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(xshuts[i], OUTPUT);
        digitalWrite(xshuts[i], LOW);
    }

    // Ponte H TB6612FNG em estado seguro antes de habilitar STBY
    pinMode(PINO_PONTE_STBY, OUTPUT);
    digitalWrite(PINO_PONTE_STBY, LOW);
    pinMode(PINO_PONTE_AIN1, OUTPUT);
    pinMode(PINO_PONTE_AIN2, OUTPUT);
    pinMode(PINO_PONTE_BIN1, OUTPUT);
    pinMode(PINO_PONTE_BIN2, OUTPUT);
    configurarPwm(PINO_PONTE_PWMA, CANAL_PWM_ESQ);
    configurarPwm(PINO_PONTE_PWMB, CANAL_PWM_DIR);
    soltarTudo();
    digitalWrite(PINO_PONTE_STBY, HIGH);

    // Encoders em quadratura
    // GPIO 34 e 35 sao input-only (sem pull-up interno); GPIO 32 e 33 tem pull-up interno
    pinMode(PINO_ENCODER_ESQ_A, INPUT);
    pinMode(PINO_ENCODER_ESQ_B, INPUT);
    pinMode(PINO_ENCODER_DIR_A, INPUT_PULLUP);
    pinMode(PINO_ENCODER_DIR_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_ESQ_A), isrEsqA, RISING);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_ESQ_B), isrEsqB, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_DIR_A), isrDirA, RISING);
    attachInterrupt(digitalPinToInterrupt(PINO_ENCODER_DIR_B), isrDirB, CHANGE);

    // ESP-NOW no Canal 1 para comunicar com o Gateway ESP8266
    sRadioOk = iniciarEspNow();

    imprimirMenu();
    imprimirRelatorio();
}

void loop() {
    // 1. Comandos pela Serial USB da ESP32
    while (Serial.available()) {
        processarComando((char)Serial.read(), "USB");
    }

    // 2. Comandos sem fio via Gateway ESP8266 (ESP-NOW)
    char cmdRadio = 0;
    while (lerComandoEspNow(cmdRadio)) {
        processarComando(cmdRadio, "ESP-NOW");
    }

    // 3. Avanca sequencia automatica (se ativa)
    atualizarSequenciaAuto();

    const uint32_t agora = millis();
    const bool emMovimento = sAutoAtivo || (sDirEsq != 0) || (sDirDir != 0);

    // 4. Log periodico dos encoders enquanto os motores estao rodando (a cada 500 ms)
    static uint32_t ultimoLogMs = 0;
    if (emMovimento && (agora - ultimoLogMs >= 500)) {
        ultimoLogMs = agora;
        const DadosEncoders enc = lerEncoders();
        Serial.printf("[ENC] ESQ=%ld (A:%lu B:%lu) | DIR=%ld (A:%lu B:%lu) | PWM=%d\n",
                      (long)enc.ticksEsq, (unsigned long)enc.subidasEsqA, (unsigned long)enc.bordasEsqB,
                      (long)enc.ticksDir, (unsigned long)enc.subidasDirA, (unsigned long)enc.bordasDirB,
                      (int)sPwmAtual);
    }

    // 5. Telemetria ESP-NOW a 1 Hz para o Gateway ESP8266
    static uint32_t ultimoTxMs = 0;
    if (agora - ultimoTxMs >= TELEMETRIA_PERIODO_MS) {
        ultimoTxMs = agora;
        enviarTelemetriaEspNow(emMovimento);
    }
}
