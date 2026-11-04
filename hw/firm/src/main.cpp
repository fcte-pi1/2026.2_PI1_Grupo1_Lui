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
 *    - Motor 1 (Esquerdo): Canal A -> GPIO 36 (VP), Canal B -> GPIO 39 (VN)
 *    - Motor 2 (Direito) : Canal A -> GPIO 35,      Canal B -> GPIO 2 (Pull-up)
 *  [SENSORES VL53L0X]
 *    - Barramento I2C: SDA -> GPIO 21, SCL -> GPIO 22
 *    - XSHUT Sensor 1 (Esquerdo): GPIO 4  (Endereço: 0x30)
 *    - XSHUT Sensor 2 (Frontal) : GPIO 16 (Endereço: 0x31)
 *    - XSHUT Sensor 3 (Direito) : GPIO 17 (Endereço: 0x32)
 *  [BATERIA]
 *    - Leitura ADC Divisor: GPIO 34 (ADC1_CH6)
 *  [RÁDIO ESP-NOW (TELEMETRIA PARA O SITE)]
 *    - Canal 1 Wi-Fi (2.4 GHz) | Broadcast: FF:FF:FF:FF:FF:FF (18 bytes a 1 Hz)
 *  [IHM / LIVRES]
 *    - Botão de Largada / Teste: GPIO 15 (Pull-Up interno)
 *    - LED de Sinalização / Livre: GPIO 14
 * ==============================================================================
 */

#include <Arduino.h>
#include "pinout.h"
#include "tb6612_driver.h"
#include "encoder_driver.h"
#include "vl53l0x_manager.h"
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
    MODO_TESTE_MOTORES_MANUAL
};

static ModoOperacao modoAtual = MODO_MENU;
static EstadoRobo estadoRobo = AGUARDANDO;

// Temporizadores não-bloqueantes
static uint32_t last_telemetry_serial_ms = 0;
static const uint32_t TELEMETRY_SERIAL_INTERVAL_MS = 100; // 10 Hz no monitor serial

static uint32_t last_espnow_tx_ms = 0;
static const uint32_t ESPNOW_TX_INTERVAL_MS = 1000;       // 1 Hz via rádio para o site (RNF-SFT03)

static uint32_t tempoInicioProva = 0;
static int32_t last_calc_ticks = 0;
static uint32_t last_calc_time_ms = 0;

static bool vl53_ok = false;
static bool radio_ok = false;

// Velocidade base para testes de movimentação (0 a 255)
static const int16_t PWM_TESTE = 90; // ~35% PWM: seguro, suave e com bom torque

// Monta e dispara pacote de telemetria binário de 18 bytes via ESP-NOW para o ESP8266 Gateway
void enviarTelemetriaEspNow(void) {
    if (!radio_ok) return;

    VL53L0X_Readings_t tof = vl53l0x_manager_read();
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

    pkt.labirinto_id = LAB_4X4;
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

    pkt.coluna = 0; // Coluna 'A' (índice 0)
    pkt.linha = 0;  // Linha '1'  (índice 0, no gateway vira linha + 1 = 1)
    pkt.orientacao = 0; // 0 = NORTE

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
    Serial.println("\n=======================================================");
    Serial.println("     MICROMOUSE PI1 2026.2 - MENU DE TESTES GRADUAIS   ");
    Serial.println("=======================================================");
    Serial.println("Escolha o subsistema que deseja testar:");
    Serial.println("  [1] - Teste de Sensores VL53L0X (Leitura contínua em mm)");
    Serial.println("  [2] - Teste Manual de Motores (Esquerdo/Direito passo a passo)");
    Serial.println("  [3] - Teste de Encoders Manual (Gire as rodas na mão)");
    Serial.println("  [4] - Teste de Movimento: Avançar 1 Célula (168 mm) e Parar");
    Serial.println("  [5] - Teste de Curva: Giro de 90° Direita no próprio eixo");
    Serial.println("  [6] - Teste de Curva: Giro de 90° Esquerda no próprio eixo");
    Serial.println("  [7] - Teste de Manobra: Meia-Volta de 180° no próprio eixo");
    Serial.println("  [8] - Teste Integrado: Avanço com Freio Anti-Colisão (< 100 mm)");
    Serial.println("  [9] - Calibração de Bateria (Leitura ADC bruta e tensão)");
    Serial.println("  [w] - Disparar Pacote ESP-NOW de Teste (Para o Gateway ESP8266)");
    Serial.println("  [0] - Modo Telemetria Contínua (Visão geral em tempo real)");
    Serial.println("  [s] - Parada Imediata de Emergência (Freio ativo)");
    Serial.println("  [m] - Exibir este menu novamente");
    Serial.println("=======================================================");
    Serial.printf("Status: ToF=%s | Rádio ESP-NOW=%s (Canal 1)\n",
                  vl53_ok ? "OK" : "FALHA", radio_ok ? "PRONTO" : "ERRO");
    Serial.print("Digite a opção desejada: ");
}

