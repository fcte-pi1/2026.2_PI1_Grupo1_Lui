# Simulações de Hardware no Wokwi

Este diretório contém os três ambientes integrados de simulação virtual desenvolvidos para o simulador [Wokwi](https://wokwi.com), permitindo executar, debugar e validar a eletrônica e os sensores do robô Micromouse antes e em paralelo à montagem física.

---

## 1. Ambientes de Simulação Disponíveis

### A. [Bancada de Sensores e Calibração](sensores/)
* **Local:** [`hw/testes/simulacoes_wokwi/sensores/`](sensores/)
* **Objetivo:** Validação do barramento I2C, leitura de 3 sensores de distância, interface gráfica no display OLED SSD1306, telemetria analógica da bateria e acionamento de pulsos dos motores.
* **Componentes Virtuais no `diagram.json`:** ESP32 DevKit, 3 sensores de distância, OLED SSD1306, Potenciômetro (bateria), LED RGB NeoPixel, Buzzer piezoelétrico e 4 botões de comando.
* **Como rodar:** Abra a pasta no VS Code com a extensão do Wokwi ou faça upload dos arquivos em [wokwi.com](https://wokwi.com).

### B. [Simulação do Protótipo Real](prototipo_real/)
* **Local:** [`hw/testes/simulacoes_wokwi/prototipo_real/`](prototipo_real/)
* **Objetivo:** Emulação do firmware final com a camada HAL (Hardware Abstraction Layer), envio de telemetria serial a cada 200 ms e recepção de comandos de largada/parada remota (`START`, `STOP`, `PAUSE`, `RESET`).
* **Arquivos:** [`sketch.ino`](prototipo_real/sketch.ino), [`diagram.json`](prototipo_real/diagram.json), [`protocol.h`](prototipo_real/protocol.h).

### C. [Simulação Total do Micromouse](simulacao_total/)
* **Local:** [`hw/testes/simulacoes_wokwi/simulacao_total/`](simulacao_total/)
* **Objetivo:** Simulação autônoma do robô navegando em labirintos completos (16x16, 4x4, 8x4 e 12x4), com algoritmo Trémaux/DFS, centralização diferencial PD e minimapa gráfico com névoa de guerra (*fog-of-war*) no OLED.
* **Arquivos:** [`sketch.ino`](simulacao_total/sketch.ino), [`diagram.json`](simulacao_total/diagram.json).

---

## 2. Como Rodar no VS Code
1. Instale a extensão oficial do **Wokwi for VS Code**.
2. Abra qualquer um dos subdiretórios acima.
3. Abra o arquivo `diagram.json`.
4. Pressione o botão verde **Play** no editor.
