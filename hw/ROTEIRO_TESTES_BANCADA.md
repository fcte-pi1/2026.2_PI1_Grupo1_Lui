# Roteiro de Testes de Bancada e Comissionamento — Micromouse (Grupo 1)

Este roteiro consolida **todos os ensaios obrigatórios de bancada e pista** antes da primeira corrida autônoma no labirinto. Ele complementa o desenho de integração ([`docs/firmware/integracao-loop.md`](../docs/firmware/integracao-loop.md)) e foca nas calibrações reais do **hardware oficial (`hw/firm`)** com as **rodas definitivas** e os **3 sensores VL53L0X (sem IMU)**.

> **Onde aplicar os valores calibrados:** todas as constantes físicas ficam centralizadas em [`src/firmware/robo/src/hardware/Pinos.h`](../src/firmware/robo/src/hardware/Pinos.h) (e espelhadas em [`hw/firm/include/pinout.h`](firm/include/pinout.h)).

---

## Pré-requisitos na bancada

- [ ] Bateria LiPo 2S carregada ($\ge 7,4\text{ V}$; ideal $8,0\text{ a }8,4\text{ V}$).
- [ ] Multímetro digital, régua milimetrada ou paquímetro, trena e gabarito/anteparo branco de MDF (ou papel branco rígido) para os testes dos sensores ToF.
- [ ] Apoio (bloco/suporte) para manter as **duas rodas suspensas no ar** durante as Fases 1, 2 e 3.
- [ ] Cabo USB conectado ao ESP32 com Monitor Serial a **`115200 bps`**.
- [ ] Gateway ESP8266 ligado no PC (para validar telemetria e parada remota **`s`**).

---

## Fase 0 — Conferência Mecânica e Medidas Reais (Rodas Definitivas)

Como a roda vista nas fotos iniciais era provisória, **meça o carrinho com as rodas definitivas montadas** antes de calibrar qualquer movimento:

| # | Medição física (régua/paquímetro) | Constante em `Pinos.h` | Valor de referência (CAD + estimativa) | Valor medido |
|---:|---|---|---|---:|
| **0.1** | **Diâmetro externo da roda com pneu/O-ring** (no chão, levemente pressionada) | `RODA_DIAMETRO_MM` | `22,0` (projeto final) ou `26,0 mm` (pneu grosso) | `_____ mm` |
| **0.2** | **Bitola (centro-a-centro dos pneus)** | `BITOLA_MM` | `~105 a 112 mm` (chassi PETG = `82,4 mm`, rodas por fora) | `_____ mm` |
| **0.3** | **Largura total externa** (ponto mais externo de uma roda até a outra) | Envelope $\le 165\text{ mm}$ | $\le 125\text{ mm}$ (folga lateral $\ge 21,5\text{ mm}$/lado no corredor de `168 mm`) | `_____ mm` |
| **0.4** | **Distância do eixo das rodas até a face do ToF frontal** ($X_{\text{tof}}$) | `DIST_ENCOSTO_FRONTAL_MM` | CAD = `65,6 mm` $\rightarrow$ no centro da célula: $84 - X_{\text{tof}}$ (usar `+5 mm` de margem: `~24 mm`) | `_____ mm` |
| **0.5** | **Distância da face de cada ToF lateral até a parede** (robô centrado em corredor de `168 mm`) | `DIST_IDEAL_PAREDE_LATERAL_MM` | CAD = `43,0 mm` ($84 - 40,8\text{ mm}$) | `_____ mm` |
| **0.6** | **Posição da largada em A1** | `COMPENSACAO_LARGADA_MM` | `0,0 mm` (eixo centrado), `10,5 mm` (chassi centrado) ou `39,0 mm` (traseira encostada na parede de trás) | `_____ mm` |

