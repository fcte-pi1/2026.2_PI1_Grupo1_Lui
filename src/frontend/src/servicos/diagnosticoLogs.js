// Motor de classificação e diagnóstico de eventos em tempo real para o painel Web.
// Converte pacotes de telemetria, mudanças de conexão WS/RF e ações do operador
// em logs coloridos por domínio provável (Software, Eletrônica, Energia, Mecânica, Rádio)
// com dicas práticas de depuração.

export const DOMINIOS = {
    software: { id: 'software', rotulo: 'Software' },
    eletronica: { id: 'eletronica', rotulo: 'Eletrônica' },
    energia: { id: 'energia', rotulo: 'Energia' },
    mecanica: { id: 'mecanica', rotulo: 'Mecânica' },
    radio: { id: 'radio', rotulo: 'Rádio / RF' },
};

// Pinagem oficial (PinosRobo.h) exibida no inspetor de saúde de hardware do painel.
export const MAPA_HARDWARE_ESP32 = [
    { id: 'tof', nome: '3x ToF VL53L0X (I2C)', pinos: 'SDA 21 · SCL 22 · XSHUT 4/16/17', enderecos: '0x30 / 0x31 / 0x32', testeUsb: '[i] / [1]' },
    { id: 'ponte', nome: 'Ponte H TB6612FNG', pinos: 'STBY 13 · PWMA 25 (26/27) · PWMB 18 (33/32)', enderecos: 'PWM 20 kHz', testeUsb: '[2] / [i]' },
    { id: 'encoders', nome: 'Encoders Quadratura', pinos: 'Esq 34/39 · Dir 35/2', enderecos: '360 pulsos/volta', testeUsb: '[3] / [i]' },
    { id: 'bateria', nome: 'Bateria LiPo 2S ADC', pinos: 'GPIO 36 (VP / ADC1_CH0)', enderecos: 'Divisor 30k/7.5k (>= 6,8 V)', testeUsb: '[9] / [i]' },
    { id: 'radio', nome: 'Rádio ESP-NOW + Gateway', pinos: 'Wi-Fi STA Canal 1 -> Serial USB', enderecos: 'Pacote 18 B @ 1 Hz', testeUsb: '[w] / [d]' },
    { id: 'ihm', nome: 'IHM Local (Botão / LED)', pinos: 'Botão GPIO 15 (Pull-Up) · LED GPIO 14', enderecos: '3 toques / 2s largada', testeUsb: '[i] / [d]' },
];

export const NIVEIS = {
    ok: 'ok',
    info: 'info',
    alerta: 'alerta',
    critico: 'critico',
};

const ORIENTACAO_TEXTO = {
    NORTE: 'NORTE',
    SUL: 'SUL',
    LESTE: 'LESTE',
    OESTE: 'OESTE',
};

function formatarCelula(posicao) {
    if (!posicao?.coluna || !posicao?.linha) return '--';
    return `${posicao.coluna}${String(posicao.linha).padStart(2, '0')}`;
}

export function formatarTempoRelogio(data = new Date()) {
    const hh = String(data.getHours()).padStart(2, '0');
    const mm = String(data.getMinutes()).padStart(2, '0');
    const ss = String(data.getSeconds()).padStart(2, '0');
    return `${hh}:${mm}:${ss}`;
}

export function formatarTempoCorrida(timestampMs) {
    if (typeof timestampMs !== 'number' || timestampMs < 0) return '00:00';
    const totalSeg = Math.floor(timestampMs / 1000);
    const min = String(Math.floor(totalSeg / 60)).padStart(2, '0');
    const seg = String(totalSeg % 60).padStart(2, '0');
    return `${min}:${seg}`;
}

function entrada(id, { nivel, dominio, secundario = null, titulo, detalhe, dica = null, timestampMs = null, hora = formatarTempoRelogio() }) {
    return {
        id,
        hora,
        tempoCorrida: timestampMs != null ? formatarTempoCorrida(timestampMs) : null,
        nivel,
        dominio,
        secundario,
        titulo,
        detalhe,
        dica,
    };
}

