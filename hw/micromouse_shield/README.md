# Projeto KiCad — PCB Shield do Micromouse (Grupo 1)

Este diretório contém os arquivos de projeto da placa de circuito impresso (**PCB Shield customizada**) do robô autônomo Micromouse para a disciplina **PI1 (2026.2, FCTE/UnB, Prof. Lui)**.

O projeto é 100% compatível com as versões **KiCad v7, v8, v9 e v10** com biblioteca embutida (*Self-Contained*), permitindo que qualquer integrante da equipe (inclusive de Software) abra e visualize o esquemático em qualquer computador sem precisar instalar bibliotecas externas. Além disso, um arquivo PDF já renderizado está disponível no próprio diretório para visualização imediata.

---

## 1. Estrutura dos Arquivos

```
hw/micromouse_shield/
├── micromouse_shield.kicad_pro        # Arquivo de configuração e metadados do projeto KiCad
├── micromouse_shield.kicad_sch        # Esquemático elétrico oficial sincronizado (Rev 2.0)
├── micromouse_shield_VERSION2.kicad_sch # Arquivo de esquemático Rev 2.0 (com proteções e Buck)
├── micromouse_shield.kicad_pcb        # PCB populada com todos os footprints oficiais, posições e furos M3
├── micromouse_shield.pdf              # PDF em alta resolução do esquemático para consulta rápida
├── micromouse_shield_sch.png          # Imagem PNG em alta resolução (300 DPI) do esquemático Rev 2.0
├── micromouse_shield_3d.png           # Renderização 3D oficial da placa Shield montada
├── micromouse_shield.net              # Netlist elétrica para verificação e importação
├── diagrama_blocos_hardware.png       # Imagem em alta resolução do Diagrama de Blocos do robô
├── diagrama_blocos_hardware.tex       # Código-fonte TikZ (LaTeX Standalone) do Diagrama de Blocos
├── diagrama_blocos_hardware.mmd       # Código-fonte Mermaid do Diagrama de Blocos
├── generate_schematic.py              # Script Python reprodutível gerador do esquemático
├── generate_pcb.py                    # Script Python reprodutível gerador da PCB via pcbnew nativo
└── README.md                          # Guia de montagem e instruções de layout
```

---

## 2. Como Abrir e Visualizar no KiCad

1. No terminal do CachyOS/Linux (ou no menu de aplicativos), execute:
   ```bash
   kicad hw/micromouse_shield/micromouse_shield.kicad_pro
   ```
2. Na janela principal do KiCad:
   - Dê duplo clique em **`micromouse_shield.kicad_sch`** (ou abra `micromouse_shield_VERSION2.kicad_sch`) para abrir o editor esquemático (**Eeschema**);
   - Ou dê duplo clique em **`micromouse_shield.kicad_pcb`** para abrir o editor de PCB (**Pcbnew**);