**Checklist mecânico de giro no próprio eixo:**
- [ ] **Cantos frontais da placa perfurada superior:** confirmar se nenhum canto vivo ultrapassa `R = 76 mm` do centro do eixo (se passar, chanfrar/arredondar os 2 cantos dianteiros da placa).
- [ ] **Roda boba (caster):** girar a esfera com o dedo em 360° — não pode travar nem arrastar lateralmente.
- [ ] **Folga no eixo D:** segurar o eixo do motor e tentar girar a roda com a mão — zero folga angular (sem escorregamento).
- [ ] **Vão livre do solo (*ground clearance*):** $\ge 3,0\text{ mm}$ sob a base inferior.

---

## Fase 1 — Elétrica, Bateria e Errata do ESP32 (Rodas no Ar)

1. **Tensão da bateria (Multímetro vs. ESP32):**
   - Medir nos terminais da LiPo 2S com multímetro ($V_{\text{real}}$).
   - No firmware integrado (`src/firmware/robo`), enviar **`d`** pela serial (ou usar o teste 9 do menu `hw/firm`) e anotar $V_{\text{lida}}$.
   - **Critério de aprovação:** $|V_{\text{real}} - V_{\text{lida}}| \le 0,15\text{ V}$.
   - *Se divergir muito:* conferir se o divisor montado é o módulo comercial **5:1 (`30k / 7,5k`)** (`BATERIA_R_SERIE_OHM = 30000`, `BATERIA_R_GND_OHM = 7500`) ou resistores discretos **`100k / 47k`** (`3,13:1`).

2. **Conferência da errata ADC1 × GPIO 36/39 (pulsos fantasma):**
   - Deixar o robô **totalmente parado** em `AGUARDANDO` com telemetria ativa (1 Hz) por **60 segundos**.
   - Enviar **`d`** no início e após 60 s e comparar `encE` e `encD`.
   - **Critério de aprovação:** `encE` e `encD` **não podem variar nem 1 pulso** com o robô parado.

3. **Interface física e telemetria:**
   - [ ] Ao ligar: LED de estado (GPIO 14) indica `AGUARDANDO` e painel web recebe pacotes a `1 Hz`.
   - [ ] Toque triplo no botão (GPIO 15) alterna `4×4 → 8×4 → 12×4` com 1, 2 e 3 piscadas.
   - [ ] Segurar o botão por 2 s arma a largada; soltar inicia `MAPEANDO`.
   - [ ] Enviar **`s`** (ou barra de espaço) pela Serial ou via [`scripts/controle_remoto.py`](../scripts/controle_remoto.py) (ESP-NOW) corta os motores imediatamente (`TRAVADO`).

---

## Fase 2 — Calibração Individual dos Sensores (Rodas no Ar)

### 2.1 Pulsos por volta e sentido dos encoders (`ENCODER_PULSOS_POR_VOLTA` e `ENCODER_*_INVERTIDO`)
> **Faça isto ANTES de qualquer teste de movimento.**

1. Fazer uma marca de caneta no pneu e no chassi.
2. Com o firmware rodando (`d` na serial ou teste 3 do `hw/firm`), girar a **roda esquerda exatamente 1 volta completa no sentido de avanço** com a mão. Repetir 3 vezes e anotar $\Delta\text{encE}$.
3. Repetir para a **roda direita** e anotar $\Delta\text{encD}$.

| Roda | Volta 1 | Volta 2 | Volta 3 | Média | Sinal no avanço |
|---|---:|---:|---:|---:|:---:|
| **Esquerda** | | | | | `+` / `-` |
| **Direita** | | | | | `+` / `-` |

- **Ajustes em `Pinos.h`:**
  - Atualizar `ENCODER_PULSOS_POR_VOLTA` com a média real (esperado: `360`; alguns N20 de 7 PPR em borda simples dão `210`, e em quadratura dão `840`).
  - Se alguma roda contou **negativo** ao avançar, trocar `ENCODER_ESQ_INVERTIDO` ou `ENCODER_DIR_INVERTIDO` para `true`.

