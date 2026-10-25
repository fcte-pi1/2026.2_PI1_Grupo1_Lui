# Teste Individual do Sensor ToF VL53L0X

Este diretório contém o firmware para teste unitário e calibração de um único sensor óptico de distância por tempo de voo (ToF) VL53L0X conectado ao ESP32.

---

## 1. Conexões Físicas no ESP32 DevKit V1
* **I2C SDA:** GPIO 21 (Pull-up de 4,7kΩ na Shield)
* **I2C SCL:** GPIO 22 (Pull-up de 4,7kΩ na Shield)
* **XSHUT:** GPIO 4 (ou GPIO 32 para o sensor frontal na Shield Rev 2.0)
* **VCC / VIN:** 3,3 V regulado
* **GND:** Terra comum

---

## 2. Arquivos
* [`TESTE_1_SENSOR_VL53L0X.ino`](TESTE_1_SENSOR_VL53L0X.ino):
  * Varredura do barramento I2C (`scanI2C()`) confirmando a presença do sensor no endereço `0x29`;
  * Pulso de reset de hardware no pino XSHUT;
  * Ajuste do orçamento de medição para $33\text{ ms}$ ($\approx 30,3\text{ Hz}$);
  * Exibição gráfica no Monitor Serial (115200 bps) em mm, cm e barra de progresso.