3. Dica rápida: Se quiser apenas consultar as conexões sem abrir o KiCad, abra diretamente o arquivo [`micromouse_shield.pdf`](file:///home/eduardolm/Documents/2026.2_PI1_Grupo1_Lui/hw/micromouse_shield/micromouse_shield.pdf) ou visualize [`micromouse_shield_sch.png`](file:///home/eduardolm/Documents/2026.2_PI1_Grupo1_Lui/hw/micromouse_shield/micromouse_shield_sch.png).

---

## 3. Arquitetura do Esquemático e Mapeamento de Redes (*Nets*) (Rev 2.0)

O circuito adota a topologia **100% Plug & Play**, utilizando apenas barras de pinos fêmea (soquetes de 2,54 mm) para receber módulos prontos e conectores com trava:

| Bloco Funcional | Referência | Componente / Módulo | Conexões e Rótulos de Redes (*Nets*) |
|---|:---:|---|---|
| **Processamento Central** | `J_ESP_L` e `J_ESP_R` | Barras Fêmea 1x15 para ESP32 DevKit V1 | Todos os 30 pinos mapeados, com GPIO 12 isolado contra travamento de boot. |
| **Driver de Motores** | `J_DRV_CTRL` e `J_DRV_PWR` | Barras Fêmea 1x08 para módulo TB6612FNG | PWM (`PWMA`, `PWMB`), Direção (`AIN1/2`, `BIN1/2`), saídas dos motores e alimentação $V_{MOT}$. |
| **IMU Inercial** | `U_IMU` | Barra Fêmea 1x08 para GY-521 (MPU6050) | $I^2C$ (`I2C_SDA` no GPIO 21, `I2C_SCL` no GPIO 22) e alimentação de 3,3 V. |
| **Sensores de Distância** | `J_TOF_F`, `J_TOF_L`, `J_TOF_R` | Barras Macho 1x05 para 3x VL53L0X | $I^2C$ compartilhado + pinos de corte dinâmico `XSHUT_F` (GPIO 32), `XSHUT_L` (GPIO 13), `XSHUT_R` (GPIO 14). |
| **Atuadores e Encoders** | `J_MOT_L` e `J_MOT_R` | Conectores JST 1x06 para motores N20 | Alimentação de motor e sinais de quadratura alimentados estritamente em **3,3 V** (`ENC_L_A/B` e `ENC_R_A/B`). |
| **Entrada de Bateria** | `TB_PWR1` | Borne/Conector Polarizado 2 vias | 1: $V_{BAT}+$ (7,4 V), 2: $GND$. |
| **Proteção por Fusível** | `F1` | Fusível Rearmável 3A | Em série com o polo positivo da bateria. |
| **Chave Geral Mecânica** | `SW1` | Chave Mecânica Liga/Desliga | Interrupção física do barramento de alimentação. |
| **Proteção Polaridade**  | `Q1` e `R_G1` | MOSFET Canal P (AO3401A) + 10 k$\Omega$ | Proteção ativa contra inversão de polaridade com perda desprezível. |
| **Conversor Buck (5V)**  | `J_BUCK1` | Barra Fêmea 1x04 para Módulo Step-Down | 1: VIN+, 2: VIN-, 3: VOUT+ (5V), 4: VOUT- (GND). |
| **Proteção Anti-Brownout**| `C_BULK1` e `C_OUT1` | Capacitores Eletrolíticos 470 µF e 100 µF | $C_{BULK1}$ colado aos pinos de potência do TB6612FNG; $C_{OUT1}$ na linha 5V. |
| **Divisor de Bateria**    | `R_DIV3`, `R_DIV4`, `C_DIV1` | Divisor Resistivo com Capacitor de Filtro | Saída atenuada ligada ao pino analógico `ADC_BAT_SENSE` (GPIO 34). |
| **IHM / Sinalização**     | `J_BUZ`, `J_RGB`, `SW_START` | Módulos KY-012, WS2812B e Botão THT | `BUZZER_SIG` (GPIO 4), `LED_RGB_DIN` (GPIO 2), `START_BTN` (GPIO 15 direto ao GND via `INPUT_PULLUP`). |

---

## 4. Passos para Gerar o Layout da Placa (PCB)

Para transformar o esquemático na placa física de circuito impresso:

1. No editor esquemático (**Eeschema**), aperte a tecla **`F8`** (ou clique em *Tools $\rightarrow$ Update PCB from Schematic*);
2. Clique em **Update PCB** para carregar todas as pegadas (*footprints*) no editor de PCB (**Pcbnew**);
3. **Disposição Recomendada dos Componentes no Chassi:**
   - **Bico Dianteiro:** Conector `J_TOF_F` (0°) centralizado; conectores `J_TOF_L` e `J_TOF_R` nas laterais apontando para 90°;
   - **Centro:** Soquetes `J_ESP_L` e `J_ESP_R` (com a ponta da antena voltada para a borda externa desobstruída);
   - **Laterais Traseiras:** Conectores `J_MOT_L` e `J_MOT_R` próximos aos motores N20;
   - **Traseira:** Borne de alimentação `TB_PWR` com fácil acesso para o chicote da bateria;
   - **Topo:** Módulo LED RGB, Buzzer e Botão de Largada.
4. **Regras de Roteamento:**
   - Trilhas de potência ($V_{MOT}$, saídas de motor e $GND$): largura mínima de **$1{,}2\,\text{mm}$**;
   - Trilhas de sinais lógicos e $I^2C$: largura de **$0{,}3\,\text{mm}$**;
   - Preenchimento de cobre inferior e superior com plano de terra unificado (**GND**).

---

## 5. Como Exportar Arquivos Gerber para Fabricação

Quando o layout das trilhas estiver concluído:
1. No **Pcbnew**, clique em **File $\rightarrow$ Fabrication Outputs $\rightarrow$ Gerbers (.gbr)**;
2. Selecione as camadas: `F.Cu`, `B.Cu`, `F.Mask`, `B.Mask`, `F.Silkscreen`, `B.Silkscreen`, `Edge.Cuts`;
3. Clique em **Generate Drill File** para gerar os furos (.drl);
4. Compacte todos os arquivos gerados em um arquivo `.zip` e envie para o fabricante (ex.: JLCPCB, PCBWay ou usinagem da UnB).

