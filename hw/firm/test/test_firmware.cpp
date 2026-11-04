#include <iostream>
#include <cassert>
#include <cmath>
#include "../include/pinout.h"
#include "../include/tb6612_driver.h"
#include "../include/encoder_driver.h"
#include "../include/vl53l0x_manager.h"
#include "../include/battery_monitor.h"
#include "../include/radio_telemetria.h"
#include "TelemetriaCodec.h"

void test_pin_definitions() {
    std::cout << "[TEST] Validando Mapeamento de Pinos..." << std::endl;

    // TB6612FNG
    assert(PIN_PWMA == 25);
    assert(PIN_AIN1 == 26);
    assert(PIN_AIN2 == 27);
    assert(PIN_STBY == 13);
    assert(PIN_BIN1 == 33);
    assert(PIN_BIN2 == 32);
    assert(PIN_PWMB == 18);

    // Encoders
    assert(PIN_ENC1_A == 36);
    assert(PIN_ENC1_B == 39);
    assert(PIN_ENC2_A == 35);
    assert(PIN_ENC2_B == 2); // Substituiu GPIO 4 para eliminar conflito!

    // Sensores VL53L0X
    assert(PIN_I2C_SDA == 21);
    assert(PIN_I2C_SCL == 22);
    assert(PIN_XSHUT_1 == 4);  // GPIO 4 dedicado ao Sensor 1
    assert(PIN_XSHUT_2 == 16); // Sensor 2
    assert(PIN_XSHUT_3 == 17); // Sensor 3

    // Endereços I2C
    assert(ADDR_VL53_S1 == 0x30);
    assert(ADDR_VL53_S2 == 0x31);
    assert(ADDR_VL53_S3 == 0x32);

    // Bateria
    assert(PIN_BATTERY_ADC == 34);

    std::cout << "  -> Todos os pinos batem 100% com a nova pinagem homologada! [OK]" << std::endl;
}

void test_battery_calculations() {
    std::cout << "[TEST] Validando Calculo de Porcentagem da Bateria LiPo 2S..." << std::endl;

    assert(battery_voltage_to_percentage(6.5f) == 0);
    assert(battery_voltage_to_percentage(6.6f) == 0);
    assert(battery_voltage_to_percentage(8.4f) == 100);
    assert(battery_voltage_to_percentage(8.5f) == 100);

    // Ponto médio: 7.5V = 50%
    uint8_t mid = battery_voltage_to_percentage(7.5f);
    assert(mid == 50);

    // Bateria nominal: 7.4V
    uint8_t nom = battery_voltage_to_percentage(7.4f);
    assert(nom >= 44 && nom <= 45);

    std::cout << "  -> Curva de descarga linear LiPo 2S validada com exatidao! [OK]" << std::endl;
}

void test_encoder_odometry() {
    std::cout << "[TEST] Validando Odometria Linear dos Encoders..." << std::endl;

    // 1 volta completa = 360 pulsos = perimetro da roda (~100.53 mm)
    float dist_1 = ((float)ENCODER_TICKS_PER_REV / (float)ENCODER_TICKS_PER_REV) * WHEEL_CIRCUMFERENCE_MM;
    assert(std::fabs(dist_1 - 100.5309649f) < 0.01f);

    // Célula padrão de micromouse: 168 mm
    // Ticks esperados para 168 mm = 168 / (100.53 / 360) = ~601.5 ticks
    float ticks_per_cell = (168.0f / WHEEL_CIRCUMFERENCE_MM) * ENCODER_TICKS_PER_REV;
    assert(ticks_per_cell > 600.0f && ticks_per_cell < 603.0f);

    std::cout << "  -> Resolucao linear do encoder: " << (WHEEL_CIRCUMFERENCE_MM / ENCODER_TICKS_PER_REV)
              << " mm/pulso. [OK]" << std::endl;
}

void test_wall_detection_thresholds() {
    std::cout << "[TEST] Validando Limiares de Deteccao de Parede..." << std::endl;

    // Menor ou igual a 110 mm = Parede Presente
    assert(70 <= DIST_PAREDE_DETECTADA_MM);
    assert(110 <= DIST_PAREDE_DETECTADA_MM);
    assert(!(115 <= DIST_PAREDE_DETECTADA_MM));

    std::cout << "  -> Limiar de 110 mm validado para celulas de 168 mm! [OK]" << std::endl;
}

void test_espnow_telemetry_packet_contract() {
    std::cout << "[TEST] Validando Contrato de Telemetria de 18 Bytes para o Gateway..." << std::endl;

    TelemetriaPacket pkt;
    pkt.timestamp_ms = 12500;
    pkt.labirinto_id = LAB_4X4;
    pkt.status_id = MAPEANDO;
    pkt.desafio_cumprido = 0;
    pkt.bateria_pct = 75;
    pkt.bateria_mv = 7800;
    pkt.velocidade_atual = 150;
    pkt.velocidade_media = 120;
    pkt.coluna = 1;
    pkt.linha = 2;
    pkt.orientacao = 0;
    pkt.paredes = 0x0D; // Norte, Leste, Oeste

    // Valida tamanho exato de 18 bytes
    assert(sizeof(TelemetriaPacket) == 18);
    assert(telemetriaValidar(pkt) == TELEMETRIA_OK);

    uint8_t buffer[18];
    size_t written = telemetriaCodificar(pkt, buffer, sizeof(buffer));
    assert(written == 18);

    // Testa envio pelo rádio
    assert(radio_telemetria_enviar(&pkt) == true);

    std::cout << "  -> Pacote de 18 bytes codificado e validado para ESP-NOW -> ESP8266 Gateway! [OK]" << std::endl;
}

int main() {
    std::cout << "=======================================================" << std::endl;
    std::cout << "  SUITE DE TESTES UNITARIOS - FIRMWARE FINAL (hw/firm) " << std::endl;
    std::cout << "=======================================================" << std::endl;

    test_pin_definitions();
    test_battery_calculations();
    test_encoder_odometry();
    test_wall_detection_thresholds();
    test_espnow_telemetry_packet_contract();

    std::cout << "=======================================================" << std::endl;
    std::cout << "  TODOS OS 5 TESTES UNITARIOS PASSARAM COM SUCESSO!    " << std::endl;
    std::cout << "=======================================================" << std::endl;
    return 0;
}
