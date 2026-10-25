import React from 'react';
import { rotuloStatus, estiloStatus } from '../servicos/formatacao';

export default function IndicadorStatus({ status, tentativa = null }) {
    const rotulo = rotuloStatus(status);
    const estilo = estiloStatus(status);
    const emAlerta = estilo === 'alerta';

    return (
        <div
            className={`indicador status estilo-${estilo}`}
            data-testid="indicador-status"
        >
            <div className="cabecalho">
                <span>Status</span>
                {tentativa ? (
                    <span className="status-tentativa" data-testid="status-tentativa">
                        {`Tentativa ${tentativa}`}
                    </span>
                ) : null}
            </div>
            <div className="valor" data-testid="valor-status">
                {emAlerta && (
                    <span
                        className="icone-alerta-status"
                        data-testid="icone-alerta-status"
                        aria-hidden="true"
                    >
                        ⚠{' '}
                    </span>
                )}
                <span>{rotulo}</span>
            </div>
            {status === 'TRAVADO' && (
                <div className="aviso-status-alerta" data-testid="aviso-travamento">
                    Corte de tração por travamento das rodas
                </div>
            )}
        </div>
    );
}
