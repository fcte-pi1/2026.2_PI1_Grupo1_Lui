#ifndef TELEMETRIA_CONFIG_H
#define TELEMETRIA_CONFIG_H

// Parâmetros de rádio e temporização compartilhados por robô e gateway.
// Ambos DEVEM usar o mesmo canal Wi-Fi para o ESP-NOW funcionar.
#define TELEMETRIA_CANAL_WIFI   1       // Canal 2,4 GHz usado pelo ESP-NOW
#define TELEMETRIA_PERIODO_MS   1000UL  // 1 Hz (RNF-SFT03)
#define TELEMETRIA_BAUD_SERIAL  115200  // Gateway -> backend (doc 4.4, seção 6.3)

#endif
