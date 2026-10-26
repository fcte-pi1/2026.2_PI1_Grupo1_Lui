/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #140 [EAP 3.2.1.1]
 * Gateway ESP-NOW -> Serial JSON Bridge
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t LED_TX = 2;
uint16_t seq_counter = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(LED_TX, OUTPUT);

    Wire.begin(21, 22);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("MICROMOUSE GATEWAY");
        display.println("ISSUE #140: JSON BRIDGE");
        display.println("ESP-NOW -> Serial 115k");
        display.display();
        delay(1500);
    }

    Serial.println("\n{\"status\":\"GATEWAY_ONLINE\",\"protocol_version\":1,\"baud\":115200}");
    Serial.println("// Pressione 's' para simular a chegada de um pacote imediato.");
}

void emit_telemetry_json(uint16_t seq, uint32_t t_ms, int rssi, int x, int y, int h, int st, int l, int f, int r, float yaw, float vbat) {
    digitalWrite(LED_TX, HIGH);

    StaticJsonDocument<256> doc;
    doc["seq"] = seq;
    doc["timestamp_ms"] = t_ms;
    doc["rssi"] = rssi;

    JsonObject pos = doc.createNestedObject("pos");
    pos["x"] = x;
    pos["y"] = y;

    doc["heading"] = h;
    doc["state"] = st;
    doc["walls"] = 0x0A;

    JsonObject sens = doc.createNestedObject("sensors_cm");
    sens["left"] = l;
    sens["front"] = f;
    sens["right"] = r;

    doc["imu_yaw"] = yaw;
    doc["battery_v"] = vbat;

    serializeJson(doc, Serial);
    Serial.println();

    digitalWrite(LED_TX, LOW);

    // Atualiza OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("GATEWAY TELEMETRY [1Hz]");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 14);
    display.printf("Seq: %4u  RSSI: %3d\n", seq, rssi);
    display.printf("Pos: (%d, %d) Head: %d\n", x, y, h);
    display.printf("Dist: L%d F%d R%d cm\n", l, f, r);
    display.printf("Bateria: %.2f V\n", vbat);
    display.printf("Yaw: %.1f deg\n", yaw);

    display.display();
}

void loop() {
    static uint32_t last_tx = 0;

    // Emissao regular a 1 Hz (1000 ms)
    if (millis() - last_tx >= 1000) {
        last_tx = millis();
        seq_counter++;

        // Simula deslocamento do robo
        int x = (seq_counter / 4) % 16;
        int y = (seq_counter / 2) % 16;
        int heading = (seq_counter / 8) % 4;
        int d_front = 15 + (seq_counter % 25);
        float battery = 8.4f - (seq_counter * 0.002f);
        if (battery < 7.0f) battery = 7.4f;

        emit_telemetry_json(seq_counter, millis(), -62 + (random(-4, 4)), x, y, heading, 2, 6, d_front, 6, 90.0f, battery);
    }

    if (Serial.available()) {
        char c = Serial.read();
        if (c == 's' || c == 'S') {
            seq_counter++;
            emit_telemetry_json(seq_counter, millis(), -58, 0, 0, 0, 1, 6, 18, 6, 0.0f, 8.2f);
        }
    }
    delay(20);
}
