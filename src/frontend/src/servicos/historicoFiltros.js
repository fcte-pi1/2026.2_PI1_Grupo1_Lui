/**
 * servicos/historicoFiltros.js
 * 
 * Regras de filtragem e formatação de histórico de corridas (HU-13 / RF-SFT08).
 * Funções puras para garantir alta performance e testabilidade conforme RNF-SFT04 (< 2.0s).
 */

/**
 * Normaliza uma string de data para o formato YYYY-MM-DD
 * para permitir comparações lexicais diretas e consistentes.
 * Aceita:
 * - ISO string: "2023-10-25T15:30:00Z" ou "2023-10-25"
 * - Formato BR: "25/10/2023" ou "25/10/2023 15:30"
 * - Objeto Date ou timestamp em ms
 * 
 * @param {string|number|Date} valor 
 * @returns {string|null} "YYYY-MM-DD" ou null se inválido
 */
export function extrairDataIso(valor) {
    if (!valor && valor !== 0) return null;

    if (typeof valor === 'string') {
        const str = valor.trim();
        // Caso 1: Formato "YYYY-MM-DD..."
        const matchIso = str.match(/^(\d{4})-(\d{2})-(\d{2})/);
        if (matchIso) {
            return `${matchIso[1]}-${matchIso[2]}-${matchIso[3]}`;
        }
        // Caso 2: Formato "DD/MM/YYYY..."
        const matchBr = str.match(/^(\d{2})\/(\d{2})\/(\d{4})/);
        if (matchBr) {
            return `${matchBr[3]}-${matchBr[2]}-${matchBr[1]}`;
        }
    }

    const d = new Date(valor);
    if (isNaN(d.getTime())) return null;

    const ano = d.getFullYear();
    const mes = String(d.getMonth() + 1).padStart(2, '0');
    const dia = String(d.getDate()).padStart(2, '0');
    return `${ano}-${mes}-${dia}`;
}

/**
 * Formata um timestamp ou string de data para "DD/MM/AAAA HH:mm" (conforme protótipo da tela de histórico)
 * 
 * @param {string|number|Date} valor 
 * @returns {string} Data formatada ou "-"
 */
export function formatarDataHora(valor) {
    if (!valor && valor !== 0) return "-";

    // Se já vier no formato "DD/MM/YYYY HH:mm", retorna como está
    if (typeof valor === 'string' && /^\d{2}\/\d{2}\/\d{4}\s+\d{2}:\d{2}$/.test(valor.trim())) {
        return valor.trim();
    }

    const d = new Date(valor);
    if (isNaN(d.getTime())) {
        return String(valor);
    }

    const dia = String(d.getDate()).padStart(2, '0');
    const mes = String(d.getMonth() + 1).padStart(2, '0');
    const ano = d.getFullYear();
    const hora = String(d.getHours()).padStart(2, '0');
    const min = String(d.getMinutes()).padStart(2, '0');

    return `${dia}/${mes}/${ano} ${hora}:${min}`;
}

/**
 * Filtra a lista de corridas com base nos critérios especificados.
 * 
 * @param {Array<Object>} corridas - Lista com as corridas
 * @param {Object} filtros
 * @param {string} [filtros.geometria] - "todas" | "4x4" | "8x4" | "12x4"
 * @param {string} [filtros.data] - Data específica ("YYYY-MM-DD" ou "DD/MM/YYYY")
 * @param {string} [filtros.dataInicio] - Data início ("YYYY-MM-DD" ou "DD/MM/YYYY")
 * @param {string} [filtros.dataFim] - Data fim ("YYYY-MM-DD" ou "DD/MM/YYYY")
 * @returns {Array<Object>} Lista filtrada de corridas
 */
export function filtrarCorridas(corridas = [], filtros = {}) {
    if (!Array.isArray(corridas) || corridas.length === 0) {
        return [];
    }

    const { geometria, data, dataInicio, dataFim } = filtros;

    const normalizarGeo = (geo) => {
        if (!geo) return '';
        const g = String(geo).toLowerCase().trim();
        return g === 'todas' || g === 'todos' ? '' : g;
    };

    const geoFiltro = normalizarGeo(geometria);
    const dataFiltroIso = extrairDataIso(data);
    const inicioIso = extrairDataIso(dataInicio);
    const fimIso = extrairDataIso(dataFim);

    return corridas.filter((c) => {
        // 1. Filtro por geometria
        if (geoFiltro) {
            const geoCorrida = String(c.geometria || c.grid || c.labirinto || '').toLowerCase().trim();
            if (geoCorrida !== geoFiltro) {
                return false;
            }
        }

        // 2. Filtro por data
        const dataCorrida = c.data || c.inicio || c.timestamp || c.dataHora;
        const dataCorridaIso = extrairDataIso(dataCorrida);

        if (dataFiltroIso) {
            if (dataCorridaIso !== dataFiltroIso) {
                return false;
            }
        }

        if (inicioIso) {
            if (!dataCorridaIso || dataCorridaIso < inicioIso) {
                return false;
            }
        }

        if (fimIso) {
            if (!dataCorridaIso || dataCorridaIso > fimIso) {
                return false;
            }
        }

        return true;
    });
}
