#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Compilando e executando testes unitarios de hw/firm..."
g++ -std=gnu++17 -Wall -Wextra -Werror \
    -I../include \
    -I../../../src/firmware/lib/telemetria/src \
    ../src/tb6612_driver.cpp \
    ../src/encoder_driver.cpp \
    ../src/vl53l0x_manager.cpp \
    ../src/battery_monitor.cpp \
    ../src/radio_telemetria.cpp \
    test_firmware.cpp -o test_runner

./test_runner
rm -f test_runner
