#include <Arduino.h>
#include "TelemetriaPacket.h"
#include "mapa/Mapa.h"

// Definições de pinos (conforme projeto de hardware 4.3)
const int BOTAO_LARGADA_PIN = 15;   // Botão de largada (GPIO 15 conforme shield 4.3)
const int BOTAO_GEOMETRIA_PIN = 4; // Botão para alterar geometria
const int LED_STATUS_PIN = 2;      // LED de indicação de estado

EstadoRobo estadoAtual = AGUARDANDO;
Mapa mapa;
TelemetriaPacket telemetria;

unsigned long ultimoTempoBotaoGeo = 0;
int botaoGeoAnterior = HIGH;

void piscarFeedbackLED(int vezes) {
    for (int i = 0; i < vezes; i++) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        delay(150);
        digitalWrite(LED_STATUS_PIN, LOW);
        delay(150);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Configuração de pinos
    pinMode(BOTAO_LARGADA_PIN, INPUT_PULLUP);
    pinMode(BOTAO_GEOMETRIA_PIN, INPUT_PULLUP);
    pinMode(LED_STATUS_PIN, OUTPUT);
    
    // Inicialização da telemetria
    memset(&telemetria, 0, sizeof(TelemetriaPacket));
    telemetria.status_id = AGUARDANDO;
    telemetria.labirinto_id = LAB_4X4;
    mapa.setGeometria(LAB_4X4);
    
    Serial.println("Robô ligado. Estado: AGUARDANDO");
    Serial.println("Pressione o botão de GEOMETRIA para selecionar a pista.");
    Serial.println("Pressione o botão de LARGADA para iniciar.");
}

void loop() {
    switch (estadoAtual) {
        case AGUARDANDO: {
            // 1. A seleção da geometria está disponível antes da largada (detecção por borda de descida)
            int leituraGeo = digitalRead(BOTAO_GEOMETRIA_PIN);
            if (leituraGeo == LOW && botaoGeoAnterior == HIGH && (millis() - ultimoTempoBotaoGeo > 50)) {
                ultimoTempoBotaoGeo = millis();
                GeometriaLabirinto geo = mapa.getGeometria();
                
                // Alterna entre as geometrias (1: 4x4, 2: 8x4, 3: 12x4)
                if (geo == LAB_4X4) geo = LAB_8X4;
                else if (geo == LAB_8X4) geo = LAB_12X4;
                else geo = LAB_4X4;
                
                // 2. Os limites da matriz em memória correspondem à geometria selecionada
                mapa.setGeometria(geo);
                
                // 3. A geometria selecionada é enviada na telemetria (campo labirinto_id)
                telemetria.labirinto_id = geo;
                
                Serial.print("Geometria selecionada: ");
                int piscaCount = 1;
                if (geo == LAB_4X4) { Serial.println("4x4"); piscaCount = 1; }
                if (geo == LAB_8X4) { Serial.println("8x4"); piscaCount = 2; }
                if (geo == LAB_12X4) { Serial.println("12x4"); piscaCount = 3; }
                
                Serial.print("Limites da matriz (linhas x colunas): ");
                Serial.print(mapa.getMaxLinhas());
                Serial.print(" x ");
                Serial.println(mapa.getMaxColunas());

                // Feedback visual piscando o LED conforme a geometria (1x=4x4, 2x=8x4, 3x=12x4)
                piscarFeedbackLED(piscaCount);
            }
            botaoGeoAnterior = leituraGeo;

            // Início da prova
            if (digitalRead(BOTAO_LARGADA_PIN) == LOW) {
                estadoAtual = MAPEANDO;
                telemetria.status_id = MAPEANDO;
                Serial.println("Largada acionada! Estado: MAPEANDO");
                delay(500); // debounce simples
            }
            break;
        }
            
        case MAPEANDO:
            // Lógica principal de navegação seria chamada aqui
            digitalWrite(LED_STATUS_PIN, (millis() / 500) % 2); // Pisca LED
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