export function diagnosticarStatus(pacote, idBase) {
    const celula = formatarCelula(pacote.posicao);
    const ori = ORIENTACAO_TEXTO[pacote.orientacao] || pacote.orientacao || '--';
    const ts = pacote.timestamp_ms;

    switch (pacote.status) {
        case 'AGUARDANDO':
            return entrada(idBase, {
                nivel: NIVEIS.info,
                dominio: 'software',
                titulo: 'Robô em espera (AGUARDANDO)',
                detalhe: `Posicionado em ${celula} (${ori}), pista ${pacote.labirinto || '--'}. Pronto para largada por botão ou comando 'k'.`,
                timestampMs: ts,
            });
        case 'MAPEANDO':
            return entrada(idBase, {
                nivel: NIVEIS.info,
                dominio: 'software',
                titulo: 'Exploração iniciada (MAPEANDO)',
                detalhe: `Algoritmo Flood Fill ativo em ${pacote.labirinto || '--'} a partir de ${celula} (${ori}).`,
                timestampMs: ts,
            });
        case 'SPEED_RUN':
            return entrada(idBase, {
                nivel: NIVEIS.info,
                dominio: 'software',
                titulo: 'Speed Run iniciado (SPEED_RUN)',
                detalhe: `Executando rota ótima em velocidade elevada na pista ${pacote.labirinto || '--'}.`,
                timestampMs: ts,
            });
        case 'CONCLUIDO':
            return entrada(idBase, {
                nivel: NIVEIS.ok,
                dominio: 'software',
                titulo: 'Objetivo alcançado (CONCLUIDO)',
                detalhe: `Chegou em ${celula} em ${formatarTempoCorrida(ts)} com velocidade média de ${pacote.velocidade_media ?? 0} cm/s.`,
                timestampMs: ts,
            });
        case 'TRAVADO':
            return entrada(idBase, {
                nivel: NIVEIS.critico,
                dominio: 'mecanica',
                secundario: 'eletronica',
                titulo: 'Corte de tração por travamento (TRAVADO)',
                detalhe: `Sem pulsos de encoder por > 1,2 s com PWM ativo em ${celula} (${ori}).`,
                dica: 'Mecânica: veja se a roda/O-ring enroscou na parede ou chassi. Eletrônica: rode teste [3] nos encoders (GPIO 34/39 esq, 35/2 dir) e [2] nos motores (STBY GPIO 13 / TB6612). Software: confira se PWM mínimo (CAL_PWM_MINIMO) vence o atrito.',
                timestampMs: ts,
            });
        case 'COLISAO':
            return entrada(idBase, {
                nivel: NIVEIS.critico,
                dominio: 'eletronica',
                secundario: 'software',
                titulo: 'Emergência frontal / colisão (COLISAO)',
                detalhe: `Sensor frontal detectou obstáculo crítico em ${celula} (${ori}).`,
                dica: 'Eletrônica: confira o ToF frontal (GPIO 16, end. 0x31) no teste [1] ou [d]. Software: se leu abaixo do real, revise CAL_TOF_OFFSET_FRONTAL_MM e CAL_DIST_ENCOSTO_FRONTAL_MM em CalibracaoRobo.h.',
                timestampMs: ts,
            });
        case 'TIMEOUT':
            return entrada(idBase, {
                nivel: NIVEIS.alerta,
                dominio: 'software',
                secundario: 'mecanica',
                titulo: 'Tempo limite de 10 min excedido (TIMEOUT)',
                detalhe: `Prova encerrada por tempo em ${celula} (${ori}).`,
                dica: 'Software: verifique se o robô entrou em loop de decisão ou falhou leituras ToF repetidas. Mecânica/Energia: confira se a bateria fraca reduziu a velocidade.',
                timestampMs: ts,
            });
        default:
            return null;
    }
}

