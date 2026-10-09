import React from 'react';
import { rotuloStatus, estiloStatus } from '../servicos/formatacao';

export default function IndicadorStatus({ status }) {
    const rotulo = rotuloStatus(status);
    const estilo = estiloStatus(status);
    
    return (
        <div className={`indicador status estilo-${estilo}`}>
            <div className="cabecalho">Status</div>
            <div className="valor">{rotulo}</div>
        </div>
    );
}
