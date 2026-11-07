/**
 * ==============================================================================
 * TESTE DE 3 SENSORES DE DISTÂNCIA ToF (VL53L0X) NO MESMO BARRAMENTO I2C
 * Micromouse - ESP32-WROOM-32D
 * ==============================================================================
 * Conexões:
 *   - Barramento I2C compartilhado: SDA -> GPIO 21 | SCL -> GPIO 22
 *   - XSHUT Sensor 1 (Esquerdo): GPIO 4  -> Reendereçado para 0x30
 *   - XSHUT Sensor 2 (Frontal) : GPIO 16 -> Reendereçado para 0x31
 *   - XSHUT Sensor 3 (Direito) : GPIO 17 -> Reendereçado para 0x32
 *   - Alimentação: 3.3V e GND comuns
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

#define PIN_SDA      21
#define PIN_SCL      22

#define PIN_XSHUT_1   4  // Sensor 1 (Esquerdo)
#define PIN_XSHUT_2  16  // Sensor 2 (Frontal)
#define PIN_XSHUT_3  17  // Sensor 3 (Direito)

#define ADDR_SENSOR_1 0x30
#define ADDR_SENSOR_2 0x31
#define ADDR_SENSOR_3 0x32

VL53L0X sensor1;
VL53L0X sensor2;
VL53L0X sensor3;

bool s1_ok = false;
bool s2_ok = false;
bool s3_ok = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=======================================================");
    Serial.println("  MICROMOUSE - TESTE DE 3 SENSORES VL53L0X (I2C MULTI)");
    Serial.println("=======================================================");
    Serial.printf("Pinos I2C: SDA=GPIO %d, SCL=GPIO %d\n", PIN_SDA, PIN_SCL);
    Serial.printf("XSHUT: S1(Esq)=GPIO %d, S2(Fre)=GPIO %d, S3(Dir)=GPIO %d\n",
                  PIN_XSHUT_1, PIN_XSHUT_2, PIN_XSHUT_3);
    Serial.println("=======================================================");

    // Inicia barramento I2C a 100 kHz
    Wire.begin(PIN_SDA, PIN_SCL, 100000);

    // 1. Desliga todos os sensores colocando XSHUT em LOW
    pinMode(PIN_XSHUT_1, OUTPUT);
    pinMode(PIN_XSHUT_2, OUTPUT);
    pinMode(PIN_XSHUT_3, OUTPUT);

    digitalWrite(PIN_XSHUT_1, LOW);
    digitalWrite(PIN_XSHUT_2, LOW);
    digitalWrite(PIN_XSHUT_3, LOW);
    delay(20);

    // 2. Inicializa Sensor 1 (Esquerdo)
    Serial.print("[1/3] Inicializando Sensor 1 (GPIO 4)... ");
    digitalWrite(PIN_XSHUT_1, HIGH);
    delay(20);
    sensor1.setBus(&Wire);
    sensor1.setTimeout(500);
    if (sensor1.init()) {
        sensor1.setAddress(ADDR_SENSOR_1);
        sensor1.setMeasurementTimingBudget(33000);
        sensor1.startContinuous();
        s1_ok = true;
        Serial.printf("OK! Endereço: 0x%02X\n", ADDR_SENSOR_1);
    } else {
        Serial.println("FALHA!");
    }

    // 3. Inicializa Sensor 2 (Frontal)
    Serial.print("[2/3] Inicializando Sensor 2 (GPIO 16)... ");
    digitalWrite(PIN_XSHUT_2, HIGH);
    delay(20);
    sensor2.setBus(&Wire);
    sensor2.setTimeout(500);
    if (sensor2.init()) {
        sensor2.setAddress(ADDR_SENSOR_2);
        sensor2.setMeasurementTimingBudget(33000);
        sensor2.startContinuous();
        s2_ok = true;
        Serial.printf("OK! Endereço: 0x%02X\n", ADDR_SENSOR_2);
    } else {
        Serial.println("FALHA!");
    }

    // 4. Inicializa Sensor 3 (Direito)
    Serial.print("[3/3] Inicializando Sensor 3 (GPIO 17)... ");
    digitalWrite(PIN_XSHUT_3, HIGH);
    delay(20);
    sensor3.setBus(&Wire);
    sensor3.setTimeout(500);
    if (sensor3.init()) {
        sensor3.setAddress(ADDR_SENSOR_3);
        sensor3.setMeasurementTimingBudget(33000);
        sensor3.startContinuous();
        s3_ok = true;
        Serial.printf("OK! Endereço: 0x%02X\n", ADDR_SENSOR_3);
    } else {
        Serial.println("FALHA!");
    }

    Serial.println("\n-------------------------------------------------------");
    if (s1_ok && s2_ok && s3_ok) {
        Serial.println("[SUCESSO] Todos os 3 sensores VL53L0X prontos e medindo!");
    } else {
        Serial.println("[AVISO] Um ou mais sensores falharam na inicialização.");
    }
    Serial.println("-------------------------------------------------------\n");
}

void loop() {
    uint16_t d1 = s1_ok ? sensor1.readRangeContinuousMillimeters() : 8190;
    uint16_t d2 = s2_ok ? sensor2.readRangeContinuousMillimeters() : 8190;
    uint16_t d3 = s3_ok ? sensor3.readRangeContinuousMillimeters() : 8190;

    char bar1[11], bar2[11], bar3[11];
    int l1 = map(constrain(d1, 20, 200), 20, 200, 1, 10);
    int l2 = map(constrain(d2, 20, 200), 20, 200, 1, 10);
    int l3 = map(constrain(d3, 20, 200), 20, 200, 1, 10);

    for (int i = 0; i < 10; i++) {
        bar1[i] = (i < l1) ? '=' : ' ';
        bar2[i] = (i < l2) ? '=' : ' ';
        bar3[i] = (i < l3) ? '=' : ' ';
    }
    bar1[10] = '\0'; bar2[10] = '\0'; bar3[10] = '\0';

    Serial.printf("[DISTÂNCIA] Esq: %4d mm [%s] | Fre: %4d mm [%s] | Dir: %4d mm [%s]\n",
                  d1, bar1, d2, bar2, d3, bar3);

    delay(100);
}