### 2.2 Offsets dos 3 sensores VL53L0X (`TOF_OFFSET_*_MM`)
> **CRÍTICO:** No ensaio registrado em [`docs/7.3 - Testes de hardware.md`](../docs/7.3%20-%20Testes%20de%20hardware.md), os 3 módulos apresentaram vieses brutos de **`+4 mm` (S1), `+34 mm` (S2 frontal!) e `+16 mm` (S3)** a `60 mm`. Sem descontar o offset, o robô pode bater na parede frontal ou andar desalinhado!

1. Posicionar um anteparo branco plano exatamente a **`60 mm`** da face óptica de cada sensor (um de cada vez), digitar **`d`** 5 vezes e anotar a média bruta (`L`, `F`, `R`).
2. Repetir a **`100 mm`** e sem anteparo (campo livre $\ge 500\text{ mm}$).

| Sensor (XSHUT / End.) | Leitura a 60 mm | Leitura a 100 mm | Campo livre | Offset = $\text{média}(L_{60}-60,\ L_{100}-100)$ |
|---|---:|---:|---:|---:|
| **Esquerdo** (GPIO 4 / `0x30`) | | | (`8190` ou `>500`) | `TOF_OFFSET_ESQ_MM = ____` |
| **Frontal** (GPIO 16 / `0x31`) | | | (`8190` ou `>500`) | `TOF_OFFSET_FRONTAL_MM = ____` |
| **Direito** (GPIO 17 / `0x32`) | | | (`8190` ou `>500`) | `TOF_OFFSET_DIR_MM = ____` |

- **Critérios de aprovação:**
  - Após preencher `TOF_OFFSET_*_MM` e regravar, o erro em `40`, `60` e `100 mm` deve ficar dentro de **$\pm 3\text{ mm}$**.
  - Em campo livre, a leitura deve ir para **`8190`** (ou `> 200 mm`) sem travar em valor antigo.
  - Em uma célula real (robô centrado): parede presente deve ler `< 105 mm` e corredor aberto `> 115 mm` (`LIMIAR_PAREDE_MM = 110`, `HISTERESE_PAREDE_MM = 5`).

---

## Fase 3 — Motores, Zona Morta e Segurança (Rodas no Ar)

1. **Sentido dos motores (`MOTOR_*_INVERTIDO`):**
   - Comandar avanço suave (teste 2 do `hw/firm` ou comando `w` no controle remoto).
   - **Critério:** ambas as rodas giram para a **frente** e ambas as contagens de encoder **sobem**.
   - Se alguma girar para trás, inverter `MOTOR_ESQ_INVERTIDO` ou `MOTOR_DIR_INVERTIDO` em `Pinos.h`.

2. **Zona morta e velocidade base (`PWM_MINIMO`, `PWM_BASE_MOVIMENTO`, `PWM_POR_MM_S`):**
   - Subir o PWM de 5 em 5 a partir de `20` (com o robô **no chão**, pois no ar o atrito é menor!) e anotar o menor PWM em que **ambas as rodas arrancam consistentemente**: esse valor é o `PWM_MINIMO` (atual: `40`).
   - Medir a velocidade em mm/s no `PWM_BASE_MOVIMENTO` (`90`): se ficar muito abaixo ou acima de `150 mm/s`, ajustar `PWM_BASE_MOVIMENTO` (mantendo sempre `PWM_MAXIMO_MOVIMENTO >= PWM_BASE_MOVIMENTO + 35` para dar folga ao controle lateral e ao anti-travamento!).

3. **Corte de travamento (HU-18):**
   - Com as rodas no ar em `MAPEANDO`, segurar **apenas uma roda** com o dedo (testar primeiro a esquerda, depois a direita).
   - **Critério de aprovação:** tração cortada em **$\le 1,2\text{ s}$**, LED entra em `TRAVADO` e painel mostra estado `TRAVADO`. (O firmware atualizado com o PR #202 usa `min(dEsq, dDir)`, impedindo que a outra roda conclua a célula sozinha.)

4. **Monitoramento de desempenho do laço:**
   - Conferir a linha `[tick]` no Monitor Serial: tempo máximo de tick bem abaixo de `10 000 µs` (ideal `< 3 000 µs`).

---

## Fase 4 — Calibração Dinâmica no Chão / Pista

> **Regra de ouro:** calibre nesta ordem exata — **1) Reta/odometria → 2) Encosto frontal → 3) Curvas 90°/180° → 4) Rumo estimado pelas paredes → 5) Pista completa.**

