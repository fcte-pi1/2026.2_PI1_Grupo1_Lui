#ifndef PINOS_H
#define PINOS_H

#include <stdint.h>
#include "CalibracaoRobo.h"

// Configuração única do hardware do robô: pinos do ESP32 DevKit V1 (30 pinos) e
// constantes físicas. Sem dependência do Arduino, para os testes nativos poderem usar
// os mesmos valores do firmware.
//
// Fonte: firmware da Eletrônica que está gravado e funcionando no robô
// (hw/firm/include/pinout.h e hw/firm/README.md, branch
// feat/firmware-final-comissionamento-telemetria). A tabela de pinagem do doc 4.3 está
// desatualizada em relação a essa pinagem.
//
// Ao acrescentar um pino, inclua-o também em PINOS_EM_USO (o teste test_pinos confere
// que nenhum GPIO é usado duas vezes).

// --- Ponte H TB6612FNG (motor 1 = esquerdo, motor 2 = direito) ---
const uint8_t PINO_PONTE_PWMA = 25;  // PWM do motor esquerdo (20 kHz)
const uint8_t PINO_PONTE_AIN1 = 26;  // sentido do motor esquerdo
const uint8_t PINO_PONTE_AIN2 = 27;  // sentido do motor esquerdo
const uint8_t PINO_PONTE_STBY = 13;  // standby: HIGH habilita a ponte H
const uint8_t PINO_PONTE_BIN1 = 33;  // sentido do motor direito
const uint8_t PINO_PONTE_BIN2 = 32;  // sentido do motor direito
const uint8_t PINO_PONTE_PWMB = 18;  // PWM do motor direito (20 kHz)

// --- Encoders em quadratura ---
// Os GPIO 34 a 39 só funcionam como entrada e não têm pull-up interno.
const uint8_t PINO_ENCODER_ESQ_A = 34;  // trocado com bateria (GPIO 36)
const uint8_t PINO_ENCODER_ESQ_B = 39;  // VN
const uint8_t PINO_ENCODER_DIR_A = 35;
const uint8_t PINO_ENCODER_DIR_B = 2;   // pino de boot do ESP32, com pull-up interno

// --- Sensores de distância: ToF VL53L0X (2x Esq+Frontal, 3x, ou 2x+Ultrassônico) ---
const uint8_t PINO_I2C_SDA = 21;
const uint8_t PINO_I2C_SCL = 22;
const uint8_t PINO_TOF_XSHUT_ESQ = 4;       // sensor 1, lateral esquerdo (0x30)
#if CAL_TEM_TOF_DIREITO
const uint8_t PINO_TOF_XSHUT_DIR = 17;      // sensor lateral direito (0x32)
#endif
#if CAL_USA_ULTRASSONICO_FRONTAL
const uint8_t PINO_ULTRASSONICO_TRIG = 16;  // reaproveita GPIO 16 do antigo ToF frontal
const uint8_t PINO_ULTRASSONICO_ECHO = 19;  // pino livre (não-strapping) para ECHO frontal
#else
const uint8_t PINO_TOF_XSHUT_FRONTAL = 16;  // sensor 2, frontal (0x31)
#endif

// --- Interface e bateria ---
const uint8_t PINO_BOTAO = 15;  // botão de largada, com pull-up interno (nível LOW = pressionado)
// Provisório: depende de confirmação da Eletrônica. Na pinagem nova o GPIO 2 (antigo
// pino do LED) é o canal B do encoder direito; o README da Eletrônica indica o GPIO 14
// como "Status LED / reserva".
const uint8_t PINO_LED_ESTADO = 14;  // LED WS2812B de estado
const uint8_t PINO_BATERIA_ADC = 36;  // ADC1_CH0 (VP), ponto médio do divisor da bateria

const uint8_t PINOS_EM_USO[] = {
    PINO_PONTE_PWMA, PINO_PONTE_AIN1, PINO_PONTE_AIN2, PINO_PONTE_STBY,
    PINO_PONTE_BIN1, PINO_PONTE_BIN2, PINO_PONTE_PWMB,
    PINO_ENCODER_ESQ_A, PINO_ENCODER_ESQ_B, PINO_ENCODER_DIR_A, PINO_ENCODER_DIR_B,
    PINO_I2C_SDA, PINO_I2C_SCL,
    PINO_TOF_XSHUT_ESQ,
#if CAL_TEM_TOF_DIREITO
    PINO_TOF_XSHUT_DIR,
#endif
#if CAL_USA_ULTRASSONICO_FRONTAL
    PINO_ULTRASSONICO_TRIG, PINO_ULTRASSONICO_ECHO,
#else
    PINO_TOF_XSHUT_FRONTAL,
#endif
    PINO_BOTAO, PINO_LED_ESTADO, PINO_BATERIA_ADC,
};
const uint8_t PINOS_EM_USO_QTD = sizeof(PINOS_EM_USO) / sizeof(PINOS_EM_USO[0]);

