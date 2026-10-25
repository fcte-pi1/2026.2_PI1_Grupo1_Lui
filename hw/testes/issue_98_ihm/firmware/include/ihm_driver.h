#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// MAPEAMENTO DE PINOS - SHIELD REV 2.0 (INTERFACE HOMEM-MAQUINA)
// ==============================================================================
#define PIN_BUTTON_START   15  // Botao de comando fisico (Pull-up interno)
#define PIN_RGB_LED         2  // LED RGB WS2812B / Sinalizacao visual
#define PIN_BUZZER          4  // Buzzer piezoeletrico ativo (>= 60 dB)

#define DEBOUNCE_TIME_MS   50  // Tempo de estabilizacao do botao

// 5 Modos Operacionais do Edital e Requisito RF-ELE05
typedef enum {
    STATE_OP_CALIBRATING = 0, // Modo 1: Calibracao (Azul)
    STATE_OP_READY,           // Modo 2: Pronto para largada (Verde)
    STATE_OP_NAVIGATING,      // Modo 3: Navegando no labirinto (Amarelo)
    STATE_OP_GOAL_REACHED,    // Modo 4: Objetivo alcancado (Roxo)
    STATE_OP_FAULT_LOW_BAT    // Modo 5: Falha / Bateria fraca (Vermelho)
} OperationalState_t;

// Padroes Sonoros do Buzzer (RF-ELE06)
typedef enum {
    BEEP_NONE = 0,
    BEEP_STARTUP_CONFIRM,  // 2 bipes curtos (1 kHz)
    BEEP_START_RUN,        // 1 bipe longo (2 kHz)
    BEEP_GOAL_VICTORY,     // Melodia trifonica de chegada
    BEEP_LOW_BAT_ALARM     // Alarme rapido intermitente (3 kHz)
} BuzzerPattern_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    bool blink;
} VisualPattern_t;

/**
 * @brief Retorna o padrao de cor RGB correspondente ao estado operacional (RF-ELE05).
 */
static inline VisualPattern_t ihm_get_visual_pattern(OperationalState_t state) {
    VisualPattern_t pat;
    switch (state) {
        case STATE_OP_CALIBRATING:
            pat.r = 0;   pat.g = 0;   pat.b = 255; pat.blink = false; // Azul
            break;
        case STATE_OP_READY:
            pat.r = 0;   pat.g = 255; pat.b = 0;   pat.blink = false; // Verde
            break;
        case STATE_OP_NAVIGATING:
            pat.r = 255; pat.g = 200; pat.b = 0;   pat.blink = false; // Amarelo
            break;
        case STATE_OP_GOAL_REACHED:
            pat.r = 180; pat.g = 0;   pat.b = 255; pat.blink = false; // Roxo
            break;
        case STATE_OP_FAULT_LOW_BAT:
        default:
            pat.r = 255; pat.g = 0;   pat.b = 0;   pat.blink = true;  // Vermelho piscante
            break;
    }
    return pat;
}

/**
 * @brief Filtro temporal de debounce para leitura confiavel de botoes.
 */
static inline bool ihm_debounce_filter(bool raw_pressed, uint32_t current_time_ms, uint32_t* last_toggle_ms, bool* current_state) {
    if (raw_pressed != *current_state) {
        if ((current_time_ms - *last_toggle_ms) >= DEBOUNCE_TIME_MS) {
            *current_state = raw_pressed;
            *last_toggle_ms = current_time_ms;
            return true; // Estado alterado e confirmado
        }
    }
    return false;
}

#ifdef __cplusplus
}
#endif
