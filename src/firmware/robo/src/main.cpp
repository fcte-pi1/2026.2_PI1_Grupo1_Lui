#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "TelemetriaPacket.h"
#include "TelemetriaCodec.h"
#include "TelemetriaConfig.h"
#include "TemporizadorPeriodico.h"
#include "botao/Botao.h"
#include "cronometro/Cronometro.h"
#include "mapa/Mapa.h"
#include "radio/RadioTelemetria.h"

// Definições de pinos (conforme projeto de hardware 4.3)
const int BOTAO_LARGADA_PIN = 15;   // Botão de controle (GPIO 15 conforme shield 4.3)
const int LED_STATUS_PIN = 2;       // LED WS2812B de indicação de estado (GPIO 2)
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, LED_STATUS_PIN, NEO_GRB + NEO_KHZ800);

EstadoRobo estadoAtual = AGUARDANDO;
Mapa mapa;
TelemetriaPacket telemetria;

// Gestos do botão (debounce, toques e largada) e cronômetro da prova
Botao botao;
Cronometro cronometro(TELEMETRIA_TIMESTAMP_MAX_MS); // limitado aos 10 min do contrato

// Telemetria ESP-NOW a 1 Hz (RNF-SFT03), sem bloquear o laço principal
TemporizadorPeriodico temporizadorTelemetria(TELEMETRIA_PERIODO_MS);
bool radioPronto = false;

void enviarTelemetria() {
    if (!radioPronto) return;
    telemetria.timestamp_ms = cronometro.decorridoMs((uint32_t)millis()); // 0 antes da largada
    telemetria.status_id = (uint8_t)estadoAtual;
    telemetria.labirinto_id = (uint8_t)mapa.getGeometria();
    // Os demais campos (posição, bateria, velocidades, paredes) serão preenchidos
    // pelas issues de navegação/sensoriamento; por ora seguem os valores atuais da struct.
    radioTelemetriaEnviar(telemetria);
}

void piscarFeedbackLED(int vezes, uint32_t cor) {
    for (int i = 0; i < vezes; i++) {
        pixels.setPixelColor(0, cor);
        pixels.show();
        delay(150);
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
        delay(150);
    }
}

void atualizarLEDStatus() {
    switch (estadoAtual) {
        case AGUARDANDO:
            if (botao.largadaArmada()) {
                // Branco constante: botão pressionado há 2 s, pode soltar para dar a largada
                pixels.setPixelColor(0, pixels.Color(255, 255, 255));
            } else {
                // Verde constante conforme 4.3
                pixels.setPixelColor(0, pixels.Color(0, 255, 0));
            }
            break;
        case MAPEANDO:
            // Amarelo piscante em exploração conforme 4.3
            if ((millis() / 500) % 2) {
                pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Amarelo
            } else {
                pixels.setPixelColor(0, pixels.Color(0, 0, 0));
            }
            break;
        case SPEED_RUN:
            pixels.setPixelColor(0, pixels.Color(0, 255, 255)); // Ciano
            break;
        case CONCLUIDO:
            pixels.setPixelColor(0, pixels.Color(128, 0, 128)); // Roxo conforme 4.3
            break;
        case COLISAO:
        case TIMEOUT:
        case TRAVADO:
            pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Vermelho em falha conforme 4.3
            break;
    }
    pixels.show();
}

void setup() {
    Serial.begin(115200);
    
    // Configuração de pinos
    pinMode(BOTAO_LARGADA_PIN, INPUT_PULLUP);
    pixels.begin();
    pixels.clear();
    pixels.show();
    
    // Inicialização da telemetria e mapa
    memset(&telemetria, 0, sizeof(TelemetriaPacket));
    telemetria.status_id = AGUARDANDO;
    telemetria.labirinto_id = LAB_4X4;
    mapa.setGeometria(LAB_4X4);

    radioPronto = radioTelemetriaInit();
    Serial.println(radioPronto ? "ESP-NOW pronto (telemetria a 1 Hz)." : "ERRO: falha ao iniciar ESP-NOW.");
    
    Serial.println("Robô ligado. Estado: AGUARDANDO");
    Serial.println("Pressione o botão 3 VEZES para selecionar a pista (toque triplo).");
    Serial.println("Para a largada, segure o botão por 2 s (LED branco) e solte.");
}

void selecionarProximaGeometria() {
    GeometriaLabirinto geo = mapa.getGeometria();

    // Alterna geometrias (4x4 -> 8x4 -> 12x4 -> 4x4)
    if (geo == LAB_4X4) geo = LAB_8X4;
    else if (geo == LAB_8X4) geo = LAB_12X4;
    else geo = LAB_4X4;

    mapa.setGeometria(geo);
    telemetria.labirinto_id = geo;

    Serial.print("Geometria selecionada: ");
    int piscaCount = 1;
    if (geo == LAB_4X4) { Serial.println("4x4"); piscaCount = 1; }
    else if (geo == LAB_8X4) { Serial.println("8x4"); piscaCount = 2; }
    else if (geo == LAB_12X4) { Serial.println("12x4"); piscaCount = 3; }

    Serial.print("Limites da matriz (linhas x colunas): ");
    Serial.print(mapa.getMaxLinhas());
    Serial.print(" x ");
    Serial.println(mapa.getMaxColunas());

    // Feedback visual com cor única (Ciano) e quantidade de piscadas
    uint32_t corCiano = pixels.Color(0, 255, 255);
    piscarFeedbackLED(piscaCount, corCiano);
}

void darLargada(uint32_t tempoAtual) {
    cronometro.iniciar(tempoAtual);
    estadoAtual = MAPEANDO;
    Serial.println("Largada! Estado: MAPEANDO (cronômetro iniciado)");
}

void loop() {
    const uint32_t tempoAtual = (uint32_t)millis();

    if (temporizadorTelemetria.vencido(tempoAtual)) {
        enviarTelemetria();
    }

    // O botão é lido em todo ciclo para o debounce acompanhar o estado real, mas os
    // gestos só têm efeito em AGUARDANDO: depois da largada não há intervenção (RNF-SFT01).
    const EventoBotao evento = botao.atualizar(digitalRead(BOTAO_LARGADA_PIN) == LOW, tempoAtual);

    switch (estadoAtual) {
        case AGUARDANDO:
            if (evento.tipo == BOTAO_LARGADA) {
                darLargada(tempoAtual);
            } else if (evento.tipo == BOTAO_TOQUES && evento.toques == 3) {
                selecionarProximaGeometria();
            }
            // 1 e 2 toques ficam reservados ao autoteste dos motores e dos sensores ToF
            // (doc 4.3), que dependem de hardware e são de outras issues.
            break;

        case MAPEANDO:
        case SPEED_RUN:
        case CONCLUIDO:
        case COLISAO:
        case TIMEOUT:
        case TRAVADO:
            break;
    }

    atualizarLEDStatus();
    delay(10);
}


