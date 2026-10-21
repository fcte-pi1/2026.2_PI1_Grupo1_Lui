import React, { useState, useMemo } from 'react';
import { filtrarCorridas, formatarDataHora } from '../servicos/historicoFiltros';
import { rotuloStatus } from '../servicos/formatacao';
import ReplayCorrida from './ReplayCorrida';
import './HistoricoCorridas.css';

/**
 * Retorna classe e ícone do status conforme protótipo visual
 */
function obterEstiloStatus(status) {
    const s = String(status || '').toUpperCase();
    if (s === 'CONCLUIDO' || s === 'SUCESSO' || s === 'CONCLUÍDO') {
        return { classe: 'sucesso', icone: '✓', texto: 'SUCESSO' };
    }
    if (s === 'COLISAO' || s === 'COLISÃO') {
        return { classe: 'alerta', icone: '✕', texto: 'COLISÃO' };
    }
    if (s === 'TRAVADO') {
        return { classe: 'alerta', icone: '✕', texto: 'TRAVADO' };
    }
    if (s === 'TIMEOUT') {
        return { classe: 'alerta', icone: '✕', texto: 'TEMPO ESGOTADO' };
    }
    return { classe: 'info', icone: '•', texto: rotuloStatus(status) };
}

/**
 * Componente HistoricoCorridas
 * 
 * Suporta dois modos:
 * 1. modo="tabela" (padrão): Exibição completa da tabela com filtros por geometria e data (HU-13 / Protótipo).
 * 2. modo="compacto": Lista simplificada usada no card lateral do painel de monitoramento.
 */
