#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa os pinos de controle e os canais PWM da Ponte H TB6612FNG.
 */
void tb6612_init(void);

/**
 * @brief Define a velocidade e o sentido do Motor 1 (Esquerdo).
 * @param speed Valor de -255 a +255.
 *              Positivo = Frente, Negativo = Ré, 0 = Parado (Roda Livre).
 */
void tb6612_set_motor_left(int16_t speed);

/**
 * @brief Define a velocidade e o sentido do Motor 2 (Direito).
 * @param speed Valor de -255 a +255.
 *              Positivo = Frente, Negativo = Ré, 0 = Parado (Roda Livre).
 */
void tb6612_set_motor_right(int16_t speed);

/**
 * @brief Define as velocidades dos dois motores simultaneamente.
 * @param speed_left Velocidade Motor Esquerdo (-255 a 255).
 * @param speed_right Velocidade Motor Direito (-255 a 255).
 */
void tb6612_set_motors(int16_t speed_left, int16_t speed_right);

/**
 * @brief Frenagem ativa por curto-circuito nos terminais dos dois motores.
 */
void tb6612_brake(void);

/**
 * @brief Para os motores em modo roda livre (alta impedância).
 */
void tb6612_stop(void);

/**
 * @brief Controla o pino STBY (Standby).
 * @param standby_active Se true, desativa a ponte H (economia); se false, ativa a ponte H.
 */
void tb6612_set_standby(bool standby_active);

#ifdef __cplusplus
}
#endif