// --- Constantes físicas (fonte única em CalibracaoRobo.h) ---
constexpr float RODA_DIAMETRO_MM = CAL_RODA_DIAMETRO_MM;
// Pulsos por volta da RODA, contando só a borda de subida do canal A (como a ISR do
// hw/firm e a de drivers/Encoders.cpp).
const uint16_t ENCODER_PULSOS_POR_VOLTA = CAL_ENCODER_PULSOS_POR_VOLTA;
// Bitola real (centro-a-centro dos O-rings): rodas montadas POR FORA do chassi.
constexpr float BITOLA_MM = CAL_BITOLA_MM;
// Deslocamento de uma roda por pulso: pi x RODA_DIAMETRO_MM / ENCODER_PULSOS_POR_VOLTA.
constexpr float RODA_MM_POR_PULSO = 3.14159265f * RODA_DIAMETRO_MM / ENCODER_PULSOS_POR_VOLTA;
// Fator empírico da odometria em linha reta (real / medido em 4–5 células no piso).
constexpr float ODOM_ESCALA_RETO = CAL_ODOM_ESCALA_RETO;
// Compensação longitudinal na PRIMEIRA reta da prova (mm).
constexpr float COMPENSACAO_LARGADA_MM = CAL_COMPENSACAO_LARGADA_MM;

// Sentidos de montagem (achado R7). Valores configurados em CalibracaoRobo.h.
const bool MOTOR_ESQ_INVERTIDO = CAL_MOTOR_ESQ_INVERTIDO;
const bool MOTOR_DIR_INVERTIDO = CAL_MOTOR_DIR_INVERTIDO;
const bool ENCODER_ESQ_INVERTIDO = CAL_ENCODER_ESQ_INVERTIDO;
const bool ENCODER_DIR_INVERTIDO = CAL_ENCODER_DIR_INVERTIDO;

// --- Ponte H: PWM do ESP32 (LEDC), como no hw/firm ---
const uint32_t PONTE_PWM_FREQ_HZ = 20000;  // acima da faixa audível
const uint8_t PONTE_PWM_BITS = 8;          // PWM de 0 a 255
const uint8_t PWM_MAXIMO = 255;  // resolução da ponte H; o freio ativo usa o máximo, como no hw/firm

// Conversão de velocidade em PWM (achado B6, controle/ConversorVelocidadePwm.h):
//   |PWM| = PWM_MINIMO + |v| x PWM_POR_MM_S, saturado em PWM_MAXIMO_MOVIMENTO.
// PWM_MINIMO vence a zona morta dos motores (10 a 15 % segundo o doc 4.3).
// PWM_BASE_MOVIMENTO (90) corresponde à velocidade base da reta (150 mm/s, como no
// hw/firm), enquanto PWM_MAXIMO_MOVIMENTO (130) dá folga simétrica acima da base para
// correção lateral, malha PI de velocidade e reforço anti-travamento na partida.
const uint8_t PWM_MINIMO = CAL_PWM_MINIMO;
const uint8_t PWM_BASE_MOVIMENTO = CAL_PWM_BASE_MOVIMENTO;
const uint8_t PWM_MAXIMO_MOVIMENTO = CAL_PWM_MAXIMO_MOVIMENTO;
constexpr float PWM_POR_MM_S = (float)(PWM_BASE_MOVIMENTO - PWM_MINIMO) / 150.0f;  // cerca de 0,33

// Rampas de aceleração/desaceleração e curva sem IMU (calibração de pista).
const uint32_t TEMPO_RAMPA_PARTIDA_MS = 120;      // rampa suave de partida (como hw/firm)
constexpr float DIST_DESACELERACAO_MM = 35.0f;    // começa a reduzir velocidade perto do alvo
constexpr float VEL_MIN_APROXIMACAO_MM_S = 65.0f; // piso de velocidade na chegada da célula
constexpr float CURVA_ANTECIPACAO_FREIO_GRAUS = CAL_CURVA_ANTECIPACAO_FREIO_GRAUS;

// --- Labirinto (regulamento) ---
constexpr uint16_t CELULA_MM = CAL_CELULA_MM;
constexpr uint16_t VAO_CORREDOR_MM = CAL_VAO_CORREDOR_MM;
constexpr float DIST_IDEAL_PAREDE_LATERAL_MM = CAL_DIST_IDEAL_PAREDE_LATERAL_MM;

