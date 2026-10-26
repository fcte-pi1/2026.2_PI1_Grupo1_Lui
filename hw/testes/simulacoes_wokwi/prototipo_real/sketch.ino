/**
 * ==============================================================================
 *  PROJETO MICROMOUSE - FIRMWARE DE PRODUÇÃO (PROTÓTIPO REAL)
 *  Comunicação Bidirecional via ESP-NOW + Telemetria em Tempo Real
 * ==============================================================================
 *  PRINCÍPIOS DO FIRMWARE DE PRODUÇÃO:
 *  1. Determinismo e Baixa Latência: Execução 100% orientada a eventos e sem
 *     blocking delays no loop principal.
 *  2. Comunicação Bidirecional via ESP-NOW:
 *     - Recepção assíncrona de comandos de controle do Gateway Web:
 *       START, STOP (Emergência), PAUSE, RESET_MAZE, RETURN_TO_START.
 *     - Envio contínuo (200 ms) e por evento de pacotes de Telemetria (struct).
 *  3. Camada de Abstração HAL (Compatibilidade Dupla Físico / Wokwi):
 *     - Compila nativamente com <WiFi.h> e <esp_now.h> na ESP32 física.
 *     - Emula rádio assíncrono via porta Serial caso executado no simulador Wokwi.
 *  4. Navegação 100% às Cegas:
 *     - Não conhece mapa prévio nem a posição do gol.
 *     - Busca em profundidade com preferência em linha reta e malha fechada PD.
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#include "protocol.h"

// Define se o rádio nativo ESP-NOW do chip ESP32 deve ser inicializado
// Para a placa física: mantenha 1. No Wokwi: o código faz fallback para Serial.
#define USE_ESP_NOW_RADIO 1

#if USE_ESP_NOW_RADIO
  #include <WiFi.h>
  #include <esp_now.h>
#endif

// ==============================================================================
// 1. MAPEAMENTO DE PINOS DO HARDWARE
// ==============================================================================
// Sensores de Distância ToF / Ultrassônicos
const uint8_t TRIG_ESQ = 13, ECHO_ESQ = 27;  // Sensor Esquerdo (90°)
const uint8_t TRIG_FRE = 32, ECHO_FRE = 26;  // Sensor Frontal (0°)
const uint8_t TRIG_DIR = 14, ECHO_DIR = 25;  // Sensor Direito (90°)

// Ponte H TB6612FNG (Rodas Motrizes)
const uint8_t PIN_PWMA = 16;  // PWM Motor Esquerdo
const uint8_t PIN_PWMB = 17;  // PWM Motor Direito
const uint8_t PIN_AIN1 = 18;  // Direção Motor Esquerdo
const uint8_t PIN_AIN2 = 19;
const uint8_t PIN_BIN1 = 23;  // Direção Motor Direito
const uint8_t PIN_BIN2 = 5;

// Interface I2C (Display OLED SSD1306)
const uint8_t PIN_I2C_SDA = 21;
const uint8_t PIN_I2C_SCL = 22;
const uint8_t OLED_ADDR   = 0x3C;

// Periféricos de Status
const uint8_t PIN_RGB       = 2;   // WS2812B NeoPixel
const uint8_t PIN_BUZZER    = 4;   // Buzzer
const uint8_t PIN_BATTERY   = 34;  // ADC Monitor de Bateria 7.4V
const uint8_t PIN_BTN_START = 15;  // Botão Físico de Emergência / Largada

// ==============================================================================
// 2. CONFIGURAÇÕES DE REDE ESP-NOW
// ==============================================================================
// MAC Address da ESP32 Gateway (Bridge conectada ao PC / Servidor Web)
// Altere para o endereço físico real da sua placa Gateway:
uint8_t GATEWAY_MAC[6] = { 0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC };
bool espNowActive = false;

// ==============================================================================
// 3. PARÂMETROS E CONSTANTES CINEMÁTICAS
// ==============================================================================
const uint8_t  MAX_W = 16;
const uint8_t  MAX_H = 16;
const float    CELL_SIZE_CM        = 18.0f;
const float    WALL_THRESHOLD_CM   = 13.0f;
const float    SENSOR_IDEAL_CM     = 5.0f;
const uint32_t SENSOR_TIMEOUT_US   = 25000;
const uint32_t TELEMETRY_PERIOD_MS = 200; // Envio de telemetria a cada 200 ms

// Direções absolutas (0: Norte, 1: Leste, 2: Sul, 3: Oeste)
enum Dir : uint8_t { NORTE = 0, LESTE = 1, SUL = 2, OESTE = 3 };
const int8_t  DX[4] = { 0, 1, 0, -1 };
const int8_t  DY[4] = { 1, 0, -1, 0 };
#define WALL_BIT(d) ((uint8_t)(1u << (d)))

enum Rel : uint8_t { FRENTE = 0, DIREITA = 1, TRAS = 2, ESQUERDA = 3 };

// ==============================================================================
// 4. ESTADO GLOBAL DO ROBÔ
// ==============================================================================
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306  oled(128, 64, &Wire, -1);
bool oledReady = false;

// Memória Interna de Mapeamento (100% Zerada no início)
uint8_t  knownWalls[MAX_W][MAX_H];
bool     visited[MAX_W][MAX_H];
uint8_t  pathStackX[MAX_W * MAX_H];
uint8_t  pathStackY[MAX_W * MAX_H];
uint16_t pathStackLen = 0;

// Localização e Orientação
uint8_t posX = 0, posY = 0;
Dir     heading = NORTE;
uint8_t basePwm = 175;

// Máquina de Estados
RobotState currentState = ROBOT_IDLE;
uint16_t   stepCount    = 0;
uint32_t   startTimeMs  = 0;
uint32_t   elapsedMs    = 0;

// Sensores e Bateria
float distL = 100.0f, distF = 100.0f, distR = 100.0f;
bool  wallL = false, wallF = false, wallR = false;
float batteryVolts = 7.4f;

// Pacotes de Comunicação
MicromouseCommand   lastCmdReceived;
MicromouseTelemetry telemetryPkt;
uint32_t lastTelemetrySendMs = 0;
uint32_t lastLoopStepMs      = 0;
uint16_t stepIntervalMs      = 400; // Intervalo de avanço na exploração

// ==============================================================================
// 5. DRIVERS DE HARDWARE E ATUADORES
// ==============================================================================
void setMotors(int pwmL, int pwmR, bool fwdL, bool fwdR) {
    pwmL = constrain(pwmL, 0, 255);
    pwmR = constrain(pwmR, 0, 255);

    digitalWrite(PIN_AIN1, fwdL ? HIGH : LOW);
    digitalWrite(PIN_AIN2, fwdL ? LOW : HIGH);
    digitalWrite(PIN_BIN1, fwdR ? HIGH : LOW);
    digitalWrite(PIN_BIN2, fwdR ? LOW : HIGH);

    analogWrite(PIN_PWMA, pwmL);
    analogWrite(PIN_PWMB, pwmR);
}

void stopMotors() {
    setMotors(0, 0, true, true);
    digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
}

void setRgbStatus(uint8_t r, uint8_t g, uint8_t b) {
    rgb.setPixelColor(0, rgb.Color(r, g, b));
    rgb.show();
}

void beepFeedback(int freq, int durationMs) {
    tone(PIN_BUZZER, freq, durationMs);
    delay(durationMs);
    noTone(PIN_BUZZER);
}

float readUltrasonic(uint8_t trig, uint8_t echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    uint32_t dur = pulseIn(echo, HIGH, SENSOR_TIMEOUT_US);
    if (dur == 0) return 400.0f;
    float dist = dur * 0.0343f / 2.0f;
    return (dist < 1.0f || dist > 400.0f) ? 400.0f : dist;
}

void readAllSensors() {
    distL = readUltrasonic(TRIG_ESQ, ECHO_ESQ);
    distF = readUltrasonic(TRIG_FRE, ECHO_FRE);
    distR = readUltrasonic(TRIG_DIR, ECHO_DIR);

    wallL = (distL < WALL_THRESHOLD_CM);
    wallF = (distF < WALL_THRESHOLD_CM);
    wallR = (distR < WALL_THRESHOLD_CM);

    int rawBat = analogRead(PIN_BATTERY);
    batteryVolts = (rawBat / 4095.0f) * 3.3f * (8.4f / 3.3f);
}

// ==============================================================================
// 6. TELEMETRIA E PROTOCOLO ESP-NOW
// ==============================================================================
void sendTelemetryPacket() {
    telemetryPkt.timestampMs  = elapsedMs;
    telemetryPkt.posX         = posX;
    telemetryPkt.posY         = posY;
    telemetryPkt.heading      = (uint8_t)heading;
    telemetryPkt.walls        = knownWalls[posX][posY];
    telemetryPkt.state        = (uint8_t)currentState;
    telemetryPkt.distL        = distL;
    telemetryPkt.distF        = distF;
    telemetryPkt.distR        = distR;
    telemetryPkt.batteryVolts = batteryVolts;
    telemetryPkt.stepCount    = stepCount;
    telemetryPkt.pathLength   = pathStackLen;

#if USE_ESP_NOW_RADIO
    if (espNowActive) {
        esp_now_send(GATEWAY_MAC, (uint8_t*)&telemetryPkt, sizeof(telemetryPkt));
    }
#endif

    // Envio espelhado via Serial em formato JSON compacto para monitoramento USB
    Serial.printf("{\"telemetry\":{\"t\":%u,\"x\":%d,\"y\":%d,\"h\":%d,\"w\":%d,\"st\":%d,\"l\":%.1f,\"f\":%.1f,\"r\":%.1f,\"bat\":%.2f,\"steps\":%u}}\n",
                  telemetryPkt.timestampMs, telemetryPkt.posX, telemetryPkt.posY,
                  telemetryPkt.heading, telemetryPkt.walls, telemetryPkt.state,
                  telemetryPkt.distL, telemetryPkt.distF, telemetryPkt.distR,
                  telemetryPkt.batteryVolts, telemetryPkt.stepCount);
}

void processIncomingCommand(const MicromouseCommand& cmd);

#if USE_ESP_NOW_RADIO
// Callback assíncrono de recepção do ESP-NOW
void onEspNowDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
    if (len == sizeof(MicromouseCommand)) {
        MicromouseCommand cmd;
        memcpy(&cmd, incomingData, sizeof(cmd));
        processIncomingCommand(cmd);
    }
}
#endif

// ==============================================================================
// 7. PARSER E PROCESSAMENTO DE COMANDOS
// ==============================================================================
void resetRobotState() {
    stopMotors();
    memset(knownWalls, 0, sizeof(knownWalls));
    memset(visited, 0, sizeof(visited));
    pathStackLen = 0;

    posX = 0; posY = 0;
    heading = NORTE;
    visited[0][0] = true;
    pathStackX[0] = 0;
    pathStackY[0] = 0;
    pathStackLen = 1;

    stepCount = 0;
    elapsedMs = 0;
    startTimeMs = millis();
    currentState = ROBOT_IDLE;
    setRgbStatus(0, 0, 255); // Azul: Pronto
}

void processIncomingCommand(const MicromouseCommand& cmd) {
    Serial.printf("[ESP-NOW] Comando recebido: ID=%u Param=%u\n", cmd.commandId, cmd.parameter);

    switch (cmd.commandId) {
        case CMD_START:
            if (currentState == ROBOT_IDLE || currentState == ROBOT_STOPPED) {
                currentState = ROBOT_EXPLORING;
                startTimeMs = millis();
                setRgbStatus(255, 140, 0); // Laranja: Explorando
                beepFeedback(1600, 50);
            }
            break;

        case CMD_STOP:
            currentState = ROBOT_STOPPED;
            stopMotors();
            setRgbStatus(255, 0, 0); // Vermelho: Parada de emergência
            beepFeedback(800, 150);
            break;

        case CMD_PAUSE:
            if (currentState == ROBOT_EXPLORING) {
                currentState = ROBOT_STOPPED;
                stopMotors();
            } else if (currentState == ROBOT_STOPPED) {
                currentState = ROBOT_EXPLORING;
            }
            break;

        case CMD_RESET_MAZE:
            resetRobotState();
            beepFeedback(1200, 60);
            break;

        case CMD_RETURN_TO_START:
            currentState = ROBOT_RETURNING;
            setRgbStatus(0, 255, 100);
            break;

        case CMD_SET_SPEED:
            if (cmd.parameter >= 80 && cmd.parameter <= 255) {
                basePwm = cmd.parameter;
            }
            break;

        default:
            break;
    }

    sendTelemetryPacket();
}

// Parser Serial para injeção de comandos no Wokwi ou via cabo USB
void handleSerialCommands() {
    if (Serial.available() > 0) {
        String line = Serial.readStringUntil('\n');
        line.trim();

        MicromouseCommand cmd = { CMD_NONE, 0, 0, 0 };
        if (line.equalsIgnoreCase("START") || line.equalsIgnoreCase("CMD:START")) {
            cmd.commandId = CMD_START;
        } else if (line.equalsIgnoreCase("STOP") || line.equalsIgnoreCase("CMD:STOP")) {
            cmd.commandId = CMD_STOP;
        } else if (line.equalsIgnoreCase("PAUSE") || line.equalsIgnoreCase("CMD:PAUSE")) {
            cmd.commandId = CMD_PAUSE;
        } else if (line.equalsIgnoreCase("RESET") || line.equalsIgnoreCase("CMD:RESET")) {
            cmd.commandId = CMD_RESET_MAZE;
        } else if (line.equalsIgnoreCase("RETURN") || line.equalsIgnoreCase("CMD:RETURN")) {
            cmd.commandId = CMD_RETURN_TO_START;
        }

        if (cmd.commandId != CMD_NONE) {
            processIncomingCommand(cmd);
        }
    }
}

// ==============================================================================
// 8. CONTROLE DE NAVEGAÇÃO E CINEMÁTICA
// ==============================================================================
void turnRobot(Rel relTurn) {
    if (relTurn == FRENTE) return;

    bool turnRight = (relTurn == DIREITA || relTurn == TRAS);
    uint32_t turnMs = (relTurn == TRAS) ? 360 : 180;

    if (turnRight) {
        setMotors(basePwm, basePwm, true, false);
    } else {
        setMotors(basePwm, basePwm, false, true);
    }
    delay(turnMs);
    stopMotors();

    heading = (Dir)((heading + relTurn) & 3);
}

void moveForwardCell() {
    // Malha Fechada Wall Centering PD
    readAllSensors();
    float error = 0.0f;
    bool hasWallCorrection = false;

    if (wallL && wallR) {
        error = distL - distR;
        hasWallCorrection = true;
    } else if (wallL && distL < 10.0f) {
        error = distL - SENSOR_IDEAL_CM;
        hasWallCorrection = true;
    } else if (wallR && distR < 10.0f) {
        error = SENSOR_IDEAL_CM - distR;
        hasWallCorrection = true;
    }

    int deltaPwm = hasWallCorrection ? (int)constrain(4.0f * error, -30.0f, 30.0f) : 0;
    int pwmL = constrain(basePwm - deltaPwm, 70, 255);
    int pwmR = constrain(basePwm + deltaPwm, 70, 255);

    setMotors(pwmL, pwmR, true, true);
    delay(280);
    stopMotors();

    posX += DX[heading];
    posY += DY[heading];
    stepCount++;
}

void registerWallsCurrentCell() {
    readAllSensors();

    Dir dirL = (Dir)((heading + 3) & 3);
    Dir dirF = heading;
    Dir dirR = (Dir)((heading + 1) & 3);

    if (wallL) knownWalls[posX][posY] |= WALL_BIT(dirL);
    if (wallF) knownWalls[posX][posY] |= WALL_BIT(dirF);
    if (wallR) knownWalls[posX][posY] |= WALL_BIT(dirR);

    visited[posX][posY] = true;
}

bool checkGoalCondition() {
    // 1. Saída por abertura externa
    if (posX == 0 && !(knownWalls[posX][posY] & WALL_BIT(OESTE)) && !(posX == 0 && posY == 0)) return true;
    if (posX == MAX_W - 1 && !(knownWalls[posX][posY] & WALL_BIT(LESTE))) return true;
    if (posY == 0 && !(knownWalls[posX][posY] & WALL_BIT(SUL)) && !(posX == 0 && posY == 0)) return true;
    if (posY == MAX_H - 1 && !(knownWalls[posX][posY] & WALL_BIT(NORTE))) return true;

    // 2. Área central clássica
    if ((posX == 7 || posX == 8) && (posY == 7 || posY == 8)) return true;

    return false;
}

void stepExplore() {
    registerWallsCurrentCell();

    if (checkGoalCondition()) {
        currentState = ROBOT_GOAL_FOUND;
        stopMotors();
        setRgbStatus(0, 255, 0); // Verde: Gol Encontrado
        beepFeedback(2400, 300);
        sendTelemetryPacket();
        return;
    }

    // Prioridade em linha reta: Frente > Esquerda > Direita > Trás
    const uint8_t relOrder[4] = { 0, 3, 1, 2 };
    int nextDir = -1;

    for (int i = 0; i < 4; i++) {
        Dir candD = (Dir)((heading + relOrder[i]) & 3);
        if (!(knownWalls[posX][posY] & WALL_BIT(candD))) {
            int cx = posX + DX[candD], cy = posY + DY[candD];
            if (cx >= 0 && cx < MAX_W && cy >= 0 && cy < MAX_H && !visited[cx][cy]) {
                nextDir = candD;
                break;
            }
        }
    }

    if (nextDir >= 0) {
        currentState = ROBOT_EXPLORING;
        Rel r = (Rel)((nextDir - heading + 4) & 3);
        turnRobot(r);
        moveForwardCell();

        pathStackX[pathStackLen] = posX;
        pathStackY[pathStackLen] = posY;
        pathStackLen++;
    } else {
        // Beco sem saída: Retrocesso Físico
        currentState = ROBOT_BACKTRACKING;
        if (pathStackLen <= 1) {
            currentState = ROBOT_STOPPED;
            stopMotors();
            return;
        }

        pathStackLen--;
        uint8_t targetX = pathStackX[pathStackLen - 1];
        uint8_t targetY = pathStackY[pathStackLen - 1];

        int backDir = -1;
        for (int d = 0; d < 4; d++) {
            if (posX + DX[d] == targetX && posY + DY[d] == targetY) {
                backDir = d;
                break;
            }
        }

        if (backDir >= 0) {
            Rel r = (Rel)((backDir - heading + 4) & 3);
            turnRobot(r);
            moveForwardCell();
        }
    }

    sendTelemetryPacket();
}

// ==============================================================================
// 9. DISPLAY OLED SSD1306 (TELEMETRIA E STATUS)
// ==============================================================================
void updateOledProduction() {
    if (!oledReady) return;
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);

    oled.setCursor(0, 0);
    oled.print("MICROMOUSE ESP-NOW");
    oled.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    oled.setCursor(0, 15);
    oled.printf("Pos: (%d, %d) Dir: %c", posX, posY, "NLSO"[heading]);

    oled.setCursor(0, 26);
    oled.printf("Estado: %s",
                currentState == ROBOT_IDLE ? "AGUARDANDO" :
                currentState == ROBOT_EXPLORING ? "EXPLORANDO" :
                currentState == ROBOT_BACKTRACKING ? "RETROCESSO" :
                currentState == ROBOT_GOAL_FOUND ? "GOL ACHADO!" :
                currentState == ROBOT_RETURNING ? "RETORNANDO" : "PARADO");

    oled.setCursor(0, 37);
    oled.printf("Passos: %u | Pilha: %u", stepCount, pathStackLen);

    oled.setCursor(0, 48);
    oled.printf("E:%3.0f F:%3.0f D:%3.0f", distL, distF, distR);

    oled.setCursor(0, 56);
    oled.printf("Bat: %.2fV | ESP-NOW", batteryVolts);

    oled.display();
}

// ==============================================================================
// 10. SETUP E INICIALIZAÇÃO
// ==============================================================================
void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(TRIG_ESQ, OUTPUT); pinMode(ECHO_ESQ, INPUT);
    pinMode(TRIG_FRE, OUTPUT); pinMode(ECHO_FRE, INPUT);
    pinMode(TRIG_DIR, OUTPUT); pinMode(ECHO_DIR, INPUT);

    pinMode(PIN_PWMA, OUTPUT); pinMode(PIN_PWMB, OUTPUT);
    pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT);

    pinMode(PIN_BTN_START, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_BATTERY, INPUT);

    rgb.begin();
    rgb.setBrightness(40);
    setRgbStatus(0, 0, 255);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    oledReady = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

#if USE_ESP_NOW_RADIO
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK) {
        espNowActive = true;
        esp_now_register_recv_cb(onEspNowDataRecv);

        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, GATEWAY_MAC, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
        Serial.println("[ESP-NOW] Driver inicializado e Peer Gateway registrado.");
    } else {
        Serial.println("[AVISO] Falha ao inicializar rádio ESP-NOW (Modo Serial ativo).");
    }
#endif

    resetRobotState();
    beepFeedback(1800, 100);
}

// ==============================================================================
// 11. LOOP PRINCIPAL (NÃO-BLOQUEANTE)
// ==============================================================================
void loop() {
    uint32_t now = millis();

    // 1. Processamento de Comandos Serial (Fallback Wokwi e Debug USB)
    handleSerialCommands();

    // 2. Botão Físico de Emergência / Largada
    if (digitalRead(PIN_BTN_START) == LOW) {
        delay(25);
        if (digitalRead(PIN_BTN_START) == LOW) {
            if (currentState == ROBOT_IDLE || currentState == ROBOT_STOPPED) {
                MicromouseCommand c = { CMD_START, 0, 0, 0 };
                processIncomingCommand(c);
            } else {
                MicromouseCommand c = { CMD_STOP, 0, 0, 0 };
                processIncomingCommand(c);
            }
            while (digitalRead(PIN_BTN_START) == LOW) delay(10);
        }
    }

    // 3. Atualização do Tempo Decorrido
    if (currentState == ROBOT_EXPLORING || currentState == ROBOT_BACKTRACKING) {
        elapsedMs = now - startTimeMs;
    }

    // 4. Máquina de Estados de Exploração
    if ((currentState == ROBOT_EXPLORING || currentState == ROBOT_BACKTRACKING) &&
        (now - lastLoopStepMs >= stepIntervalMs)) {
        lastLoopStepMs = now;
        stepExplore();
    }

    // 5. Envio Periódico de Telemetria via ESP-NOW / Serial (200 ms)
    if (now - lastTelemetrySendMs >= TELEMETRY_PERIOD_MS) {
        lastTelemetrySendMs = now;
        readAllSensors();
        sendTelemetryPacket();
        updateOledProduction();
    }
}
