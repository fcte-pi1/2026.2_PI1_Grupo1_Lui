#ifndef NAVEGADOR_H
#define NAVEGADOR_H

#include <stdint.h>
#include "../mapa/Mapa.h"
#include "InterfacesNavegacao.h"

// Estado da navegação autônoma (HU-06). É independente do EstadoRobo da telemetria;
// estadoRobo() faz a correspondência quando for preciso publicar.
enum EstadoNavegacao : uint8_t {
    NAV_NAO_INICIADO,
    NAV_EXPLORANDO,
    NAV_CHEGOU,        // a célula de destino foi alcançada
    NAV_INALCANCAVEL,  // nenhuma passagem confirmada leva a uma célula ainda não visitada
    NAV_FALHA          // falhas de movimento/leitura seguidas ou limite de passos excedido
};

// Resultado de decidir(): próximo movimento planejado. Decidir não move o robô nem
// altera a posição lógica; isso só acontece em confirmarAvanco().
struct DecisaoNavegacao {
    bool movimento;    // false: não há movimento a executar (encerrado ou sem rota)
    Direcao direcao;   // direção absoluta do próximo avanço
    Giro giro;         // giro necessário a partir da orientação atual
    bool retorno;      // true se a próxima célula já foi visitada (volta por caminho conhecido)
};

// Navegação lógica do Micromouse: decide a próxima célula usando somente o mapa
// descoberto (HU-03) e atualiza posição, orientação e mapa quando o movimento físico é
// confirmado. Não conhece motores nem sensores reais; ver InterfacesNavegacao.h.
//
// Estratégia (exploração por fronteiras com planejamento em BFS):
//  - uma célula visitada tem os quatro lados conhecidos (três lidos + o de trás, gravado
//    por registrarMovimento); logo, cada passagem confirmada que sai de uma célula
//    visitada para uma não visitada é uma fronteira;
//  - a cada decisão, uma BFS sobre as passagens confirmadas entre células visitadas
//    calcula a distância até cada fronteira;
//  - escolhe-se a fronteira de menor custo (distância percorrida + distância de Manhattan
//    ao destino); o destino, se for uma fronteira, tem prioridade absoluta;
//  - o primeiro passo do menor caminho até a fronteira escolhida é a decisão. Se a fronteira
//    não é vizinha, esse passo é um retorno por células já visitadas (beco sem saída).
//
// Término (labirinto finito, estático, leituras corretas): toda fronteira escolhida é
// alcançada em no máximo (nº de células - 1) passos, pois o alvo permanece o mesmo
// enquanto o robô o persegue (o custo do alvo cai 1 por passo e o dos demais cai no
// máximo 1, com desempate por chave fixa independente da posição). Ao chegar, a célula
// passa a visitada e o conjunto de células não visitadas diminui. Como ele é finito, a
// exploração acaba com a chegada ao destino ou sem fronteiras (NAV_INALCANCAVEL).
class Navegador {
public:
    // Teto de segurança de decisões: o pior caso teórico é (células)^2 = 2304 passos.
    static const uint16_t LIMITE_PASSOS = 4096;
    // Falhas consecutivas (movimento ou leitura) toleradas antes de NAV_FALHA.
    static const uint8_t LIMITE_FALHAS = 3;

private:
    static const uint8_t COLUNAS_MAX = 4;
    static const uint8_t CELULAS_MAX = 48;  // 12 linhas x 4 colunas
    static const uint8_t SEM_DISTANCIA = 0xFF;

    Mapa& mapa;
    EstadoNavegacao estado_;
    uint8_t linha_, coluna_;
    Direcao orientacao_;
    uint8_t linhaObjetivo_, colunaObjetivo_;

    uint16_t decisoes_, deslocamentos_, retornos_, giros_, falhas_;
    uint8_t falhasSeguidas_;

public:
    explicit Navegador(Mapa& m)
        : mapa(m), estado_(NAV_NAO_INICIADO), linha_(0), coluna_(0), orientacao_(DIR_LESTE),
          linhaObjetivo_(0), colunaObjetivo_(0), decisoes_(0), deslocamentos_(0),
          retornos_(0), giros_(0), falhas_(0), falhasSeguidas_(0) {}

    // Prepara a corrida sobre o mapa já configurado na geometria escolhida: largada em A1
    // (0, 0) virado para o leste e destino no canto oposto, na última linha da coluna D.
    // Não limpa o mapa: a geometria é escolhida antes da largada, via Mapa::setGeometria().
    void iniciar() {
        linha_ = 0;
        coluna_ = 0;
        orientacao_ = DIR_LESTE;
        linhaObjetivo_ = (uint8_t)(mapa.getMaxLinhas() - 1);
        colunaObjetivo_ = (uint8_t)(mapa.getMaxColunas() - 1);
        decisoes_ = deslocamentos_ = retornos_ = giros_ = falhas_ = 0;
        falhasSeguidas_ = 0;
        mapa.marcarVisitada(linha_, coluna_);
        estado_ = NAV_EXPLORANDO;
    }

