#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "======================================================================"
echo "  TESTES AUTOMATIZADOS - ISSUE #140: PROTOCOLO & GATEWAY JSON"
echo "======================================================================"

echo -e "\n[1/2] Compilando e executando testes unitarios em C++17..."
g++ -std=gnu++17 -Wall -Wextra -Werror -DNATIVE_BUILD -I../firmware/include test_protocol.cpp -o runner_cxx
./runner_cxx
rm -f runner_cxx

echo -e "\n[2/2] Validando decodificacao de 18 bytes e schema JSON em Python..."
python3 test_runner.py

echo -e "\n======================================================================"
echo "  TODOS OS TESTES DA ISSUE #140 PASSARAM COM SUCESSO!"
echo "======================================================================"
