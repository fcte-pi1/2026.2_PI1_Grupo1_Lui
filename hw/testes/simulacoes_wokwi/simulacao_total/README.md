# Wokwi: Simulação Total do Micromouse ESP32

Este módulo do Wokwi é a emulação completa do robô autônomo Micromouse navegando 100% às cegas no labirinto.

## Características de Operação
* **Zero Conhecimento Prévio:** Robô entra na arena sem saber o mapa de paredes e sem saber onde fica o gol (a meta pode estar em qualquer célula do labirinto).
* **Navegação Inteligente:** Busca em Profundidade com Retrocesso (Trémaux) e preferência por avanço em linha reta.
* **Chassi Diferencial com Controle PD:** Malha fechada de centralização nos corredores via sensores laterais.
* **Display OLED SSD1306:** Minimapa com Fog-of-War (revela apenas as células descobertas) e modo alternável com desenho vetorial do chassi (duas rodas motrizes e roda boba/caster).
* **Speed Run:** Após encontrar o objetivo, o robô calcula a rota limpa direta e permite disparar o Speed Run em alta velocidade.

## Menu de Comandos no Monitor Serial (115200 bps)
Digite os comandos abaixo diretamente no terminal serial do Wokwi:

### Comandos de Navegação e Algoritmo
* `START`: Inicia a exploração autônoma às cegas (ou pausa/retoma a corrida).
* `STEP`: Executa um único passo de exploração (modo passo a passo para depuração).
* `SPEED`: Dispara o Speed Run pela rota direta descoberta.
* `RESET`: Reinicia a posição do robô e limpa toda a memória do labirinto.
* `MAP`: Imprime o mapa ASCII atualizado em tempo real no terminal.
* `STATUS`: Mostra posição `(x, y)`, orientação, passos e tensão da bateria.
* `DEMO`: Executa rotina demonstrativa com manobras, giro 360° e avanço.

### Suíte de Testes Manuais de Motores
* `F` ou `FRENTE`: Avança 1 célula (18 cm) com centralização PD ativa.
* `T` ou `TRAS`: Recua 1 célula.
* `L` ou `ESQ`: Giro pivotante de 90° à esquerda no próprio eixo.
* `R` ou `DIR`: Giro pivotante de 90° à direita no próprio eixo.
* `U` ou `VOLTA`: Meia volta rápida (giro de 180°).
* `PWM <60..255>`: Ajusta a potência base dos motores (ex: `PWM 190`).

### Configurações
* `MAZE <0..3>`: Troca a arena do labirinto (0: MAZE01 16x16, 1: Bench 4x4, 2: Bench 8x4, 3: Bench 12x4).
* `VIEW`: Alterna entre a visualização do Minimapa e o desenho do Chassi no OLED.
* `AJUDA`: Reexibe o menu de comandos.

## Botões Físicos no Wokwi
* **Botão Verde [1]:** START / PAUSA / Disparo de Speed Run.
* **Botão Azul [2]:** Alterna visualização no OLED (Mapa Fog-of-War vs Chassi).
* **Botão Amarelo [3]:** Executa 1 passo manual de exploração (STEP).
* **Botão Laranja [4]:** Alterna entre os 4 labirintos disponíveis.
