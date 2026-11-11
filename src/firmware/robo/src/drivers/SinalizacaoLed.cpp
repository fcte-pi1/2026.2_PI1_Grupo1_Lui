#include "drivers/SinalizacaoLed.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "hardware/Pinos.h"
#include "sinalizacao/PadraoLed.h"

// O objeto do WS2812B só é usado com TIPO_LED_ESTADO == LED_WS2812B. Construí-lo não mexe
// no pino; begin() só é chamado em iniciar().
static Adafruit_NeoPixel pixel(1, PINO_LED_ESTADO, NEO_GRB + NEO_KHZ800);

void SinalizacaoLed::iniciar() {
    if (TIPO_LED_ESTADO == LED_WS2812B) {
        pixel.begin();
        pixel.clear();
        pixel.show();
    } else {
        pinMode(PINO_LED_ESTADO, OUTPUT);
        digitalWrite(PINO_LED_ESTADO, LOW);
    }
}

void SinalizacaoLed::atualizar(uint32_t agoraMs, bool largadaArmada) {
    const uint32_t desdeAvisoMs = agoraMs - avisoInicioMs_;
    if (avisoVezes_ > 0 && avisoEmAndamento(avisoVezes_, desdeAvisoMs)) {
        const bool aceso = avisoAceso(avisoVezes_, desdeAvisoMs);
        if (TIPO_LED_ESTADO == LED_WS2812B) {
            pixel.setPixelColor(0, aceso ? pixel.Color(avisoCor_.r, avisoCor_.g, avisoCor_.b) : 0);
            pixel.show();
        } else {
            digitalWrite(PINO_LED_ESTADO, aceso ? HIGH : LOW);
        }
        return;
    }
    avisoVezes_ = 0;

    if (TIPO_LED_ESTADO == LED_WS2812B) {
        const CorLed c = corDoEstado(estado_, largadaArmada, agoraMs);
        pixel.setPixelColor(0, pixel.Color(c.r, c.g, c.b));
        pixel.show();
    } else {
        digitalWrite(PINO_LED_ESTADO, ledComumAceso(estado_, largadaArmada, agoraMs) ? HIGH : LOW);
    }
}

void SinalizacaoLed::apagar() {
    if (TIPO_LED_ESTADO == LED_WS2812B) {
        pixel.clear();
        pixel.show();
    } else {
        digitalWrite(PINO_LED_ESTADO, LOW);
    }
}
