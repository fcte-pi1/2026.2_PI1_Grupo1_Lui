import React from 'react';
import { bateriaBaixa } from '../servicos/formatacao';

export default function IndicadorBateria({ porcentagem, volts }) {
    const isBaixa = bateriaBaixa(porcentagem);
    
    return (
        <div className={`indicador bateria ${isBaixa ? 'alerta' : ''}`}>
            <div className="cabecalho">Bateria</div>
            <div className="valores">
                <span className="valor-porcentagem">{porcentagem != null ? porcentagem : 0}%</span>
                <span className="valor-volts">({volts != null ? volts.toFixed(2) : '0.00'} V)</span>
            </div>
            {isBaixa && <div className="aviso-bateria">Nível Crítico!</div>}
        </div>
    );
}