### 4.1 Odometria em linha reta (`ODOM_ESCALA_RETO`)
1. Marcar o ponto inicial no piso real da arena (ou mesa semelhante) e comandar um avanço reto de **4 células (`720 mm`)** em corredor aberto (ou sem parede frontal).
2. Medir com trena a distância real percorrida $D_{\text{real}}$ (mm).
3. Calcular:
   $$\text{ODOM\_ESCALA\_RETO}_{\text{novo}} = \text{ODOM\_ESCALA\_RETO}_{\text{atual}} \times \frac{720}{D_{\text{real}}}$$
4. **Critério de aprovação:** erro acumulado em 4 células (`720 mm`) **$\le \pm 8\text{ mm}$** e desvio lateral com duas paredes **$< 15\text{ mm}$** (CT-01).

### 4.2 Encosto frontal e parada segura (`DIST_ENCOSTO_FRONTAL_MM`, `DIST_MINIMA_SEGURA_MM`)
1. Colocar o robô **parado no centro geométrico da célula** com parede à frente e ler `F` pelo comando **`d`**.
2. Definir `DIST_ENCOSTO_FRONTAL_MM` como $F_{\text{centro}} + 5\text{ mm}$ (referência CAD: `18,4 + 5,6 = 24 mm`) e `DIST_MINIMA_SEGURA_MM` como $F_{\text{centro}} - 3\text{ mm}$ (mínimo `15 mm`, sempre `< DIST_ENCOSTO_FRONTAL_MM`).
3. Mandar avançar 1 célula contra uma parede frontal: o robô deve desacelerar nos últimos `35 mm` e parar suavemente sem encostar, deixando espaço livre para girar 90° e 180°.

### 4.3 Curvas de 90° e meia-volta de 180° (`BITOLA_MM` e `CURVA_ANTECIPACAO_FREIO_GRAUS`)
1. Marcar uma linha de referência no chão (ou usar fita crepe em cruz 90°).
2. Comandar **4 giros seguidos de 90° à esquerda** (volta completa de 360°) e depois **4 giros à direita**:
   - Se o robô girar **menos** que 360° (ex.: 340°), **aumentar `BITOLA_MM`** proporcionalmente:
     $$\text{BITOLA\_MM}_{\text{nova}} = \text{BITOLA\_MM}_{\text{atual}} \times \frac{360}{\theta_{\text{medido}}}$$
   - Se girar **mais** que 360° (ex.: 380°), **diminuir `BITOLA_MM`** pela mesma fórmula (ou aumentar `CURVA_ANTECIPACAO_FREIO_GRAUS` de `0.5f` para `1.0f–2.0f` se o excesso for apenas deslizamento na frenagem).
3. Testar **meia-volta (180°) dentro de um beco sem saída real (3 paredes)**:
   - **Critério de aprovação:** erro angular médio $\le \pm 3^\circ$ por curva (RF-SFT01 / CT-02) e **nenhuma parte do chassi ou da placa superior raspa nas paredes**.

### 4.4 Validação do rumo estimado pelas paredes (PR #202 — "IMU Virtual")
1. Colocar o robô **propositalmente torto em ~`+5°`** na entrada de um corredor com 2 paredes laterais.
2. Mandar avançar 1 célula e em seguida fazer uma curva de 90° (ou avançar para uma célula sem paredes):
   - **Critério de aprovação:** após a curva o robô sai alinhado à grade ($\le 3^\circ$), e no trecho sem paredes o desvio lateral fica $< 10\text{ mm}$.