// ------------------------------------------------------------------------------
// TESTE 1: LEITURA CONTÍNUA DOS SENSORES VL53L0X
// ------------------------------------------------------------------------------
void executarLoopTesteSensores(void) {
    VL53L0X_Readings_t tof = vl53l0x_manager_read();

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
// TESTE 2: CONTROLE MANUAL DOS MOTORES
// ------------------------------------------------------------------------------
void exibirMenuMotoresManual(void) {
    Serial.println("\n--- [MODO 2] TESTE MANUAL DE MOTORES ---");
    Serial.println("Comandos no teclado:");
    Serial.println("  [e] - Motor Esquerdo FRENTE  (500 ms) | [E] - Motor Esquerdo RÉ (500 ms)");
    Serial.println("  [d] - Motor Direito  FRENTE  (500 ms) | [D] - Motor Direito  RÉ (500 ms)");
    Serial.println("  [f] - Ambos os motores FRENTE (500 ms)");
    Serial.println("  [t] - Ambos os motores RÉ    (500 ms)");
    Serial.println("  [s] ou [espaço] - Freio ativo");
    Serial.println("  [x] - Sair deste teste e voltar ao menu principal");
    Serial.println("----------------------------------------");
}

void processarComandoMotorManual(char c) {
    switch (c) {
        case 'e':
            Serial.println("-> Acionando Motor Esquerdo FRENTE...");
            tb6612_set_motor_left(PWM_TESTE);
            delay(500);
            tb6612_brake();
            Serial.printf("   Ticks Esq: %d\n", encoder_get_ticks_left());
            break;
        case 'E':
            Serial.println("-> Acionando Motor Esquerdo RÉ...");
            tb6612_set_motor_left(-PWM_TESTE);
            delay(500);
            tb6612_brake();
            Serial.printf("   Ticks Esq: %d\n", encoder_get_ticks_left());
            break;
        case 'd':
            Serial.println("-> Acionando Motor Direito FRENTE...");
            tb6612_set_motor_right(PWM_TESTE);
            delay(500);
            tb6612_brake();
            Serial.printf("   Ticks Dir: %d\n", encoder_get_ticks_right());
            break;
        case 'D':
            Serial.println("-> Acionando Motor Direito RÉ...");
            tb6612_set_motor_right(-PWM_TESTE);
            delay(500);
            tb6612_brake();
            Serial.printf("   Ticks Dir: %d\n", encoder_get_ticks_right());
            break;
        case 'f':
            Serial.println("-> Ambos os motores FRENTE...");
            tb6612_set_motors(PWM_TESTE, PWM_TESTE);
            delay(500);
            tb6612_brake();
            Serial.printf("   Ticks: Esq=%d | Dir=%d\n", encoder_get_ticks_left(), encoder_get_ticks_right());
            break;
        case 't':
            Serial.println("-> Ambos os motores RÉ...");
            tb6612_set_motors(-PWM_TESTE, -PWM_TESTE);
            delay(500);
            tb6612_brake();
            Serial.printf("   Ticks: Esq=%d | Dir=%d\n", encoder_get_ticks_left(), encoder_get_ticks_right());
            break;
        case 's':
        case ' ':
            tb6612_brake();
            Serial.println("-> Freio ativado.");
            break;
        case 'x':
        case 'X':
            tb6612_stop();
            modoAtual = MODO_MENU;
            exibirMenuPrincipal();
            break;
        default:
            break;
    }
}

// ------------------------------------------------------------------------------
// TESTE 3: ENCODERS MANUAL (GIRANDO NA MÃO)
// ------------------------------------------------------------------------------
void executarLoopTesteEncoders(void) {
    int32_t tl = encoder_get_ticks_left();
    int32_t tr = encoder_get_ticks_right();
    float dl = encoder_get_dist_left_mm();
    float dr = encoder_get_dist_right_mm();

    Serial.printf("[ENCODER MANUAL] Ticks Esq: %5d (%5.1f mm) | Ticks Dir: %5d (%5.1f mm) | [Pressione 'x' p/ sair]\n",
                  tl, dl, tr, dr);
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

    int32_t target_ticks = (int32_t)((CELL_SIZE_MM / WHEEL_CIRCUMFERENCE_MM) * ENCODER_TICKS_PER_REV);
    Serial.printf("Distância alvo: %.1f mm (~%d ticks por roda)\n", CELL_SIZE_MM, target_ticks);

    tb6612_set_motors(PWM_TESTE, PWM_TESTE);

    uint32_t start_time = millis();
    bool timeout = false;

    while (true) {
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        int32_t media = (tl + tr) / 2;

        if (media >= target_ticks) {
            break;
        }

        if (millis() - start_time > 3000) {
            timeout = true;
            break;
        }

        delay(5);
    }

    tb6612_brake();
    delay(200);
    tb6612_stop();

    estadoRobo = AGUARDANDO;
    enviarTelemetriaEspNow(); // Notifica o site da parada

    Serial.printf("Resultado: %s\n", timeout ? "[TIMEOUT DE SEGURANÇA!]" : "[CHEGOU NA DISTÂNCIA ALVO]");
    Serial.printf("  Ticks finais: Esq=%d (%.1f mm) | Dir=%d (%.1f mm)\n",
                  encoder_get_ticks_left(), encoder_get_dist_left_mm(),
                  encoder_get_ticks_right(), encoder_get_dist_right_mm());
    Serial.println("-------------------------------------------------------\n");
    exibirMenuPrincipal();
}

// ------------------------------------------------------------------------------
// TESTE 5 & 6: CURVA DE 90° (DIREITA / ESQUERDA) NO PRÓPRIO EIXO
// ------------------------------------------------------------------------------
void executarCurva90Graus(bool paraDireita) {
    Serial.printf("\n>>> [TESTE %s] GIRO DE 90° PARA A %s <<<\n",
                  paraDireita ? "5" : "6", paraDireita ? "DIREITA" : "ESQUERDA");
    Serial.println("Iniciando giro em 2 segundos...");
    delay(2000);

    tempoInicioProva = millis();
    estadoRobo = MAPEANDO;
    enviarTelemetriaEspNow();

    encoder_reset();

    float arco_mm = (3.14159265f * TRACK_WIDTH_MM) / 4.0f;
    int32_t target_ticks = (int32_t)((arco_mm / WHEEL_CIRCUMFERENCE_MM) * ENCODER_TICKS_PER_REV);

    Serial.printf("Arco da roda: %.1f mm (~%d ticks)\n", arco_mm, target_ticks);

    if (paraDireita) {
        tb6612_set_motors(PWM_TESTE, -PWM_TESTE);
    } else {
        tb6612_set_motors(-PWM_TESTE, PWM_TESTE);
    }

    uint32_t start_time = millis();
    bool timeout = false;

    while (true) {
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        int32_t media = (tl + tr) / 2;

        if (media >= target_ticks) {
            break;
        }

        if (millis() - start_time > 2000) {
            timeout = true;
            break;
        }
        delay(5);
    }

    tb6612_brake();
    delay(200);
    tb6612_stop();

    estadoRobo = AGUARDANDO;
    enviarTelemetriaEspNow();

    Serial.printf("Resultado: %s\n", timeout ? "[TIMEOUT]" : "[GIRO CONCLUÍDO]");
    Serial.printf("  Ticks: Esq=%d | Dir=%d\n", encoder_get_ticks_left(), encoder_get_ticks_right());
    Serial.println("DICA: Se girou mais ou menos de 90 graus, ajuste TRACK_WIDTH_MM em pinout.h!\n");
    exibirMenuPrincipal();
}

// ------------------------------------------------------------------------------
// TESTE 7: MEIA VOLTA 180°
// ------------------------------------------------------------------------------
void executarMeiaVolta180(void) {
    Serial.println("\n>>> [TESTE 7] GIRO DE 180° (MEIA VOLTA) <<<");
    Serial.println("Iniciando em 2 segundos...");
    delay(2000);

    tempoInicioProva = millis();
    estadoRobo = MAPEANDO;
    enviarTelemetriaEspNow();

    encoder_reset();

    float arco_mm = (3.14159265f * TRACK_WIDTH_MM) / 2.0f;
    int32_t target_ticks = (int32_t)((arco_mm / WHEEL_CIRCUMFERENCE_MM) * ENCODER_TICKS_PER_REV);

    Serial.printf("Arco: %.1f mm (~%d ticks)\n", arco_mm, target_ticks);

    tb6612_set_motors(PWM_TESTE, -PWM_TESTE);

    uint32_t start_time = millis();
    while (true) {
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        if ((tl + tr) / 2 >= target_ticks || (millis() - start_time > 3500)) break;
        delay(5);
    }

    tb6612_brake();
    delay(200);
    tb6612_stop();

    estadoRobo = AGUARDANDO;
    enviarTelemetriaEspNow();

    Serial.println("Meia volta finalizada!\n");
    exibirMenuPrincipal();
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
    tb6612_set_motors(PWM_TESTE - 15, PWM_TESTE - 15);

    uint32_t start_time = millis();
    bool obstaculo = false;
    uint16_t dist_final = 0;

    while (millis() - start_time < 5000) {
        VL53L0X_Readings_t tof = vl53l0x_manager_read();

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
    for (int i = 0; i < 32; i++) {
        raw_sum += analogRead(PIN_BATTERY_ADC);
        delay(5);
    }
    float raw_avg = (float)raw_sum / 32.0f;
    float v_pin = (raw_avg / 4095.0f) * BATTERY_ADC_VREF;
    float v_bat = v_pin * BATTERY_DIVIDER_RATIO;
    uint8_t pct = battery_voltage_to_percentage(v_bat);

    Serial.printf("  - Leitura bruta ADC (0-4095) : %.1f\n", raw_avg);
    Serial.printf("  - Tensão no pino GPIO 34     : %.3f V\n", v_pin);
    Serial.printf("  - Fator do divisor aplicado  : %.2f x\n", BATTERY_DIVIDER_RATIO);
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

    // 1. Inicializa drivers de hardware
    tb6612_init();
    tb6612_stop();
    encoder_init();
    battery_monitor_init();

    // 2. Inicializa os 3 sensores VL53L0X
    Serial.print("[*] Inicializando os 3 Sensores VL53L0X...");
    vl53_ok = vl53l0x_manager_init();
    Serial.println(vl53_ok ? " [OK]" : " [AVISO: Um ou mais sensores não responderam]");

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

    // --- 2. PROCESSAMENTO DE COMANDOS VIA MONITOR SERIAL ---
    if (Serial.available()) {
        char cmd = (char)Serial.read();

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
            VL53L0X_Readings_t tof = vl53l0x_manager_read();
            BatteryStatus_t bat = battery_monitor_read();
            int32_t tl = encoder_get_ticks_left();
            int32_t tr = encoder_get_ticks_right();

            Serial.printf("[TELEMETRIA] ToF(E/F/D): [%4d | %4d | %4d mm] | Enc: [L:%5d | R:%5d] | Bat: %4.2fV (%3d%%) | Rádio: [OK 1Hz]\n",
                          tof.left_valid ? tof.dist_left_mm : -1,
                          tof.front_valid ? tof.dist_front_mm : -1,
                          tof.right_valid ? tof.dist_right_mm : -1,
                          tl, tr, bat.voltage_volts, bat.percentage);
        }

        // Pisca LED indicador externo
        digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
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
