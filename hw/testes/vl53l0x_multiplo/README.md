# Teste de 3 Sensores ToF VL53L0X (Esquerdo, Frontal e Direito)

Este diretório contém o firmware para validação de múltiplos sensores de distância VL53L0X compartilhando o mesmo barramento I2C, utilizando chaveamento dinâmico via pinos digitais XSHUT para atribuição sequencial de novos endereços I2C.

---

## 1. Conexões de Hardware
* **Barramento I2C (Compartilhado):** SDA $\rightarrow$ GPIO 21 | SCL $\rightarrow$ GPIO 22
* **XSHUT Sensor Frontal (0°):** GPIO 32 $\rightarrow$ Endereço reatribuído: `0x30`
* **XSHUT Sensor Esquerdo (90°):** GPIO 13 $\rightarrow$ Endereço reatribuído: `0x31`
* **XSHUT Sensor Direito (90°):** GPIO 14 $\rightarrow$ Endereço reatribuído: `0x32`

---

## 2. Arquivos
* [`TESTE_3_SENSORES_VL53L0X.ino`](TESTE_3_SENSORES_VL53L0X.ino):
  * Inicialização sequencial colocando todos os sensores em desligamento (XSHUT em nível lógico baixo);
  * Habilitação individual de cada sensor seguida de mudança imediata do endereço padrão `0x29` via software;
  * Leitura contínua em laço fechado para detecção de paredes frontais e centralização entre paredes laterais nos corredores da arena.