    EstadoNavegacao estado() const { return estado_; }
    bool emExploracao() const { return estado_ == NAV_EXPLORANDO; }
    bool chegou() const { return estado_ == NAV_CHEGOU; }
    uint8_t linha() const { return linha_; }
    uint8_t coluna() const { return coluna_; }
    Direcao orientacao() const { return orientacao_; }
    uint8_t linhaObjetivo() const { return linhaObjetivo_; }
    uint8_t colunaObjetivo() const { return colunaObjetivo_; }
    uint16_t decisoes() const { return decisoes_; }
    uint16_t deslocamentos() const { return deslocamentos_; }
    uint16_t retornos() const { return retornos_; }
    uint16_t giros() const { return giros_; }
    uint16_t falhas() const { return falhas_; }

    bool noObjetivo() const {
        return linha_ == linhaObjetivo_ && coluna_ == colunaObjetivo_;
    }

    // Registra no mapa as leituras feitas na célula atual (frente, esquerda, direita em
    // relação à orientação atual). Retorna false se estiver encerrada ou se a leitura
    // for incoerente com o mapa (por exemplo, passagem no perímetro).
    bool observar(bool paredeFrente, bool paredeEsquerda, bool paredeDireita) {
        if (estado_ != NAV_EXPLORANDO) return false;
        return mapa.registrarLeituras(linha_, coluna_, orientacao_,
                                      paredeFrente, paredeEsquerda, paredeDireita);
    }

    // Camada de decisão: planeja o próximo avanço sem alterar posição nem orientação.
    // Só considera passagens confirmadas; lado desconhecido nunca é tratado como livre.
    // Sem nenhuma fronteira acessível, encerra com NAV_INALCANCAVEL.
    DecisaoNavegacao decidir() {
        DecisaoNavegacao d = {false, orientacao_, GIRO_NENHUM, false};
        if (estado_ != NAV_EXPLORANDO) return d;
        decisoes_++;
        if (!planejar(d)) estado_ = NAV_INALCANCAVEL;
        return d;
    }

    // Camada de atualização: o executor confirmou um giro, então a orientação lógica muda.
    void confirmarGiro(Giro giro) {
        if (estado_ != NAV_EXPLORANDO) return;
        switch (giro) {
            case GIRO_DIREITA:     orientacao_ = girarDireita(orientacao_); break;
            case GIRO_ESQUERDA:    orientacao_ = girarEsquerda(orientacao_); break;
            case GIRO_MEIA_VOLTA:  orientacao_ = direcaoOposta(orientacao_); break;
            case GIRO_NENHUM:      return;
        }
        giros_++;
        falhasSeguidas_ = 0;
    }

    // Camada de atualização: o executor confirmou o avanço de uma célula na orientação
    // atual. Atualiza o mapa (HU-03), a posição e detecta a chegada. Retorna false, sem
    // alterar nada, se o avanço sair da geometria ou se a navegação já estiver encerrada.
    bool confirmarAvanco() {
        if (estado_ != NAV_EXPLORANDO) return false;
        uint8_t l, c;
        if (!mapa.vizinha(linha_, coluna_, orientacao_, l, c)) return false;
        const bool eraVisitada = mapa.foiVisitada(l, c);
        if (!mapa.registrarMovimento(linha_, coluna_, orientacao_)) return false;
        linha_ = l;
        coluna_ = c;
        deslocamentos_++;
        if (eraVisitada) retornos_++;
        falhasSeguidas_ = 0;
        if (noObjetivo()) estado_ = NAV_CHEGOU;
        return true;
    }

    // O movimento solicitado não foi confirmado: o estado lógico permanece como estava.
    void registrarFalha() {
        if (estado_ != NAV_EXPLORANDO) return;
        falhas_++;
        if (++falhasSeguidas_ >= LIMITE_FALHAS) estado_ = NAV_FALHA;
    }

    // Executa um ciclo completo: ler, atualizar o mapa, decidir, girar e avançar,
    // confirmando cada etapa antes de atualizar o estado lógico. Retorna true enquanto
    // ainda houver exploração a fazer.
    bool passo(SensoresParedes& sensores, ExecutorMovimento& executor) {
        if (estado_ != NAV_EXPLORANDO) return false;
        if (decisoes_ >= LIMITE_PASSOS) {
            estado_ = NAV_FALHA;
            return false;
        }
        bool frente, esquerda, direita;
        if (!sensores.lerParedes(frente, esquerda, direita)) {
            registrarFalha();
            return emExploracao();
        }
        observar(frente, esquerda, direita);

        const DecisaoNavegacao d = decidir();
        if (!d.movimento) return false;

        if (d.giro != GIRO_NENHUM) {
            if (!executor.girar(d.giro)) {
                registrarFalha();
                return emExploracao();
            }
            confirmarGiro(d.giro);
        }
        if (!executor.avancarCelula()) {
            registrarFalha();
            return emExploracao();
        }
        confirmarAvanco();
        return emExploracao();
    }

    // Preenche os campos de posição do pacote de telemetria. Não altera os demais.
    void preencherPosicao(TelemetriaPacket& pacote) const {
        pacote.linha = linha_;
        pacote.coluna = coluna_;
        pacote.orientacao = (uint8_t)orientacao_;
        pacote.paredes = mapa.getParedes(linha_, coluna_);
    }

