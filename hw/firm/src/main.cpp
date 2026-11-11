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
    Serial.println("  [1] - Teste de Sensores VL53L0X (Leitura contínua em mm)");
    Serial.println("  [2] - Teste Manual de Motores (Esquerdo/Direito passo a passo)");
    Serial.println("  [3] - Teste de Encoders Manual (Gire as rodas na mão)");
    Serial.println("  [4] - Teste de Movimento: Avançar 1 Célula (168 mm) e Parar");
    Serial.println("  [5] - Teste de Curva: Giro de 90° Direita no próprio eixo");
    Serial.println("  [6] - Teste de Curva: Giro de 90° Esquerda no próprio eixo");
    Serial.println("  [7] - Teste de Manobra: Meia-Volta de 180° no próprio eixo");
    Serial.println("  [8] - Teste Integrado: Avanço com Freio Anti-Colisão (< 100 mm)");
    Serial.println("  [9] - Calibração de Bateria (Leitura ADC bruta e tensão)");
    Serial.printf( "  [g] - Alternar Geometria do Labirinto (Atual: %s -> 4x4, 8x4, 12x4)\n", strGeo);
    Serial.println("  [w] - Disparar Pacote ESP-NOW de Teste (Para o Gateway ESP8266)");
    Serial.println("  [0] - Modo Telemetria Contínua (Visão geral em tempo real)");
    Serial.println("  [s] - Parada Imediata de Emergência (Freio ativo)");
    Serial.println("  [m] - Exibir este menu novamente");
    Serial.println("=======================================================");
    Serial.printf("Status: ToF=%s | Rádio ESP-NOW=%s (Canal 1) | Geometria=%s\n",
                  vl53_ok ? "OK" : "FALHA", radio_ok ? "PRONTO" : "ERRO", strGeo);
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

    uint32_t start_time = millis();
    uint32_t last_tof_ctrl = 0;
    bool timeout = false;
    bool parada_frontal = false;

    // Parâmetros de controle refinado (PD Lateral + Modo Monoparede)
    const float KP_PAREDE = 0.35f;
    const float KD_PAREDE = 0.20f;  // Amortece o arrasto da roda boba e mata oscilação
    const float KP_ENCODER = 0.50f;
    const float DIST_IDEAL_CENTRO_MM = 55.0f; // Distância ideal até a parede lateral no centro da célula

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
            VL53L0X_Readings_t tof = vl53l0x_manager_read();

            // Encosto frontal óptico: se detectou parede à frente a <= 45 mm, freia imediatamente
            if (tof.front_valid && tof.dist_front_mm <= 45 && media > (target_ticks / 3)) {
                parada_frontal = true;
                break;
            }

            // Rampa suave de partida nos primeiros 120 ms (evita tranco na roda boba e patinação)
            int16_t pwm_base = PWM_TESTE;
            uint32_t tempo_corrido = now - start_time;
            if (tempo_corrido < 120) {
                pwm_base = map(tempo_corrido, 0, 120, 50, PWM_TESTE);
            }

            int16_t pwm_l = pwm_base;
            int16_t pwm_r = pwm_base;

            bool temEsq = (tof.left_valid && tof.dist_left_mm < 130);
            bool temDir = (tof.right_valid && tof.dist_right_mm < 130);

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
                float erro_lateral = (float)tof.dist_left_mm - DIST_IDEAL_CENTRO_MM;
                int16_t correcao = (int16_t)(KP_PAREDE * erro_lateral * 1.5f);
                correcao = constrain(correcao, -20, 20);
                pwm_l -= correcao;
                pwm_r += correcao;
            } else if (temDir) {
                // Caso C: Modo Monoparede Direita (Abertura ou curva à esquerda)
                float erro_lateral = DIST_IDEAL_CENTRO_MM - (float)tof.dist_right_mm;
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

    Serial.printf("Resultado: %s\n", timeout ? "[TIMEOUT DE SEGURANÇA!]" : (parada_frontal ? "[ENCOSTO FRONTAL ÓPTICO A 45 mm]" : "[CHEGOU NA DISTÂNCIA ALVO POR ENCODERS]"));
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

    uint32_t start_time = millis();
    bool timeout = false;
    const int16_t PWM_RAMPA = 55; // PWM reduzido para frenagem suave sem derrapagem
    const float KP_GIRO_SYNC = 0.5f;

    while (true) {
        uint32_t now = millis();
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        int32_t media = (tl + tr) / 2;

        if (media >= target_ticks) {
            break;
        }

        if (now - start_time > 2500) {
            timeout = true;
            break;
        }

        // 1. Rampa de desaceleração nos últimos 20% da curva (elimina inércia de giro)
        int16_t pwm_base = (media > (target_ticks * 0.80f)) ? PWM_RAMPA : PWM_TESTE;

        // 2. Equalização entre rodas para giro perfeito sobre o próprio eixo (sem translação)
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

    uint32_t start_time = millis();
    const int16_t PWM_RAMPA = 55;
    const float KP_GIRO_SYNC = 0.5f;

    while (true) {
        uint32_t now = millis();
        int32_t tl = abs(encoder_get_ticks_left());
        int32_t tr = abs(encoder_get_ticks_right());
        int32_t media = (tl + tr) / 2;

        if (media >= target_ticks || (now - start_time > 4000)) break;

        int16_t pwm_base = (media > (target_ticks * 0.85f)) ? PWM_RAMPA : PWM_TESTE;
        int32_t diff = tl - tr;
        int16_t ajuste = (int16_t)(diff * KP_GIRO_SYNC);
        ajuste = constrain(ajuste, -15, 15);

        tb6612_set_motors(pwm_base - ajuste, -(pwm_base + ajuste));
        delay(3);
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

    // --- 2. PROCESSAMENTO DE COMANDOS (MONITOR SERIAL OU ESP-NOW REMOTO) ---
    char cmd = 0;
    bool temCmd = false;

    if (Serial.available()) {
        cmd = (char)Serial.read();
        temCmd = true;
    } else if (radio_telemetria_ler_comando(&cmd)) {
        Serial.printf("
[ESP-NOW REMOTO] Comando recebido pelo rádio: '%c'
", cmd);
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