export function diagnosticarAnomaliasTelemetria(pacote, contexto, proximoId) {
    const logs = [];
    const ts = pacote.timestamp_ms;

    // 1. Bateria baixa / crítica (corte de largada é 6.80 V em CalibracaoRobo.h)
    if (typeof pacote.bateria_volts === 'number') {
        if (pacote.bateria_volts < 6.8 && !contexto.bateriaCriticaAvisada) {
            logs.push(entrada(proximoId(), {
                nivel: NIVEIS.critico,
                dominio: 'energia',
                secundario: 'eletronica',
                titulo: `Bateria abaixo do mínimo (${pacote.bateria_volts.toFixed(2)} V)`,
                detalhe: 'Tensão abaixo de 6,80 V: o firmware bloqueia nova largada e há risco de brownout no ESP32.',
                dica: 'Energia: recarregue a LiPo 2S antes de rodar. Eletrônica: se no multímetro estiver > 7,4 V, cheque o divisor 30kΩ/7,5kΩ no GPIO 36 e CAL_BATERIA_FATOR_AJUSTE.',
                timestampMs: ts,
            }));
        } else if (pacote.bateria_volts >= 6.8 && pacote.bateria_volts < 7.05 && !contexto.bateriaBaixaAvisada) {
            logs.push(entrada(proximoId(), {
                nivel: NIVEIS.alerta,
                dominio: 'energia',
                titulo: `Bateria em atenção (${pacote.bateria_volts.toFixed(2)} V / ${pacote.bateria_pct ?? '--'}%)`,
                detalhe: 'Carga baixa — considere recarregar após esta bateria de testes.',
                dica: 'Energia: mantenha a LiPo 2S acima de 7,2 V para manter torque constante nos motores N20.',
                timestampMs: ts,
            }));
        }
    }

    // 2. Sinal RF fraco
    if (typeof pacote.rssi === 'number' && pacote.rssi < -82 && !contexto.rssiFracoAvisado) {
        logs.push(entrada(proximoId(), {
            nivel: NIVEIS.alerta,
            dominio: 'radio',
            titulo: `Sinal ESP-NOW fraco (${pacote.rssi} dBm)`,
            detalhe: 'RSSI abaixo de -82 dBm pode causar perda intermitente de pacotes no gateway.',
            dica: 'Rádio: aproxime o gateway ESP8266 da pista ou afaste obstáculos metálicos entre a antena do ESP32 e o PC.',
            timestampMs: ts,
        }));
    }

    // 3. Velocidade zerada persistente durante exploração/speed run
    if ((pacote.status === 'MAPEANDO' || pacote.status === 'SPEED_RUN')
        && pacote.velocidade_atual === 0
        && contexto.pacotesParadosSeguidos === 3) {
        logs.push(entrada(proximoId(), {
            nivel: NIVEIS.alerta,
            dominio: 'eletronica',
            secundario: 'mecanica',
            titulo: 'Velocidade 0 cm/s persistente em corrida',
            detalhe: `3 pacotes seguidos com velocidade zero em ${formatarCelula(pacote.posicao)}.`,
            dica: 'Se não estiver em pausa de leitura ToF ou curva no eixo: cheque encoder solto (GPIO 34/39 esq, 35/2 dir), pino STBY (GPIO 13) ou roda travada.',
            timestampMs: ts,
        }));
    }

    return logs;
}

export function diagnosticarAcaoResolver({ sucesso, detalhe = '', modoSimulador = false }, id) {
    if (sucesso && !modoSimulador) {
        return entrada(id, {
            nivel: NIVEIS.ok,
            dominio: 'software',
            secundario: 'radio',
            titulo: "Comando remoto 'k' enviado ao robô",
            detalhe: 'Backend escreveu "k" na serial do gateway ESP8266 -> broadcast ESP-NOW.',
            dica: 'Se o robô continuar em AGUARDANDO: confira se está rodando firmware robo/geral atualizado, se a bateria está ≥ 6,8 V e se o gateway está no canal 1.',
        });
    }
    if (sucesso && modoSimulador) {
        return entrada(id, {
            nivel: NIVEIS.info,
            dominio: 'software',
            titulo: 'Modo simulador ativo',
            detalhe: detalhe || 'O simulador inicia a corrida automaticamente ao conectar o painel.',
        });
    }
    return entrada(id, {
        nivel: NIVEIS.critico,
        dominio: 'software',
        secundario: 'eletronica',
        titulo: "Falha ao enviar largada 'k'",
        detalhe: detalhe || 'Não foi possível enviar o comando pela serial do gateway.',
        dica: 'Software: rode o backend com TELEMETRIA_FONTE=serial e SERIAL_PORTA correta, e feche outros scripts/monitores usando a mesma porta USB.',
    });
}
