import React, { useMemo, useState } from 'react';
import { DOMINIOS, MAPA_HARDWARE_ESP32 } from '../servicos/diagnosticoLogs';
import {
    IconeChip,
    IconeDica,
    IconeDominio,
    IconeEnergia,
    IconeMecanica,
    IconeRadio,
    IconeSensor,
    IconeSoftware,
} from './IconesDiagnostico';
import './PainelDiagnosticoLogs.css';

const FILTROS = [
    { id: 'todos', rotulo: 'Todos' },
    { id: 'software', rotulo: 'Software' },
    { id: 'eletronica', rotulo: 'Eletrônica' },
    { id: 'energia', rotulo: 'Energia' },
    { id: 'mecanica', rotulo: 'Mecânica' },
    { id: 'radio', rotulo: 'Rádio' },
];

function inferirSaudeSubsistema(id, pacote, conectado, rfPerdida) {
    if (!conectado) return { nivel: 'critico', texto: 'WS Off' };
    if (id === 'radio') {
        if (rfPerdida) return { nivel: 'alerta', texto: 'RF Perdida' };
        if (typeof pacote?.rssi === 'number') {
            return { nivel: pacote.rssi < -82 ? 'alerta' : 'ok', texto: `${pacote.rssi} dBm` };
        }
        return { nivel: pacote ? 'ok' : 'neutro', texto: pacote ? 'RF OK' : 'Aguardando' };
    }
    if (!pacote) return { nivel: 'neutro', texto: 'Sem telemetria' };
    if (id === 'bateria') {
        if (typeof pacote.bateria_volts !== 'number') return { nivel: 'neutro', texto: '--' };
        if (pacote.bateria_volts < 6.8) return { nivel: 'critico', texto: `${pacote.bateria_volts.toFixed(2)} V` };
        if (pacote.bateria_volts < 7.05) return { nivel: 'alerta', texto: `${pacote.bateria_volts.toFixed(2)} V` };
        return { nivel: 'ok', texto: `${pacote.bateria_volts.toFixed(2)} V` };
    }
    if (id === 'tof') {
        if (pacote.status === 'COLISAO') return { nivel: 'critico', texto: 'Colisão frontal' };
        return { nivel: 'ok', texto: '3x ativos' };
    }
    if (id === 'ponte' || id === 'encoders') {
        if (pacote.status === 'TRAVADO') return { nivel: 'critico', texto: 'Corte / Sem pulso' };
        return { nivel: 'ok', texto: `${pacote.velocidade_atual ?? 0} cm/s` };
    }
    return { nivel: 'ok', texto: pacote.status || 'OK' };
}

