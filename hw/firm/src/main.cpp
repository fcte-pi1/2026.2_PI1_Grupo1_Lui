/**
 * ==============================================================================
 * PROJETO MICROMOUSE - GRUPO 1 (PI1 2026.2 FCTE/UnB)
 * FIRMWARE FINAL DE COMISSIONAMENTO, TESTES GRADUAIS E TELEMETRIA ESP-NOW
 * Placa do Robô: ESP32-WROOM-32D (30 Pinos)
 * Gateway Receptor: ESP8266 (USB do PC Server) -> Backend FastAPI -> Frontend Web
 * ==============================================================================
 * 
 * MAPA DE PINAGEM UNIFICADO (SEM CONFLITOS):
 * ------------------------------------------------------------------------------
 *  [TB6612FNG]
 *    - PWMA: GPIO 25 | AIN1: GPIO 26 | AIN2: GPIO 27
 *    - STBY: GPIO 13 (Habilita a ponte H em nível HIGH)
 *    - BIN1: GPIO 33 | BIN2: GPIO 32 | PWMB: GPIO 18
 *  [ENCODERS N20]
 *    - Motor 1 (Esquerdo): Canal A -> GPIO 34,      Canal B -> GPIO 39 (VN)
 *    - Motor 2 (Direito) : Canal A -> GPIO 35,      Canal B -> GPIO 2 (Pull-up)
 *  [SENSORES DE DISTÂNCIA]
 *    - Modo 0 (3x VL53L0X): Esq=GPIO 4 (0x30), Frontal=GPIO 16 (0x31), Dir=GPIO 17 (0x32)
 *    - Modo 1 (2x ToF + US): Esq=GPIO 4 (0x30), Dir=GPIO 17 (0x32), Frontal=US (TRIG=16, ECHO=19)
 *    - Modo 2 (2x VL53L0X): Esq=GPIO 4 (0x30), Frontal=GPIO 16 (0x31)
 *  [BATERIA]
 *    - Leitura ADC Divisor: GPIO 36 (VP / ADC1_CH0)
 *  [RÁDIO ESP-NOW (TELEMETRIA PARA O SITE)]
 *    - Canal 1 Wi-Fi (2.4 GHz) | Broadcast: FF:FF:FF:FF:FF:FF (18 bytes a 1 Hz)
 *  [IHM / LIVRES]
 *    - Botão de Largada / Teste: GPIO 15 (Pull-Up interno)
 *    - LED de Sinalização / Livre: GPIO 14
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "pinout.h"
#include "tb6612_driver.h"
#include "encoder_driver.h"
#include "vl53l0x_manager.h"
#include "ultrasonic_driver.h"
#include "battery_monitor.h"
#include "radio_telemetria.h"
#include "TelemetriaPacket.h"
#include "TelemetriaCodec.h"

// Modos de Operação do Firmware
enum ModoOperacao {
    MODO_MENU,
    MODO_TELEMETRIA_CONTINUA,
    MODO_TESTE_SENSORES,
    MODO_TESTE_ENCODERS_MANUAL,
    MODO_TESTE_MOTORES_MANUAL,
    MODO_CALIBRACAO
};

static ModoOperacao modoAtual = MODO_MENU;
static EstadoRobo estadoRobo = AGUARDANDO;
static GeometriaLabirinto geometriaAtual = LAB_8X4; // Geometria selecionada (LAB_4X4, LAB_8X4 ou LAB_12X4)

// Posição e orientação de grade ativas para a telemetria do site
static int8_t posLinha = 0;      // 0 a 11 (Linhas 1 a 12)
static int8_t posColuna = 0;     // 0 a 3  (Colunas A a D)
static uint8_t orientacaoAtual = 0; // 0: NORTE, 1: SUL, 2: LESTE, 3: OESTE

// Temporizadores não-bloqueantes
static uint32_t last_telemetry_serial_ms = 0;
static const uint32_t TELEMETRY_SERIAL_INTERVAL_MS = 100; // 10 Hz no monitor serial

static uint32_t last_espnow_tx_ms = 0;
static const uint32_t ESPNOW_TX_INTERVAL_MS = 1000;       // 1 Hz via rádio para o site (RNF-SFT03)

static uint32_t tempoInicioProva = 0;
static int32_t last_calc_ticks = 0;
static uint32_t last_calc_time_ms = 0;

static bool vl53_ok = false;
#if CAL_USA_ULTRASSONICO_FRONTAL
static bool ultra_ok = false;
#endif
static bool radio_ok = false;

// Parâmetros ativos de calibração (inicializados da fonte única CalibracaoRobo.h)
static float calRodaDiametroMm = WHEEL_DIAMETER_MM;
static float calBitolaMm = TRACK_WIDTH_MM;
static float calOdomEscalaReto = ODOM_ESCALA_RETO;
static float calCurvaAntecipacaoGraus = CURVA_ANTECIPACAO_FREIO_GRAUS;
static float calDistLateralIdealMm = DIST_IDEAL_PAREDE_LATERAL_MM;
static uint16_t calDistEncostoFrontalMm = DIST_ENCOSTO_FRONTAL_MM;
static int16_t calPwmTeste = PWM_BASE_MOVIMENTO;
static int16_t calPwmManual = (PWM_BASE_MOVIMENTO >= 160) ? PWM_BASE_MOVIMENTO : 160;
static int8_t manualDirL = 0;
static int8_t manualDirR = 0;
static uint32_t last_manual_log_ms = 0;
static int16_t calOffEsqMm = TOF_OFFSET_ESQ_MM;
static int16_t calOffFroMm = TOF_OFFSET_FRONTAL_MM;
static int16_t calOffDirMm = TOF_OFFSET_DIR_MM;
static bool calInvMotEsq = MOTOR_ESQ_INVERTIDO;
static bool calInvMotDir = MOTOR_DIR_INVERTIDO;
static bool calInvEncEsq = ENCODER_ESQ_INVERTIDO;
static bool calInvEncDir = ENCODER_DIR_INVERTIDO;
static float calBatFator = BATTERY_CALIBRATION_FACTOR;

static inline float circunferenciaAtivaMm(void) {
    return 3.14159265f * calRodaDiametroMm;
}

static inline float ticksParaMmAtivo(int32_t ticks) {
    return ((float)ticks / (float)ENCODER_TICKS_PER_REV) * circunferenciaAtivaMm();
}

static void aplicarCalibracaoNosDrivers(void) {
    tb6612_set_inversion(calInvMotEsq, calInvMotDir);
    encoder_set_inversion(calInvEncEsq, calInvEncDir);
    vl53l0x_manager_set_offsets(calOffEsqMm, calOffFroMm, calOffDirMm);
    ultrasonic_set_offset(calOffFroMm);
    battery_monitor_set_factor(calBatFator);
}

static VL53L0X_Readings_t lerSensoresUnificados(void) {
    VL53L0X_Readings_t r = vl53l0x_manager_read();
#if CAL_USA_ULTRASSONICO_FRONTAL
    uint16_t dist_f = 8190;
    uint16_t raw_f = 8190;
    r.front_valid = ultrasonic_read(&dist_f, &raw_f);
    r.dist_front_mm = dist_f;
    r.raw_front_mm = raw_f;
    r.wall_front = (r.front_valid && r.dist_front_mm <= DIST_PAREDE_DETECTADA_MM);
#endif
    return r;
}

static void resetarCalibracaoParaPadrao(void) {
    calRodaDiametroMm = WHEEL_DIAMETER_MM;
    calBitolaMm = TRACK_WIDTH_MM;
    calOdomEscalaReto = ODOM_ESCALA_RETO;
    calCurvaAntecipacaoGraus = CURVA_ANTECIPACAO_FREIO_GRAUS;
    calDistLateralIdealMm = DIST_IDEAL_PAREDE_LATERAL_MM;
    calDistEncostoFrontalMm = DIST_ENCOSTO_FRONTAL_MM;
    calPwmTeste = PWM_BASE_MOVIMENTO;
    calOffEsqMm = TOF_OFFSET_ESQ_MM;
    calOffFroMm = TOF_OFFSET_FRONTAL_MM;
    calOffDirMm = TOF_OFFSET_DIR_MM;
    calInvMotEsq = MOTOR_ESQ_INVERTIDO;
    calInvMotDir = MOTOR_DIR_INVERTIDO;
    calInvEncEsq = ENCODER_ESQ_INVERTIDO;
    calInvEncDir = ENCODER_DIR_INVERTIDO;
    calBatFator = BATTERY_CALIBRATION_FACTOR;
    aplicarCalibracaoNosDrivers();
}

// Monta e dispara pacote de telemetria binário de 18 bytes via ESP-NOW para o ESP8266 Gateway
void enviarTelemetriaEspNow(void) {
    if (!radio_ok) return;

    VL53L0X_Readings_t tof = lerSensoresUnificados();
    BatteryStatus_t bat = battery_monitor_read();

    TelemetriaPacket pkt;
    memset(&pkt, 0, sizeof(pkt));

    // 1. Timestamp da prova em ms (0 em AGUARDANDO; máx 600.000 ms)
    if (estadoRobo == AGUARDANDO) {
        pkt.timestamp_ms = 0;
    } else {
        uint32_t decorrido = millis() - tempoInicioProva;
        if (decorrido > TELEMETRIA_TIMESTAMP_MAX_MS) decorrido = TELEMETRIA_TIMESTAMP_MAX_MS;
        pkt.timestamp_ms = decorrido;
    }

    pkt.labirinto_id = (uint8_t)geometriaAtual;
    pkt.status_id = (uint8_t)estadoRobo;
    pkt.desafio_cumprido = (estadoRobo == CONCLUIDO) ? 1 : 0;
    pkt.bateria_pct = bat.percentage;
    pkt.bateria_mv = bat.voltage_mv;

    // 2. Velocidade instantânea (mm/s) e média (mm/s) calculadas a partir dos encoders
    uint32_t now = millis();
    uint32_t dt = now - last_calc_time_ms;
    int32_t current_ticks = (encoder_get_ticks_left() + encoder_get_ticks_right()) / 2;

    if (dt >= 200 && last_calc_time_ms > 0) {
        int32_t delta_ticks = current_ticks - last_calc_ticks;
        float delta_mm = ((float)delta_ticks / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM;
        float v_inst = (delta_mm / (float)dt) * 1000.0f;
        pkt.velocidade_atual = (int16_t)v_inst;
        last_calc_ticks = current_ticks;
        last_calc_time_ms = now;
    } else if (last_calc_time_ms == 0) {
        last_calc_ticks = current_ticks;
        last_calc_time_ms = now;
        pkt.velocidade_atual = 0;
    }

    if (pkt.timestamp_ms > 0) {
        float total_dist_mm = (encoder_get_dist_left_mm() + encoder_get_dist_right_mm()) / 2.0f;
        pkt.velocidade_media = (int16_t)((total_dist_mm / (float)pkt.timestamp_ms) * 1000.0f);
    } else {
        pkt.velocidade_media = 0;
    }

    pkt.coluna = constrain(posColuna, 0, 3);
    pkt.linha = constrain(posLinha, 0, 11);
    pkt.orientacao = orientacaoAtual;

    // 3. Bitmask de paredes detectadas: bit0=Norte(Frente), bit2=Leste(Direita), bit3=Oeste(Esquerda)
    uint8_t mask_paredes = 0;
    if (tof.wall_front) mask_paredes |= (1 << 0); // Norte
    if (tof.wall_right) mask_paredes |= (1 << 2); // Leste
    if (tof.wall_left)  mask_paredes |= (1 << 3); // Oeste
    pkt.paredes = mask_paredes;

    // Validação estrita do contrato antes do envio
    if (telemetriaValidar(pkt) == TELEMETRIA_OK) {
        radio_telemetria_enviar(&pkt);
    }
}

void exibirMenuPrincipal(void) {
    const char* strGeo = (geometriaAtual == LAB_4X4) ? "4x4" : (geometriaAtual == LAB_8X4) ? "8x4" : "12x4";

    Serial.println("\n=======================================================");
    Serial.println("     MICROMOUSE PI1 2026.2 - MENU DE TESTES GRADUAIS   ");
    Serial.println("=======================================================");
    Serial.println("Escolha o subsistema que deseja testar:");
#if CAL_MODO_SENSORES == 2
    Serial.println("  [1] - Teste de Sensores (2x VL53L0X: Esquerdo + Frontal)");
#elif CAL_USA_ULTRASSONICO_FRONTAL
    Serial.println("  [1] - Teste de Sensores (2x ToF Laterais + 1x Ultrassônico Frontal)");
#else
    Serial.println("  [1] - Teste de Sensores VL53L0X (Leitura contínua em mm)");
#endif
    Serial.println("  [2] - Teste Manual de Motores (Esquerdo/Direito passo a passo)");
    Serial.println("  [3] - Teste de Encoders Manual (Gire as rodas na mão)");
    Serial.println("  [4] - Teste de Movimento: Avançar 1 Célula (168 mm) e Parar");
    Serial.println("  [5] - Teste de Curva: Giro de 90° Direita no próprio eixo");
    Serial.println("  [6] - Teste de Curva: Giro de 90° Esquerda no próprio eixo");
    Serial.println("  [7] - Teste de Manobra: Meia-Volta de 180° no próprio eixo");
    Serial.println("  [8] - Teste Integrado: Avanço com Freio Anti-Colisão (< 100 mm)");
    Serial.println("  [9] - Calibração de Bateria (Leitura ADC bruta e tensão)");
    Serial.println("  [k] - Calibração Guiada dos 3 Sensores ToF (offsets a 60 mm / 100 mm)");
    Serial.println("  [d] - Diagnóstico rápido 1-linha (ToF bruto/cal, Enc, Bat, Calibração)");
    Serial.println("  [i] - Analisar pinos livres, níveis I2C e testar XSHUT individual");
    Serial.println("  [c] - Calibração ao vivo via Serial (roda, bitola, offsets, inversões, dump)");
    Serial.printf( "  [g] - Alternar Geometria do Labirinto (Atual: %s -> 4x4, 8x4, 12x4)\n", strGeo);
    Serial.println("  [w] - Disparar Pacote ESP-NOW de Teste (Para o Gateway ESP8266)");
    Serial.println("  [0] - Modo Telemetria Contínua (Visão geral em tempo real)");
    Serial.println("  [s] - Parada Imediata de Emergência (Freio ativo)");
    Serial.println("  [m] - Exibir este menu novamente");
    Serial.println("=======================================================");
#if CAL_USA_ULTRASSONICO_FRONTAL
    Serial.printf("Status: ToF(Lat)=%s | US(Fre)=%s | Rádio ESP-NOW=%s (Canal 1) | Geometria=%s\n",
                  vl53_ok ? "OK" : "FALHA", ultra_ok ? "OK" : "FALHA",
                  radio_ok ? "PRONTO" : "ERRO", strGeo);
#elif CAL_MODO_SENSORES == 2
    Serial.printf("Status: ToF(2x E/F)=%s | Rádio ESP-NOW=%s (Canal 1) | Geometria=%s\n",
                  vl53_ok ? "OK" : "FALHA", radio_ok ? "PRONTO" : "ERRO", strGeo);
#else
    Serial.printf("Status: ToF=%s | Rádio ESP-NOW=%s (Canal 1) | Geometria=%s\n",
                  vl53_ok ? "OK" : "FALHA", radio_ok ? "PRONTO" : "ERRO", strGeo);
#endif
    Serial.printf("Calibração: Roda=%.1fmm | Bitola=%.1fmm | Escala=%.3f | PWM=%d | Off(E/F/D)=%d/%d/%d\n",
                  calRodaDiametroMm, calBitolaMm, calOdomEscalaReto, calPwmTeste,
                  calOffEsqMm, calOffFroMm, calOffDirMm);
    Serial.print("Digite a opção desejada: ");
}

void imprimirDiagnosticoRapido(void) {
    VL53L0X_Readings_t tof = lerSensoresUnificados();
    BatteryStatus_t bat = battery_monitor_read();
    int32_t tl = encoder_get_ticks_left();
    int32_t tr = encoder_get_ticks_right();
    Serial.printf("[DIAG] ToF cal(E/F/D)=[%d|%d|%d] bruto=[%d|%d|%d] | encE=%ld (%.1fmm) encD=%ld (%.1fmm) | Vbat=%.2fV (%d%%)\n",
                  tof.left_valid ? (int)tof.dist_left_mm : -1,
                  tof.front_valid ? (int)tof.dist_front_mm : -1,
                  tof.right_valid ? (int)tof.dist_right_mm : -1,
                  tof.left_valid ? (int)tof.raw_left_mm : -1,
                  tof.front_valid ? (int)tof.raw_front_mm : -1,
                  tof.right_valid ? (int)tof.raw_right_mm : -1,
                  (long)tl, ticksParaMmAtivo(tl),
                  (long)tr, ticksParaMmAtivo(tr),
                  bat.voltage_volts, bat.percentage);
}

static void escanearParI2C(uint8_t sda, uint8_t scl, const char* nome) {
    Wire.end();
    delay(5);
    Wire.begin(sda, scl, I2C_CLOCK_SPEED);
    Serial.printf("  - %s (SDA=%u, SCL=%u):", nome, (unsigned)sda, (unsigned)scl);
    uint8_t achados = 0;
    for (uint8_t addr = 0x20; addr <= 0x3F; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf(" 0x%02X", (unsigned)addr);
            achados++;
        }
    }
    if (achados == 0) Serial.print(" nenhum");
    Serial.println();
}

void executarAnalisePortasI2C(void) {
    Serial.println("\n>>> [ANALISE COMPLETA DE PORTAS E I2C] <<<");
    Wire.end();
    tb6612_stop();
    tb6612_set_standby(true); // Mantem ponte H desabilitada durante checagem

    // 1. Pinos bidirecionais (com pull-up e pull-down internos ~45k: seguro mesmo com perifericos)
    const uint8_t pinosIO[] = {2, 4, 5, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
    Serial.println("Teste IO (esperado livre: PU=1 PD=0):");
    for (size_t i = 0; i < sizeof(pinosIO); i++) {
        uint8_t p = pinosIO[i];
        pinMode(p, INPUT_PULLUP);
        delayMicroseconds(150);
        int pu = digitalRead(p);
        pinMode(p, INPUT_PULLDOWN);
        delayMicroseconds(150);
        int pd = digitalRead(p);
        const char* status = (pu == 1 && pd == 0) ? "OK" :
                             (pu == 0 && pd == 0) ? "PRESO_LOW" :
                             (pu == 1 && pd == 1) ? "PRESO_HIGH" : "INSTAVEL";
        Serial.printf("  GPIO %2u: PU=%d PD=%d -> %s\n", (unsigned)p, pu, pd, status);
    }

    // 2. Pinos Input-Only / ADC / Encoders (nao tem pull-up/down interno)
    const uint8_t pinosIn[] = {34, 35, 36, 39};
    Serial.println("Teste Input-Only / ADC (encoders e bateria):");
    for (size_t i = 0; i < sizeof(pinosIn); i++) {
        uint8_t p = pinosIn[i];
        pinMode(p, INPUT);
        delayMicroseconds(100);
        int mv = analogReadMilliVolts(p);
        Serial.printf("  GPIO %2u: dig=%d | adc=%d mV\n", (unsigned)p, digitalRead(p), mv);
    }

    // Restaura modos originais dos drivers apos o teste de IO
    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);
    tb6612_init();
    tb6612_stop();
    encoder_init();
    aplicarCalibracaoNosDrivers();

    const uint8_t xshuts[] = {
        PIN_XSHUT_1,
#if !CAL_USA_ULTRASSONICO_FRONTAL
        PIN_XSHUT_2,
#endif
#if CAL_TEM_TOF_DIREITO
        PIN_XSHUT_3
#endif
    };
    const char* nomesXshut[] = {
        "Esq(GPIO4)",
#if !CAL_USA_ULTRASSONICO_FRONTAL
        "Fre(GPIO16)",
#endif
#if CAL_TEM_TOF_DIREITO
        "Dir(GPIO17)"
#endif
    };
    const size_t nXshut = sizeof(xshuts);

    // Testa cada sensor isolado mantendo os demais XSHUT em LOW
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_CLOCK_SPEED);
    for (size_t idx = 0; idx < nXshut; idx++) {
        for (size_t j = 0; j < nXshut; j++) {
            pinMode(xshuts[j], OUTPUT);
            digitalWrite(xshuts[j], (j == idx) ? HIGH : LOW);
        }
        delay(20);
        Serial.printf("  * Isolado %s no I2C (%u/%u):", nomesXshut[idx], (unsigned)PIN_I2C_SDA, (unsigned)PIN_I2C_SCL);
        bool achou = false;
        for (uint8_t addr = 0x20; addr <= 0x3F; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf(" ACK=0x%02X", (unsigned)addr);
                achou = true;
            }
        }
        if (achou) {
            VL53L0X probe;
            probe.setBus(&Wire);
            probe.setTimeout(300);
            bool ok = probe.init();
            Serial.printf(" | init=%s", ok ? "OK" : "FALHA");
            if (ok) {
                uint16_t mm = probe.readRangeSingleMillimeters();
                Serial.printf(" | leitura=%u mm", (unsigned)mm);
            }
        } else {
            Serial.print(" sem resposta");
        }
        Serial.println();
    }

    // Mantem todos XSHUT HIGH para testar pares I2C alternativos
    for (size_t j = 0; j < nXshut; j++) digitalWrite(xshuts[j], HIGH);
    delay(20);
    Serial.println("Varredura em outros pares de pinos livres:");
    escanearParI2C(21, 22, "Atual");
    escanearParI2C(21, 23, "Sem D22 (SCL=23)");
    escanearParI2C(23, 19, "Alternativo 1");
    escanearParI2C(19, 5,  "Alternativo 2");

    // Restaura driver oficial (libera barramento do par alternativo antes)
    Wire.end();
    delay(5);
    vl53_ok = vl53l0x_manager_init();
    aplicarCalibracaoNosDrivers();
    Serial.printf("Restauracao driver 3xToF: %s\n", vl53_ok ? "OK" : "FALHA");
    Serial.println("-----------------------------------\n");
}

void exibirAjudaCalibracao(void) {
    Serial.println("\n--- [MODO c] CALIBRAÇÃO AO VIVO (RAM) ---");
    Serial.println("Comandos (digite a linha + Enter):");
    Serial.println("  roda <mm>          -> ex: roda 22.0");
    Serial.println("  bitola <mm>        -> ex: bitola 108.5");
    Serial.println("  escala <fator>     -> ex: escala 1.012");
    Serial.println("  pwm <0..255>       -> ex: pwm 90");
    Serial.println("  encosto <mm>       -> ex: encosto 24");
    Serial.println("  lat <mm>           -> ex: lat 43.0");
    Serial.println("  off <e> <f> <d>    -> offsets ToF em mm (ex: off 4 34 16)");
    Serial.println("  caltof [canal] [mm]-> calibra ToF auto (ex: caltof 60, caltof fre 100)");
    Serial.println("  mot <0|1> <0|1>    -> inverte motor esq/dir (ex: mot 0 1)");
    Serial.println("  enc <0|1> <0|1>    -> inverte encoder esq/dir (ex: enc 0 1)");
    Serial.println("  bat <fator>        -> fator fino bateria (ex: bat 1.03)");
    Serial.println("  vreal <volts>      -> auto-calibra fator pela tensao do multimetro (ex: vreal 8.15)");
    Serial.println("  dump               -> imprime linhas prontas para CalibracaoRobo.h");
    Serial.println("  reset              -> restaura valores compilados de CalibracaoRobo.h");
    Serial.println("  x                  -> voltar ao menu principal");
    Serial.println("------------------------------------------");
}

void imprimirDumpCalibracao(void) {
    Serial.println("\n// --- Copie para src/firmware/lib/calibracao/src/CalibracaoRobo.h ---");
    Serial.printf("constexpr float CAL_RODA_DIAMETRO_MM = %.2ff;\n", calRodaDiametroMm);
    Serial.printf("constexpr float CAL_BITOLA_MM = %.2ff;\n", calBitolaMm);
    Serial.printf("constexpr float CAL_ODOM_ESCALA_RETO = %.4ff;\n", calOdomEscalaReto);
    Serial.printf("constexpr bool CAL_MOTOR_ESQ_INVERTIDO = %s;\n", calInvMotEsq ? "true" : "false");
    Serial.printf("constexpr bool CAL_MOTOR_DIR_INVERTIDO = %s;\n", calInvMotDir ? "true" : "false");
    Serial.printf("constexpr bool CAL_ENCODER_ESQ_INVERTIDO = %s;\n", calInvEncEsq ? "true" : "false");
    Serial.printf("constexpr bool CAL_ENCODER_DIR_INVERTIDO = %s;\n", calInvEncDir ? "true" : "false");
    Serial.printf("constexpr uint8_t CAL_PWM_BASE_MOVIMENTO = %d;\n", (int)calPwmTeste);
    Serial.printf("constexpr float CAL_DIST_IDEAL_PAREDE_LATERAL_MM = %.1ff;\n", calDistLateralIdealMm);
    Serial.printf("constexpr int16_t CAL_TOF_OFFSET_ESQ_MM = %d;\n", (int)calOffEsqMm);
    Serial.printf("constexpr int16_t CAL_TOF_OFFSET_FRONTAL_MM = %d;\n", (int)calOffFroMm);
    Serial.printf("constexpr int16_t CAL_TOF_OFFSET_DIR_MM = %d;\n", (int)calOffDirMm);
    Serial.printf("constexpr uint16_t CAL_DIST_ENCOSTO_FRONTAL_MM = %u;\n", (unsigned)calDistEncostoFrontalMm);
    Serial.printf("constexpr float CAL_BATERIA_FATOR_AJUSTE = %.4ff;\n", calBatFator);
    Serial.println("// ----------------------------------------------------------------------");
}

static void calibrarSensoresToF(uint8_t mask, uint16_t distMm) {
    const uint8_t AMOSTRAS = 10;
    uint32_t sum[3] = {0, 0, 0};
    uint8_t cnt[3] = {0, 0, 0};
    for (uint8_t s = 0; s < AMOSTRAS; s++) {
        VL53L0X_Readings_t r = lerSensoresUnificados();
        if ((mask & 0x01) && r.left_valid && r.raw_left_mm < 2000) { sum[0] += r.raw_left_mm; cnt[0]++; }
        if ((mask & 0x02) && r.front_valid && r.raw_front_mm < 2000) { sum[1] += r.raw_front_mm; cnt[1]++; }
        if ((mask & 0x04) && r.right_valid && r.raw_right_mm < 2000) { sum[2] += r.raw_right_mm; cnt[2]++; }
        delay(35);
    }

    const char* nomes[3] = {"ESQUERDO", "FRONTAL", "DIREITO"};
    int16_t* offs[3] = {&calOffEsqMm, &calOffFroMm, &calOffDirMm};
    for (uint8_t i = 0; i < 3; i++) {
        if (!(mask & (1u << i))) continue;
        if (cnt[i] >= 3) {
            *offs[i] = vl53l0x_calcular_offset_mm(sum[i], cnt[i], distMm);
            Serial.printf("[CAL-TOF] %s: bruto_medio=%.1f mm -> offset=%d mm (ref=%u mm)\n",
                          nomes[i], (float)sum[i] / (float)cnt[i], (int)*offs[i], (unsigned)distMm);
        } else {
            Serial.printf("[CAL-TOF] %s: sem alvo valido (<2000 mm) ou offline; mantido (%d mm).\n",
                          nomes[i], (int)*offs[i]);
        }
    }
    aplicarCalibracaoNosDrivers();
    imprimirDiagnosticoRapido();
}

void executarCalibracaoGuiadaToF(void) {
    while (Serial.available()) Serial.read(); // descarta \r\n residual do 'k'
    Serial.println("\n>>> [MODO k] CALIBRACAO GUIADA DOS 3 SENSORES ToF <<<");
    Serial.println("Para cada sensor: posicione o anteparo branco a 60 mm (ou digite outra distancia em mm),");
    Serial.println("e aperte ENTER para medir 10 amostras. Digite 'p' para pular ou 'x' para cancelar.\n");

#if CAL_USA_ULTRASSONICO_FRONTAL
    const char* nomes[3] = {"ESQUERDO (GPIO 4 / 0x30)", "FRONTAL (US TRIG=16 ECHO=19)", "DIREITO (GPIO 17 / 0x32)"};
#else
    const char* nomes[3] = {"ESQUERDO (GPIO 4 / 0x30)", "FRONTAL (GPIO 16 / 0x31)", "DIREITO (GPIO 17 / 0x32)"};
#endif
    for (uint8_t i = 0; i < 3; i++) {
#if !CAL_TEM_TOF_DIREITO
        if (i == 2) continue;
#endif
        Serial.printf("-> Sensor %s [ENTER=60mm | <mm> | p=pular | x=sair]: ", nomes[i]);
        while (!Serial.available()) delay(20);
        String resp = Serial.readStringUntil('\n');
        resp.trim();
        Serial.println(resp);

        if (resp.equalsIgnoreCase("x")) {
            Serial.println("[CAL-TOF] Calibracao cancelada.");
            exibirMenuPrincipal();
            return;
        }
        if (resp.equalsIgnoreCase("p")) {
            Serial.println("   Pulado.");
            continue;
        }

        int distMm = (resp.length() == 0) ? 60 : resp.toInt();
        if (distMm < 20 || distMm > 500) {
            Serial.println("   Distancia invalida (use 20..500 mm); usando 60 mm.");
            distMm = 60;
        }
        calibrarSensoresToF((uint8_t)(1u << i), (uint16_t)distMm);
    }

    Serial.println("\n[CAL-TOF] Offsets prontos para src/firmware/lib/calibracao/src/CalibracaoRobo.h:");
    Serial.printf("constexpr int16_t CAL_TOF_OFFSET_ESQ_MM = %d;\n", (int)calOffEsqMm);
    Serial.printf("constexpr int16_t CAL_TOF_OFFSET_FRONTAL_MM = %d;\n", (int)calOffFroMm);
    Serial.printf("constexpr int16_t CAL_TOF_OFFSET_DIR_MM = %d;\n", (int)calOffDirMm);
    Serial.println("-------------------------------------------------------\n");
    exibirMenuPrincipal();
}

void processarLinhaCalibracao(String linha) {
    linha.trim();
    if (linha.length() == 0) return;

    if (linha.equalsIgnoreCase("x")) {
        modoAtual = MODO_MENU;
        exibirMenuPrincipal();
        return;
    }
    if (linha.equalsIgnoreCase("dump")) {
        imprimirDumpCalibracao();
        return;
    }
    if (linha.equalsIgnoreCase("reset")) {
        resetarCalibracaoParaPadrao();
        Serial.println("[CAL] Valores restaurados de CalibracaoRobo.h.");
        return;
    }

    float f1 = 0;
    int i1 = 0, i2 = 0, i3 = 0;
    if (sscanf(linha.c_str(), "roda %f", &f1) == 1 && f1 >= 10.0f && f1 <= 60.0f) {
        calRodaDiametroMm = f1;
        Serial.printf("[CAL] Roda = %.2f mm\n", calRodaDiametroMm);
    } else if (sscanf(linha.c_str(), "bitola %f", &f1) == 1 && f1 >= 40.0f && f1 <= 160.0f) {
        calBitolaMm = f1;
        Serial.printf("[CAL] Bitola = %.2f mm\n", calBitolaMm);
    } else if (sscanf(linha.c_str(), "escala %f", &f1) == 1 && f1 >= 0.5f && f1 <= 1.5f) {
        calOdomEscalaReto = f1;
        Serial.printf("[CAL] Escala reto = %.4f\n", calOdomEscalaReto);
    } else if (sscanf(linha.c_str(), "pwm %d", &i1) == 1 && i1 >= 20 && i1 <= 255) {
        calPwmTeste = (int16_t)i1;
        calPwmManual = (int16_t)i1;
        Serial.printf("[CAL] PWM base = %d\n", (int)calPwmTeste);
    } else if (sscanf(linha.c_str(), "encosto %d", &i1) == 1 && i1 >= 10 && i1 <= 100) {
        calDistEncostoFrontalMm = (uint16_t)i1;
        Serial.printf("[CAL] Encosto frontal = %u mm\n", (unsigned)calDistEncostoFrontalMm);
    } else if (sscanf(linha.c_str(), "lat %f", &f1) == 1 && f1 >= 15.0f && f1 <= 90.0f) {
        calDistLateralIdealMm = f1;
        Serial.printf("[CAL] Dist lateral ideal = %.1f mm\n", calDistLateralIdealMm);
    } else if (sscanf(linha.c_str(), "off %d %d %d", &i1, &i2, &i3) == 3) {
        calOffEsqMm = (int16_t)i1;
        calOffFroMm = (int16_t)i2;
        calOffDirMm = (int16_t)i3;
        aplicarCalibracaoNosDrivers();
        Serial.printf("[CAL] Offsets ToF (E/F/D) = %d / %d / %d mm\n", i1, i2, i3);
    } else if (sscanf(linha.c_str(), "mot %d %d", &i1, &i2) == 2) {
        calInvMotEsq = (i1 != 0);
        calInvMotDir = (i2 != 0);
        aplicarCalibracaoNosDrivers();
        Serial.printf("[CAL] Inversao motor (E/D) = %d / %d\n", calInvMotEsq, calInvMotDir);
    } else if (sscanf(linha.c_str(), "enc %d %d", &i1, &i2) == 2) {
        calInvEncEsq = (i1 != 0);
        calInvEncDir = (i2 != 0);
        aplicarCalibracaoNosDrivers();
        Serial.printf("[CAL] Inversao encoder (E/D) = %d / %d\n", calInvEncEsq, calInvEncDir);
    } else if (sscanf(linha.c_str(), "bat %f", &f1) == 1 && f1 > 0.2f && f1 < 5.0f) {
        calBatFator = f1;
        aplicarCalibracaoNosDrivers();
        Serial.printf("[CAL] Fator bateria = %.4f (V=%.2fV)\n", calBatFator, battery_monitor_read().voltage_volts);
    } else if (sscanf(linha.c_str(), "vreal %f", &f1) == 1 && f1 >= 4.0f && f1 <= 10.0f) {
        battery_monitor_set_factor(1.0f);
        float v_base = battery_monitor_read().voltage_volts;
        if (v_base > 0.5f) {
            calBatFator = f1 / v_base;
            aplicarCalibracaoNosDrivers();
            Serial.printf("[CAL] V_base=%.3fV -> calBatFator=%.4f | Nova leitura=%.2fV\n",
                          v_base, calBatFator, battery_monitor_read().voltage_volts);
        } else {
            Serial.printf("[CAL] Pino GPIO %d lendo < 0.5V (divisor desconectado ou bateria ausente).\n",
                          PIN_BATTERY_ADC);
        }
    } else if (linha.startsWith("caltof")) {
        char canalStr[16] = "todos";
        int distMm = 60;
        int parsed = sscanf(linha.c_str(), "caltof %15s %d", canalStr, &distMm);
        if (parsed == 1 && atoi(canalStr) > 0) {
            distMm = atoi(canalStr);
            strcpy(canalStr, "todos");
        }
        if (distMm < 20 || distMm > 500) {
            Serial.println("[CAL] Distancia alvo invalida (use 20..500 mm, ex: 'caltof 60').");
            return;
        }
        uint8_t mask = 0;
        if (strcasecmp(canalStr, "todos") == 0) mask = 0x07;
        else if (strcasecmp(canalStr, "esq") == 0) mask = 0x01;
        else if (strcasecmp(canalStr, "fre") == 0) mask = 0x02;
        else if (strcasecmp(canalStr, "dir") == 0) mask = 0x04;
        else {
            Serial.println("[CAL] Use: caltof [esq|fre|dir|todos] [mm] (ex: 'caltof fre 60').");
            return;
        }
        calibrarSensoresToF(mask, (uint16_t)distMm);
    } else {
        Serial.println("[CAL] Comando invalido. Digite 'dump', 'reset', 'x' ou ex: 'bitola 108.5'.");
    }
}

// ------------------------------------------------------------------------------
// TESTE 1: LEITURA CONTÍNUA DOS SENSORES VL53L0X
// ------------------------------------------------------------------------------
void executarLoopTesteSensores(void) {
    VL53L0X_Readings_t tof = lerSensoresUnificados();

    char barL[11], barF[11], barR[11];
    int l1 = map(constrain(tof.dist_left_mm, 20, 200), 20, 200, 1, 10);
    int l2 = map(constrain(tof.dist_front_mm, 20, 200), 20, 200, 1, 10);
    int l3 = map(constrain(tof.dist_right_mm, 20, 200), 20, 200, 1, 10);

    for (int i = 0; i < 10; i++) {
        barL[i] = (i < l1) ? '=' : ' ';
        barF[i] = (i < l2) ? '=' : ' ';
        barR[i] = (i < l3) ? '=' : ' ';
    }
    barL[10] = '\0'; barF[10] = '\0'; barR[10] = '\0';

    Serial.printf("[ToF] Esq: %4d mm [%s] %s | Fre: %4d mm [%s] %s | Dir: %4d mm [%s] %s\n",
                  tof.left_valid ? tof.dist_left_mm : -1, barL, tof.wall_left ? "(PAREDE)" : "        ",
                  tof.front_valid ? tof.dist_front_mm : -1, barF, tof.wall_front ? "(PAREDE)" : "        ",
                  tof.right_valid ? tof.dist_right_mm : -1, barR, tof.wall_right ? "(PAREDE)" : "        ");
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// TESTE 2: CONTROLE MANUAL DOS MOTORES (ACIONAMENTO CONTÍNUO COM PWM AJUSTÁVEL)
// ------------------------------------------------------------------------------
static void aplicarEstadoMotorManual(void) {
    int16_t pwmL = (int16_t)manualDirL * calPwmManual;
    int16_t pwmR = (int16_t)manualDirR * calPwmManual;
    tb6612_set_motors(pwmL, pwmR);
}

void exibirMenuMotoresManual(void) {
    Serial.printf("\n--- [MODO 2] TESTE MANUAL DE MOTORES (PWM = %d/255) ---\n", (int)calPwmManual);
    Serial.println("Comandos no teclado (acionamento contínuo até parar):");
    Serial.println("  [e] - Motor Esquerdo FRENTE  | [E] - Motor Esquerdo RÉ");
    Serial.println("  [d] - Motor Direito  FRENTE  | [D] - Motor Direito  RÉ");
    Serial.println("  [f] - Ambos os motores FRENTE | [t] - Ambos os motores RÉ");
    Serial.println("  [+] / [-] - Aumentar ou reduzir PWM em +/-20 (40..255)");
    Serial.println("  [s] ou [espaço] - Freio ativo (parar motores)");
    Serial.println("  [x] - Sair deste teste e voltar ao menu principal");
    Serial.println("-----------------------------------------------------------");
}

void processarComandoMotorManual(char c) {
    struct ComandoMotorSpec {
        char key;
        int8_t dirL;
        int8_t dirR;
        const char* label;
    };
    static const ComandoMotorSpec kComandos[] = {
        {'e',  1,  0, "Motor Esquerdo FRENTE"},
        {'E', -1,  0, "Motor Esquerdo RÉ"},
        {'d',  0,  1, "Motor Direito FRENTE"},
        {'D',  0, -1, "Motor Direito RÉ"},
        {'f',  1,  1, "Ambos os motores FRENTE"},
        {'t', -1, -1, "Ambos os motores RÉ"},
    };

    for (size_t i = 0; i < sizeof(kComandos) / sizeof(kComandos[0]); i++) {
        if (c == kComandos[i].key) {
            manualDirL = kComandos[i].dirL;
            manualDirR = kComandos[i].dirR;
            aplicarEstadoMotorManual();
            last_manual_log_ms = millis();
            Serial.printf("-> %s (PWM=%d contínuo; envie 's' para parar)\n",
                          kComandos[i].label, (int)calPwmManual);
            return;
        }
    }

    if (c == '+' || c == '=') {
        calPwmManual = (int16_t)constrain((int)calPwmManual + 20, 40, 255);
        if (manualDirL != 0 || manualDirR != 0) aplicarEstadoMotorManual();
        Serial.printf("-> PWM Modo 2 = %d/255\n", (int)calPwmManual);
    } else if (c == '-') {
        calPwmManual = (int16_t)constrain((int)calPwmManual - 20, 40, 255);
        if (manualDirL != 0 || manualDirR != 0) aplicarEstadoMotorManual();
        Serial.printf("-> PWM Modo 2 = %d/255\n", (int)calPwmManual);
    } else if (c == 's' || c == 'S' || c == ' ') {
        manualDirL = 0;
        manualDirR = 0;
        tb6612_brake();
        Serial.printf("-> Freio ativado. Ticks: Esq=%ld | Dir=%ld\n",
                      (long)encoder_get_ticks_left(), (long)encoder_get_ticks_right());
    } else if (c == 'x' || c == 'X') {
        manualDirL = 0;
        manualDirR = 0;
        tb6612_stop();
        modoAtual = MODO_MENU;
        exibirMenuPrincipal();
    }
}

// ------------------------------------------------------------------------------
// TESTE 3: ENCODERS MANUAL (GIRANDO NA MÃO)
// ------------------------------------------------------------------------------
void executarLoopTesteEncoders(void) {
    int32_t tl = encoder_get_ticks_left();
    int32_t tr = encoder_get_ticks_right();
    float dl = ticksParaMmAtivo(tl);
    float dr = ticksParaMmAtivo(tr);

    Serial.printf("[ENCODER MANUAL] Ticks Esq: %5ld (%5.1f mm) | Ticks Dir: %5ld (%5.1f mm) | [Pressione 'x' p/ sair]\n",
                  (long)tl, dl, (long)tr, dr);
}

// ------------------------------------------------------------------------------
// TESTE 4: AVANÇAR 1 CÉLULA (168 mm) COM ODOMETRIA EM MALHA FECHADA
// ------------------------------------------------------------------------------
void executarAvancoUmaCelula(void) {
    Serial.println("\n>>> [TESTE 4] AVANÇAR 1 CÉLULA (168 mm) <<<");
    Serial.println("Coloque o carrinho no chão da arena ou pista de teste.");
    Serial.println("Iniciando em 2 segundos...");
    delay(2000);

    tempoInicioProva = millis();
    estadoRobo = MAPEANDO;
    enviarTelemetriaEspNow(); // Notifica o site que começou a mover

    encoder_reset();

    int32_t target_ticks = (int32_t)(((CELL_SIZE_MM * calOdomEscalaReto) / circunferenciaAtivaMm()) * ENCODER_TICKS_PER_REV);
    Serial.printf("Distância alvo: %.1f mm (~%ld ticks por roda)\n", CELL_SIZE_MM, (long)target_ticks);

    uint32_t start_time = millis();
    uint32_t last_tof_ctrl = 0;
    bool timeout = false;
    bool parada_frontal = false;

    // Parâmetros de controle refinado (PD Lateral + Modo Monoparede)
    const float KP_PAREDE = 0.35f;
    const float KD_PAREDE = 0.20f;  // Amortece o arrasto da roda boba e mata oscilação
    const float KP_ENCODER = 0.50f;
    const uint16_t LIMIAR_CORREDOR_MM = DIST_PAREDE_DETECTADA_MM + 20;

    float ultimo_erro_lateral = 0.0f;
    uint32_t ultimo_tempo_pd = millis();

    while (true) {
        uint32_t now = millis();
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        int32_t media = (tl + tr) / 2;

        // 1. Chegada na distância por odometria
        if (media >= target_ticks) {
            break;
        }

        // 2. Timeout de segurança (3 segundos)
        if (now - start_time > 3000) {
            timeout = true;
            break;
        }

        // 3. Controle e Sincronização ToF + Encoders a cada 20 ms (~50 Hz)
        if (now - last_tof_ctrl >= 20) {
            last_tof_ctrl = now;
            VL53L0X_Readings_t tof = lerSensoresUnificados();

            // Encosto frontal óptico: se detectou parede à frente a <= DIST_ENCOSTO_FRONTAL_MM, freia
            if (tof.front_valid && tof.dist_front_mm <= calDistEncostoFrontalMm && media > (target_ticks / 3)) {
                parada_frontal = true;
                break;
            }

            // Rampa suave de partida nos primeiros 120 ms (evita tranco na roda boba e patinação)
            int16_t pwm_base = calPwmTeste;
            uint32_t tempo_corrido = now - start_time;
            if (tempo_corrido < 120) {
                pwm_base = map(tempo_corrido, 0, 120, CAL_PWM_MINIMO, calPwmTeste);
            }

            int16_t pwm_l = pwm_base;
            int16_t pwm_r = pwm_base;

            bool temEsq = (tof.left_valid && tof.dist_left_mm < LIMIAR_CORREDOR_MM);
            bool temDir = (tof.right_valid && tof.dist_right_mm < LIMIAR_CORREDOR_MM);

            if (temEsq && temDir) {
                // Caso A: Duas paredes (Corredor fechado) -> Controle PD
                float erro_lateral = (float)tof.dist_left_mm - (float)tof.dist_right_mm;
                float dt = (now - ultimo_tempo_pd) / 1000.0f;
                float derivada = (dt > 0.005f) ? (erro_lateral - ultimo_erro_lateral) / dt : 0.0f;

                ultimo_erro_lateral = erro_lateral;
                ultimo_tempo_pd = now;

                int16_t correcao = (int16_t)(KP_PAREDE * erro_lateral + KD_PAREDE * derivada * 0.05f);
                correcao = constrain(correcao, -25, 25);
                pwm_l -= correcao;
                pwm_r += correcao;
            } else if (temEsq) {
                // Caso B: Modo Monoparede Esquerda (Abertura ou curva à direita)
                float erro_lateral = (float)tof.dist_left_mm - calDistLateralIdealMm;
                int16_t correcao = (int16_t)(KP_PAREDE * erro_lateral * 1.5f);
                correcao = constrain(correcao, -20, 20);
                pwm_l -= correcao;
                pwm_r += correcao;
            } else if (temDir) {
                // Caso C: Modo Monoparede Direita (Abertura ou curva à esquerda)
                float erro_lateral = calDistLateralIdealMm - (float)tof.dist_right_mm;
                int16_t correcao = (int16_t)(KP_PAREDE * erro_lateral * 1.5f);
                correcao = constrain(correcao, -20, 20);
                pwm_l -= correcao;
                pwm_r += correcao;
            } else {
                // Caso D: Cruzamento aberto (sem paredes) -> Trava o rumo pelos encoders
                int32_t diff_ticks = encoder_get_ticks_left() - encoder_get_ticks_right();
                int16_t correcao = (int16_t)(KP_ENCODER * diff_ticks);
                correcao = constrain(correcao, -20, 20);
                pwm_l -= correcao;
                pwm_r += correcao;
            }

            tb6612_set_motors(pwm_l, pwm_r);
        }

        delay(2);
    }

    tb6612_brake();
    delay(200);
    tb6612_stop();

    // Atualiza a posição de grade do robô conforme a orientação do avanço
    int maxL = (geometriaAtual == LAB_4X4) ? 3 : (geometriaAtual == LAB_8X4) ? 7 : 11;
    if (orientacaoAtual == 0 && posLinha < maxL) posLinha++;      // Norte -> sobe linha
    else if (orientacaoAtual == 1 && posLinha > 0) posLinha--; // Sul -> desce linha
    else if (orientacaoAtual == 2 && posColuna < 3) posColuna++; // Leste -> avança coluna
    else if (orientacaoAtual == 3 && posColuna > 0) posColuna--; // Oeste -> recua coluna

    estadoRobo = AGUARDANDO;
    enviarTelemetriaEspNow(); // Notifica o site da nova célula e da parada

    Serial.printf("Resultado: %s\n", timeout ? "[TIMEOUT DE SEGURANÇA!]" : (parada_frontal ? "[ENCOSTO FRONTAL ÓPTICO]" : "[CHEGOU NA DISTÂNCIA ALVO POR ENCODERS]"));
    Serial.printf("  Ticks finais: Esq=%ld (%.1f mm) | Dir=%ld (%.1f mm)\n",
                  (long)encoder_get_ticks_left(), ticksParaMmAtivo(encoder_get_ticks_left()),
                  (long)encoder_get_ticks_right(), ticksParaMmAtivo(encoder_get_ticks_right()));
    Serial.println("-------------------------------------------------------\n");
    exibirMenuPrincipal();
}

// ------------------------------------------------------------------------------
// TESTE 5, 6 & 7: GIRO DE 90° OU 180° NO PRÓPRIO EIXO
// ------------------------------------------------------------------------------
void executarGiroGraus(float graus, bool paraDireita, uint32_t timeoutMs, const char* titulo) {
    Serial.printf("\n>>> %s <<<\n", titulo);
    Serial.println("Iniciando giro em 2 segundos...");
    delay(2000);

    tempoInicioProva = millis();
    estadoRobo = MAPEANDO;
    enviarTelemetriaEspNow();

    encoder_reset();

    float grausEfetivos = (graus > calCurvaAntecipacaoGraus) ? (graus - calCurvaAntecipacaoGraus) : graus;
    float arco_mm = (3.14159265f * calBitolaMm) * (grausEfetivos / 360.0f);
    int32_t target_ticks = (int32_t)((arco_mm / circunferenciaAtivaMm()) * ENCODER_TICKS_PER_REV);

    Serial.printf("Arco da roda: %.1f mm (~%ld ticks, bitola=%.1f mm)\n",
                  arco_mm, (long)target_ticks, calBitolaMm);

    uint32_t start_time = millis();
    bool timeout = false;
    const int16_t PWM_RAMPA = 55;
    const float KP_GIRO_SYNC = 0.5f;

    while (true) {
        uint32_t now = millis();
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        int32_t media = (tl + tr) / 2;

        if (media >= target_ticks) {
            break;
        }
        if (now - start_time > timeoutMs) {
            timeout = true;
            break;
        }

        int16_t pwm_base = (media > (target_ticks * 0.80f)) ? PWM_RAMPA : calPwmTeste;
        int32_t diff = tl - tr;
        int16_t ajuste = (int16_t)(diff * KP_GIRO_SYNC);
        ajuste = constrain(ajuste, -15, 15);

        int16_t pwm_l = pwm_base - ajuste;
        int16_t pwm_r = pwm_base + ajuste;

        if (paraDireita) {
            tb6612_set_motors(pwm_l, -pwm_r);
        } else {
            tb6612_set_motors(-pwm_l, pwm_r);
        }

        delay(3);
    }

    tb6612_brake();
    delay(200);
    tb6612_stop();

    estadoRobo = AGUARDANDO;
    enviarTelemetriaEspNow();

    Serial.printf("Resultado: %s\n", timeout ? "[TIMEOUT]" : "[GIRO CONCLUÍDO]");
    Serial.printf("  Ticks: Esq=%ld | Dir=%ld\n",
                  (long)encoder_get_ticks_left(), (long)encoder_get_ticks_right());
    Serial.println("DICA: Use '[c]' -> 'bitola <mm>' para calibrar o angulo ao vivo!\n");
    exibirMenuPrincipal();
}

void executarCurva90Graus(bool paraDireita) {
    executarGiroGraus(90.0f, paraDireita, 2500,
                      paraDireita ? "[TESTE 5] GIRO DE 90° PARA A DIREITA"
                                  : "[TESTE 6] GIRO DE 90° PARA A ESQUERDA");
}

void executarMeiaVolta180(void) {
    executarGiroGraus(180.0f, true, 4000, "[TESTE 7] GIRO DE 180° (MEIA VOLTA)");
}

// ------------------------------------------------------------------------------
// TESTE 8: AVANÇO COM FRENAGEM AUTOMÁTICA ANTI-COLISÃO
// ------------------------------------------------------------------------------
void executarAvancoAntiColisao(void) {
    Serial.println("\n>>> [TESTE 8] AVANÇO COM FREIO ANTI-COLISÃO <<<");
    Serial.println("O carrinho vai avançar devagar e frear sozinho se detectar");
    Serial.println("obstáculo/parede à frente a menos de 100 mm!");
    Serial.println("Iniciando em 2 segundos...");
    delay(2000);

    tempoInicioProva = millis();
    estadoRobo = MAPEANDO;
    enviarTelemetriaEspNow();

    encoder_reset();
    tb6612_set_motors(calPwmTeste - 15, calPwmTeste - 15);

    uint32_t start_time = millis();
    bool obstaculo = false;
    uint16_t dist_final = 0;

    while (millis() - start_time < 5000) {
        VL53L0X_Readings_t tof = lerSensoresUnificados();

        if (tof.front_valid && tof.dist_front_mm <= 100) {
            dist_final = tof.dist_front_mm;
            obstaculo = true;
            break;
        }
        delay(20);
    }

    tb6612_brake();
    delay(300);
    tb6612_stop();

    estadoRobo = AGUARDANDO;
    enviarTelemetriaEspNow();

    if (obstaculo) {
        Serial.printf("\n[SUCESSO] OBSTÁCULO DETECTADO A %d mm! FREIO ATIVADO.\n", dist_final);
        Serial.printf("  Distância percorrida até a parada: %.1f mm\n", encoder_get_dist_left_mm());
    } else {
        Serial.println("\n[TEMPO ESGOTADO] Nenhum obstáculo detectado nos 5 segundos.");
    }
    Serial.println("-------------------------------------------------------\n");
    exibirMenuPrincipal();
}

// ------------------------------------------------------------------------------
// TESTE 9: RELATÓRIO E CALIBRAÇÃO DE BATERIA
// ------------------------------------------------------------------------------
void executarRelatorioBateria(void) {
    Serial.println("\n>>> [TESTE 9] DIAGNÓSTICO E CALIBRAÇÃO DA BATERIA <<<");
    uint32_t raw_sum = 0;
    uint32_t mv_sum = 0;
    for (int i = 0; i < 32; i++) {
        raw_sum += analogRead(PIN_BATTERY_ADC);
        mv_sum += analogReadMilliVolts(PIN_BATTERY_ADC);
        delay(5);
    }
    float raw_avg = (float)raw_sum / 32.0f;
    float v_pin = ((float)mv_sum / 32.0f) / 1000.0f;
    float v_bat = v_pin * BATTERY_DIVIDER_RATIO * calBatFator;
    uint8_t pct = battery_voltage_to_percentage(v_bat);

    Serial.printf("  - Leitura bruta ADC (0-4095) : %.1f\n", raw_avg);
    Serial.printf("  - Tensão calibrada GPIO %2d   : %.3f V (eFuse mV)\n", PIN_BATTERY_ADC, v_pin);
    Serial.printf("  - Divisor x Fator fino       : %.2f x %.4f\n", BATTERY_DIVIDER_RATIO, calBatFator);
    Serial.printf("  - Tensão Calculada Bateria   : %.2f V (%d mV)\n", v_bat, (int)(v_bat * 1000));
    Serial.printf("  - Estimativa de Carga        : %d %%\n", pct);
    Serial.printf("  - Alerta de Bateria Baixa    : %s\n", (v_bat < 6.8f) ? "SIM (RECARREGAR!)" : "NÃO (Tensão Segura)");
    Serial.println("-------------------------------------------------------\n");
    exibirMenuPrincipal();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_BUTTON_START, INPUT_PULLUP);
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    // 1. Inicializa drivers de hardware e aplica sentidos/offsets de CalibracaoRobo.h
    tb6612_init();
    tb6612_stop();
    encoder_init();
    battery_monitor_init();
    aplicarCalibracaoNosDrivers();

#if CAL_USA_ULTRASSONICO_FRONTAL
    Serial.print("[*] Inicializando os 2 VL53L0X Laterais...");
    vl53_ok = vl53l0x_manager_init();
    Serial.println(vl53_ok ? " [OK]" : " [AVISO: Um ou mais ToF laterais não responderam]");

    Serial.print("[*] Inicializando Ultrassônico Frontal (TRIG=16, ECHO=19)...");
    ultra_ok = ultrasonic_init();
    Serial.println(ultra_ok ? " [OK]" : " [AVISO: Ultrassônico frontal não respondeu]");
#elif CAL_MODO_SENSORES == 2
    Serial.print("[*] Inicializando 2 Sensores VL53L0X (Esquerdo + Frontal)...");
    vl53_ok = vl53l0x_manager_init();
    Serial.println(vl53_ok ? " [OK]" : " [AVISO: Um ou mais sensores não responderam]");
#else
    Serial.print("[*] Inicializando os 3 Sensores VL53L0X...");
    vl53_ok = vl53l0x_manager_init();
    Serial.println(vl53_ok ? " [OK]" : " [AVISO: Um ou mais sensores não responderam]");
#endif

    // 3. Inicializa o rádio ESP-NOW para conexão com o Gateway ESP8266 (Website)
    Serial.print("[*] Inicializando Rádio ESP-NOW no Canal 1 (Para Gateway ESP8266)...");
    radio_ok = radio_telemetria_init();
    Serial.println(radio_ok ? " [PRONTO (1 Hz Broadcast)]" : " [FALHA NO RÁDIO]");

    exibirMenuPrincipal();
}

void loop() {
    uint32_t now = millis();

    // --- 1. TRANSMISSÃO DE TELEMETRIA ESP-NOW A 1 Hz (PARA O GATEWAY ESP8266 / SITE) ---
    if (now - last_espnow_tx_ms >= ESPNOW_TX_INTERVAL_MS) {
        last_espnow_tx_ms = now;
        enviarTelemetriaEspNow();
    }

    if (modoAtual == MODO_CALIBRACAO) {
        if (Serial.available()) {
            String linha = Serial.readStringUntil('\n');
            processarLinhaCalibracao(linha);
        }
        return;
    }

    // --- 2. PROCESSAMENTO DE COMANDOS (MONITOR SERIAL OU ESP-NOW REMOTO) ---
    char cmd = 0;
    bool temCmd = false;

    if (Serial.available()) {
        cmd = (char)Serial.read();
        temCmd = true;
    } else if (radio_telemetria_ler_comando(&cmd)) {
        Serial.printf("\n[ESP-NOW REMOTO] Comando recebido pelo rádio: '%c'\n", cmd);
        temCmd = true;
    }

    if (temCmd) {

        if (modoAtual == MODO_TESTE_MOTORES_MANUAL) {
            processarComandoMotorManual(cmd);
            return;
        }

        if (modoAtual == MODO_TESTE_SENSORES || modoAtual == MODO_TESTE_ENCODERS_MANUAL || modoAtual == MODO_TELEMETRIA_CONTINUA) {
            if (cmd == 'x' || cmd == 'X' || cmd == 'm' || cmd == 'M' || cmd == '\n' || cmd == '\r') {
                modoAtual = MODO_MENU;
                tb6612_stop();
                exibirMenuPrincipal();
                return;
            }
        }

        switch (cmd) {
            case '1':
                Serial.println("\n[INICIANDO TESTE 1] Leitura dos Sensores ToF. Pressione 'x' para sair.\n");
                modoAtual = MODO_TESTE_SENSORES;
                break;
            case '2':
                modoAtual = MODO_TESTE_MOTORES_MANUAL;
                exibirMenuMotoresManual();
                break;
            case '3':
                Serial.println("\n[INICIANDO TESTE 3] Encoders. Gire as rodas na mão! Pressione 'x' para sair.\n");
                encoder_reset();
                modoAtual = MODO_TESTE_ENCODERS_MANUAL;
                break;
            case '4':
                executarAvancoUmaCelula();
                break;
            case '5':
                executarCurva90Graus(true); // Direita
                break;
            case '6':
                executarCurva90Graus(false); // Esquerda
                break;
            case '7':
                executarMeiaVolta180();
                break;
            case '8':
                executarAvancoAntiColisao();
                break;
            case '9':
                executarRelatorioBateria();
                break;
            case 'k':
            case 'K':
                executarCalibracaoGuiadaToF();
                break;
            case 'd':
            case 'D':
                imprimirDiagnosticoRapido();
                break;
            case 'i':
            case 'I':
                executarAnalisePortasI2C();
                break;
            case 'c':
            case 'C':
                modoAtual = MODO_CALIBRACAO;
                exibirAjudaCalibracao();
                break;
            case 'g':
            case 'G':
                if (geometriaAtual == LAB_4X4) geometriaAtual = LAB_8X4;
                else if (geometriaAtual == LAB_8X4) geometriaAtual = LAB_12X4;
                else geometriaAtual = LAB_4X4;
                Serial.printf("\n[GEOMETRIA] Labirinto alterado para: %s\n",
                              (geometriaAtual == LAB_4X4) ? "4x4" : (geometriaAtual == LAB_8X4) ? "8x4" : "12x4");
                enviarTelemetriaEspNow();
                exibirMenuPrincipal();
                break;
            case 'w':
            case 'W':
                enviarTelemetriaEspNow();
                Serial.println("\n[ESP-NOW] Pacote de telemetria de 18 bytes transmitido agora!");
                Serial.println("  -> Verifique no Gateway ESP8266 a emissão da linha JSON.");
                break;
            case '0':
                Serial.println("\n[MODO TELEMETRIA GERAL] Pressione 'x' para voltar ao menu.\n");
                modoAtual = MODO_TELEMETRIA_CONTINUA;
                break;
            case 's':
            case 'S':
                tb6612_brake();
                Serial.println("\n[PARADA DE EMERGÊNCIA] Motores travados.");
                break;
            case 'm':
            case 'M':
            case 'h':
            case 'H':
                exibirMenuPrincipal();
                break;
            default:
                break;
        }
    }

    // --- 3. EXECUÇÃO PERIÓDICA DOS MODOS CONTÍNUOS NA SERIAL (10 Hz) ---
    if (now - last_telemetry_serial_ms >= TELEMETRY_SERIAL_INTERVAL_MS) {
        last_telemetry_serial_ms = now;

        if (modoAtual == MODO_TESTE_SENSORES) {
            executarLoopTesteSensores();
        } else if (modoAtual == MODO_TESTE_ENCODERS_MANUAL) {
            executarLoopTesteEncoders();
        } else if (modoAtual == MODO_TELEMETRIA_CONTINUA) {
            VL53L0X_Readings_t tof = lerSensoresUnificados();
            BatteryStatus_t bat = battery_monitor_read();
            int32_t tl = encoder_get_ticks_left();
            int32_t tr = encoder_get_ticks_right();

            Serial.printf("[TELEMETRIA] ToF(E/F/D): [%4d | %4d | %4d mm] | Enc: [L:%5d | R:%5d] | Bat: %4.2fV (%3d%%) | Rádio: [OK 1Hz]\n",
                          tof.left_valid ? tof.dist_left_mm : -1,
                          tof.front_valid ? tof.dist_front_mm : -1,
                          tof.right_valid ? tof.dist_right_mm : -1,
                          (int)tl, (int)tr, bat.voltage_volts, bat.percentage);
        }

        // Pisca LED indicador externo
        digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
    }

    if (modoAtual == MODO_TESTE_MOTORES_MANUAL &&
        (manualDirL != 0 || manualDirR != 0) &&
        (now - last_manual_log_ms >= 500)) {
        last_manual_log_ms = now;
        Serial.printf("[MODO 2] PWM=%d | Ticks Esq=%ld | Dir=%ld\n",
                      (int)calPwmManual,
                      (long)encoder_get_ticks_left(),
                      (long)encoder_get_ticks_right());
    }

    // --- 4. BOTÃO FÍSICO DE LARGADA (GPIO 15) ---
    if (digitalRead(PIN_BUTTON_START) == LOW) {
        delay(50); // Debounce
        if (digitalRead(PIN_BUTTON_START) == LOW) {
            Serial.println("\n[BOTÃO FÍSICO DETECTADO] Disparando avanço de 1 célula (168 mm)!");
            executarAvancoUmaCelula();
            while (digitalRead(PIN_BUTTON_START) == LOW) delay(10);
        }
    }
}
