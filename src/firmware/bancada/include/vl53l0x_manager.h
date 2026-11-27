#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t dist_left_mm;    // Distancia calibrada (bruta - offset) a esquerda
    uint16_t dist_front_mm;   // Distancia calibrada a frente
    uint16_t dist_right_mm;   // Distancia calibrada a direita

    uint16_t raw_left_mm;     // Leitura bruta do sensor esquerdo (antes do offset)
    uint16_t raw_front_mm;    // Leitura bruta do sensor frontal
    uint16_t raw_right_mm;    // Leitura bruta do sensor direito

    bool wall_left;           // Parede presente a esquerda (<= DIST_PAREDE_DETECTADA_MM)
    bool wall_front;          // Parede presente a frente (<= DIST_PAREDE_DETECTADA_MM)
    bool wall_right;          // Parede presente a direita (<= DIST_PAREDE_DETECTADA_MM)

    bool left_valid;          // Leitura valida sem timeout
    bool front_valid;         // Leitura valida sem timeout
    bool right_valid;         // Leitura valida sem timeout
} VL53L0X_Readings_t;

/**
 * @brief Inicializa o barramento I2C e os 3 sensores VL53L0X via pinos XSHUT sequenciais.
 * @return true se todos os sensores responderem com sucesso; false se algum falhar.
 */
bool vl53l0x_manager_init(void);

/**
 * @brief Executa a leitura continua dos 3 sensores de distancia ToF aplicando os offsets ativos.
 */
VL53L0X_Readings_t vl53l0x_manager_read(void);

/**
 * @brief Atualiza os offsets individuais em tempo de execucao (mm lidos acima do real).
 */
void vl53l0x_manager_set_offsets(int16_t left_mm, int16_t front_mm, int16_t right_mm);

/**
 * @brief Calcula o offset inteiro arredondado (media_bruta - dist_real_mm).
 */
int16_t vl53l0x_calcular_offset_mm(uint32_t soma_bruta_mm, uint8_t amostras, uint16_t dist_real_mm);

#ifdef __cplusplus
}
#endif