// --- Sensores ToF VL53L0X ---
// Endereços I2C atribuídos no boot pela sequência dos XSHUT, como no hw/firm.
const uint8_t TOF_ENDERECO_ESQ = 0x30;
const uint8_t TOF_ENDERECO_FRONTAL = 0x31;
const uint8_t TOF_ENDERECO_DIR = 0x32;
// Offset individual de calibração (mm que o sensor lê ACIMA da distância real).
const int16_t TOF_OFFSET_ESQ_MM = CAL_TOF_OFFSET_ESQ_MM;
const int16_t TOF_OFFSET_FRONTAL_MM = CAL_TOF_OFFSET_FRONTAL_MM;
const int16_t TOF_OFFSET_DIR_MM = CAL_TOF_OFFSET_DIR_MM;
const uint16_t TOF_DIST_MIN_VALIDA_MM = 10;   // permite leituras válidas perto do encosto frontal (15–24 mm)
const uint16_t TOF_DIST_MAX_VALIDA_MM = 2000; // acima disso: sem alvo ao alcance
const uint32_t TOF_I2C_HZ = 100000;          // 100 kHz, o do hw/firm (fiação com jumpers)
const uint32_t TOF_ORCAMENTO_US = 33000;     // orçamento de medição: cerca de 30 Hz por sensor
const uint32_t ULTRASSONICO_TIMEOUT_US = 25000; // ~4,2 m máximo; sem eco vira corredor aberto
const uint8_t ULTRASSONICO_PERIODO_MS = 40;     // ~25 Hz (evita ecos fantasmas do pulso anterior)
const uint16_t ULTRASSONICO_DIST_MIN_MM = 20;   // faixa mínima confiável do HC-SR04
// A leitura não espera o sensor (achado B10): o driver consulta "amostra pronta" e, se um
// sensor ficar sem amostra nova por mais que TOF_VALIDADE_MS, a leitura dele passa a ser
// inválida. O timeout da biblioteca é só uma segunda barreira.
const uint16_t TOF_VALIDADE_MS = 100;
const uint16_t TOF_TIMEOUT_BIBLIOTECA_MS = 20;
const uint8_t TOF_PERIODO_CONSULTA_MS = 5;   // intervalo mínimo entre consultas ao barramento
// Erro de I2C (sensor solto ou barramento travado): cada transação espera no máximo
// TOF_I2C_TIMEOUT_MS e o sensor fica TOF_ESPERA_APOS_ERRO_MS sem ser consultado. No pior
// caso, cerca de 3 x 5 ms a cada 50 ms, longe de travar o laço.
const uint8_t TOF_I2C_TIMEOUT_MS = 5;
const uint8_t TOF_ESPERA_APOS_ERRO_MS = 50;
const uint16_t TOF_LEITURA_INVALIDA_MM = 8190;  // valor que o VL53L0X devolve sem alvo

// Distâncias frontais do avanço de uma célula (geometria real de base superior2.CATPart):
// Face óptica do ToF frontal fica a +65,6 mm à frente do eixo (suporte +15,5 + 2,6 mm módulo).
// Com o eixo no centro exato da célula (84,0 mm da parede), o frontal lê 84,0 - 65,6 = 18,4 mm.
// Parar em DIST_ENCOSTO_FRONTAL_MM = 24 mm deixa o eixo 5,6 mm antes do centro com folga
// equilibrada de giro (+18,3 mm frente / +17,4 mm trás) e dentro da faixa confiável do VL53L0X.
// DIST_MINIMA_SEGURA_MM = 15 mm evita falsos disparos de COLISAO ao aproximar de parede normal.
const uint16_t DIST_MINIMA_SEGURA_MM = CAL_DIST_MINIMA_SEGURA_MM;
const uint16_t DIST_ENCOSTO_FRONTAL_MM = CAL_DIST_ENCOSTO_FRONTAL_MM;

// Classificação de parede (achado R3, sensores/ClassificadorParedes.h).
const uint16_t LIMIAR_PAREDE_MM = CAL_LIMIAR_PAREDE_MM;
const uint16_t HISTERESE_PAREDE_MM = CAL_HISTERESE_PAREDE_MM;

// --- LED de estado (achado R5, confirmado pela pinagem oficial do hw/firm) ---
// No hardware montado (hw/firm), o GPIO 14 aciona um LED comum com padrões de piscada.
enum TipoLedEstado : uint8_t {
    LED_WS2812B,
    LED_COMUM
};
const TipoLedEstado TIPO_LED_ESTADO = LED_COMUM;

// Divisor de tensão da bateria (resistor em série com a bateria e resistor para o GND,
// com o ponto médio em PINO_BATERIA_ADC). Valores do firmware que roda no robô: módulo
// sensor de tensão comercial de 30 kΩ / 7,5 kΩ, proporção 5:1 (bateria cheia de 8,4 V
// dá 1,68 V no pino). Depende de confirmação da Eletrônica: o doc 4.3 indica
// 100 kΩ / 47 kΩ (proporção de cerca de 3,13:1).
const uint32_t BATERIA_R_SERIE_OHM = CAL_BATERIA_R_SERIE_OHM;
const uint32_t BATERIA_R_GND_OHM = CAL_BATERIA_R_GND_OHM;
constexpr float BATERIA_FATOR_AJUSTE = CAL_BATERIA_FATOR_AJUSTE;

// Tensão mínima para aceitar a largada (achado R4): 3,4 V por célula, o alarme de bateria
// do hw/firm. Durante a prova não há corte por bateria (seção 3.7 do desenho).
const uint16_t BATERIA_MINIMA_LARGADA_MV = CAL_BATERIA_MINIMA_LARGADA_MV;

#endif
