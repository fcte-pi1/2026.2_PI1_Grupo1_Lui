/**
 * ==============================================================================
 * FIRMWARE DE TESTE INDIVIDUAL - 1 SENSOR VL53L0X POR VEZ (PORTAS CONFIGURÁVEIS)
 * Micromouse PI1 2026.2 - ESP32
 * ==============================================================================
 *
 * Ideal para testar a saúde de cada sensor isoladamente antes da montagem final
 * ou quando ainda não se definiu quais GPIOs serão usados.
 *
 * RECURSOS:
 *   1. Troca as portas SDA, SCL e XSHUT ao vivo pelo Monitor Serial (sem recompilar).
 *   2. Comando "auto" varre combinações seguras de GPIOs para achar o sensor.
 *   3. Detecta automaticamente se SDA e SCL foram ligados invertidos.
 *   4. Exibe distância em mm/cm, barra gráfica e contador de leituras OK vs erros.
 *
 * COMANDOS NO MONITOR SERIAL (115200 bps):
 *   p <sda> <scl> [xshut]  -> Define portas (ex: "p 21 22" ou "p 21 22 4")
 *   x <xshut>              -> Define só o pino XSHUT (-1 para ignorar XSHUT)
 *   s                      -> Escaneia o barramento I2C atual (+ checa inversão)
 *   a                      -> Auto-scan em GPIOs seguros para achar SDA/SCL
 *   r                      -> Reinicializa o sensor atual
 *   c                      -> Pausa / Retoma leitura contínua
 *   m                      -> Exibe este menu
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// ==============================================================================
// CONFIGURAÇÃO PADRÃO AO LIGAR (Pode alterar aqui ou mudar ao vivo pela Serial)
// ==============================================================================
#define DEFAULT_PIN_SDA    21
#define DEFAULT_PIN_SCL    22
#define DEFAULT_PIN_XSHUT   4   // GPIO 4 configurado como XSHUT padrão (-1 desativa)
#define I2C_FREQ_HZ        100000
#define PIN_LED_ONBOARD    2

// GPIOs seguros da ESP32 para auto-scan (evita Flash 6-11, UART 1/3, Input-only 34-39
// e evita acionar pinos típicos da Ponte H: 13, 18, 25, 26, 27, 32, 33)
static const int SAFE_SCAN_PINS[] = {4, 5, 14, 16, 17, 19, 21, 22, 23};
static const int NUM_SAFE_SCAN_PINS = sizeof(SAFE_SCAN_PINS) / sizeof(SAFE_SCAN_PINS[0]);

// Estado atual das portas e do sensor
static int g_pin_sda = DEFAULT_PIN_SDA;
static int g_pin_scl = DEFAULT_PIN_SCL;
static int g_pin_xshut = DEFAULT_PIN_XSHUT;

static VL53L0X g_sensor;
static bool g_sensor_ok = false;
static bool g_leitura_ativa = true;

static uint32_t g_leituras_ok = 0;
static uint32_t g_leituras_erro = 0;
static uint32_t g_erros_consecutivos = 0;
static uint32_t g_ultimo_retry_ms = 0;
static uint32_t g_ultima_leitura_ms = 0;

static char g_cmd_buf[64];
static size_t g_cmd_len = 0;

static bool gpioValidoOutput(int pin) {
    if (pin < 0 || pin > 33) return false;
    if (pin >= 6 && pin <= 11) return false; // SPI Flash interna
    if (pin == 1 || pin == 3) return false;  // UART0 (Monitor Serial)
    return true;
}

static void aplicarXshutReset(void) {
    if (g_pin_xshut < 0) return;

    if (!gpioValidoOutput(g_pin_xshut)) {
        Serial.printf("[ERRO] GPIO %d não pode ser usado como saída XSHUT.\n", g_pin_xshut);
        return;
    }

    pinMode(g_pin_xshut, OUTPUT);
    digitalWrite(g_pin_xshut, LOW);
    delay(30);
    digitalWrite(g_pin_xshut, HIGH);
    delay(30);
}

static void iniciarBarramentoI2C(int sda, int scl) {
    Wire.end();
    delay(10);
    Wire.begin(sda, scl, I2C_FREQ_HZ);
}

static uint8_t escanearBarramentoI2C(int sda, int scl, bool verboso, uint8_t* primeiro_addr_out) {
    iniciarBarramentoI2C(sda, scl);

    uint8_t count = 0;
    uint8_t primeiro = 0;

    if (verboso) {
        Serial.printf("\n--- [I2C SCAN] SDA=GPIO %d | SCL=GPIO %d ---\n", sda, scl);
    }

    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            if (primeiro == 0) primeiro = addr;
            count++;
            if (verboso) {
                Serial.printf("  [+] Endereço 0x%02X encontrado", addr);
                if (addr == 0x29) {
                    Serial.print("  <-- VL53L0X (endereço padrão)");
                } else if (addr >= 0x30 && addr <= 0x33) {
                    Serial.print("  <-- VL53L0X (endereço remapeado)");
                }
                Serial.println();
            }
        }
    }

    if (verboso) {
        if (count == 0) {
            Serial.println("  [!] Nenhum dispositivo respondeu neste par de pinos.");
        }
        Serial.println("-------------------------------------------------");
    }

    if (primeiro_addr_out) {
        *primeiro_addr_out = primeiro;
    }
    return count;
}

static bool inicializarSensorAtual(bool verboso) {
    if (!gpioValidoOutput(g_pin_sda) || !gpioValidoOutput(g_pin_scl) || g_pin_sda == g_pin_scl) {
        Serial.println("[ERRO] Portas SDA/SCL inválidas.");
        g_sensor_ok = false;
        return false;
    }

    aplicarXshutReset();

    uint8_t addr_encontrado = 0;
    uint8_t qtd = escanearBarramentoI2C(g_pin_sda, g_pin_scl, verboso, &addr_encontrado);

    if (qtd == 0) {
        // Verifica rapidamente se SDA e SCL não estão apenas invertidos
        uint8_t addr_inv = 0;
        if (escanearBarramentoI2C(g_pin_scl, g_pin_sda, false, &addr_inv) > 0) {
            Serial.printf("\n[DICA IMPORTANTE] Encontrei um dispositivo em 0x%02X com os pinos INVERTIDOS!\n", addr_inv);
            Serial.printf("  -> Parece que SDA está no GPIO %d e SCL no GPIO %d.\n", g_pin_scl, g_pin_sda);
            Serial.printf("  -> Digite: p %d %d\n\n", g_pin_scl, g_pin_sda);
            // Volta barramento para a config atual
            iniciarBarramentoI2C(g_pin_sda, g_pin_scl);
        }
        g_sensor_ok = false;
        return false;
    }

    // Se o sensor ainda estava com endereço alterado sem power-cycle, ajusta o objeto
    if (addr_encontrado != 0x29 && addr_encontrado >= 0x30 && addr_encontrado <= 0x33) {
        g_sensor.setAddress(addr_encontrado);
    } else {
        g_sensor.setAddress(0x29);
    }

    g_sensor.setBus(&Wire);
    g_sensor.setTimeout(300);

    if (!g_sensor.init()) {
        if (verboso) {
            Serial.println("[FALHA] Dispositivo respondeu no I2C, mas VL53L0X.init() falhou.");
        }
        g_sensor_ok = false;
        return false;
    }

    g_sensor.setMeasurementTimingBudget(33000); // ~33 ms (boa precisão)
    g_sensor.startContinuous();

    g_sensor_ok = true;
    g_leituras_ok = 0;
    g_leituras_erro = 0;
    g_erros_consecutivos = 0;

    Serial.printf("\n[OK] SENSOR VL53L0X SAUDÁVEL E INICIADO! (SDA=%d, SCL=%d, XSHUT=%d, Addr=0x%02X)\n",
                  g_pin_sda, g_pin_scl, g_pin_xshut, addr_encontrado ? addr_encontrado : 0x29);
    Serial.println("Aproxime a mão na frente do sensor (digite 'c' para pausar leituras).\n");
    return true;
}

static void executarAutoScanPinos(void) {
    Serial.println("\n=======================================================");
    Serial.println("   AUTO-SCAN SEGURO DE PINOS I2C (1 SENSOR POR VEZ)");
    Serial.println("=======================================================");
    Serial.print("GPIOs testados: ");
    for (int i = 0; i < NUM_SAFE_SCAN_PINS; i++) {
        Serial.printf("%d%s", SAFE_SCAN_PINS[i], (i < NUM_SAFE_SCAN_PINS - 1) ? ", " : "\n");
    }
    if (g_pin_xshut >= 0) {
        Serial.printf("Mantendo XSHUT (GPIO %d) em HIGH durante a busca...\n", g_pin_xshut);
        aplicarXshutReset();
    } else {
        Serial.println("Obs: Se XSHUT estiver ligado à ESP32, defina antes com: x <gpio>");
    }

    bool achou = false;
    for (int i = 0; i < NUM_SAFE_SCAN_PINS && !achou; i++) {
        for (int j = 0; j < NUM_SAFE_SCAN_PINS && !achou; j++) {
            if (i == j) continue;
            int cand_sda = SAFE_SCAN_PINS[i];
            int cand_scl = SAFE_SCAN_PINS[j];

            if (cand_sda == g_pin_xshut || cand_scl == g_pin_xshut) continue;

            uint8_t addr = 0;
            if (escanearBarramentoI2C(cand_sda, cand_scl, false, &addr) > 0) {
                Serial.printf("\n[ENCONTRADO!] Dispositivo I2C (0x%02X) em SDA=GPIO %d | SCL=GPIO %d!\n",
                              addr, cand_sda, cand_scl);
                g_pin_sda = cand_sda;
                g_pin_scl = cand_scl;
                achou = true;
            }
        }
    }

    if (achou) {
        inicializarSensorAtual(true);
    } else {
        Serial.println("\n[NADA ENCONTRADO] Nenhum sensor respondeu nos GPIOs seguros.");
        Serial.println("Verifique: VCC (3.3V), GND e se o pino XSHUT precisa de HIGH (comando 'x <pino>').");
        iniciarBarramentoI2C(g_pin_sda, g_pin_scl);
    }
}

static void exibirMenu(void) {
    Serial.println("\n=======================================================");
    Serial.println("   TESTE INDIVIDUAL DE SENSOR VL53L0X (1 POR VEZ)");
    Serial.println("=======================================================");
    Serial.printf("Configuração atual -> SDA: GPIO %d | SCL: GPIO %d | XSHUT: %s\n",
                  g_pin_sda, g_pin_scl,
                  (g_pin_xshut >= 0) ? String("GPIO " + String(g_pin_xshut)).c_str() : "DESATIVADO (-1)");
    Serial.printf("Status do sensor   -> %s\n", g_sensor_ok ? "CONECTADO E SAUDÁVEL" : "AGUARDANDO DETECÇÃO");
    Serial.println("-------------------------------------------------------");
    Serial.println("Comandos (digite e aperte ENTER):");
    Serial.println("  p <sda> <scl> [xshut]  -> Trocar pinos (ex: p 21 22 ou p 21 22 4)");
    Serial.println("  x <gpio>               -> Definir pino XSHUT (-1 para desativar)");
    Serial.println("  s                      -> Escanear barramento I2C atual");
    Serial.println("  a                      -> Auto-scan de GPIOs para achar o sensor");
    Serial.println("  r                      -> Reinicializar sensor atual");
    Serial.println("  c                      -> Pausar/retomar leitura contínua");
    Serial.println("  m                      -> Exibir este menu");
    Serial.println("=======================================================\n");
}

static void processarLinhaComando(char* linha) {
    // Remove espaços iniciais
    while (*linha && isspace((unsigned char)*linha)) linha++;
    if (*linha == '\0') return;

    char cmd = (char)tolower((unsigned char)linha[0]);

    if (cmd == 'm' || cmd == 'h' || cmd == '?') {
        exibirMenu();
    } else if (cmd == 'c') {
        g_leitura_ativa = !g_leitura_ativa;
        Serial.printf("\n[INFO] Leitura contínua %s.\n", g_leitura_ativa ? "RETOMADA" : "PAUSADA");
    } else if (cmd == 's') {
        uint8_t addr = 0;
        uint8_t n = escanearBarramentoI2C(g_pin_sda, g_pin_scl, true, &addr);
        if (n == 0) {
            uint8_t addr_inv = 0;
            if (escanearBarramentoI2C(g_pin_scl, g_pin_sda, false, &addr_inv) > 0) {
                Serial.printf("[DICA] Encontrado em 0x%02X com SDA=%d e SCL=%d (invertido!).\n",
                              addr_inv, g_pin_scl, g_pin_sda);
                iniciarBarramentoI2C(g_pin_sda, g_pin_scl);
            }
        }
    } else if (cmd == 'a') {
        executarAutoScanPinos();
    } else if (cmd == 'r') {
        Serial.println("\n[*] Reinicializando sensor...");
        inicializarSensorAtual(true);
    } else if (cmd == 'x') {
        char* ptr = linha + 1;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0') {
            Serial.println("[USO] Exemplo: x 4   (ou x -1 para desativar XSHUT)");
            return;
        }
        int novo_xshut = atoi(ptr);
        if (novo_xshut >= 0 && !gpioValidoOutput(novo_xshut)) {
            Serial.printf("[ERRO] GPIO %d inválido para XSHUT.\n", novo_xshut);
            return;
        }
        g_pin_xshut = novo_xshut;
        Serial.printf("[CONFIG] XSHUT atualizado para %d. Tentando inicializar...\n", g_pin_xshut);
        inicializarSensorAtual(true);
    } else if (cmd == 'p') {
        int sda = -1, scl = -1, xshut = g_pin_xshut;
        int parsed = sscanf(linha + 1, "%d %d %d", &sda, &scl, &xshut);
        if (parsed < 2) {
            Serial.println("[USO] Exemplo: p 21 22   ou   p 21 22 4");
            return;
        }
        if (!gpioValidoOutput(sda) || !gpioValidoOutput(scl) || sda == scl) {
            Serial.println("[ERRO] Pinos SDA/SCL inválidos ou iguais (evite 1, 3, 6-11, 34-39).");
            return;
        }
        if (parsed >= 3 && xshut >= 0 && !gpioValidoOutput(xshut)) {
            Serial.printf("[ERRO] Pino XSHUT (GPIO %d) inválido.\n", xshut);
            return;
        }

        g_pin_sda = sda;
        g_pin_scl = scl;
        if (parsed >= 3) g_pin_xshut = xshut;

        Serial.printf("\n[CONFIG] Novas portas -> SDA=%d | SCL=%d | XSHUT=%d\n",
                      g_pin_sda, g_pin_scl, g_pin_xshut);
        inicializarSensorAtual(true);
    } else {
        Serial.printf("[AVISO] Comando desconhecido: '%s'. Digite 'm' para ver a ajuda.\n", linha);
    }
}

void setup(void) {
    Serial.begin(115200);
    pinMode(PIN_LED_ONBOARD, OUTPUT);
    digitalWrite(PIN_LED_ONBOARD, LOW);
    delay(800);

    exibirMenu();
    Serial.println("[*] Tentando detectar sensor nas portas padrão...");
    inicializarSensorAtual(true);
}

void loop(void) {
    // 1. Leitura de comandos vindos da Serial
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\r' || ch == '\n') {
            if (g_cmd_len > 0) {
                g_cmd_buf[g_cmd_len] = '\0';
                processarLinhaComando(g_cmd_buf);
                g_cmd_len = 0;
            }
        } else if (g_cmd_len < sizeof(g_cmd_buf) - 1) {
            g_cmd_buf[g_cmd_len++] = ch;
        }
    }

    uint32_t now = millis();

    // 2. Se o sensor ainda não foi detectado, tenta reconectar a cada 2.5s
    if (!g_sensor_ok) {
        if (now - g_ultimo_retry_ms >= 2500) {
            g_ultimo_retry_ms = now;
            digitalWrite(PIN_LED_ONBOARD, !digitalRead(PIN_LED_ONBOARD));
            Serial.printf("[*] Aguardando sensor (SDA=%d, SCL=%d, XSHUT=%d)... (digite 'a' para auto-scan ou 'p <sda> <scl>')\n",
                          g_pin_sda, g_pin_scl, g_pin_xshut);
            inicializarSensorAtual(false);
        }
        return;
    }

    // 3. Leitura contínua de saúde do sensor (~10 Hz)
    if (g_leitura_ativa && (now - g_ultima_leitura_ms >= 100)) {
        g_ultima_leitura_ms = now;

        uint16_t dist_mm = g_sensor.readRangeContinuousMillimeters();

        if (g_sensor.timeoutOccurred()) {
            g_leituras_erro++;
            g_erros_consecutivos++;
            Serial.printf("[ERRO TIMEOUT] Falha de leitura I2C (ok=%lu | erros=%lu)\n",
                          (unsigned long)g_leituras_ok, (unsigned long)g_leituras_erro);

            if (g_erros_consecutivos >= 10) {
                Serial.println("[ALERTA] 10 falhas seguidas! Sensor desconectado ou mau contato. Tentando reinicializar...");
                g_sensor_ok = false;
            }
        } else if (dist_mm >= 8190 || dist_mm == 0) {
            g_leituras_ok++;
            g_erros_consecutivos = 0;
            Serial.printf("[SAUDÁVEL] Fora de alcance (> 1.2m / sem reflexão) | OK=%lu Erros=%lu\n",
                          (unsigned long)g_leituras_ok, (unsigned long)g_leituras_erro);
        } else {
            g_leituras_ok++;
            g_erros_consecutivos = 0;

            char bar[25];
            int chars = map(constrain(dist_mm, 20, 400), 20, 400, 1, 24);
            for (int i = 0; i < 24; i++) bar[i] = (i < chars) ? '=' : ' ';
            bar[24] = '\0';

            Serial.printf("[OK] %4u mm (%5.1f cm) | [%s] | OK=%lu Erros=%lu\n",
                          dist_mm, dist_mm / 10.0f, bar,
                          (unsigned long)g_leituras_ok, (unsigned long)g_leituras_erro);

            digitalWrite(PIN_LED_ONBOARD, !digitalRead(PIN_LED_ONBOARD));
        }
    }
}
