import React from 'react';
import { formatarTempo, LIMITE_PROVA_MS } from '../servicos/formatacao';

export default function IndicadorTempo({ timestampMs }) {
    const msLimitado = Math.min(Math.max(timestampMs || 0, 0), LIMITE_PROVA_MS);
    const porcentagem = (msLimitado / LIMITE_PROVA_MS) * 100;
    
    return (
        <div className="indicador tempo">
            <div className="cabecalho">Tempo Decorrido</div>
            <div className="valor">
                <span className="tempo-atual">{formatarTempo(msLimitado)}</span>
                <span className="limite"> / 10:00 min</span>
            </div>
            <div className="barra-progresso">
                <div 
                    className="progresso" 
                    style={{ width: `${porcentagem}%` }}
                ></div>
            </div>
        </div>
    );
}
