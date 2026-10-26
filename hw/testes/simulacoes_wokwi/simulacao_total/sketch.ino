/**
 * ==============================================================================
 *  PROJETO MICROMOUSE - SIMULAÇÃO TOTAL INTEGRADA
 *  Emulação Completa de Hardware ESP32 no Wokwi
 * ==============================================================================
 *  PRINCÍPIO DE OPERAÇÃO: "EXPLORAÇÃO PURA ÀS CEGAS"
 *  ---------------------------------------------------------------------------
 *  1. O robô NÃO possui mapa inicial do labirinto (matriz de paredes zerada).
 *  2. O robô NÃO sabe onde fica o gol/saída (descoberta dinâmica por sensores).
 *  3. Mapeamento online célula a célula pelos 3 sensores de distância ToF.
 *  4. Navegação autônoma por Busca em Profundidade com Retrocesso Físico (Trémaux):
 *     - Avança preferencialmente em linha reta (Frente > Esquerda > Direita > Trás).
 *     - Em becos sem saída (dead-ends), realiza retrocesso físico pela pilha.
 *  5. Chassi Diferencial com Controle PD de Centralização (Wall Centering PD).
 *  6. Ao encontrar a saída: registra rota limpa, sinaliza com alarme e permite
 *     executar Speed Run imediato!
 *
 *  SUÍTE DE TESTES MANUAIS E DEMONSTRAÇÃO (Menu Serial a 115200 bps):
 *  - Testes individuais de motores: Frente (F), Ré (T), Giro 90° Esq (L),
 *    Giro 90° Dir (R), Giro 180° (U), Ajuste de PWM (PWM <val>).
 *  - Teste de sensores ToF individuais e contínuos.
 *  - Simulação de erros (Colisão, Timeout, Sem Saída).
 *  - Demonstração automatizada passo a passo (DEMO).
 *  - Modos de OLED: Mapa com Fog-of-War e Desenho Vetorial do Chassi com Roda Boba.
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ==============================================================================
// 1. PINOS DO HARDWARE (Conforme diagram.json)
// ==============================================================================
const uint8_t PIN_BTN_START = 15;  // Botão Verde: Início / Pausa / Speed Run
const uint8_t PIN_BTN_MODE  = 12;  // Botão Azul: Modo de Exibição / Operação
const uint8_t PIN_BTN_STEP  = 33;  // Botão Amarelo: Passo Manual / Velocidade
const uint8_t PIN_BTN_MAZE  = 0;   // Botão Laranja: Trocar Labirinto / Som

const uint8_t PIN_RGB       = 2;   // WS2812B Status LED
const uint8_t PIN_BUZZER    = 4;   // Buzzer Piezoelétrico

// Ponte H TB6612FNG (2 Rodas Motrizes)
const uint8_t PIN_PWMA      = 16;  // PWM Motor Esquerdo (LED Verde)
const uint8_t PIN_PWMB      = 17;  // PWM Motor Direito  (LED Azul)
const uint8_t PIN_AIN1      = 18;  // Sentido Motor Esquerdo (LED Amarelo)
const uint8_t PIN_AIN2      = 19;
const uint8_t PIN_BIN1      = 23;  // Sentido Motor Direito  (LED Amarelo)
const uint8_t PIN_BIN2      = 5;

// I2C: Display OLED SSD1306 + IMU MPU6050
const uint8_t PIN_I2C_SDA   = 21;
const uint8_t PIN_I2C_SCL   = 22;
const uint8_t OLED_ADDR     = 0x3C;
const uint8_t MPU_ADDR      = 0x68;

// Monitor de Bateria
const uint8_t PIN_BATTERY   = 34;

// Sensores de Distância (HC-SR04 emulando ToF VL53L0X)
const uint8_t TRIG_ESQ = 13, ECHO_ESQ = 27;  // ToF Esquerdo (90°)
const uint8_t TRIG_FRE = 32, ECHO_FRE = 26;  // ToF Frontal (0°)
const uint8_t TRIG_DIR = 14, ECHO_DIR = 25;  // ToF Direito (90°)

// ==============================================================================
// 2. PARÂMETROS MECÂNICOS E CINEMÁTICOS DO ROBÔ
// ==============================================================================
const float   WHEEL_BASE_CM       = 8.0f;   // Distância entre eixos das rodas motrizes (8,0 cm)
const float   WHEEL_DIAMETER_CM   = 3.2f;   // Diâmetro das rodas (3,2 cm)
const float   CASTER_OFFSET_CM    = 3.5f;   // Distância da Roda Normal (Caster) ao eixo motriz
const float   CELL_SIZE_CM        = 18.0f;  // Célula oficial Micromouse (18 cm x 18 cm)
const float   SENSOR_TO_WALL_CM   = 5.0f;   // Distância ideal centralizada (18 - 8)/2 = 5,0 cm

// Sensoriamento
const float    WALL_THRESHOLD_CM     = 13.0f; // Distância < 13 cm => Parede detectada
const float    MAX_RANGE_CM          = 400.0f;
const uint32_t ULTRASONIC_TIMEOUT_US = 25000;

// Temporizações (ms)
const uint16_t STEP_PERIODS_MS[] = { 1000, 500, 250, 120, 50 };
const uint8_t  NUM_SPEEDS        = sizeof(STEP_PERIODS_MS) / sizeof(STEP_PERIODS_MS[0]);

// ==============================================================================
// 3. AMBIENTES FÍSICOS (Labirintos da Arena Externa no Wokwi)
// ==============================================================================
const char* const MAZE01_TXT[] = {
    "o---o---o---o---o---o---o---o---o---o---o---o---o---o---o---o---o",
    "|   |                                               |           |",
    "o   o---o   o   o---o---o   o   o   o---o---o   o---o---o   o   o",
    "|   |       |       |       |   |       |                   |   |",
    "o   o   o---o---o   o   o---o   o---o   o   o---o---o   o---o   o",
    "|                   |       |   |       |       |           |   |",
    "o   o   o---o---o   o---o   o   o   o---o---o   o---o   o   o   o",
    "|   |       |       |                   |       |       |       |",
    "o   o---o   o   o   o   o---o---o   o   o   o   o   o---o---o   o",
    "|   |           |           |       |   |   |           |       |",
    "o   o   o   o---o---o   o   o---o---o---o   o---o   o   o   o   o",
    "|       |       |       |   |       |       |       |       |   |",
    "o   o---o---o   o   o---o   o---o   o   o   o   o---o   o---o   o",
    "|               |       |   |           |           |       |   |",
    "o   o   o   o---o---o   o   o   o---o   o---o   o   o   o   o   o",
    "|   |   |                   | G   G |   |       |       |       |",
    "o---o   o---o   o---o---o   o   o   o   o   o   o---o   o---o   o",
    "|   |   |           |   |   | G   G |       |   |       |       |",
    "o   o   o   o---o---o   o---o---o---o   o---o---o---o---o   o   o",
    "|               |       |       |       |   |       |       |   |",
    "o   o---o---o   o   o---o---o   o---o---o   o   o   o   o---o---o",
    "|       |                           |   |       |               |",
    "o---o   o   o   o---o---o   o   o---o   o---o   o---o---o   o   o",
    "|   |       |       |       |       |   |       |   |       |   |",
    "o   o---o   o---o   o   o---o---o   o   o   o---o   o   o---o   o",
    "|   |       |               |                   |           |   |",
    "o   o   o   o   o---o---o   o   o---o---o---o   o---o---o   o   o",
    "|       |           |               |       |                   |",
    "o   o   o---o   o   o---o---o   o   o   o---o---o   o   o   o   o",
    "|   |   |       |       |       |                   |   |   |   |",
    "o   o   o   o---o---o   o   o---o---o   o---o---o   o---o---o   o",
    "| S |                                       |                   |",
    "o---o---o---o---o---o---o---o---o---o---o---o---o---o---o---o---o",
};

const char* const BENCH_4X4_TXT[] = {
    "o---o---o---o---o",
    "|           | G |",
    "o---o   o   o   o",
    "|       |       |",
    "o   o---o---o   o",
    "|   |       |   |",
    "o   o   o   o   o",
    "| S             |",
    "o---o---o---o---o",
};

const char* const BENCH_8X4_TXT[] = {
    "o---o---o---o---o",
    "|           | G |",
    "o   o---o   o   o",
    "|   |   |   |   |",
    "o   o   o   o   o",
    "|   |       |   |",
    "o   o---o   o   o",
    "|       |       |",
    "o   o   o---o   o",
    "|   |       |   |",
    "o   o---o   o   o",
    "|   |       |   |",
    "o   o   o---o   o",
    "|   |   |       |",
    "o---o   o   o---o",
    "| S     |       |",
    "o---o---o---o---o",
};

const char* const BENCH_12X4_TXT[] = {
    "o---o---o---o---o",
    "|             G |",
    "o   o---o---o   o",
    "|   |           |",
    "o---o   o---o   o",
    "|       |   |   |",
    "o   o---o   o   o",
    "|   |       |   |",
    "o   o   o---o   o",
    "|   |           |",
    "o   o---o---o   o",
    "|           |   |",
    "o   o---o   o   o",
    "|       |   |   |",
    "o---o   o   o---o",
    "|       |       |",
    "o---o---o   o   o",
    "|       |   |   |",
    "o   o   o---o   o",
    "|   |       |   |",
    "o   o---o   o   o",
    "|   |   |       |",
    "o   o   o---o   o",
    "| S |           |",
    "o---o---o---o---o",
};

struct MazeDef {
    const char*        name;
    const char* const* lines;
    uint8_t            numLines;
};

#define MAZE_ENTRY(label, arr) { label, arr, (uint8_t)(sizeof(arr) / sizeof(arr[0])) }

const MazeDef MAZES[] = {
    MAZE_ENTRY("MAZE01",    MAZE01_TXT),
    MAZE_ENTRY("BENCH4x4",  BENCH_4X4_TXT),
    MAZE_ENTRY("BENCH8x4",  BENCH_8X4_TXT),
    MAZE_ENTRY("BENCH12x4", BENCH_12X4_TXT),
};
const uint8_t NUM_MAZES = sizeof(MAZES) / sizeof(MAZES[0]);

// ==============================================================================
// 4. TIPOS, ENUMS E CONSTANTES
// ==============================================================================
const uint8_t MAX_W = 16;
const uint8_t MAX_H = 16;

// Direções absolutas (0: Norte, 1: Leste, 2: Sul, 3: Oeste)
enum Dir : uint8_t { NORTE = 0, LESTE = 1, SUL = 2, OESTE = 3 };
const int8_t      DX[4]         = { 0, 1, 0, -1 };
const int8_t      DY[4]         = { 1, 0, -1, 0 };
const char* const DIR_NAMES[4]  = { "NORTE", "LESTE", "SUL", "OESTE" };
const char        DIR_LETTER[4] = { 'N', 'L', 'S', 'O' };
#define WALL_BIT(d) ((uint8_t)(1u << (d)))

// Direções relativas
enum Rel : uint8_t { FRENTE = 0, DIREITA = 1, TRAS = 2, ESQUERDA = 3 };

// Modos de Exibição
enum ViewMode : uint8_t {
    VIEW_MAZE_MAP = 0, // Minimapa com Fog-of-War e telemetria
    VIEW_CHASSIS,      // Chassi vetorial com rodas e roda boba
    VIEW_SENSORS,      // Barras gráficas dos sensores ToF
    NUM_VIEW_MODES
};

// Máquina de Estados da Corrida / Exploração
enum RunState : uint8_t {
    ST_AGUARDANDO = 0,
    ST_EXPLORANDO,     // Exploração ativa às cegas
    ST_RETROCEDENDO,   // Backtracking por beco sem saída
    ST_PRONTO,         // Saída/Gol encontrada! Rota limpa pronta
    ST_SPEED_RUN,      // Percorrendo a rota direta em velocidade máxima
    ST_CONCLUIDO,      // Prova finalizada com sucesso
    ST_SEM_SAIDA,      // Erro: labirinto fechado sem saída
    ST_COLISAO,        // Erro: colisão detectada
    ST_MANUAL_TEST     // Executando teste manual isolado
};

const char* const STATE_NAMES[] = {
    "AGUARDANDO", "EXPLORANDO", "RETROCESSO", "SAIDA_ACHADA", "SPEED_RUN",
    "CONCLUIDO", "SEM_SAIDA", "COLISAO", "TESTE_MANUAL"
};

struct WallReading {
    float distL, distF, distR;
    bool  wallL, wallF, wallR;
};

// ==============================================================================
// 5. ESTADO GLOBAL E MODELO CINEMÁTICO
// ==============================================================================
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306  oled(128, 64, &Wire, -1);
bool oledReady = false;

// Ambiente físico externo (Gabarito da arena para o Wokwi)
uint8_t mazeIndex = 0;
uint8_t mazeW = 16, mazeH = 16;
uint8_t startX = 0, startY = 0;
uint8_t trueWalls[MAX_W][MAX_H];
bool    trueGoal[MAX_W][MAX_H];

// MEMÓRIA INTERNA DO ROBÔ (Inicia 100% ZERADA - O robô não sabe nada!)
uint8_t  knownWalls[MAX_W][MAX_H];
bool     visited[MAX_W][MAX_H];
bool     inPathStack[MAX_W][MAX_H];
uint8_t  pathStackX[MAX_W * MAX_H];
uint8_t  pathStackY[MAX_W * MAX_H];
uint16_t pathStackLen = 0;

// Localização e Saída
uint8_t  posX = 0, posY = 0;
Dir      heading = NORTE;
bool     exitFound = false;
uint8_t  exitX = 0xFF, exitY = 0xFF;

// Rota do Speed Run
Dir      speedPath[MAX_W * MAX_H];
uint16_t speedPathLen = 0;
uint16_t speedPathIdx = 0;

// Cinemática Diferencial Contínua (2 Rodas Motrizes + 1 Caster)
struct RobotKinematics {
    float odomX_cm;
    float odomY_cm;
    float odomTheta_rad;
    float odomTheta_deg;
    float casterAngle_deg;
    bool  casterRolling;
    int   pwmL;
    int   pwmR;
    bool  fwdL;
    bool  fwdR;
} car = { 9.0f, 9.0f, 1.570796f, 90.0f, 0.0f, false, 0, 0, true, true };

// Controle de Execução
ViewMode viewMode    = VIEW_MAZE_MAP;
RunState state       = ST_AGUARDANDO;
bool     running     = false;
uint8_t  speedIdx    = 1; // 500 ms/passo
int      userBasePwm = 175;
bool     soundOn     = true;

uint32_t raceTimeMs      = 0;
uint32_t exitTimeMs      = 0;
uint16_t stepCount       = 0;
uint16_t backtrackCount  = 0;

// Leituras e Periféricos
WallReading lastReading = { MAX_RANGE_CM, MAX_RANGE_CM, MAX_RANGE_CM, false, false, false };
float batteryVolts = 7.8f;
int   batteryPct   = 85;
bool  imuOk        = false;
float gyroZ        = 0.0f;

uint32_t lastStepMs = 0, lastUiMs = 0, lastTelemetryMs = 0;
char serialLine[64];
uint8_t serialLen = 0;
uint8_t treadAnimPhase = 0;

// ==============================================================================
// 6. UTILITÁRIOS
// ==============================================================================
Dir      absDir(Dir h, Rel r)                  { return (Dir)((h + r) & 3); }
Rel      relDir(Dir h, Dir d)                  { return (Rel)((d - h + 4) & 3); }
Dir      oppositeDir(Dir d)                    { return (Dir)((d + 2) & 3); }
bool     inBounds(int x, int y)                { return x >= 0 && x < mazeW && y >= 0 && y < mazeH; }
uint16_t stepPeriodMs()                        { return STEP_PERIODS_MS[speedIdx]; }
float    clampFloat(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
int      clampInt(int v, int lo, int hi)       { return v < lo ? lo : (v > hi ? hi : v); }
char     colLetter(int x)                      { return (char)('A' + x); }

// ==============================================================================
// 7. SINALIZAÇÃO VISUAL E AUDITIVA
// ==============================================================================
void beep(int freq, int durationMs) {
    if (!soundOn) return;
    tone(PIN_BUZZER, freq, durationMs);
    delay(durationMs);
    noTone(PIN_BUZZER);
}

void soundChime() {
    beep(1200, 40); delay(20);
    beep(1600, 50); delay(20);
    beep(2000, 60);
}

void soundVictory() {
    beep(1318, 90); delay(30);
    beep(1567, 90); delay(30);
    beep(2093, 140); delay(40);
    beep(2637, 200);
}

void soundBacktrack() {
    beep(600, 40); delay(15);
    beep(450, 60);
}

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
    rgb.setPixelColor(0, rgb.Color(r, g, b));
    rgb.show();
}

void updateRgb() {
    switch (state) {
        case ST_AGUARDANDO:   setRgb(0, 40, 255);    break; // Azul
        case ST_EXPLORANDO:   setRgb(255, 140, 0);   break; // Laranja
        case ST_RETROCEDENDO: setRgb(255, 0, 120);   break; // Magenta
        case ST_PRONTO:       setRgb(0, 255, 180);   break; // Ciano
        case ST_SPEED_RUN:    setRgb(0, 255, 0);     break; // Verde
        case ST_CONCLUIDO:    setRgb(255, 255, 255); break; // Branco
        case ST_MANUAL_TEST:  setRgb(180, 0, 255);   break; // Roxo
        default:              setRgb(255, 0, 0);     break; // Vermelho
    }
}

// ==============================================================================
// 8. CARREGAMENTO DO LABIRINTO DA ARENA EXTERNA
// ==============================================================================
char mazeChar(const MazeDef& m, int line, int col) {
    if (line < 0 || line >= m.numLines || col < 0) return ' ';
    const char* s = m.lines[line];
    int len = strlen(s);
    return col < len ? s[col] : ' ';
}

void loadMaze(uint8_t idx) {
    if (idx >= NUM_MAZES) idx = 0;
    mazeIndex = idx;
    const MazeDef& m = MAZES[idx];

    mazeH = (m.numLines - 1) / 2;
    int maxLen = 0;
    for (int i = 0; i < m.numLines; i++) {
        int l = strlen(m.lines[i]);
        if (l > maxLen) maxLen = l;
    }
    mazeW = (maxLen - 1) / 4;
    if (mazeW > MAX_W) mazeW = MAX_W;
    if (mazeH > MAX_H) mazeH = MAX_H;

    memset(trueWalls, 0, sizeof(trueWalls));
    memset(trueGoal, 0, sizeof(trueGoal));
    startX = 0; startY = 0;

    int goalCount = 0;
    for (int y = 0; y < mazeH; y++) {
        int textRow = m.numLines - 2 - 2 * y;
        for (int x = 0; x < mazeW; x++) {
            int cellCol = 1 + 4 * x;
            char marker = mazeChar(m, textRow, cellCol + 1);
            if (marker == 'S') { startX = x; startY = y; }
            if (marker == 'G') { trueGoal[x][y] = true; goalCount++; }

            if (mazeChar(m, textRow - 1, cellCol) == '-') trueWalls[x][y] |= WALL_BIT(NORTE);
            if (mazeChar(m, textRow + 1, cellCol) == '-') trueWalls[x][y] |= WALL_BIT(SUL);
            if (mazeChar(m, textRow, cellCol - 1) == '|') trueWalls[x][y] |= WALL_BIT(OESTE);
            if (mazeChar(m, textRow, cellCol + 3) == '|') trueWalls[x][y] |= WALL_BIT(LESTE);
        }
    }

    if (goalCount == 0) {
        if (mazeW == 16 && mazeH == 16) {
            trueGoal[7][7] = trueGoal[7][8] = trueGoal[8][7] = trueGoal[8][8] = true;
        } else {
            trueGoal[mazeW - 1][mazeH - 1] = true;
        }
    }
}

// ==============================================================================
// 9. CONTROLE DE MOTORES DA PONTE H TB6612
// ==============================================================================
void setHardwareMotors(int pwmL, int pwmR, bool fwdL, bool fwdR) {
    car.pwmL = clampInt(pwmL, 0, 255);
    car.pwmR = clampInt(pwmR, 0, 255);
    car.fwdL = fwdL;
    car.fwdR = fwdR;

    digitalWrite(PIN_AIN1, fwdL ? HIGH : LOW);
    digitalWrite(PIN_AIN2, fwdL ? LOW : HIGH);
    digitalWrite(PIN_BIN1, fwdR ? HIGH : LOW);
    digitalWrite(PIN_BIN2, fwdR ? LOW : HIGH);

    analogWrite(PIN_PWMA, car.pwmL);
    analogWrite(PIN_PWMB, car.pwmR);
}

void stopHardwareMotors() {
    setHardwareMotors(0, 0, true, true);
    digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
    car.casterRolling = false;
}

void resetRobot() {
    stopHardwareMotors();

    // 100% ZERADO: Zero mapa inicial!
    memset(knownWalls, 0, sizeof(knownWalls));
    memset(visited, 0, sizeof(visited));
    memset(inPathStack, 0, sizeof(inPathStack));
    pathStackLen = 0;

    posX = startX;
    posY = startY;
    heading = NORTE;
    visited[posX][posY] = true;

    pathStackX[0] = posX;
    pathStackY[0] = posY;
    pathStackLen = 1;
    inPathStack[posX][posY] = true;

    car.odomX_cm = startX * CELL_SIZE_CM + (CELL_SIZE_CM / 2.0f);
    car.odomY_cm = startY * CELL_SIZE_CM + (CELL_SIZE_CM / 2.0f);
    car.odomTheta_deg = 90.0f;
    car.odomTheta_rad = 1.5707963f;

    stepCount = 0;
    backtrackCount = 0;
    raceTimeMs = 0;
    exitTimeMs = 0;
    exitFound = false;
    exitX = 0xFF; exitY = 0xFF;
    speedPathLen = 0;
    speedPathIdx = 0;
    state = ST_AGUARDANDO;
    running = false;
}

// ==============================================================================
// 10. SENSORES ToF / ULTRASSÔNICOS
// ==============================================================================
float readUltrasonicCm(uint8_t trigPin, uint8_t echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    uint32_t pulseDuration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT_US);
    if (pulseDuration == 0) return MAX_RANGE_CM;
    float dist = pulseDuration * 0.0343f / 2.0f;
    return (dist > MAX_RANGE_CM || dist < 1.0f) ? MAX_RANGE_CM : dist;
}

float distanceInDirection(int x, int y, Dir d) {
    float dist = SENSOR_TO_WALL_CM;
    int cx = x, cy = y;
    while (inBounds(cx, cy)) {
        if (trueWalls[cx][cy] & WALL_BIT(d)) return dist;
        cx += DX[d];
        cy += DY[d];
        dist += CELL_SIZE_CM;
    }
    return dist;
}

WallReading senseWalls() {
    WallReading r;
    r.distL = distanceInDirection(posX, posY, absDir(heading, ESQUERDA));
    r.distF = distanceInDirection(posX, posY, absDir(heading, FRENTE));
    r.distR = distanceInDirection(posX, posY, absDir(heading, DIREITA));
    r.wallL = (r.distL < WALL_THRESHOLD_CM);
    r.wallF = (r.distF < WALL_THRESHOLD_CM);
    r.wallR = (r.distR < WALL_THRESHOLD_CM);
    return r;
}

void registerWall(int x, int y, Dir d, bool present) {
    if (!inBounds(x, y)) return;
    if (present) knownWalls[x][y] |= WALL_BIT(d);
    else         knownWalls[x][y] &= ~WALL_BIT(d);

    int nx = x + DX[d], ny = y + DY[d];
    if (inBounds(nx, ny)) {
        Dir opp = oppositeDir(d);
        if (present) knownWalls[nx][ny] |= WALL_BIT(opp);
        else         knownWalls[nx][ny] &= ~WALL_BIT(opp);
    }
}

void mapCurrentCell(const WallReading& r) {
    registerWall(posX, posY, absDir(heading, ESQUERDA), r.wallL);
    registerWall(posX, posY, absDir(heading, FRENTE),   r.wallF);
    registerWall(posX, posY, absDir(heading, DIREITA),  r.wallR);
    visited[posX][posY] = true;
}

// ==============================================================================
// 11. DETECÇÃO DINÂMICA DE SAÍDA (GOL DESCONHECIDO)
// ==============================================================================
bool isExitCell(int x, int y) {
    // 1. Saída por abertura no perímetro externo
    if (x == 0 && !(knownWalls[x][y] & WALL_BIT(OESTE)) && !(x == startX && y == startY)) return true;
    if (x == mazeW - 1 && !(knownWalls[x][y] & WALL_BIT(LESTE))) return true;
    if (y == 0 && !(knownWalls[x][y] & WALL_BIT(SUL)) && !(x == startX && y == startY)) return true;
    if (y == mazeH - 1 && !(knownWalls[x][y] & WALL_BIT(NORTE))) return true;

    // 2. Célula de objetivo no labirinto com perímetro fechado
    if (trueGoal[x][y]) return true;

    return false;
}

// ==============================================================================
// 12. MOVIMENTAÇÃO E CONTROLE PD DE CENTRALIZAÇÃO
// ==============================================================================
void diffDriveTurn(Rel relativeTurn) {
    if (relativeTurn == FRENTE) return;

    bool turnRight = (relativeTurn == DIREITA || relativeTurn == TRAS);
    uint32_t turnDurationMs = (relativeTurn == TRAS) ? 350 : 180;
    int pwmTurn = (state == ST_SPEED_RUN) ? 220 : userBasePwm;

    if (turnRight) {
        setHardwareMotors(pwmTurn, pwmTurn, true, false);
    } else {
        setHardwareMotors(pwmTurn, pwmTurn, false, true);
    }
    delay(turnDurationMs);
    stopHardwareMotors();
}

bool diffDriveMoveStraight(float distanceCm) {
    (void)distanceCm;
    int nx = posX + DX[heading], ny = posY + DY[heading];
    bool blocked = (trueWalls[posX][posY] & WALL_BIT(heading));

    if (blocked) {
        state = ST_COLISAO;
        running = false;
        stopHardwareMotors();
        Serial.println("\n[ERRO] Colisao com parede detectada!");
        beep(800, 200);
        return false;
    }

    uint32_t moveDurationMs = (state == ST_SPEED_RUN) ? 200 : 300;
    int basePwm = (state == ST_SPEED_RUN) ? 240 : userBasePwm;

    // Malha Fechada Wall Centering PD
    WallReading r = senseWalls();
    float lateralError = 0.0f;
    bool hasWallCorrection = false;

    if (r.wallL && r.wallR) {
        lateralError = r.distL - r.distR;
        hasWallCorrection = true;
    } else if (r.wallL && r.distL < 10.0f) {
        lateralError = r.distL - SENSOR_TO_WALL_CM;
        hasWallCorrection = true;
    } else if (r.wallR && r.distR < 10.0f) {
        lateralError = SENSOR_TO_WALL_CM - r.distR;
        hasWallCorrection = true;
    }

    float kp = 4.0f;
    int deltaPwm = hasWallCorrection ? (int)clampFloat(kp * lateralError, -35.0f, 35.0f) : 0;

    int pwmL = clampInt(basePwm - deltaPwm, 80, 255);
    int pwmR = clampInt(basePwm + deltaPwm, 80, 255);

    setHardwareMotors(pwmL, pwmR, true, true);
    delay(moveDurationMs);
    stopHardwareMotors();

    posX = nx;
    posY = ny;
    stepCount++;

    car.odomX_cm = posX * CELL_SIZE_CM + (CELL_SIZE_CM / 2.0f);
    car.odomY_cm = posY * CELL_SIZE_CM + (CELL_SIZE_CM / 2.0f);

    return true;
}

void turnTo(Dir target) {
    Rel r = relDir(heading, target);
    if (r == FRENTE) return;
    diffDriveTurn(r);
    heading = target;
}

// ==============================================================================
// 13. ALGORITMO DE BUSCA ÀS CEGAS E SPEED RUN
// ==============================================================================
void printMapAscii();

void onExitFound() {
    exitFound = true;
    exitX = posX;
    exitY = posY;
    exitTimeMs = raceTimeMs;
    running = false;
    state = ST_PRONTO;

    speedPathLen = (pathStackLen > 1) ? (pathStackLen - 1) : 0;
    for (int i = 0; i < speedPathLen; i++) {
        int dx = pathStackX[i + 1] - pathStackX[i];
        int dy = pathStackY[i + 1] - pathStackY[i];
        for (int d = 0; d < 4; d++) {
            if (DX[d] == dx && DY[d] == dy) {
                speedPath[i] = (Dir)d;
                break;
            }
        }
    }

    Serial.println("\n=======================================================");
    Serial.printf("  [SAIDA ENCONTRADA!] Posicao: %c%d (x=%d, y=%d)\n", colLetter(exitX), exitY + 1, exitX, exitY);
    Serial.printf("  Passos de exploracao: %u | Retrocessos: %u\n", stepCount, backtrackCount);
    Serial.printf("  Rota Limpa Direta: %u celulas\n", pathStackLen);
    Serial.println("=======================================================");
    Serial.println("  Digite SPEED ou aperte o botao VERDE para o Speed Run!");
    soundVictory();
    printMapAscii();
}

void stepBlindExplore() {
    lastReading = senseWalls();
    mapCurrentCell(lastReading);

    if (isExitCell(posX, posY)) {
        onExitFound();
        return;
    }

    // Prioridade em linha reta: Frente (0) > Esquerda (3) > Direita (1) > Trás (2)
    const uint8_t relOrder[4] = { 0, 3, 1, 2 };
    int nextDir = -1;

    for (int i = 0; i < 4; i++) {
        uint8_t candD = (heading + relOrder[i]) & 3;
        if (!(knownWalls[posX][posY] & WALL_BIT(candD))) {
            int cx = posX + DX[candD], cy = posY + DY[candD];
            if (inBounds(cx, cy) && !visited[cx][cy]) {
                nextDir = candD;
                break;
            }
        }
    }

    if (nextDir >= 0) {
        state = ST_EXPLORANDO;
        turnTo((Dir)nextDir);
        if (!diffDriveMoveStraight(CELL_SIZE_CM)) return;

        visited[posX][posY] = true;
        pathStackX[pathStackLen] = posX;
        pathStackY[pathStackLen] = posY;
        inPathStack[posX][posY] = true;
        pathStackLen++;
    } else {
        // Beco sem saída: Retrocesso Físico
        state = ST_RETROCEDENDO;
        backtrackCount++;

        if (pathStackLen <= 1) {
            state = ST_SEM_SAIDA;
            running = false;
            stopHardwareMotors();
            Serial.println("[AVISO] Labirinto totalmente explorado sem mais saidas.");
            return;
        }

        inPathStack[posX][posY] = false;
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
            soundBacktrack();
            turnTo((Dir)backDir);
            diffDriveMoveStraight(CELL_SIZE_CM);
        }
    }
}

void beginSpeedRun() {
    if (speedPathLen == 0) {
        Serial.println("[ERRO] Nenhuma rota de saida descoberta.");
        return;
    }
    posX = startX;
    posY = startY;
    heading = NORTE;
    speedPathIdx = 0;
    state = ST_SPEED_RUN;
    running = true;
    Serial.printf("\n[SPEED RUN] Iniciado: %u celulas diretas!\n", speedPathLen);
    soundChime();
}

void stepSpeedRun() {
    if (speedPathIdx >= speedPathLen) {
        state = ST_CONCLUIDO;
        running = false;
        Serial.println("\n[SUCESSO] Speed Run concluido!");
        soundVictory();
        return;
    }
    turnTo(speedPath[speedPathIdx++]);
    if (!diffDriveMoveStraight(CELL_SIZE_CM)) return;
}

// ==============================================================================
// 14. SUÍTE DE TESTES MANUAIS DE ATUADORES E MOTORES
// ==============================================================================
void testManualMove(Rel dir) {
    state = ST_MANUAL_TEST;
    running = false;
    Serial.printf("[MANUAL] Executando comando de motor: %s\n",
                  dir == FRENTE ? "AVANCAR FRENTE" :
                  dir == TRAS ? "RECUAR" :
                  dir == ESQUERDA ? "GIRO 90 ESQUERDA" : "GIRO 90 DIREITA");

    if (dir == FRENTE) {
        diffDriveMoveStraight(CELL_SIZE_CM);
    } else if (dir == TRAS) {
        diffDriveTurn(TRAS);
        diffDriveMoveStraight(CELL_SIZE_CM);
    } else {
        diffDriveTurn(dir);
        heading = absDir(heading, dir);
    }
    beep(1500, 30);
}

void runDemonstrationRoutine() {
    Serial.println("\n=======================================================");
    Serial.println("  INICIANDO ROTINA DE DEMONSTRACAO PASSO A PASSO");
    Serial.println("=======================================================");
    resetRobot();

    Serial.println("Fase 1: Manobra de Calibracao e Giro Pivotante 360 Graus");
    diffDriveTurn(DIREITA); delay(200);
    diffDriveTurn(DIREITA); delay(200);
    diffDriveTurn(DIREITA); delay(200);
    diffDriveTurn(DIREITA); delay(200);

    Serial.println("Fase 2: Avanco de 2 Celulas com Leitura de Sensores");
    stepBlindExplore(); delay(200);
    stepBlindExplore(); delay(200);

    Serial.println("Fase 3: Exibicao do Mapa Descoberto");
    printMapAscii();

    Serial.println("Demonstracao concluida! O robo esta pronto para a busca completa.");
}

// ==============================================================================
// 15. DISPLAY OLED SSD1306 (Fog-of-War e Chassi Diferencial)
// ==============================================================================
void drawMazeMap() {
    int cs = 64 / mazeW;
    if (64 / mazeH < cs) cs = 64 / mazeH;
    if (cs > 15) cs = 15;
    int ox = (64 - mazeW * cs) / 2;
    int oy = (64 - mazeH * cs) / 2;
    if (oy < 0) oy = 0;

    auto GX = [&](int gx) { return clampInt(ox + gx * cs, 0, 127); };
    auto GY = [&](int gt) { return clampInt(oy + gt * cs, 0, 63); };
    bool blink = (millis() / 250) % 2;

    for (int x = 0; x < mazeW; x++) {
        for (int y = 0; y < mazeH; y++) {
            if (!visited[x][y]) continue; // Fog-of-War: só desenha o conhecido!

            int t = mazeH - 1 - y;
            int x0 = GX(x), x1 = GX(x + 1), y0 = GY(t), y1 = GY(t + 1);
            uint8_t w = knownWalls[x][y];

            if (w & WALL_BIT(NORTE)) oled.drawLine(x0, y0, x1, y0, SSD1306_WHITE);
            if (w & WALL_BIT(OESTE)) oled.drawLine(x0, y0, x0, y1, SSD1306_WHITE);
            if (w & WALL_BIT(LESTE)) oled.drawLine(x1, y0, x1, y1, SSD1306_WHITE);
            if (w & WALL_BIT(SUL))   oled.drawLine(x0, y1, x1, y1, SSD1306_WHITE);

            int cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;

            if (x == posX && y == posY) {
                if (blink) oled.fillRect(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, SSD1306_WHITE);
                else       oled.drawPixel(cx, cy, SSD1306_WHITE);
            } else if (exitFound && x == exitX && y == exitY) {
                oled.drawRect(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, SSD1306_WHITE);
            } else if (inPathStack[x][y]) {
                oled.fillRect(cx - 1, cy - 1, 2, 2, SSD1306_WHITE);
            }
        }
    }
}

void updateOled() {
    if (!oledReady) return;
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);

    if (viewMode == VIEW_MAZE_MAP) {
        drawMazeMap();
        const int tx = 68;
        oled.setTextSize(1);
        oled.setCursor(tx, 0);  oled.print(MAZES[mazeIndex].name);
        oled.setCursor(tx, 10); oled.print(STATE_NAMES[state]);
        oled.setCursor(tx, 20); oled.printf("%c%d %c", colLetter(posX), posY + 1, DIR_LETTER[heading]);
        oled.setCursor(tx, 32); oled.printf("P:%u B:%u", stepCount, backtrackCount);
        oled.setCursor(tx, 44); oled.printf("Rota: %u", pathStackLen);
        oled.setCursor(tx, 54); oled.printf("Bat:%.1fV", batteryVolts);
    } else {
        // Visualização do Chassi Diferencial e Roda Boba
        const int cx = 64, cy = 32;
        oled.drawRoundRect(cx - 16, cy - 22, 32, 44, 4, SSD1306_WHITE);
        // Rodas motrizes
        oled.fillRect(cx - 24, cy - 8, 7, 18, SSD1306_WHITE);
        oled.fillRect(cx + 17, cy - 8, 7, 18, SSD1306_WHITE);
        // Caster (Roda Boba)
        oled.drawCircle(cx, cy - 14, 5, SSD1306_WHITE);
        oled.setCursor(0, 0); oled.printf("CHASSI: %c%d %s", colLetter(posX), posY + 1, DIR_NAMES[heading]);
        oled.setCursor(0, 54); oled.printf("PWM L:%d R:%d", car.pwmL, car.pwmR);
    }

    oled.display();
}

// ==============================================================================
// 16. MAPA ASCII NO TERMINAL E TELEMETRIA
// ==============================================================================
void printMapAscii() {
    Serial.println("\n--- MAPA CONHECIDO DO MICROMOUSE (ASCII) ---");
    for (int y = mazeH - 1; y >= 0; y--) {
        // Paredes horizontais (Norte)
        for (int x = 0; x < mazeW; x++) {
            Serial.print("+");
            Serial.print((knownWalls[x][y] & WALL_BIT(NORTE)) ? "---" : "   ");
        }
        Serial.println("+");

        // Paredes verticais (Oeste/Leste) e células
        for (int x = 0; x < mazeW; x++) {
            Serial.print((knownWalls[x][y] & WALL_BIT(OESTE)) ? "|" : " ");
            if (x == posX && y == posY) {
                Serial.printf(" %c ", DIR_LETTER[heading]);
            } else if (exitFound && x == exitX && y == exitY) {
                Serial.print(" G ");
            } else if (inPathStack[x][y]) {
                Serial.print(" * ");
            } else if (visited[x][y]) {
                Serial.print(" . ");
            } else {
                Serial.print("   ");
            }
        }
        Serial.println("|");
    }
    // Borda Sul final
    for (int x = 0; x < mazeW; x++) Serial.print("+---");
    Serial.println("+\n");
}

// ==============================================================================
// 17. MENU SERIAL INTERATIVO
// ==============================================================================
void printMenu() {
    Serial.println("\n=======================================================");
    Serial.println("  SIMULACAO TOTAL MICROMOUSE - MENU DE TESTES E CONTROLE");
    Serial.println("=======================================================");
    Serial.println(" [START]  : Iniciar exploracao autonoma as cegas");
    Serial.println(" [STEP]   : Executar 1 unico passo de exploracao");
    Serial.println(" [SPEED]  : Iniciar Speed Run direto ate o gol descoberto");
    Serial.println(" [RESET]  : Reiniciar robo e limpar mapa da memoria");
    Serial.println(" [MAP]    : Imprimir mapa ASCII revelado no terminal");
    Serial.println(" [STATUS] : Exibir relatorio de telemetria");
    Serial.println(" [DEMO]   : Executar demonstracao automatizada passo a passo");
    Serial.println("--- TESTES MANUAIS DE MOTORES ---");
    Serial.println(" [F] ou FRENTE  : Avancar 1 celula (18 cm) com controle PD");
    Serial.println(" [T] ou TRAS    : Recuar 1 celula");
    Serial.println(" [L] ou ESQ     : Giro de 90 graus a esquerda no eixo");
    Serial.println(" [R] ou DIR     : Giro de 90 graus a direita no eixo");
    Serial.println(" [U] ou VOLTA   : Meia volta (giro de 180 graus)");
    Serial.println(" [PWM <val>]    : Ajustar potencia base dos motores (ex: PWM 190)");
    Serial.println("--- CONFIGURACOES ---");
    Serial.println(" [MAZE <0..3>]  : Trocar labirinto (0:MAZE01, 1:4x4, 2:8x4, 3:12x4)");
    Serial.println(" [VIEW]         : Alternar visualizacao do OLED (Mapa / Chassi)");
    Serial.println(" [AJUDA]        : Exibir este menu");
    Serial.println("=======================================================\n");
}

void processSerialCommand(const char* cmd) {
    if (strcasecmp(cmd, "START") == 0) {
        if (state == ST_AGUARDANDO || state == ST_MANUAL_TEST) {
            resetRobot();
            state = ST_EXPLORANDO;
            running = true;
            Serial.println("[START] Exploracao 100% as cegas iniciada!");
            soundChime();
        } else {
            running = !running;
            Serial.printf("[PAUSA] Execucao %s.\n", running ? "RETOMADA" : "PAUSADA");
        }
    } else if (strcasecmp(cmd, "STEP") == 0) {
        running = false;
        stepBlindExplore();
    } else if (strcasecmp(cmd, "SPEED") == 0) {
        beginSpeedRun();
    } else if (strcasecmp(cmd, "RESET") == 0) {
        resetRobot();
        Serial.println("[RESET] Robo reiniciado e memoria zerada.");
    } else if (strcasecmp(cmd, "MAP") == 0) {
        printMapAscii();
    } else if (strcasecmp(cmd, "STATUS") == 0) {
        Serial.printf("[STATUS] Pos: (%d,%d) | Dir: %s | Passos: %u | Bateria: %.2fV\n",
                      posX, posY, DIR_NAMES[heading], stepCount, batteryVolts);
    } else if (strcasecmp(cmd, "DEMO") == 0) {
        runDemonstrationRoutine();
    } else if (strcasecmp(cmd, "F") == 0 || strcasecmp(cmd, "FRENTE") == 0) {
        testManualMove(FRENTE);
    } else if (strcasecmp(cmd, "T") == 0 || strcasecmp(cmd, "TRAS") == 0) {
        testManualMove(TRAS);
    } else if (strcasecmp(cmd, "L") == 0 || strcasecmp(cmd, "ESQ") == 0) {
        testManualMove(ESQUERDA);
    } else if (strcasecmp(cmd, "R") == 0 || strcasecmp(cmd, "DIR") == 0) {
        testManualMove(DIREITA);
    } else if (strcasecmp(cmd, "U") == 0 || strcasecmp(cmd, "VOLTA") == 0) {
        diffDriveTurn(TRAS);
    } else if (strncasecmp(cmd, "PWM", 3) == 0) {
        int val = atoi(cmd + 3);
        if (val >= 60 && val <= 255) {
            userBasePwm = val;
            Serial.printf("[PWM] Potencia base ajustada para %d.\n", userBasePwm);
        } else {
            Serial.println("[ERRO] Use: PWM <60..255>");
        }
    } else if (strncasecmp(cmd, "MAZE", 4) == 0) {
        int idx = atoi(cmd + 4);
        loadMaze((uint8_t)idx);
        resetRobot();
        Serial.printf("[LABIRINTO] Carregado: %s (%dx%d).\n", MAZES[mazeIndex].name, mazeW, mazeH);
    } else if (strcasecmp(cmd, "VIEW") == 0) {
        viewMode = (ViewMode)((viewMode + 1) % NUM_VIEW_MODES);
    } else {
        printMenu();
    }
}

void handleSerial() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (serialLen > 0) {
                serialLine[serialLen] = '\0';
                processSerialCommand(serialLine);
                serialLen = 0;
            }
        } else if (serialLen < sizeof(serialLine) - 1) {
            serialLine[serialLen++] = c;
        }
    }
}

// ==============================================================================
// 18. BOTÕES FÍSICOS
// ==============================================================================
void handleButtons() {
    if (digitalRead(PIN_BTN_START) == LOW) {
        delay(25);
        if (digitalRead(PIN_BTN_START) == LOW) {
            processSerialCommand("START");
            while (digitalRead(PIN_BTN_START) == LOW) delay(10);
        }
    }
    if (digitalRead(PIN_BTN_MODE) == LOW) {
        delay(25);
        if (digitalRead(PIN_BTN_MODE) == LOW) {
            viewMode = (ViewMode)((viewMode + 1) % NUM_VIEW_MODES);
            while (digitalRead(PIN_BTN_MODE) == LOW) delay(10);
        }
    }
    if (digitalRead(PIN_BTN_STEP) == LOW) {
        delay(25);
        if (digitalRead(PIN_BTN_STEP) == LOW) {
            processSerialCommand("STEP");
            while (digitalRead(PIN_BTN_STEP) == LOW) delay(10);
        }
    }
    if (digitalRead(PIN_BTN_MAZE) == LOW) {
        delay(25);
        if (digitalRead(PIN_BTN_MAZE) == LOW) {
            loadMaze((mazeIndex + 1) % NUM_MAZES);
            resetRobot();
            while (digitalRead(PIN_BTN_MAZE) == LOW) delay(10);
        }
    }
}

// ==============================================================================
// 19. SETUP E LOOP PRINCIPAL
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
    pinMode(PIN_BTN_MODE,  INPUT_PULLUP);
    pinMode(PIN_BTN_STEP,  INPUT_PULLUP);
    pinMode(PIN_BTN_MAZE,  INPUT_PULLUP);

    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_BATTERY, INPUT);

    rgb.begin();
    rgb.setBrightness(40);
    setRgb(0, 0, 255);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    oledReady = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

    loadMaze(0); // MAZE01 (16x16)
    resetRobot();

    printMenu();
    updateRgb();
}

void loop() {
    uint32_t now = millis();

    handleSerial();
    handleButtons();

    // Execução autônoma do robô
    if (running && (now - lastStepMs >= stepPeriodMs())) {
        lastStepMs = now;
        raceTimeMs += stepPeriodMs();

        if (state == ST_EXPLORANDO || state == ST_RETROCEDENDO) {
            stepBlindExplore();
        } else if (state == ST_SPEED_RUN) {
            stepSpeedRun();
        }
    }

    // Atualização visual do OLED
    if (now - lastUiMs >= 100) {
        lastUiMs = now;
        updateOled();
        updateRgb();
    }
}