export default function HistoricoCorridas({
    corridas = [],
    modo = 'tabela',
    corridaSelecionadaId = null,
    aoSelecionarCorrida = () => {},
    exibirInspecao = false
}) {
    const [filtroGeometria, setFiltroGeometria] = useState('todas');
    const [filtroDataInicio, setFiltroDataInicio] = useState('');
    const [filtroDataFim, setFiltroDataFim] = useState('');
    const [selecionadaInternaId, setSelecionadaInternaId] = useState(null);

    const corridasFiltradas = useMemo(() => {
        return filtrarCorridas(corridas, {
            geometria: filtroGeometria,
            dataInicio: filtroDataInicio,
            dataFim: filtroDataFim
        });
    }, [corridas, filtroGeometria, filtroDataInicio, filtroDataFim]);

    const idAtivo = corridaSelecionadaId ?? selecionadaInternaId ?? (exibirInspecao && corridasFiltradas[0]?.id);
    const corridaAtiva = useMemo(() => {
        if (!idAtivo) return corridasFiltradas[0] || null;
        return corridasFiltradas.find((c) => c.id === idAtivo) || corridasFiltradas[0] || null;
    }, [corridasFiltradas, idAtivo]);

    const selecionarCorrida = (corrida) => {
        setSelecionadaInternaId(corrida?.id ?? null);
        aoSelecionarCorrida(corrida);
    };

    const limparFiltros = () => {
        setFiltroGeometria('todas');
        setFiltroDataInicio('');
        setFiltroDataFim('');
    };

    // Modo compacto (compatível com o card lateral no Painel de monitoramento)
    if (modo === 'compacto') {
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

    // Modo tabela completo com filtros (HU-13 / CT-11)
    return (
        <div className="historico-geral-container" data-testid="historico-geral">
            <div className="historico-header">
                <h2 className="historico-titulo">Histórico Geral (General History)</h2>
                
                <div className="historico-filtros" data-testid="historico-filtros">
                    <div className="filtro-grupo">
                        <label htmlFor="filtro-geometria">Geometria:</label>
                        <select
                            id="filtro-geometria"
                            data-testid="filtro-geometria"
                            className="filtro-select"
                            value={filtroGeometria}
                            onChange={(e) => setFiltroGeometria(e.target.value)}
                        >
                            <option value="todas">Todas</option>
                            <option value="4x4">4x4</option>
                            <option value="8x4">8x4</option>
                            <option value="12x4">12x4</option>
                        </select>
                    </div>

                    <div className="filtro-grupo">
                        <label htmlFor="filtro-data-inicio">De:</label>
                        <input
                            type="date"
                            id="filtro-data-inicio"
                            data-testid="filtro-data-inicio"
                            className="filtro-input"
                            value={filtroDataInicio}
                            onChange={(e) => setFiltroDataInicio(e.target.value)}
                        />
                    </div>

                    <div className="filtro-grupo">
                        <label htmlFor="filtro-data-fim">Até:</label>
                        <input
                            type="date"
                            id="filtro-data-fim"
                            data-testid="filtro-data-fim"
                            className="filtro-input"
                            value={filtroDataFim}
                            onChange={(e) => setFiltroDataFim(e.target.value)}
                        />
                    </div>

                    {(filtroGeometria !== 'todas' || filtroDataInicio || filtroDataFim) && (
                        <button
                            type="button"
                            className="botao-limpar-filtros"
                            data-testid="botao-limpar-filtros"
                            onClick={limparFiltros}
                        >
                            Limpar
                        </button>
                    )}
                </div>
            </div>

            <div className="tabela-wrapper">
                <table className="tabela-historico" data-testid="tabela-historico">
                    <thead>
                        <tr>
                            <th>#</th>
                            <th>Data</th>
                            <th>Grid</th>
                            <th>Modo</th>
                            <th>Tentativas</th>
                            <th>Tempo Final</th>
                            <th>Vel. Média</th>
                            <th>Células Visitadas</th>
                            <th>Status</th>
                        </tr>
                    </thead>
                    <tbody>
                        {corridasFiltradas.length === 0 ? (
                            <tr>
                                <td colSpan="9" className="historico-tabela-vazia" data-testid="tabela-vazia">
                                    Nenhuma corrida encontrada
                                </td>
                            </tr>
                        ) : (
                            corridasFiltradas.map((c, idx) => {
                                const idExibicao = c.id || String(idx + 1).padStart(3, '0');
                                const statusFormatado = obterEstiloStatus(c.status);
                                const isSelecionada = idAtivo !== null && idAtivo !== undefined && idAtivo === c.id;

                                return (
                                    <tr
                                        key={c.id || idx}
                                        className={`linha-corrida ${isSelecionada ? 'selecionada' : ''}`}
                                        data-testid={`linha-corrida-${c.id || idx}`}
                                        onClick={() => selecionarCorrida(c)}
                                    >
                                        <td className="col-id">{idExibicao}</td>
                                        <td>{formatarDataHora(c.data || c.inicio)}</td>
                                        <td className="col-geometria">{c.geometria || c.grid || c.labirinto || '-'}</td>
                                        <td>{c.modo || 'Exploração'}</td>
                                        <td>{c.tentativas || c.tentativa || '1 de 1'}</td>
                                        <td>{c.tempoFinal || c.tempo_total || '-'}</td>
                                        <td>{c.velMedia || (c.velocidade_media ? `${c.velocidade_media} cm/s` : '-')}</td>
                                        <td>{c.celulasVisitadas || (c.celulas_visitadas ? `${c.celulas_visitadas}` : '-')}</td>
                                        <td>
                                            <span className={`status-badge ${statusFormatado.classe}`}>
                                                <span>{statusFormatado.icone}</span>
                                                <span>{statusFormatado.texto}</span>
                                            </span>
                                        </td>
                                    </tr>
                                );
                            })
                        )}
                    </tbody>
                </table>
            </div>

            {exibirInspecao && (
                <div style={{ marginTop: '24px' }} data-testid="secao-inspecao-historico">
                    <ReplayCorrida corrida={corridaAtiva} />
                </div>
            )}
        </div>
    );
}