    // Estado equivalente no contrato da telemetria (válido após iniciar()).
    EstadoRobo estadoRobo() const {
        switch (estado_) {
            case NAV_EXPLORANDO: return MAPEANDO;
            case NAV_CHEGOU:     return CONCLUIDO;
            case NAV_INALCANCAVEL:
            case NAV_FALHA:      return TRAVADO;
            case NAV_NAO_INICIADO: break;
        }
        return AGUARDANDO;
    }

private:
    uint8_t indice(uint8_t l, uint8_t c) const {
        return (uint8_t)(l * COLUNAS_MAX + c);
    }

    uint8_t manhattanAoObjetivo(uint8_t l, uint8_t c) const {
        const uint8_t dl = l > linhaObjetivo_ ? l - linhaObjetivo_ : linhaObjetivo_ - l;
        const uint8_t dc = c > colunaObjetivo_ ? c - colunaObjetivo_ : colunaObjetivo_ - c;
        return (uint8_t)(dl + dc);
    }

    // Passagem confirmada: lado observado e sem parede. Lado desconhecido não conta.
    bool passagemConfirmada(uint8_t l, uint8_t c, Direcao dir) const {
        return mapa.ladoConhecido(l, c, dir) && !mapa.temParede(l, c, dir);
    }

    // BFS a partir da célula atual sobre passagens confirmadas entre células visitadas,
    // avaliando como candidatas as fronteiras (vizinhas não visitadas). Retorna false se
    // não existir nenhuma.
    bool planejar(DecisaoNavegacao& saida) const {
        uint8_t dist[CELULAS_MAX];
        uint8_t primeira[CELULAS_MAX];  // primeiro passo do caminho até a célula
        uint8_t fila[CELULAS_MAX];
        for (uint8_t i = 0; i < CELULAS_MAX; i++) dist[i] = SEM_DISTANCIA;

        // Ordem de expansão: frente, direita, esquerda, trás. Favorece caminhos com
        // menos giros e é fixa, o que mantém o comportamento determinístico.
        const Direcao ordem[4] = {orientacao_, girarDireita(orientacao_),
                                  girarEsquerda(orientacao_), direcaoOposta(orientacao_)};

        uint8_t ini = 0, fim = 0;
        const uint8_t origem = indice(linha_, coluna_);
        dist[origem] = 0;
        fila[fim++] = origem;

        bool achou = false;
        int16_t melhorCusto = 0;
        uint8_t melhorH = 0, melhorL = 0, melhorC = 0;
        Direcao melhorDir = orientacao_;

        while (ini < fim) {
            const uint8_t u = fila[ini++];
            const uint8_t lu = (uint8_t)(u / COLUNAS_MAX);
            const uint8_t cu = (uint8_t)(u % COLUNAS_MAX);
            for (uint8_t k = 0; k < 4; k++) {
                const Direcao dir = ordem[k];
                uint8_t lv, cv;
                if (!passagemConfirmada(lu, cu, dir)) continue;
                if (!mapa.vizinha(lu, cu, dir, lv, cv)) continue;
                const uint8_t passoInicial =
                    (dist[u] == 0) ? (uint8_t)dir : primeira[u];

                if (!mapa.foiVisitada(lv, cv)) {
                    const uint8_t h = manhattanAoObjetivo(lv, cv);
                    const bool destino = (lv == linhaObjetivo_ && cv == colunaObjetivo_);
                    const int16_t custo = destino ? -1 : (int16_t)(dist[u] + 1 + h);
                    const bool melhor = !achou || custo < melhorCusto ||
                        (custo == melhorCusto &&
                         (h < melhorH ||
                          (h == melhorH && (lv < melhorL ||
                                            (lv == melhorL && cv < melhorC)))));
                    if (melhor) {
                        achou = true;
                        melhorCusto = custo;
                        melhorH = h;
                        melhorL = lv;
                        melhorC = cv;
                        melhorDir = (Direcao)passoInicial;
                    }
                } else {
                    const uint8_t v = indice(lv, cv);
                    if (dist[v] == SEM_DISTANCIA) {
                        dist[v] = (uint8_t)(dist[u] + 1);
                        primeira[v] = passoInicial;
                        fila[fim++] = v;
                    }
                }
            }
        }
        if (!achou) return false;

        saida.movimento = true;
        saida.direcao = melhorDir;
        saida.giro = giroPara(melhorDir);
        uint8_t l, c;
        saida.retorno = mapa.vizinha(linha_, coluna_, melhorDir, l, c) && mapa.foiVisitada(l, c);
        return true;
    }

    Giro giroPara(Direcao alvo) const {
        if (alvo == orientacao_) return GIRO_NENHUM;
        if (alvo == girarDireita(orientacao_)) return GIRO_DIREITA;
        if (alvo == girarEsquerda(orientacao_)) return GIRO_ESQUERDA;
        return GIRO_MEIA_VOLTA;
    }
};

#endif
