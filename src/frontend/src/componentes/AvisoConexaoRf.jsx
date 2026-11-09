import React from 'react';

// Aviso da HU-09: o painel está ligado ao backend, mas os pacotes do robô pararam
// de chegar. Os indicadores continuam com o último pacote recebido.
export default function AvisoConexaoRf({ perdida = false, segundosSemPacote = 0 }) {
    if (!perdida) return null;
    return (
        <div className="aviso-rf" role="alert" data-testid="aviso-rf">
            <strong>Conexão RF perdida</strong>
            <span className="aviso-rf-detalhe" data-testid="aviso-rf-detalhe">
                {`Nenhum pacote do robô há ${segundosSemPacote} s. Os dados exibidos são do último pacote recebido.`}
            </span>
        </div>
    );
}
