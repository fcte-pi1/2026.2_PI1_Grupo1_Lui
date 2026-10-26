/**
 * ==============================================================================
 * TESTE DE 1 SENSOR DE DISTÂNCIA / MOVIMENTO (VL53L0X - ToF)
 * Micromouse - ESP32
 * ==============================================================================
 * Conexões:
 *   - SDA   -> GPIO 21
 *   - SCL   -> GPIO 22
 *   - XSHUT -> GPIO 4
 *   - VIN   -> 3.3V (ou 5V se o módulo tiver regulador interno)
 *   - GND   -> GND
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

#define PIN_SDA   21
#define PIN_SCL   22
#define PIN_XSHUT 4
#define PIN_LED   2  // LED onboard para feedback visual

VL53L0X sensor;
bool sensor_iniciado = false;

bool scanI2C() {
    Serial.println("\n--- [I2C SCAN] Varrendo barramento (SDA=21, SCL=22) ---");
    byte count = 0;
    bool achou_vl53 = false;

    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("  [+] Dispositivo encontrado no endereco: 0x%02X", addr);
            if (addr == 0x29) {
                Serial.print("  <-- SENSOR VL53L0X DETECTADO!");
                achou_vl53 = true;
            }
            Serial.println();
            count++;
        }
    }

    if (count == 0) {
        Serial.println("  [!] Nenhum dispositivo I2C respondeu.");
    } else {
        Serial.printf("  Total de dispositivos no barramento: %d\n", count);
    }
    Serial.println("-------------------------------------------------------");
    return achou_vl53;
}

bool inicializarSensor() {
    // Reset de hardware do sensor via XSHUT:
    // Nível baixo desliga; Nível alto liga.
    pinMode(PIN_XSHUT, OUTPUT);
    digitalWrite(PIN_XSHUT, LOW);
    delay(50);
    digitalWrite(PIN_XSHUT, HIGH);
    delay(50);

    bool detectado = scanI2C();

    sensor.setBus(&Wire);
    sensor.setTimeout(500);

    if (!sensor.init()) {
        return false;
    }

    // Configura orçamento de medição para ~33 ms (~30 Hz)
    sensor.setMeasurementTimingBudget(33000);
    sensor.startContinuous();
    return true;
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    delay(1000);

    Serial.println("\n=======================================================");
    Serial.println("     MICROMOUSE - TESTE INDIVIDUAL SENSOR VL53L0X     ");
    Serial.println("=======================================================");
    Serial.printf("Portas configuradas:\n");
    Serial.printf("  - SDA   : GPIO %d\n", PIN_SDA);
    Serial.printf("  - SCL   : GPIO %d\n", PIN_SCL);
    Serial.printf("  - XSHUT : GPIO %d\n", PIN_XSHUT);
    Serial.println("=======================================================");

    // Inicia barramento I2C a 100 kHz (mais estável para protoboard/jumpers)
    Wire.begin(PIN_SDA, PIN_SCL, 100000);

    Serial.println("[*] Testando comunicacao inicial...");
    sensor_iniciado = inicializarSensor();

    if (!sensor_iniciado) {
        Serial.println("\n[AVISO] Sensor VL53L0X ainda nao respondeu.");
        Serial.println("Verifique os jumpers:");
        Serial.println("  1. VIN/VCC -> 3.3V da ESP32");
        Serial.println("  2. GND     -> GND da ESP32");
        Serial.println("  3. SDA     -> Pino GPIO 21");
        Serial.println("  4. SCL     -> Pino GPIO 22");
        Serial.println("  5. XSHUT   -> Pino GPIO 4");
        Serial.println("Tentando reconectar a cada 2 segundos...\n");
    } else {
        Serial.println("\n[SUCESSO] Sensor VL53L0X pronto e efetuando leituras contínuas!");
    }
}

void loop() {
    if (!sensor_iniciado) {
        // Pisca LED indicando que está aguardando o sensor ser conectado
        digitalWrite(PIN_LED, HIGH);
        delay(200);
        digitalWrite(PIN_LED, LOW);
        delay(1800);

        Serial.println("[*] Tentando reconectar ao sensor...");
        sensor_iniciado = inicializarSensor();
        if (sensor_iniciado) {
            Serial.println("\n[SUCESSO] Sensor detectado e inicializado com sucesso!\n");
        }
        return;
    }

    uint16_t dist_mm = sensor.readRangeContinuousMillimeters();

    if (sensor.timeoutOccurred()) {
        Serial.println("[TIMEOUT] Sensor nao respondeu dentro do tempo limite.");
    } else if (dist_mm >= 8190 || dist_mm == 0) {
        Serial.println("[ALCANCE] Objeto fora de alcance / Sem reflexão");
    } else {
        float dist_cm = dist_mm / 10.0;

        // Barra gráfica para visualização rápida no Monitor Serial
        char bar[31];
        int num_chars = map(constrain(dist_mm, 20, 500), 20, 500, 1, 30);
        for (int i = 0; i < num_chars; i++) bar[i] = '=';
        bar[num_chars] = '\0';

        Serial.printf("[DISTANCIA] %4d mm | %5.1f cm  | [%-30s]\n", dist_mm, dist_cm, bar);

        // Pisca LED onboard a cada leitura com sucesso
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    }

    delay(100);
}
