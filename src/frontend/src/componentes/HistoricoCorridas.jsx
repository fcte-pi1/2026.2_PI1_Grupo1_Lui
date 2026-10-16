import React from 'react';

export default function HistoricoCorridas({ corridas = [] }) {
    return (
        <div className="indicador historico" data-testid="historico-corridas">
            <div className="cabecalho">HISTÓRICO</div>
            <div className="historico-itens">
                <p>Execuções Anteriores</p>
                {corridas.length === 0 ? (
                    <div className="historico-vazio" data-testid="historico-vazio" style={{ color: '#718096', fontSize: '0.9rem', fontStyle: 'italic', padding: '8px 0' }}>
                        Nenhuma corrida registrada
                    </div>
                ) : (
                    corridas.map((corrida, idx) => (
                        <div key={corrida.id || idx} className="historico-item" data-testid={`historico-item-${idx}`}>
                            <span>{corrida.nome || `Tentativa ${idx + 1}`}</span>
                            <strong>{corrida.status || corrida.resultado || 'Concluído'}</strong>
                        </div>
                    ))
                )}
            </div>
        </div>
    );
}
