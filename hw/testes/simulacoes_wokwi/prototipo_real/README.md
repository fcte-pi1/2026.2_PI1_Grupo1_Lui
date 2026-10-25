# Firmware de Produção: Protótipo Real Micromouse (ESP-NOW)

Este diretório contém a versão final de produção do firmware para gravação na placa **ESP32 física do robô Micromouse**, integrando navegação autônoma às cegas e telemetria bidirecional via **ESP-NOW**.

---

## 1. Arquitetura de Comunicação ESP-NOW

A arquitetura do sistema é dividida em duas pontas:
1. **Micromouse (ESP32 no Robô):** Executa o loop de controle, cinemática, leitura dos sensores e navegação no labirinto. Transmite telemetria periódica e recebe comandos de controle.
2. **Gateway / Bridge (ESP32 conectada ao PC):** Recebe os pacotes ESP-NOW do robô, converte para JSON/WebSockets e permite ao operador visualizar a arena em tempo real em um Dashboard Web e emitir comandos.

```
+-------------------+                   +-------------------+                   +--------------------+
|   MICROMOUSE      |  ESP-NOW (2.4GHz) |    ESP32 GATEWAY  |   USB / Serial    |    DASHBOARD WEB   |
|   (ESP32 Real)    | <===============> |    (Bridge USB)   | <===============> |   (Monitor/Operador)
|  Navegação Cega   |  Pacotes Binários |  Conversor JSON   |    WebSockets     |   Visualização 2D  |
+-------------------+                   +-------------------+                   +--------------------+
```

---

## 2. Estrutura dos Pacotes de Dados (`protocol.h`)

Para garantir determinismo, baixíssimo consumo de banda e zero overhead de parsing no microcontrolador, os dados são transmitidos em **estruturas binárias compactas (`struct __attribute__((packed))`)**:

### A. Pacote de Comandos (Gateway -> Micromouse) - 6 bytes
```cpp
struct __attribute__((packed)) MicromouseCommand {
    uint8_t  commandId;   // Identificador da ação (1: START, 2: STOP, etc.)
    uint8_t  parameter;   // Parâmetro opcional (ex: PWM ou ID de teste)
    uint16_t seqNumber;   // Contador sequencial de mensagens
    uint16_t checksum;    // Verificação de integridade
};
```
* **Comandos Suportados:**
  * `CMD_START` (1): Inicia a exploração autônoma às cegas.
  * `CMD_STOP` (2): Parada imediata de emergência (corta PWM dos motores).
  * `CMD_PAUSE` (3): Pausa/retoma a exploração no ponto atual.
  * `CMD_RESET_MAZE` (4): Reinicia a odometria e zera a matriz de paredes na memória.
  * `CMD_RETURN_TO_START` (5): Comanda o retorno ótimo até a célula de largada `(0,0)`.
  * `CMD_SET_SPEED` (6): Ajusta dinamicamente a velocidade PWM dos motores.

### B. Pacote de Telemetria (Micromouse -> Gateway) - 24 bytes
Transmitido a cada **200 ms** e instantaneamente a cada evento de mudança de célula ou estado:
```cpp
struct __attribute__((packed)) MicromouseTelemetry {
    uint32_t timestampMs;  // Tempo decorrido de corrida (ms)
    uint8_t  posX;         // Coordenada X (0 a 15)
    uint8_t  posY;         // Coordenada Y (0 a 15)
    uint8_t  heading;      // Orientação (0: Norte, 1: Leste, 2: Sul, 3: Oeste)
    uint8_t  walls;        // Máscara de paredes da célula (N, L, S, O)
    uint8_t  state;        // Estado (0: IDLE, 1: EXPLORING, 2: BACKTRACKING, 3: GOAL_FOUND...)
    float    distL;        // Distância do sensor ToF Esquerdo (cm)
    float    distF;        // Distância do sensor ToF Frontal (cm)
    float    distR;        // Distância do sensor ToF Direito (cm)
    float    batteryVolts; // Leitura da tensão da bateria (V)
    uint16_t stepCount;    // Total de células percorridas
    uint16_t pathLength;   // Comprimento da rota limpa direta
};
```

---

## 3. Configuração do MAC Address do Gateway

Para que o robô consiga enviar os dados via ESP-NOW para a sua placa Bridge, descubra o MAC da ESP32 Gateway executando:
```cpp
#include <WiFi.h>
void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    Serial.println(WiFi.macAddress());
}
void loop() {}
```
Copie o endereço obtido e atualize a variável `GATEWAY_MAC` no topo do arquivo `sketch.ino`:
```cpp
uint8_t GATEWAY_MAC[6] = { 0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC }; // Substitua pelo MAC da sua placa
```

---

## 4. Camada de Abstração HAL e Simulação no Wokwi

Como o simulador Wokwi não possui suporte a rádio ESP-NOW entre dois projetos distintos, o firmware conta com uma **camada dupla automática**:
1. **Na Placa Física Real:** O rádio ESP-NOW opera nativamente em conjunto com o Wi-Fi em modo Station.
2. **No Simulador Wokwi ou via Cabo USB:** O robô aceita os comandos pelo Monitor Serial:
   * Digite `START` ou `CMD:START`
   * Digite `STOP` ou `CMD:STOP`
   * Digite `PAUSE` ou `CMD:PAUSE`
   * Digite `RESET` ou `CMD:RESET`
   * Digite `RETURN` ou `CMD:RETURN`
   E emite os pacotes de telemetria serializados em JSON compacto no terminal serial (`{"telemetry":{...}}`), permitindo a integração direta com servidores locais via porta serial (ex: NodeJS ou Python).

---

## 5. Como Gravar na Placa ESP32 Real

1. Abra a pasta `PROTOTIPO_REAL` na Arduino IDE ou VS Code (PlatformIO).
2. Selecione a placa: **ESP32 Dev Module**.
3. Verifique as configurações:
   * **CPU Frequency:** 240MHz (WiFi/BT)
   * **Flash Frequency:** 80MHz
   * **Upload Speed:** 921600
4. Conecte o cabo USB e clique em **Upload**.
