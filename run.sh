#!/usr/bin/env bash
# ==============================================================================
# SCRIPT DE INICIALIZAÇÃO UNIFICADA - MICROMOUSE (BACKEND + FRONTEND)
# Grupo 1 - PI1 2026.2 (Prof. Lui - FCTE/UnB)
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "======================================================================"
echo "         INICIANDO PLATAFORMA MICROMOUSE (SITE + SERVIDOR)            "
echo "======================================================================"

# ------------------------------------------------------------------------------
# 1. VERIFICAÇÃO / PREPARAÇÃO DO AMBIENTE PYTHON (.venv)
# ------------------------------------------------------------------------------
VENV_DIR="$SCRIPT_DIR/src/backend/.venv"

if [ ! -d "$VENV_DIR" ] || [ ! -f "$VENV_DIR/bin/activate" ]; then
    echo "[*] Criando ambiente virtual Python (.venv)..."
    python3 -m venv "$VENV_DIR"
    ln -sfn "$VENV_DIR" "$SCRIPT_DIR/.venv"
    echo "[*] Instalando dependências do backend..."
    "$VENV_DIR/bin/pip" install --quiet -r "$SCRIPT_DIR/src/backend/requirements.txt"
fi

# ------------------------------------------------------------------------------
# 2. IDENTIFICAÇÃO DE ARGUMENTOS, LABIRINTO E PORTA SERIAL DO GATEWAY (ESP8266)
# ------------------------------------------------------------------------------
PORTA_ARG=""
MODO_SIMULADOR=false

for arg in "$@"; do
    case "$arg" in
        --simulador|-s)
            MODO_SIMULADOR=true
            ;;
        4x4|8x4|12x4)
            export SIMULADOR_LABIRINTO="$arg"
            ;;
        /dev/tty*)
            PORTA_ARG="$arg"
            ;;
    esac
done

if [ -n "$PORTA_ARG" ] && [ -e "$PORTA_ARG" ]; then
    PORTA_DETECTADA="$PORTA_ARG"
elif [ -n "$SERIAL_PORTA" ] && [ -e "$SERIAL_PORTA" ]; then
    PORTA_DETECTADA="$SERIAL_PORTA"
elif [ -e "/dev/ttyUSB1" ]; then
    PORTA_DETECTADA="/dev/ttyUSB1"
elif [ -e "/dev/ttyUSB0" ]; then
    PORTA_DETECTADA="/dev/ttyUSB0"
elif [ -e "/dev/ttyACM0" ]; then
    PORTA_DETECTADA="/dev/ttyACM0"
else
    PORTA_DETECTADA=""
fi

if [ "$MODO_SIMULADOR" = true ]; then
    echo "[*] Modo forçado: SIMULADOR DE LABIRINTO VIRTUAL"
    export TELEMETRIA_FONTE="simulador"
    export SIMULADOR_CENARIO="${SIMULADOR_CENARIO:-completa}"
    export SIMULADOR_LABIRINTO="${SIMULADOR_LABIRINTO:-8x4}"
elif [ -n "$PORTA_DETECTADA" ]; then
    echo "[+] Gateway ESP8266 detectado na porta: $PORTA_DETECTADA"
    export TELEMETRIA_FONTE="serial"
    export SERIAL_PORTA="$PORTA_DETECTADA"
    export SERIAL_BAUD=115200
else
    echo "[!] AVISO: Nenhuma placa serial USB encontrada em /dev/ttyUSB*."
    echo "[*] Ativando modo SIMULADOR automático para exibição no site."
    export TELEMETRIA_FONTE="simulador"
    export SIMULADOR_CENARIO="${SIMULADOR_CENARIO:-completa}"
    export SIMULADOR_LABIRINTO="${SIMULADOR_LABIRINTO:-8x4}"
fi

# ------------------------------------------------------------------------------
# 3. VERIFICAÇÃO DO FRONTEND (NODE_MODULES)
# ------------------------------------------------------------------------------
if [ ! -d "$SCRIPT_DIR/src/frontend/node_modules" ]; then
    echo "[*] Instalando dependências do frontend (npm install)..."
    (cd "$SCRIPT_DIR/src/frontend" && npm install --silent)
fi

# ------------------------------------------------------------------------------
# 4. FUNÇÃO DE LIMPEZA E ENCERRAMENTO SEGURO (Ctrl+C)
# ------------------------------------------------------------------------------
cleanup() {
    trap - SIGINT SIGTERM EXIT
    echo ""
    echo "======================================================================"
    echo "  Encerrando servidor backend e frontend..."
    echo "======================================================================"
    if [ -n "$BACKEND_PID" ]; then
        kill "$BACKEND_PID" 2>/dev/null || true
    fi
    if [ -n "$FRONTEND_PID" ]; then
        kill "$FRONTEND_PID" 2>/dev/null || true
    fi
    wait 2>/dev/null || true
    echo "  Todos os processos foram encerrados com sucesso."
    exit 0
}

trap cleanup SIGINT SIGTERM EXIT

# ------------------------------------------------------------------------------
# 5. INICIALIZAÇÃO DO BACKEND (FASTAPI / UVICORN NA PORTA 8000)
# ------------------------------------------------------------------------------
echo -e "\n[1/2] Iniciando Backend FastAPI (Porta 8000)..."
"$VENV_DIR/bin/uvicorn" app.main:app \
    --app-dir "$SCRIPT_DIR/src/backend" \
    --host 0.0.0.0 \
    --port 8000 \
    --log-level info &
BACKEND_PID=$!

# Aguarda inicialização do backend
sleep 1.5

# ------------------------------------------------------------------------------
# 6. INICIALIZAÇÃO DO FRONTEND (VITE REACT NA PORTA 5173)
# ------------------------------------------------------------------------------
echo -e "\n[2/2] Iniciando Frontend React (Vite)..."
(cd "$SCRIPT_DIR/src/frontend" && npx vite --host 0.0.0.0 --port 5173) &
FRONTEND_PID=$!

echo ""
echo "======================================================================"
echo "  SISTEMA OPERACIONAL EM EXECUÇÃO:"
echo "  - Painel Web (Frontend) : http://localhost:5173"
echo "  - Servidor API (Backend): http://localhost:8000"
if [ "$TELEMETRIA_FONTE" = "serial" ]; then
echo "  - Telemetria Gateway   : $SERIAL_PORTA (115200 bps)"
echo "  - Geometria do Robô    : Determinada dinamicamente pelo Robô ESP32"
else
echo "  - Telemetria Gateway   : Simulador Virtual ($SIMULADOR_LABIRINTO)"
fi
echo "======================================================================"
echo "  Pressione [Ctrl + C] para encerrar a aplicação a qualquer momento."
echo "======================================================================"

# Mantém o script ativo aguardando os processos
wait "$BACKEND_PID" "$FRONTEND_PID"
