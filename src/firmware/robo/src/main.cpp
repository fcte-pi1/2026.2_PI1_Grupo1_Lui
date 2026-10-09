#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "TelemetriaPacket.h"
#include "mapa/Mapa.h"

// Definições de pinos (conforme projeto de hardware 4.3)
const int BOTAO_LARGADA_PIN = 15;   // Botão de controle (GPIO 15 conforme shield 4.3)
const int LED_STATUS_PIN = 2;       // LED WS2812B de indicação de estado (GPIO 2)
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, LED_STATUS_PIN, NEO_GRB + NEO_KHZ800);

EstadoRobo estadoAtual = AGUARDANDO;
Mapa mapa;
TelemetriaPacket telemetria;

// Controle de debounce e gestos do botão
int estadoBotaoEstavel = HIGH;
int ultimaLeituraBotao = HIGH;
unsigned long ultimoTempoDebounce = 0;
const unsigned long DEBOUNCE_DELAY_MS = 25; // 25ms de leitura estável para debounce

unsigned long tempoPressionado = 0;
unsigned long ultimoTempoToque = 0;
int contagemToques = 0;
const unsigned long JANELA_GESTO_MS = 400;          // Janela de silêncio para considerar fim dos toques
const unsigned long LIMITE_PRESSAO_LONGA_MS = 1000;  // Pressão >= 1s não conta como toque curto

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
            // Verde constante conforme 4.3
            pixels.setPixelColor(0, pixels.Color(0, 255, 0));
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
    
    Serial.println("Robô ligado. Estado: AGUARDANDO");
    Serial.println("Pressione o botão 3 VEZES para selecionar a pista (toque triplo).");
}

void loop() {
    unsigned long tempoAtual = millis();

    switch (estadoAtual) {
        case AGUARDANDO: {
            int leituraBruta = digitalRead(BOTAO_LARGADA_PIN);

            // Debounce com leitura estável de 25ms
            if (leituraBruta != ultimaLeituraBotao) {
                ultimoTempoDebounce = tempoAtual;
            }
            ultimaLeituraBotao = leituraBruta;

            if ((tempoAtual - ultimoTempoDebounce) > DEBOUNCE_DELAY_MS) {
                if (leituraBruta != estadoBotaoEstavel) {
                    estadoBotaoEstavel = leituraBruta;

                    if (estadoBotaoEstavel == LOW) {
                        // Pressionou
                        tempoPressionado = tempoAtual;
                    } else {
                        // Soltou (borda de subida estável)
                        unsigned long duracaoPressao = tempoAtual - tempoPressionado;
                        if (duracaoPressao < LIMITE_PRESSAO_LONGA_MS) {
                            contagemToques++;
                            ultimoTempoToque = tempoAtual;
                        }
                        // Pressão longa >= 1s ignorada para a contagem de toques
                    }
                }
            }

            // Processa sequência de toques após janela de silêncio
            if (contagemToques > 0 && (tempoAtual - ultimoTempoToque > JANELA_GESTO_MS)) {
                if (contagemToques == 3) { // Toque triplo -> Geometria
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
                
                contagemToques = 0;
            }
            break;
        }

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