3. Repetir com **`-5°`** para o outro lado.
   - *Se a correção piorar o ângulo (sinal invertido):* conferir `ENCODER_*_INVERTIDO` e verificar se `TOF_XSHUT_ESQ` (GPIO 4) está fisicamente à esquerda e `TOF_XSHUT_DIR` (GPIO 17) à direita!

---

## Fase 5 — Ensaio Completo em Pista `4×4` (Checklist Go / No-Go)

### Posicionamento obrigatório na largada (chegada na diagonal oposta)

O firmware não usa sensor de cor no piso: ele navega por coordenadas de grade ([`Mapa.h`](../src/firmware/robo/src/mapa/Mapa.h), [`Navegador.h`](../src/firmware/robo/src/navegacao/Navegador.h) e [`ParadaObjetivo.h`](../src/firmware/robo/src/parada/ParadaObjetivo.h)), largando em **`A1 = (0, 0)`** virado para **`DIR_LESTE`** e parando automaticamente na diagonal oposta (**`D4`** na `4×4`, **`D8`** na `8×4`, **`D12`** na `12×4`).

```text
  PAREDE EXTERNA (sempre à ESQUERDA do robô na largada)
 +-----------------------------------+
 | [ROBÔ ->] (0,0) A1  ...  (0,3) D1 |  <- 4 células à FRENTE do robô
 |                                   |
 |   interior do labirinto fica      |  <- 4, 8 ou 12 células à DIREITA
 |   à FRENTE e à DIREITA do robô    |
 |                                   |
 | (N-1,0)           (N-1,3) CHEGADA |  <- diagonal oposta (parada automática)
 +-----------------------------------+
```

**Regra prática ao colocar o carrinho na pista:**
- [ ] **Paredes externas** na célula inicial devem ficar **ATRÁS** e **À ESQUERDA** do robô.
- [ ] **Interior do labirinto** deve ficar **À FRENTE** (lado de **4 células**) e **À DIREITA** (lado de **4, 8 ou 12 células**).
- [ ] *Exemplo (olhando de cima com largada no canto inferior esquerdo e chegada no superior direito):* posicionar o robô **apontando para CIMA** (parede externa atrás e à esquerda; nunca apontando para a direita com o labirinto aberto à esquerda, senão o mapa descarta passagens à esquerda como fora do perímetro!).

### Critérios Go / No-Go na pista `4×4`

Antes de liberar para a avaliação oficial:
- [ ] Rodar **3 tentativas completas consecutivas** em pista `4×4` com o painel web ao vivo conectado via Gateway ESP8266.
- [ ] Confirmar que o mapa montado no painel bate **100%** com as paredes reais da pista (sem paredes fantasma em células já percorridas).
- [ ] Confirmar **F5 no navegador** durante a corrida: o mapa reconstrói imediatamente do histórico do backend.
- [ ] Conferir parada automática no objetivo (`CONCLUIDO`, LED roxo/padrão final, cronômetro congelado) e retomada limpa ao segurar o botão por 2 s (`AGUARDANDO`).
- [ ] Anotar abaixo os valores finais validados antes de fechar o PR da equipe:

```cpp
// Registro da calibração validada em bancada (copiar para src/firmware/robo/src/hardware/Pinos.h)
// Data: ___/___/2026 | Responsáveis: ___________________________
// RODA_DIAMETRO_MM             = ______f;
// BITOLA_MM                    = ______f;
// ENCODER_PULSOS_POR_VOLTA     = ______;
// ODOM_ESCALA_RETO             = ______f;
// COMPENSACAO_LARGADA_MM       = ______f;
// TOF_OFFSET_ESQ_MM            = ______;
// TOF_OFFSET_FRONTAL_MM        = ______;
// TOF_OFFSET_DIR_MM            = ______;
// DIST_IDEAL_PAREDE_LATERAL_MM = ______f;
// DIST_ENCOSTO_FRONTAL_MM      = ______;
// DIST_MINIMA_SEGURA_MM        = ______;
// CURVA_ANTECIPACAO_FREIO_GRAUS= ______f;
```
