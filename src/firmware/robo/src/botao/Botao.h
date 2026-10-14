#ifndef BOTAO_H
#define BOTAO_H

#include <stdint.h>

// Gestos do botão de largada (GPIO 15, doc 4.3), sem dependência do Arduino: o laço
// principal informa a leitura bruta e o tempo atual a cada ciclo, e a classe devolve
// os gestos reconhecidos.
//  - Debounce: uma mudança só é aceita depois de a leitura ficar estável por mais de 25 ms.
//  - Toques: pressões de menos de 1 s são contadas, e a quantidade é entregue quando
//    passam mais de 400 ms sem novo toque.
//  - Largada: soltar o botão depois de pelo menos 2 s pressionado. A largada acontece na
//    soltura, para o robô não arrancar com a mão do operador em cima. Pressões entre
//    1 s e 2 s são ignoradas.
// As contas de tempo usam aritmética unsigned de 32 bits e continuam corretas quando
// millis() volta a zero.

enum TipoEventoBotao : uint8_t {
    BOTAO_SEM_EVENTO,
    BOTAO_TOQUES,   // fim de uma sequência de toques curtos
    BOTAO_LARGADA   // botão solto depois de pelo menos 2 s pressionado
};

struct EventoBotao {
    TipoEventoBotao tipo;
    uint8_t toques;  // quantidade de toques da sequência (só em BOTAO_TOQUES)
};

class Botao {
public:
    static const uint32_t DEBOUNCE_MS = 25;
    static const uint32_t JANELA_TOQUES_MS = 400;
    static const uint32_t LIMITE_TOQUE_MS = 1000;
    static const uint32_t PRESSAO_LARGADA_MS = 2000;

    Botao() { reiniciar(); }

    void reiniciar() {
        ultimaLeitura_ = false;
        estavel_ = false;
        ultimaMudancaMs_ = 0;
        inicioPressaoMs_ = 0;
        ultimoToqueMs_ = 0;
        agoraMs_ = 0;
        toques_ = 0;
    }

    // 'pressionado' é a leitura bruta, sem debounce (com pull-up, nível LOW).
    // Deve ser chamado a cada ciclo do laço principal.
    EventoBotao atualizar(bool pressionado, uint32_t agoraMs) {
        agoraMs_ = agoraMs;
        if (pressionado != ultimaLeitura_) {
            ultimaMudancaMs_ = agoraMs;
        }
        ultimaLeitura_ = pressionado;

        if ((uint32_t)(agoraMs - ultimaMudancaMs_) > DEBOUNCE_MS && pressionado != estavel_) {
            estavel_ = pressionado;
            if (estavel_) {
                inicioPressaoMs_ = agoraMs;
            } else {
                const uint32_t duracao = agoraMs - inicioPressaoMs_;
                if (duracao >= PRESSAO_LARGADA_MS) {
                    // Toques anteriores já foram entregues: a janela de 400 ms fecha
                    // durante a pressão, bem antes dos 2 s.
                    return evento(BOTAO_LARGADA, 0);
                }
                if (duracao < LIMITE_TOQUE_MS) {
                    if (toques_ < 255) toques_++;
                    ultimoToqueMs_ = agoraMs;
                }
            }
        }

        if (toques_ > 0 && (uint32_t)(agoraMs - ultimoToqueMs_) > JANELA_TOQUES_MS) {
            const uint8_t quantidade = toques_;
            toques_ = 0;
            return evento(BOTAO_TOQUES, quantidade);
        }
        return evento(BOTAO_SEM_EVENTO, 0);
    }

    // Estado do botão depois do debounce.
    bool pressionado() const { return estavel_; }

    // Botão pressionado há pelo menos 2 s: soltar agora dá a largada.
    bool largadaArmada() const {
        return estavel_ && (uint32_t)(agoraMs_ - inicioPressaoMs_) >= PRESSAO_LARGADA_MS;
    }

private:
    static EventoBotao evento(TipoEventoBotao tipo, uint8_t toques) {
        EventoBotao e;
        e.tipo = tipo;
        e.toques = toques;
        return e;
    }

    bool ultimaLeitura_;
    bool estavel_;
    uint32_t ultimaMudancaMs_;
    uint32_t inicioPressaoMs_;
    uint32_t ultimoToqueMs_;
    uint32_t agoraMs_;
    uint8_t toques_;
};

#endif
