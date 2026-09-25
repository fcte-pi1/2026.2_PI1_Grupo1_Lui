# MER 
## Este documento descreve quais são as entidades presentes no banco de dados juntamente com seus atributos
## 1. Entidades e seus atributos:
### 1.1 - Corrida
Representa as informações da corrida que serão armazenadas
```tetx
  idCorrida (Identificador Único)
  inicio (horario de inicio da corrida)
  fim (horario de finalização da corrida)
  tempo_total_ms (tempo total da corrida)
  labirinto (4x4, 4x8, 4x12)
  Velocidade_media 
  bateria_inicial_pct (bateria inicial em %)
  bateria_final_pct (bateria final em %)
  status_final (Concluído, Colisão, Timeout)
  desafio_cumprido (Cumpriu / Não Cumpriu)
  tentativa (Número sequencial da tentativa)
  modo (Exploração ou Speed Run)
  celulas_visitadas (quantidade de celulas visitadas)
```
### 1.2 - Celula
Representa a posição do robo a cada segundo
```tetx
  idCelula (Identificador Único)
  Corrida_idCorrida (associação a corrida)
  coluna ('A', 'B', 'C', 'D') 
  linha (1 a 12)
  parede_norte (Detectada / Não detectada)
  Parede_sul (Detectada / Não detectada)
  parede_leste (Detectada / Não detectada)
  parede_oeste (Detectada / Não detectada)
  visitada_em_ms (Norte, Sul, Leste, Oeste) 
```
### 1.3 - Leitura
Representa o pacote enviado pelo robo
 ```text
  idLeitura (Identificador Único)
  Corrida_idCorrida (associação a corrida)
  timestamp_ms (Tempo decorrido no momento da leitura)
  Tamanho_Mapa (4x4, 4x8, 4x12)
  bateria_pct (bateria em %)
  bateria_volts 
  velocidade_atual
  velocidade_media 
  coluna ('A', 'B', 'C', 'D')
  linha (1 a 12)
  orientacao (Norte, Sul, Leste, Oeste)
  status (Aguardando, Mapeando, Speed Run, Concluido, Colisão, Timeout) 
```
