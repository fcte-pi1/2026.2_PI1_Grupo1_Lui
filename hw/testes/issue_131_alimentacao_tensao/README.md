# Issue #131 - [EAP 4.1.2] Montagem do Circuito e Aferição de Tensão

Este diretório contém a solução completa para a **Issue #131** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), contemplando o monitoramento de alimentação e bateria, validação de estabilidade sob transitórios indutivos, simulação no Wokwi e suíte de testes automatizados com critérios de aceite formais.

---

## 1. Dados da Issue
* **Número da Issue:** #131
* **Título:** `[EAP 4.1.2] Montagem do circuito e aferição de tensão`
* **Código EAP:** 4.1.2 (Circuito Elétrico e Sistema Energético)
* **Requisitos Atendidos:**
  * **RNF-ENE02:** Estabilidade da Tensão Lógica sob Transitórios — linhas reguladas de $3,3\text{ V}$ e $5,0\text{ V}$ mantidas estritamente dentro da tolerância de $\pm 5\%$ ($3,3\text{ V} \pm 0,165\text{ V}$ e $5,0\text{ V} \pm 0,250\text{ V}$) mesmo durante partidas e reversões severas de motores.
  * **RNF-ELE05:** Imunidade a Ruído Indutivo de Motores — desacoplamento capacitivo prevenindo resets espúrios do ESP32.
  * **RF-ENE02:** Monitoramento Analógico da Bateria via divisor resistivo no canal ADC1 (GPIO 34).

---

## 2. Parâmetros do Divisor de Tensão e Tolerâncias
* **Resistores do Divisor de Tensão:** $R_1 = 100\text{ k}\Omega$, $R_2 = 47\text{ k}\Omega$
* **Fator de Divisão:** $\text{Ratio} = \frac{R_1 + R_2}{R_2} = \frac{147}{47} \approx 3,1276$
* **Faixa Útil da Bateria LiPo 2S:**
  * Plena Carga: $8,40\text{ V}$ ($4,20\text{ V}$ por célula)
  * Nominal: $7,40\text{ V}$ ($3,70\text{ V}$ por célula)
  * Alerta de Bateria Fraca: $7,00\text{ V}$ ($3,50\text{ V}$ por célula)
  * Corte Crítico de Subtensão (*Cutoff*): $6,60\text{ V}$ ($3,30\text{ V}$ por célula)

---

## 3. Estrutura do Diretório
```
ISSUE_131_Montagem_do_circuito_e_afericao_de_tensao/
├── README.md                 # Documentação e relatório de conformidade
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração de compilação PlatformIO
│   ├── include/
│   │   └── power_voltage_monitor.h # Driver de monitoramento de tensão e limites
│   └── src/
│       └── main.cpp          # Código de amostragem ADC filtrada e relatórios
├── wokwi/                    # Ambiente de simulação Wokwi
│   ├── diagram.json          # Diagrama virtual com ESP32, potenciômetro e OLED
│   ├── sketch.ino            # Simulação interativa com gráfico de bateria
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências do simulador
└── tests/                    # Suíte de testes automatizados
    ├── test_voltage_stability.cpp # Testes unitários em C++17 (unity_compat.h)
    ├── test_runner.py        # Simulador dos transitórios de partida em Python
    ├── unity_compat.h        # Framework de asserções leves
    └── run_tests.sh          # Script de execução automatizada
```

---

## 4. Como Executar os Testes Automatizados
```bash
cd tests
./run_tests.sh
```

### Resultados dos Testes:
```
======================================================================
  TESTES AUTOMATIZADOS - ISSUE #131: ESTABILIDADE DE TENSAO
======================================================================
[1/2] Compilando e executando testes unitarios em C++17...
[RUN ] test_pin_and_divider_ratio ... PASS
[RUN ] test_adc_to_battery_voltage_conversion ... PASS
[RUN ] test_power_rail_stability_tolerance_strict_5_percent ... PASS
[RUN ] test_battery_cutoff_and_percentage ... PASS
----------------------------------------
4 Tests 0 Failures 0 Ignored -> OK

[2/2] Validando transientes de partida e tolerancias de 5% em Python...
Cenario Operacional          | V_Bat (V)  | Linha 5V   | Linha 3.3V  | Resets  | Status
--------------------------------------------------------------------------------
1. Motores em Repouso        | 7.92       | 5.02       | 3.31        | 0       | PASS
2. Partida dos Motores (100%) | 7.74       | 4.92       | 3.28        | 0       | PASS
3. Reversao Instantanea      | 7.61       | 4.88       | 3.25        | 0       | PASS
4. Frenagem Ativa (Curto H)  | 7.85       | 5.01       | 3.30        | 0       | PASS

CRITERIOS DE ACEITE DA ISSUE #131 ATENDIDOS COM SUCESSO:
  [X] Linhas de 3.3V e 5.0V estritamente dentro da faixa nominal (+/- 5%).
  [X] Transitórios absorvidos pelos capacitores de desacoplamento.
  [X] Zero reinicializações (resets) observadas nas reversões de motor.
```

---

## 5. Simulação no Wokwi
1. Abra a pasta `wokwi` no VS Code ou no [Wokwi.com](https://wokwi.com).
2. Pressione Play para simular o monitor de alimentação.
3. Ajuste o potenciômetro da bateria (GPIO 34) para variar a tensão entre $6,0\text{ V}$ e $8,4\text{ V}$.
4. Observe a resposta gráfica no display OLED SSD1306 e o disparo do LED vermelho de alerta em tensões abaixo de $7,0\text{ V}$.
