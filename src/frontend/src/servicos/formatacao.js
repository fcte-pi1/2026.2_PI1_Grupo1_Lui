export const LIMIAR_BATERIA_BAIXA = 20;
export const LIMITE_PROVA_MS = 600000;

export function formatarTempo(ms) {
    if (typeof ms !== 'number' || ms < 0) return "00:00";
    const totalSeconds = Math.floor(ms / 1000);
    const minutes = Math.floor(totalSeconds / 60);
    const seconds = totalSeconds % 60;
    
    return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
}

export function rotuloStatus(status) {
    const mapaStatus = {
        "AGUARDANDO": "Em espera",
        "MAPEANDO": "Explorando",
        "SPEED_RUN": "Speed Run",
        "CONCLUIDO": "Concluído",
        "COLISAO": "Colisão",
        "TIMEOUT": "Tempo esgotado",
        "TRAVADO": "Travado"
    };
    return mapaStatus[status] || "Desconhecido";
}

export function bateriaBaixa(porcentagem) {
    return porcentagem <= LIMIAR_BATERIA_BAIXA;
}

export function estiloStatus(status) {
    if (status === "TRAVADO" || status === "COLISAO" || status === "TIMEOUT") {
        return "alerta";
    }
    return "normal";
}
