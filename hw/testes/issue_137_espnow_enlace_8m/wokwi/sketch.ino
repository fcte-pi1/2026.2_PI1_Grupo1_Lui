/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #137 [EAP 3.1.5.1]
 * Ensaio de Perda de Pacotes e RSSI do Enlace ESP-NOW a 8 metros
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const uint8_t LED_OK = 2;
const uint8_t LED_LOSS = 4;

uint32_t current_sent = 0;
uint32_t current_rcv = 0;
uint32_t current_lost = 0;
float current_rssi = -64.0f;
int simulated_dist_m = 8;
bool is_inside_maze = false;

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(LED_OK, OUTPUT);
    pinMode(LED_LOSS, OUTPUT);

    Wire.begin(21, 22);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("MICROMOUSE - GRUPO 1");
        display.println("ISSUE #137: ESP-NOW");
        display.println("PERDA DE PACOTES @ 8m");
        display.display();
        delay(1500);
    }

    Serial.println("\n=======================================================");
    Serial.println("  WOKWI SIMULATOR - ISSUE #137: ENLACE ESP-NOW A 8m");
    Serial.println("=======================================================");
    Serial.println("Controles via Serial:");
    Serial.println("  '1' -> Distancia: 2m  | '2' -> Distancia: 4m");
    Serial.println("  '3' -> Distancia: 8m  | '4' -> Distancia: 8m (No Labirinto)");
    Serial.println("  't' -> Executar ensaio automatico de 1000 pacotes");
}

void run_1000_packets_test(int dist, bool in_maze) {
    float base_rssi = (dist == 2) ? -48.0f : (dist == 4) ? -56.0f : (in_maze ? -69.5f : -65.0f);
    float loss_prob = (dist == 2) ? 0.001f : (dist == 4) ? 0.003f : (in_maze ? 0.012f : 0.008f);

    Serial.printf("\n[INICIANDO TESTE] Distancia: %d m %s | Enviando 1000 pacotes...\n",
                  dist, in_maze ? "(Dentro do Labirinto)" : "(Visada Direta)");

    current_sent = 0;
    current_rcv = 0;
    current_lost = 0;
    float rssi_sum = 0;

    for (int i = 1; i <= 1000; i++) {
        current_sent++;
        float sample_rssi = base_rssi + ((random(-20, 20)) / 10.0f);

        // Simulacao estocastica de perda
        if (random(0, 10000) / 10000.0f > loss_prob) {
            current_rcv++;
            rssi_sum += sample_rssi;
            digitalWrite(LED_OK, HIGH);
            digitalWrite(LED_LOSS, LOW);
        } else {
            current_lost++;
            digitalWrite(LED_OK, LOW);
            digitalWrite(LED_LOSS, HIGH);
            Serial.printf("  [DROP] Pacote seq %d perdido!\n", i);
        }

        if (i % 200 == 0) {
            float p_loss = (float)current_lost / (float)current_sent * 100.0f;
            Serial.printf("  Progresso: %4d / 1000 | Recebidos: %4d | Perda: %.2f%% | RSSI: %.1f dBm\n",
                          i, current_rcv, p_loss, sample_rssi);
        }
        delay(5);
    }

    digitalWrite(LED_OK, HIGH);
    digitalWrite(LED_LOSS, LOW);

    float final_loss = (float)current_lost / 1000.0f * 100.0f;
    float avg_rssi = rssi_sum / current_rcv;

    Serial.println("=========================================================================");
    Serial.printf("RESULTADO: %d Enviados | %d Recebidos | %d Perdidos (%.2f%%) | RSSI Medio: %.1f dBm\n",
                  current_sent, current_rcv, current_lost, final_loss, avg_rssi);
    Serial.printf("CRITERIO RNF-ELE03 (< 2%% perda): %s\n", (final_loss < 2.0f) ? "ATENDIDO (PASS)" : "REPROVADO");
    Serial.println("=========================================================================");
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '1') { simulated_dist_m = 2; is_inside_maze = false; Serial.println("Cenario: 2 metros"); }
        else if (c == '2') { simulated_dist_m = 4; is_inside_maze = false; Serial.println("Cenario: 4 metros"); }
        else if (c == '3') { simulated_dist_m = 8; is_inside_maze = false; Serial.println("Cenario: 8 metros (Visada)"); }
        else if (c == '4') { simulated_dist_m = 8; is_inside_maze = true; Serial.println("Cenario: 8 metros (No Labirinto)"); }
        else if (c == 't' || c == 'T') { run_1000_packets_test(simulated_dist_m, is_inside_maze); }
    }

    // Atualiza OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("ESP-NOW LINK MONITOR");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 16);
    display.printf("Dist: %d m %s\n", simulated_dist_m, is_inside_maze ? "[LAB]" : "[VIS]");
    display.printf("TX: %4d  RX: %4d\n", current_sent, current_rcv);

    float loss_pct = (current_sent > 0) ? ((float)current_lost / (float)current_sent * 100.0f) : 0.0f;
    display.printf("Perda: %.2f %%\n", loss_pct);
    display.printf("RSSI:  %.1f dBm\n", (simulated_dist_m == 2 ? -48.0 : (simulated_dist_m == 4 ? -56.0 : (is_inside_maze ? -69.5 : -65.0))));

    display.setCursor(0, 52);
    display.printf("Status: %s\n", (loss_pct < 2.0f) ? "RNF-ELE03 OK" : "ALERTA PERDA");

    display.display();
    delay(200);
}
