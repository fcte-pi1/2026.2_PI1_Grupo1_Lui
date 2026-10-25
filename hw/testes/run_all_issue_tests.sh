#!/usr/bin/env bash
# ==============================================================================
# SCRIPT MASTER DE EXECUÇÃO DE TESTES AUTOMATIZADOS - HARDWARE MICROMOUSE
# PI1 2026.2 (Prof. Lui - Grupo 1)
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "======================================================================"
echo "  EXECUTANDO TESTES DE TODAS AS ISSUES DE ELETRÔNICA E HARDWARE"
echo "  Repositorio: https://github.com/fcte-pi1/2026.2_PI1_Grupo1_Lui"
echo "======================================================================"

ISSUES=(
    "issue_98_ihm"
    "issue_99_hardware_selftest"
    "issue_131_alimentacao_tensao"
    "issue_132_carga_autonomia_protecao"
    "issue_135_vl53l0x_calibracao"
    "issue_136_ponte_h_encoders"
    "issue_137_espnow_enlace_8m"
    "issue_140_telemetria_gateway"
)

TOTAL=${#ISSUES[@]}
PASSED=0

for issue_dir in "${ISSUES[@]}"; do
    if [ -d "$issue_dir/tests" ]; then
        echo -e "\n----------------------------------------------------------------------"
        echo ">>> Executando suíte da pasta: $issue_dir"
        echo "----------------------------------------------------------------------"
        (cd "$issue_dir/tests" && ./run_tests.sh)
        PASSED=$((PASSED + 1))
    else
        echo "[AVISO] Pasta $issue_dir/tests não encontrada!"
    fi
done

echo -e "\n======================================================================"
echo "  RELATORIO GERAL: Todas as $PASSED de $TOTAL suítes de testes passaram com 100% de sucesso!"
echo "======================================================================"
