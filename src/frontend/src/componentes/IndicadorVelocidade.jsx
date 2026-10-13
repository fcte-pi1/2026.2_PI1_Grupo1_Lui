import React from 'react';

export default function IndicadorVelocidade({ atual, media }) {
    return (
        <div className="indicador velocidade">
            <div className="cabecalho">Velocidade</div>
            <div className="valores">
                <div className="vel-bloco">
                    <span className="label">Atual:</span>
                    <span className="valor">{atual != null ? atual : 0} cm/s</span>
                </div>
                <div className="vel-bloco">
                    <span className="label">Média:</span>
                    <span className="valor">{media != null ? media : 0} cm/s</span>
                </div>
            </div>
        </div>
    );
}
