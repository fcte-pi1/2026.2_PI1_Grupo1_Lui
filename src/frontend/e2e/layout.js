// Verificações de layout compartilhadas pelos testes de desktop (RNF-SFT05, #65) e
// de celular (RNF-SFT06, #66).

export const MAPA = '.dashboard-layout > [data-testid="labirinto-container"]';

// Componentes que o operador precisa ver, pelo seletor de cada um.
export const COMPONENTES = {
    'mapa ao vivo': MAPA,
    status: '[data-testid="indicador-status"]',
    tempo: '[data-testid="indicador-tempo"]',
    velocidade: '[data-testid="indicador-velocidade"]',
    bateria: '[data-testid="indicador-bateria"]',
    'histórico da sessão': '[data-testid="historico-corridas"]',
    'histórico geral': '[data-testid="historico-geral"]',
    'replay da corrida': '[data-testid="replay-corrida"]',
};

// Procura, dentro da página, o que passa dos limites: rolagem horizontal da página,
// blocos do layout fora do seu container, qualquer elemento fora do seu cartão e
// áreas com rolagem própria que precisam rolar na horizontal.
//
// Roda no navegador com page.evaluate(procurarDistorcoes, opcoes), então não pode
// usar nada de fora da função. `rolagemPermitida` lista os seletores das áreas com
// rolagem própria que podem rolar na horizontal (no celular, a tabela do histórico).
export function procurarDistorcoes({ rolagemPermitida = [] } = {}) {
    const TOLERANCIA = 1;
    const problemas = [];
    const caixa = (el) => el.getBoundingClientRect();
    const nome = (el) => {
        if (el.dataset?.testid) return `[data-testid="${el.dataset.testid}"]`;
        const classes = typeof el.className === 'string' ? el.className.trim().split(/\s+/).filter(Boolean) : [];
        return el.tagName.toLowerCase() + classes.map((c) => `.${c}`).join('');
    };
    const rolaSozinho = (el) => ['auto', 'scroll'].includes(getComputedStyle(el).overflowX);
    const verificar = (filho, pai) => {
        const f = caixa(filho);
        const p = caixa(pai);
        if (f.width === 0 && f.height === 0) return;
        const fora = [
            f.left < p.left - TOLERANCIA && `${Math.round(p.left - f.left)} px à esquerda`,
            f.right > p.right + TOLERANCIA && `${Math.round(f.right - p.right)} px à direita`,
            f.top < p.top - TOLERANCIA && `${Math.round(p.top - f.top)} px acima`,
            f.bottom > p.bottom + TOLERANCIA && `${Math.round(f.bottom - p.bottom)} px abaixo`,
        ].filter(Boolean);
        if (fora.length) problemas.push(`${nome(filho)} passa de ${nome(pai)} (${fora.join(', ')})`);
    };

    const raiz = document.documentElement;
    if (raiz.scrollWidth > raiz.clientWidth) {
        problemas.push(`rolagem horizontal na página (${raiz.scrollWidth} px de conteúdo em ${raiz.clientWidth} px)`);
    }

    // Blocos do layout dentro dos seus containers.
    for (const seletor of ['.painel-container', '.dashboard-layout', '.indicadores-coluna', '.secao-historico-geral']) {
        const container = document.querySelector(seletor);
        if (container) for (const filho of container.children) verificar(filho, container);
    }

    // Todo elemento de um cartão dentro do cartão, salvo o que fica numa área com
    // rolagem própria (essa área é conferida abaixo).
    const cartoes = document.querySelectorAll(
        '.indicador, [data-testid="labirinto-container"], .historico-geral-container, .replay-card'
    );
    for (const cartao of cartoes) {
        for (const elemento of cartao.querySelectorAll('*')) {
            let ancestral = elemento.parentElement;
            let dentroDeRolagem = false;
            while (ancestral && ancestral !== cartao) {
                if (rolaSozinho(ancestral)) dentroDeRolagem = true;
                ancestral = ancestral.parentElement;
            }
            if (!dentroDeRolagem) verificar(elemento, cartao);
        }
    }

    for (const elemento of document.querySelectorAll('.painel-container *')) {
        if (rolagemPermitida.some((seletor) => elemento.matches(seletor))) continue;
        if (rolaSozinho(elemento) && elemento.scrollWidth > elemento.clientWidth + TOLERANCIA) {
            problemas.push(`${nome(elemento)} precisa de rolagem horizontal (${elemento.scrollWidth} px em ${elemento.clientWidth} px)`);
        }
    }
    return [...new Set(problemas)];
}
