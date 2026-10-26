#ifndef MICROMOUSE_PROTOCOL_H
#define MICROMOUSE_PROTOCOL_H

#include <stdint.h>

/**
 * ==============================================================================
 * PROTOCOLO DE COMUNICAÇÃO ESP-NOW - MICROMOUSE PI1 2026.2
 * ==============================================================================
 * Define os pacotes binários estruturados trocados entre:
 *  - Robô Micromouse (Transmissor de Telemetria / Receptor de Comandos)
 *  - Gateway ESP32 Bridge (Receptor de Telemetria / Emissor de Comandos Web)
 * ==============================================================================
 */

// Identificadores de Comandos (Gateway -> Micromouse)
enum CommandId : uint8_t {
    CMD_NONE            = 0,
    CMD_START           = 1, // Iniciar exploração autônoma às cegas
    CMD_STOP            = 2, // Parada imediata de emergência
    CMD_PAUSE           = 3, // Pausar/Retomar movimento
    CMD_RESET_MAZE      = 4, // Limpar mapa da memória e reiniciar odometria
    CMD_RETURN_TO_START = 5, // Navegar de volta à largada (0,0) pela rota ótima
    CMD_SET_SPEED       = 6  // Ajustar velocidade base PWM
};

// Estados do Micromouse
enum RobotState : uint8_t {
    ROBOT_IDLE          = 0, // Aguardando comando de largada
    ROBOT_EXPLORING     = 1, // Explorando labirinto às cegas
    ROBOT_BACKTRACKING  = 2, // Retrocedendo de beco sem saída
    ROBOT_GOAL_FOUND    = 3, // Gol encontrado! Rota limpa calculada
    ROBOT_RETURNING     = 4, // Retornando à base ou executando Speed Run
    ROBOT_STOPPED       = 5, // Parado por comando de emergência
    ROBOT_ERROR         = 6  // Estado de falha mecânica ou colisão
};

// Estrutura de Pacote de Comando (Gateway -> Micromouse)
// Tamanho fixo: 6 bytes
struct __attribute__((packed)) MicromouseCommand {
    uint8_t  commandId;   // Valor de CommandId
    uint8_t  parameter;   // Parâmetro opcional (ex: PWM ou ID de teste)
    uint16_t seqNumber;   // Número sequencial do pacote
    uint16_t checksum;    // Soma de verificação simples
};

// Estrutura de Pacote de Telemetria (Micromouse -> Gateway)
// Tamanho fixo: 24 bytes
struct __attribute__((packed)) MicromouseTelemetry {
    uint32_t timestampMs;  // Tempo decorrido de prova (ms)
    uint8_t  posX;         // Coordenada X na matriz (0 a 15)
    uint8_t  posY;         // Coordenada Y na matriz (0 a 15)
    uint8_t  heading;      // Orientação: 0=N, 1=L, 2=S, 3=O
    uint8_t  walls;        // Paredes na célula atual: bit 0=N, 1=L, 2=S, 3=O
    uint8_t  state;        // RobotState
    float    distL;        // Distância medida à esquerda (cm)
    float    distF;        // Distância medida à frente (cm)
    float    distR;        // Distância medida à direita (cm)
    float    batteryVolts; // Tensão da bateria (V)
    uint16_t stepCount;    // Total de passos de células percorridas
    uint16_t pathLength;   // Comprimento da rota limpa atual até a largada
};

#endif // MICROMOUSE_PROTOCOL_H
