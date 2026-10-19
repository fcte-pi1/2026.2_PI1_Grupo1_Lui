// src/telemetria/Pacote.cpp
#include "telemetria/Pacote.h"

TelemetriaPacket montarPacote(const DadosProva& d, int16_t velAtualMmS, int16_t velMediaMmS, uint16_t bateriaMv, uint8_t bateriaPct) {
    TelemetriaPacket p;
    p.timestamp_ms = d.tempoProvaMs;
    p.labirinto_id = d.geometria;
    p.status_id = d.estado;
    p.desafio_cumprido = d.desafioCumprido ? 1 : 0;
    p.bateria_pct = bateriaPct;
    p.bateria_mv = bateriaMv;
    p.velocidade_atual = velAtualMmS;
    p.velocidade_media = velMediaMmS;
    p.coluna = d.pose.coluna;
    p.linha = d.pose.linha;
    p.orientacao = d.pose.orientacao;
    p.paredes = d.paredesAtuais;
    return p;
}