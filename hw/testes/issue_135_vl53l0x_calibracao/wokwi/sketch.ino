/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #135 [EAP 3.1.1.1]
 * Calibracao e Leitura de 1 Sensor de Distancia VL53L0X
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 * PORTAS / PINOS:
 *   - I2C SDA: GPIO 21
 *   - I2C SCL: GPIO 22
 *   - SENSOR TRIG/XSHUT: GPIO 32
 *   - SENSOR ECHO:       GPIO 26
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t PIN_TRIG = 32;
const uint8_t PIN_ECHO = 26;

float read_distance_mm() {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long duration = pulseIn(PIN_ECHO, HIGH, 30000);
    if (duration == 0) return 300.0f; // Sem anteparo

    float dist_cm = (duration * 0.0343f) / 2.0f;
    return dist_cm * 10.0f; // Converte para mm
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    Wire.begin(21, 22);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    Serial.println("\n=======================================================");
    Serial.println("  WOKWI - ISSUE #135: 1 SENSOR VL53L0X");
    Serial.println("=======================================================");
    Serial.println("Pinos Conectados:");
    Serial.println("  - SDA: GPIO 21 | SCL: GPIO 22");
    Serial.println("  - Sensor: GPIO 32 / GPIO 26");
    Serial.println("Digite 't' para calibracao automatica nos 7 pontos.");
}

void run_test_7_points() {
    const float points[] = {20.0f, 40.0f, 60.0f, 80.0f, 100.0f, 150.0f, 200.0f};
    Serial.println("\n-----------------------------------------------------------------");
    Serial.println("  CALIBRACAO NOS 7 PONTOS REQUERIDOS (20 LEITURAS POR PONTO)");
    Serial.println("-----------------------------------------------------------------");
    Serial.printf("%-10s | %-12s | %-14s | %-12s | %-8s\n",
                  "Ref (mm)", "Media (mm)", "DesvPadrao", "Erro (mm)", "Status");
    Serial.println("-----------------------------------------------------------------");

    for (int p = 0; p < 7; p++) {
        float ref = points[p];
        float sum = 0;
        float samples[20];

        for (int i = 0; i < 20; i++) {
            float raw = read_distance_mm();
            samples[i] = raw;
            sum += raw;
            delay(20);
        }
        float mean = sum / 20.0f;
        float sq_diff = 0;
        for (int i = 0; i < 20; i++) sq_diff += (samples[i] - mean) * (samples[i] - mean);
        float std_dev = sqrt(sq_diff / 19.0f);
        float err = fabs(mean - ref);

        Serial.printf("%-10.1f | %-12.2f | %-14.2f | %-12.2f | %-8s\n",
                      ref, mean, std_dev, err, (err <= 15.0f ? "OK" : "ALERTA"));
    }
    Serial.println("-----------------------------------------------------------------");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 't' || c == 'T') {
            run_test_7_points();
        }
    }

    float d_mm = read_distance_mm();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("VL53L0X DISTANCE [mm]");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 18);
    display.setTextSize(2);
    display.printf("%4.0f mm\n", d_mm);

    display.setTextSize(1);
    display.setCursor(0, 42);
    if (d_mm >= 30.0f && d_mm <= 180.0f) {
        display.println("FAIXA UTIL: NORMAL (OK)");
    } else if (d_mm < 30.0f) {
        display.println("FAIXA UTIL: MUITO PERTO");
    } else {
        display.println("FAIXA UTIL: LIVRE");
    }

    // Barra gráfica
    int bar_w = constrain((int)(d_mm / 200.0f * 120.0f), 0, 120);
    display.drawRect(4, 54, 120, 8, WHITE);
    display.fillRect(4, 54, bar_w, 8, WHITE);

    display.display();

    Serial.printf("[DISTANCIA] VL53L0X: %5.1f mm (Faixa util: %s)\n",
                  d_mm, (d_mm >= 30.0f && d_mm <= 180.0f) ? "OK" : "FORA");
    delay(100);
}