export default function PainelDiagnosticoLogs({
    logs = [],
    eventosWebhook = [],
    pacote = null,
    conectado = false,
    rfPerdida = false,
    aoLimpar = () => {},
    aoLimparWebhook = () => {},
}) {
    const [abaAtiva, setAbaAtiva] = useState('diagnostico');
    const [filtroDominio, setFiltroDominio] = useState('todos');
    const [somenteAlertas, setSomenteAlertas] = useState(false);

    const contadores = useMemo(() => {
        const mapa = { todos: logs.length, software: 0, eletronica: 0, energia: 0, mecanica: 0, radio: 0 };
        for (const item of logs) {
            if (mapa[item.dominio] !== undefined) mapa[item.dominio]++;
            if (item.secundario && mapa[item.secundario] !== undefined) mapa[item.secundario]++;
        }
        return mapa;
    }, [logs]);

    const logsFiltrados = useMemo(() => {
        return logs.filter((item) => {
            if (somenteAlertas && item.nivel !== 'alerta' && item.nivel !== 'critico') return false;
            if (filtroDominio === 'todos') return true;
            return item.dominio === filtroDominio || item.secundario === filtroDominio;
        });
    }, [logs, filtroDominio, somenteAlertas]);

    const paredes = pacote?.paredes || {};
    const saudeRf = !conectado ? 'critico' : rfPerdida ? 'alerta' : 'ok';
    const saudeBat = typeof pacote?.bateria_volts !== 'number'
        ? 'neutro'
        : pacote.bateria_volts < 6.8 ? 'critico' : pacote.bateria_volts < 7.05 ? 'alerta' : 'ok';
    const saudeMov = (pacote?.status === 'TRAVADO' || pacote?.status === 'COLISAO')
        ? 'critico'
        : pacote?.status === 'TIMEOUT' ? 'alerta' : pacote?.status ? 'ok' : 'neutro';

    return (
        <section className="indicador painel-diagnostico" data-testid="painel-diagnostico">
            <div className="diag-topo">
                <div className="diag-titulo-grupo">
                    <span className={`diag-pulso ${saudeMov === 'critico' || saudeRf === 'critico' ? 'critico' : 'ativo'}`} />
                    <div className="cabecalho">MONITOR &amp; WEBHOOK</div>
                </div>

                <div className="diag-saude-pills">
                    <span className={`saude-pill nivel-${saudeRf}`}>
                        <IconeRadio />
                        {!conectado ? 'WS Off' : rfPerdida ? 'RF Perdida' : typeof pacote?.rssi === 'number' ? `${pacote.rssi} dBm` : 'RF OK'}
                    </span>
                    <span className={`saude-pill nivel-${saudeBat}`}>
                        <IconeEnergia />
                        {typeof pacote?.bateria_volts === 'number' ? `${pacote.bateria_volts.toFixed(2)} V` : '-- V'}
                    </span>
                    <span className={`saude-pill nivel-${saudeMov}`}>
                        <IconeMecanica />
                        {pacote?.status || 'SEM DADOS'}
                    </span>
                    <span className="saude-pill" title="Paredes detectadas pelos 3 ToF na célula atual">
                        <IconeSensor />
                        <span className={paredes.norte ? 'parede-on' : 'parede-off'}>N</span>
                        <span className={paredes.leste ? 'parede-on' : 'parede-off'}>L</span>
                        <span className={paredes.sul ? 'parede-on' : 'parede-off'}>S</span>
                        <span className={paredes.oeste ? 'parede-on' : 'parede-off'}>O</span>
                    </span>
                </div>
            </div>

            <div className="diag-abas" role="tablist" aria-label="Abas de diagnóstico e webhook">
                <button
                    type="button"
                    role="tab"
                    aria-selected={abaAtiva === 'diagnostico'}
                    className={`diag-aba-btn ${abaAtiva === 'diagnostico' ? 'ativo' : ''}`}
                    data-testid="aba-diagnostico"
                    onClick={() => setAbaAtiva('diagnostico')}
                >
                    <IconeDica />
                    Diagnóstico + Portas ({logs.length})
                </button>
                <button
                    type="button"
                    role="tab"
                    aria-selected={abaAtiva === 'webhook'}
                    className={`diag-aba-btn ${abaAtiva === 'webhook' ? 'ativo' : ''}`}
                    data-testid="aba-webhook"
                    onClick={() => setAbaAtiva('webhook')}
                >
                    <IconeSoftware />
                    Webhook ({eventosWebhook.length})
                </button>
            </div>

            {abaAtiva === 'diagnostico' && (
                <div className="diag-conteudo">
                    <div className="diag-secao" data-testid="card-diagnostico-logs">
                        <div className="diag-secao-topo">
                            <span className="diag-secao-titulo">
                                <IconeDica />
                                Diagnóstico &amp; Hints
                            </span>
                            <div className="diag-acoes-direita">
                                <label className="diag-toggle">
                                    <input
                                        type="checkbox"
                                        checked={somenteAlertas}
                                        onChange={(e) => setSomenteAlertas(e.target.checked)}
                                    />
                                    Só alertas
                                </label>
                                <button type="button" className="diag-limpar-btn" onClick={aoLimpar}>
                                    Limpar
                                </button>
                            </div>
                        </div>

                        <div className="diag-filtros" role="group" aria-label="Filtrar logs por área">
                            {FILTROS.map((f) => (
                                <button
                                    key={f.id}
                                    type="button"
                                    className={`diag-filtro-btn ${filtroDominio === f.id ? 'ativo' : ''}`}
                                    onClick={() => setFiltroDominio(f.id)}
                                >
                                    {f.id !== 'todos' && <IconeDominio id={f.id} />}
                                    <span>{f.rotulo}</span>
                                    <span className="diag-contador">{contadores[f.id]}</span>
                                </button>
                            ))}
                        </div>

                        <div className="diag-lista" data-testid="diag-lista">
                            {logsFiltrados.length === 0 ? (
                                <div className="diag-vazio">
                                    Sem eventos no momento. Logs e dicas aparecerão aqui.
                                </div>
                            ) : (
                                logsFiltrados.map((item) => {
                                    const dom = DOMINIOS[item.dominio] || DOMINIOS.software;
                                    const domSec = item.secundario ? DOMINIOS[item.secundario] : null;
                                    return (
                                        <article key={item.id} className={`diag-item nivel-${item.nivel}`}>
                                            <div className="diag-item-cabecalho">
                                                <span className="diag-hora">
                                                    {item.hora}
                                                    {item.tempoCorrida ? ` · ${item.tempoCorrida}` : ''}
                                                </span>
                                                <span className={`diag-badge-nivel nivel-${item.nivel}`}>
                                                    {item.nivel.toUpperCase()}
                                                </span>
                                                <span className={`diag-badge-dominio dom-${dom.id}`}>
                                                    <IconeDominio id={dom.id} />
                                                    {dom.rotulo}
                                                </span>
                                                {domSec && (
                                                    <span className={`diag-badge-dominio dom-${domSec.id} secundario`}>
                                                        <IconeDominio id={domSec.id} />
                                                        {domSec.rotulo}
                                                    </span>
                                                )}
                                                <strong className="diag-item-titulo">{item.titulo}</strong>
                                            </div>
                                            <p className="diag-item-detalhe">{item.detalhe}</p>
                                            {item.dica && (
                                                <p className="diag-hint-linha">
                                                    <span className="diag-hint-label">
                                                        <IconeDica /> Dica:
                                                    </span>{' '}
                                                    {item.dica}
                                                </p>
                                            )}
                                        </article>
                                    );
                                })
                            )}
                        </div>
                    </div>

                    <div className="diag-secao com-divisor" data-testid="card-portas-esp">
                        <div className="diag-secao-topo">
                            <span className="diag-secao-titulo">
                                <IconeChip />
                                Portas &amp; Sensores ESP32
                            </span>
                            <span className="diag-secao-meta">
                                USB/Rádio: <code>i</code> ou <code>d</code>
                            </span>
                        </div>

                        <div className="diag-portas-grid" data-testid="diag-portas-painel">
                            {MAPA_HARDWARE_ESP32.map((hw) => {
                                const s = inferirSaudeSubsistema(hw.id, pacote, conectado, rfPerdida);
                                return (
                                    <div key={hw.id} className={`porta-linha nivel-${s.nivel}`}>
                                        <div className="porta-card-topo">
                                            <strong>{hw.nome}</strong>
                                            <span className={`porta-status nivel-${s.nivel}`}>{s.texto}</span>
                                        </div>
                                        <div className="porta-pinos">{hw.pinos}</div>
                                        <div className="porta-meta">
                                            <span>{hw.enderecos}</span>
                                            <span><code>{hw.testeUsb}</code></span>
                                        </div>
                                    </div>
                                );
                            })}
                        </div>
                    </div>
                </div>
            )}

            {abaAtiva === 'webhook' && (
                <div className="diag-secao" data-testid="card-webhook-stream">
                    <div className="webhook-cabecalho">
                        <span>Stream: <code>/ws/telemetria</code> &amp; <code>POST /api/comandos/largada</code></span>
                        <button type="button" className="diag-limpar-btn" onClick={aoLimparWebhook}>
                            Limpar stream
                        </button>
                    </div>
                    <div className="diag-lista" data-testid="webhook-lista">
                        {eventosWebhook.length === 0 ? (
                            <div className="diag-vazio">
                                Aguardando pacotes JSON do WebSocket ou comandos enviados pelo painel.
                            </div>
                        ) : (
                            eventosWebhook.map((ev) => (
                                <article key={ev.id} className="webhook-item">
                                    <div className="webhook-item-topo">
                                        <span className="diag-hora">{ev.hora}</span>
                                        <span className={`diag-badge-nivel nivel-${ev.direcao === 'TX' ? 'ok' : 'info'}`}>
                                            {ev.direcao}
                                        </span>
                                        <strong>{ev.origem}</strong>
                                    </div>
                                    <pre className="webhook-json">{ev.payload}</pre>
                                </article>
                            ))
                        )}
                    </div>
                </div>
            )}
        </section>
    );
}
