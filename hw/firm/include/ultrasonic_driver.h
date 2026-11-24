#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configura TRIG/ECHO do ultrassônico frontal e verifica se o ECHO responde ao pulso.
 */
bool ultrasonic_init(void);

/**
 * @brief Atualiza o offset individual (mm lidos acima do real) do sensor frontal.
 */
void ultrasonic_set_offset(int16_t offset_mm);

/**
 * @brief Lê a distância frontal pelo ultrassônico (medição não-bloqueante por interrupção).
 */
bool ultrasonic_read(uint16_t* out_dist_mm, uint16_t* out_raw_mm);

#ifdef __cplusplus
}
#endif
