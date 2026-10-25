/**
 * ==============================================================================
 * SIMULACAO WOKWI: ISSUE #136 [EAP 3.1.2.1]
 * Acionamento da Ponte H e Leitura de Encoders
 * Micromouse - PI1 2026.2 (Prof. Lui)
 * ==============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pinos Shield Rev 2.0
const uint8_t PIN_PWMA = 16, PIN_AIN1 = 18;
const uint8_t PIN_PWMB = 17, PIN_BIN1 = 23;
const uint8_t PIN_ENC_L = 27, PIN_ENC_R = 25;

volatile long ticks_left = 0;
volatile long ticks_right = 0;

int current_pwm_percent = 50;
bool current_forward = true;

void IRAM_ATTR isr_l() { ticks_left++; }
void IRAM_ATTR isr_r() { ticks_right++; }

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_PWMA, OUTPUT); pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_PWMB, OUTPUT); pinMode(PIN_BIN1, OUTPUT);

    pinMode(PIN_ENC_L, INPUT_PULLUP);
    pinMode(PIN_ENC_R, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L), isr_l, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R), isr_r, FALLING);

    // Configura PWM inicial
    analogWrite(PIN_PWMA, 127);
    analogWrite(PIN_PWMB, 127);
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_BIN1, HIGH);

    Wire.begin(21, 22);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("MICROMOUSE - GRUPO 1");
        display.println("ISSUE #136: PONTE H");
        display.println("& ENCODERS ODOMETRIA");
        display.display();
        delay(1500);
    }

    Serial.println("\n=======================================================");
    Serial.println("  WOKWI SIMULATOR - ISSUE #136: PONTE H E ENCODERS");
    Serial.println("=======================================================");
    Serial.println("Teclas de atalho no Serial:");
    Serial.println("  '1' -> 25% PWM  | '2' -> 50% PWM  | '3' -> 75% PWM  | '4' -> 100% PWM");
    Serial.println("  'f' -> Frente   | 'r' -> Re       | 'b' -> Freio");
    Serial.println("Pressione os botoes 'q' e 'w' para gerar pulsos de encoder!");
}

void apply_motor_control(int percent, bool fwd, bool brake = false) {
    int pwm_val = (percent * 255) / 100;
    if (brake) {
        digitalWrite(PIN_AIN1, HIGH);
        digitalWrite(PIN_BIN1, HIGH);
        analogWrite(PIN_PWMA, 255);
        analogWrite(PIN_PWMB, 255);
        Serial.println("[FREIO ATIVO] Motores travados (curto indutivo).");
        return;
    }

    digitalWrite(PIN_AIN1, fwd ? HIGH : LOW);
    digitalWrite(PIN_BIN1, fwd ? HIGH : LOW);
    analogWrite(PIN_PWMA, pwm_val);
    analogWrite(PIN_PWMB, pwm_val);
    Serial.printf("[MOTORES] Sentido: %s | PWM: %d%% (%d/255)\n",
                  fwd ? "FRENTE" : "RE", percent, pwm_val);
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '1') { current_pwm_percent = 25; apply_motor_control(25, current_forward); }
        else if (c == '2') { current_pwm_percent = 50; apply_motor_control(50, current_forward); }
        else if (c == '3') { current_pwm_percent = 75; apply_motor_control(75, current_forward); }
        else if (c == '4') { current_pwm_percent = 100; apply_motor_control(100, current_forward); }
        else if (c == 'f' || c == 'F') { current_forward = true; apply_motor_control(current_pwm_percent, true); }
        else if (c == 'r' || c == 'R') { current_forward = false; apply_motor_control(current_pwm_percent, false); }
        else if (c == 'b' || c == 'B') { apply_motor_control(0, false, true); }
    }

    // Auto-incremento dos encoders proporcional ao PWM quando motores acionados
    static uint32_t last_tick = 0;
    if (millis() - last_tick >= 100) {
        last_tick = millis();
        int step = (current_pwm_percent / 10);
        if (current_forward) {
            ticks_left += step;
            ticks_right += step;
        } else {
            ticks_left -= step;
            ticks_right -= step;
        }
    }

    // Atualiza OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("MOTOR & ENCODER MONITOR");
    display.drawLine(0, 10, 128, 10, WHITE);

    display.setCursor(0, 16);
    display.printf("PWM: %3d%%  Dir: %s\n", current_pwm_percent, current_forward ? "FWD" : "REV");
    display.printf("Ticks L: %6ld\n", ticks_left);
    display.printf("Ticks R: %6ld\n", ticks_right);

    float dist_mm = (ticks_left / 360.0f) * 100.5f;
    display.printf("Dist linear: %5.1f mm\n", dist_mm);

    display.display();
    delay(100);
}
