#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa as interrupções de hardware para os dois encoders de quadratura.
 */
void encoder_init(void);

/**
 * @brief Retorna o total de ticks acumulados pelo Motor 1 (Esquerdo).
 */
int32_t encoder_get_ticks_left(void);

/**
 * @brief Retorna o total de ticks acumulados pelo Motor 2 (Direito).
 */
int32_t encoder_get_ticks_right(void);

/**
 * @brief Zera a contagem de ambos os encoders.
 */
void encoder_reset(void);

/**
 * @brief Converte ticks do motor esquerdo para distância linear em milímetros.
 */
float encoder_get_dist_left_mm(void);

/**
 * @brief Converte ticks do motor direito para distância linear em milímetros.
 */
float encoder_get_dist_right_mm(void);

/**
 * @brief Inverte a convenção de sentido de giro para alinhamento com a montagem física.
 * @param invert_left Inverte sentido do encoder esquerdo se true.
 * @param invert_right Inverte sentido do encoder direito se true.
 */
void encoder_set_inversion(bool invert_left, bool invert_right);

#ifdef __cplusplus
}
#endif
