#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "TelemetriaPacket.h"
#include "mapa/Mapa.h"

// Definições de pinos (conforme projeto de hardware 4.3)
const int BOTAO_LARGADA_PIN = 15;   // Botão de largada (GPIO 15 conforme shield 4.3)
const int LED_STATUS_PIN = 2;       // LED WS2812B de indicação de estado
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, LED_STATUS_PIN, NEO_GRB + NEO_KHZ800);

EstadoRobo estadoAtual = AGUARDANDO;
Mapa mapa;
TelemetriaPacket telemetria;

// Variáveis para controle de toque no botão
unsigned long ultimoTempoToque = 0;
int contagemToques = 0;
int botaoAnterior = HIGH;

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

void setup() {
    Serial.begin(115200);
    
    // Configuração de pinos
    pinMode(BOTAO_LARGADA_PIN, INPUT_PULLUP);
    pixels.begin();
    pixels.clear();
    pixels.show();
    
    // Inicialização da telemetria
    memset(&telemetria, 0, sizeof(TelemetriaPacket));
    telemetria.status_id = AGUARDANDO;
    telemetria.labirinto_id = LAB_4X4;
    mapa.setGeometria(LAB_4X4);
    
    Serial.println("Robô ligado. Estado: AGUARDANDO");
    Serial.println("Pressione o botão 3 VEZES para selecionar a pista (toque triplo).");
    Serial.println("Pressione o botão 1 VEZ para iniciar a largada (toque simples).");
}

void loop() {
    switch (estadoAtual) {
        case AGUARDANDO: {
            int leituraBotao = digitalRead(BOTAO_LARGADA_PIN);
            unsigned long tempoAtual = millis();
            
            // Detecta borda de descida (pressionamento)
            if (leituraBotao == LOW && botaoAnterior == HIGH && (tempoAtual - ultimoTempoToque > 50)) {
                contagemToques++;
                ultimoTempoToque = tempoAtual;
            }
            botaoAnterior = leituraBotao;

            // Processa os toques se passou o tempo limite (ex: 400ms do último toque)
            if (contagemToques > 0 && (tempoAtual - ultimoTempoToque > 400)) {
                if (contagemToques >= 3) { // Toque triplo -> Geometria
                    GeometriaLabirinto geo = mapa.getGeometria();
                    
                    // Alterna entre as geometrias (1: 4x4, 2: 8x4, 3: 12x4)
                    if (geo == LAB_4X4) geo = LAB_8X4;
                    else if (geo == LAB_8X4) geo = LAB_12X4;
                    else geo = LAB_4X4;
                    
                    mapa.setGeometria(geo);
                    telemetria.labirinto_id = geo;
                    
                    Serial.print("Geometria selecionada: ");
                    int piscaCount = 1;
                    uint32_t cor = pixels.Color(0, 255, 0); // Verde por padrão
                    
                    if (geo == LAB_4X4) { Serial.println("4x4"); piscaCount = 1; cor = pixels.Color(0, 255, 0); } // Verde
                    if (geo == LAB_8X4) { Serial.println("8x4"); piscaCount = 2; cor = pixels.Color(255, 255, 0); } // Amarelo
                    if (geo == LAB_12X4) { Serial.println("12x4"); piscaCount = 3; cor = pixels.Color(255, 0, 0); } // Vermelho
                    
                    Serial.print("Limites da matriz (linhas x colunas): ");
                    Serial.print(mapa.getMaxLinhas());
                    Serial.print(" x ");
                    Serial.println(mapa.getMaxColunas());

                    piscarFeedbackLED(piscaCount, cor);
                } 
                else if (contagemToques == 1) { // Toque simples -> Largada
                    estadoAtual = MAPEANDO;
                    telemetria.status_id = MAPEANDO;
                    Serial.println("Largada acionada! Estado: MAPEANDO");
                }
                
                // Reseta a contagem
                contagemToques = 0;
            }
            break;
        }
            
        case MAPEANDO:
            // Lógica principal de navegação seria chamada aqui
            if ((millis() / 500) % 2) {
                pixels.setPixelColor(0, pixels.Color(0, 0, 255)); // Azul
            } else {
                pixels.setPixelColor(0, pixels.Color(0, 0, 0));
            }
            pixels.show();
            break;
            
        case SPEED_RUN:
        case CONCLUIDO:
        case COLISAO:
        case TIMEOUT:
        case TRAVADO:
            // Implementação futura
            break;
    }
    
    // A tarefa de envio da telemetria deve ser executada separadamente 
    // a 1Hz pelo FreeRTOS.
    delay(10);
}

