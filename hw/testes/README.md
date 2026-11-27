# Referências de Hardware e Simulador MMS

Este diretório mantém a documentação de referência de pinagem e o simulador de labirinto **MMS**.

> [!NOTE]
> Os firmwares de bancada, diagnóstico ToF e testes unitários de firmware ficam centralizados em [`src/firmware/`](../../src/firmware/).

## Conteúdo

| Item | Caminho | Descrição |
|:---|:---|:---|
| **Tabela de Pinagem Oficial** | [`TABELA_PINAGEM_PORTAS_HARDWARE.md`](TABELA_PINAGEM_PORTAS_HARDWARE.md) | Mapeamento oficial dos 30 pinos do ESP32 DevKit V1 (espelhando `PinosRobo.h`). |
| **Simulador de Labirinto MMS** | [`mms/`](mms/) | Algoritmo de exploração (`DEFAULT.py`), interface `API.py` e labirinto de referência (`MAZE01.txt`). |
