import React, { useState, useMemo, useEffect } from 'react';
import { filtrarCorridas, formatarDataHora } from '../servicos/historicoFiltros';
import { rotuloStatus, rotuloModo, formatarTempo, compararTemposSpeedRun } from '../servicos/formatacao';
import { obterCorrida } from '../servicos/corridasApi';
import ReplayCorrida from './ReplayCorrida';
import './HistoricoCorridas.css';

function ehCorridaConcluida(status) {
    const s = String(status || '').toUpperCase();
    return s === 'CONCLUIDO' || s === 'SUCESSO' || s === 'CONCLUÍDO';
}

/**
 * Calcula em O(N) a comparação de tempo de cada corrida Speed Run com a
 * exploração concluída anterior na mesma geometria (HU-16 / RF-SFT09).
 */
function calcularComparacoesPorCorrida(corridas) {
    const mapa = new Map();
    if (!Array.isArray(corridas) || corridas.length === 0) return mapa;

    const ordemReversa =
        corridas.length >= 2 &&
        typeof corridas[0]?.id === 'number' &&
        typeof corridas[corridas.length - 1]?.id === 'number' &&
        corridas[0].id > corridas[corridas.length - 1].id;

    const ultimaExploracaoPorGeometria = new Map();
    const total = corridas.length;

    for (let passo = 0; passo < total; passo++) {
        const idx = ordemReversa ? total - 1 - passo : passo;
        const c = corridas[idx];
        if (!c) continue;
        const chave = c.id ?? idx;

        if (c.comparacao) {
            mapa.set(chave, c.comparacao);
            continue;
        }

        const geo = c.geometria || c.grid || c.labirinto || '4x4';
        const modo = rotuloModo(c.modo);
        const tempoMs = typeof c.tempo_total_ms === 'number' ? c.tempo_total_ms : c.timestamp_ms;

        if (modo === 'Exploração' && ehCorridaConcluida(c.status || c.resultado) && typeof tempoMs === 'number' && tempoMs > 0) {
            ultimaExploracaoPorGeometria.set(geo, tempoMs);
        } else if (modo === 'Speed Run' && typeof tempoMs === 'number' && tempoMs > 0) {
            const tempoExp =
                typeof c.tempo_exploracao_ms === 'number'
                    ? c.tempo_exploracao_ms
                    : ultimaExploracaoPorGeometria.get(geo);
            if (typeof tempoExp === 'number' && tempoExp > 0) {
                const comp = compararTemposSpeedRun(tempoExp, tempoMs);
                if (comp) {
                    mapa.set(chave, comp);
                }
            }
        }
    }

    return mapa;
}

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
    if (s === 'INTERROMPIDA') {
        return { classe: 'alerta', icone: '!', texto: 'INTERROMPIDA' };
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
    exibirInspecao = false,
    aoFiltrar = null
}) {
    const [filtroGeometria, setFiltroGeometria] = useState('todas');
    const [filtroDataInicio, setFiltroDataInicio] = useState('');
    const [filtroDataFim, setFiltroDataFim] = useState('');
    const [selecionadaInternaId, setSelecionadaInternaId] = useState(null);
    const [corridaDetalhada, setCorridaDetalhada] = useState(null);

    const mudarFiltros = (novos) => {
        const proximo = {
            geometria: novos.geometria !== undefined ? novos.geometria : filtroGeometria,
            dataInicio: novos.dataInicio !== undefined ? novos.dataInicio : filtroDataInicio,
            dataFim: novos.dataFim !== undefined ? novos.dataFim : filtroDataFim,
        };
        if (novos.geometria !== undefined) setFiltroGeometria(novos.geometria);
        if (novos.dataInicio !== undefined) setFiltroDataInicio(novos.dataInicio);
        if (novos.dataFim !== undefined) setFiltroDataFim(novos.dataFim);

        if (typeof aoFiltrar === 'function') {
            aoFiltrar(proximo);
        }
    };

    const corridasFiltradas = useMemo(() => {
        return filtrarCorridas(corridas, {
            geometria: filtroGeometria,
            dataInicio: filtroDataInicio,
            dataFim: filtroDataFim
        });
    }, [corridas, filtroGeometria, filtroDataInicio, filtroDataFim]);

    const comparacoesPorCorrida = useMemo(() => {
        return calcularComparacoesPorCorrida(corridas);
    }, [corridas]);

    const idAtivo = corridaSelecionadaId ?? selecionadaInternaId ?? (exibirInspecao && corridasFiltradas[0]?.id);
    const corridaAtiva = useMemo(() => {
        if (!idAtivo) return corridasFiltradas[0] || null;
        return corridasFiltradas.find((c) => c.id === idAtivo) || corridasFiltradas[0] || null;
    }, [corridasFiltradas, idAtivo]);

    useEffect(() => {
        if (!exibirInspecao || !corridaAtiva) {
            setCorridaDetalhada(null);
            return;
        }

        if (corridaAtiva.leituras && corridaAtiva.leituras.length > 0) {
            setCorridaDetalhada(corridaAtiva);
            return;
        }

        if (corridaAtiva.id != null) {
            let ativo = true;
            obterCorrida(corridaAtiva.id)
                .then((dados) => {
                    if (ativo) setCorridaDetalhada(dados);
                })
                .catch(() => {
                    if (ativo) setCorridaDetalhada(corridaAtiva);
                });
            return () => {
                ativo = false;
            };
        }

        setCorridaDetalhada(corridaAtiva);
    }, [exibirInspecao, corridaAtiva]);

    const selecionarCorrida = (corrida) => {
        setSelecionadaInternaId(corrida?.id ?? null);
        aoSelecionarCorrida(corrida);
    };

    const limparFiltros = () => {
        mudarFiltros({ geometria: 'todas', dataInicio: '', dataFim: '' });
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
                        corridas.map((corrida, idx) => {
                            const chave = corrida.id ?? idx;
                            const comparacao = comparacoesPorCorrida.get(chave) || corrida.comparacao || null;
                            const tempoMs = typeof corrida.tempo_total_ms === 'number' ? corrida.tempo_total_ms : corrida.timestamp_ms;
                            const textoModo = (corrida.modo || corrida.rotuloModo)
                                ? (corrida.rotuloModo || rotuloModo(corrida.modo))
                                : null;
                            const textoStatus = corrida.resultado || corrida.status || 'Concluído';

                            const rotuloTentativa = `${corrida.nome || `Tentativa ${idx + 1}`}${textoModo ? ` · ${textoModo}` : ''}`;
                            const rotuloResultado = `${typeof tempoMs === 'number' ? `${formatarTempo(tempoMs)} · ` : ''}${textoStatus}`;
                            const rotuloComparacao = comparacao
                                ? `${comparacao.reducaoPct > 0 ? `-${comparacao.reducaoPct}%` : `${comparacao.reducaoPct}%`} vs. Exploração (${comparacao.tempoExploracaoFormatado})`
                                : null;

                            return (
                                <div key={corrida.id || idx} className="historico-item" data-testid={`historico-item-${idx}`}>
                                    <div className="historico-item-cabecalho">
                                        <span>{rotuloTentativa}</span>
                                        <strong>{rotuloResultado}</strong>
                                    </div>
                                    {rotuloComparacao && (
                                        <div className="historico-comparacao-speed-run" data-testid="comparacao-speed-run">
                                            {rotuloComparacao}
                                        </div>
                                    )}
                                </div>
                            );
                        })
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
                            onChange={(e) => mudarFiltros({ geometria: e.target.value })}
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
                            onChange={(e) => mudarFiltros({ dataInicio: e.target.value })}
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
                            onChange={(e) => mudarFiltros({ dataFim: e.target.value })}
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
                                const idExibicao = c.id != null ? String(c.id).padStart(3, '0') : String(idx + 1).padStart(3, '0');
                                const statusFormatado = obterEstiloStatus(c.status);
                                const isSelecionada = idAtivo !== null && idAtivo !== undefined && idAtivo === c.id;
                                const comparacao = comparacoesPorCorrida.get(c.id ?? idx) || c.comparacao || null;

                                const tempoFormatado = c.tempoFinal || c.tempo_total || (typeof c.tempo_total_ms === 'number' ? formatarTempo(c.tempo_total_ms) : '-');
                                const modoFormatado = rotuloModo(c.modo);
                                const tentativasFormatadas = c.tentativas || (c.tentativa != null ? `Tentativa ${c.tentativa}` : '—');
                                const velMediaFormatada = c.velMedia || (typeof c.velocidade_media === 'number' ? `${c.velocidade_media} cm/s` : '-');
                                const celulasFormatadas = c.celulasVisitadas || (c.celulas_visitadas != null ? `${c.celulas_visitadas}` : '-');

                                return (
                                    <tr
                                        key={c.id || idx}
                                        className={`linha-corrida ${isSelecionada ? 'selecionada' : ''}`}
                                        data-testid={`linha-corrida-${c.id || idx}`}
                                        onClick={() => selecionarCorrida(c)}
                                    >
                                        <td className="col-id">{idExibicao}</td>
                                        <td>{formatarDataHora(c.data || c.inicio || c.iniciada_em)}</td>
                                        <td className="col-geometria">{c.geometria || c.grid || c.labirinto || '-'}</td>
                                        <td>{modoFormatado}</td>
                                        <td>{tentativasFormatadas}</td>
                                        <td>
                                            <span>{tempoFormatado}</span>
                                            {comparacao && (
                                                <span
                                                    className="comparacao-speed-run-badge"
                                                    data-testid="comparacao-tabela-speed-run"
                                                >
                                                    {` (${comparacao.reducaoPct > 0 ? `-${comparacao.reducaoPct}%` : `${comparacao.reducaoPct}%`} vs. ${comparacao.tempoExploracaoFormatado})`}
                                                </span>
                                            )}
                                        </td>
                                        <td>{velMediaFormatada}</td>
                                        <td>{celulasFormatadas}</td>
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
                    <ReplayCorrida corrida={corridaDetalhada || corridaAtiva} />
                </div>
            )}
        </div>
    );
}
