#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t dist_left_mm;    // Distância medida à esquerda (Sensor 1 - GPIO 4)
    uint16_t dist_front_mm;   // Distância medida à frente (Sensor 2 - GPIO 16)
    uint16_t dist_right_mm;   // Distância medida à direita (Sensor 3 - GPIO 17)

    bool wall_left;           // Parede presente à esquerda (< limiar)
    bool wall_front;          // Parede presente à frente (< limiar)
    bool wall_right;          // Parede presente à direita (< limiar)

    bool left_valid;          // Leitura válida sem timeout
    bool front_valid;         // Leitura válida sem timeout
    bool right_valid;         // Leitura válida sem timeout
} VL53L0X_Readings_t;

/**
 * @brief Inicializa o barramento I2C e os 3 sensores VL53L0X via pinos XSHUT sequenciais.
 * @return true se todos os sensores responderem com sucesso; false se algum falhar.
 */
bool vl53l0x_manager_init(void);

/**
 * @brief Executa a leitura contínua dos 3 sensores de distância ToF.
 * @return Estrutura com as distâncias em mm, flags de validade e detecção de parede.
 */
VL53L0X_Readings_t vl53l0x_manager_read(void);

#ifdef __cplusplus
}
#endif
