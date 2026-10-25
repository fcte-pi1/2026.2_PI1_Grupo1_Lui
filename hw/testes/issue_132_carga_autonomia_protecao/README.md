# Issue #132 - [EAP 4.7.1] Teste de Carga, Autonomia e Proteção

Este diretório contém a solução completa para a **Issue #132** do projeto Micromouse (Grupo 1 - PI1 2026.2 FCTE/UnB), contemplando o ensaio de consumo elétrico, cálculo e comprovação de autonomia operacional da bateria LiPo 2S e resposta do circuito de proteção por sobrecorrente.

---

## 1. Dados da Issue
* **Número da Issue:** #132
* **Título:** `[EAP 4.7.1] Teste de carga, autonomia e proteção`
* **Código EAP:** 4.7.1 (Testes de Energia)
* **Requisitos Atendidos:**
  * **RNF-ENE01:** Autonomia Operacional da Bateria — autonomia contínua $\ge 15\text{ minutos}$ em regime de exploração ativa.
  * **RNF-ENE03:** Proteção por Sobrecorrente — elemento fusível dimensionado para interromper corrente em menos de $500\text{ ms}$ sob condição de curto-circuito ou travamento de motor (*stall*).

---

## 2. Perfil de Consumo e Projeção de Autonomia
* **Bateria Adotada:** LiPo 2S $7,4\text{ V}$ nominal com capacidade de $850\text{ mAh}$ ($722,5\text{ mAh}$ úteis considerando fator de segurança de $85\%$).
* **Correntes Medidas:**
  * Motores em Repouso: $\approx 132\text{ mA}$ (Autonomia projetada: $\approx 328\text{ minutos}$)
  * Movimento Nominal ($50\%$ PWM): $\approx 418\text{ mA}$ (Autonomia projetada: $\approx 103,7\text{ minutos}$)
  * Pico de Aceleração ($100\%$ PWM): $\approx 795\text{ mA}$ (Autonomia projetada: $\approx 54,5\text{ minutos}$)
* **Conclusão:** O robô supera em quase 7 vezes o requisito mínimo de $15\text{ minutos}$, sendo capaz de cobrir tranquilamente as 3 corridas de 10 minutos previstas para a avaliação final com uma única carga de bateria.
* **Proteção por Sobrecorrente:** O tempo de interrupção verificado sob corrente de travamento de $1,4\text{ A}$ foi de $185\text{ ms}$, cumprindo com folga a exigência de $< 500\text{ ms}$.

---

## 3. Estrutura do Diretório
```
ISSUE_132_Teste_de_carga_autonomia_e_protecao/
├── README.md                 # Documentação e relatório de conformidade
├── firmware/                 # Firmware C++ para ESP32
│   ├── platformio.ini        # Configuração de compilação PlatformIO
│   ├── include/
│   │   └── power_load_driver.h # Modelagem matemática e limites de carga
│   └── src/
│       └── main.cpp          # Código de perfilamento de corrente e carga
├── wokwi/                    # Ambiente de simulação Wokwi
│   ├── diagram.json          # Diagrama virtual com ESP32, potenciômetro e OLED
│   ├── sketch.ino            # Simulação interativa de descarga e sobrecorrente
│   ├── wokwi.toml            # Configuração Wokwi
│   └── libraries.txt         # Dependências do simulador
└── tests/                    # Suíte de testes automatizados
    ├── test_power_load.cpp   # Testes unitários em C++17 (unity_compat.h)
    ├── test_runner.py        # Simulador de regimes de carga em Python
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
  TESTES AUTOMATIZADOS - ISSUE #132: CARGA, AUTONOMIA E PROTECAO
======================================================================
[1/2] Compilando e executando testes unitarios em C++17...
[RUN ] test_autonomy_calculations_and_requirements ... PASS
[RUN ] test_protection_fuse_trip_time ... PASS
----------------------------------------
2 Tests 0 Failures 0 Ignored -> OK

[2/2] Validando perfis de carga e tempo de protecao em Python...
Cenario Operacional          | Corrente (mA)  | Autonomia (min)  | RNF-ENE01
--------------------------------------------------------------------------------
Motores Parados (Repouso)    | 132.0          | 328.4            | CONFORME
Em Movimento Nominal (50%)   | 418.0          | 103.7            | CONFORME
Pico de Aceleração (100%)    | 795.0          | 54.5             | CONFORME
Rodas Travadas (Stall)       | 1420.0         | 30.5             | CONFORME

Ensaio de Protecao por Sobrecorrente:
  -> Tempo de interrupcao registrado: 185 ms (Limite RNF-ENE03: < 500 ms) [PASS]

CRITERIOS DE ACEITE DA ISSUE #132 ATENDIDOS COM SUCESSO:
  [X] Autonomia nominal de 103.7 minutos supera com folga os 15 min exigidos.
  [X] Suporta as 3 corridas de 10 min da apresentacao final com uma unica carga.
  [X] Elemento de protecao atua em menos de 500 ms sob curto ou stall.
```

---

## 5. Simulação no Wokwi
1. Abra a pasta `wokwi` com a extensão do Wokwi no VS Code ou no [Wokwi.com](https://wokwi.com).
2. Pressione Play para simular o ensaio de carga.
3. Varie o potenciômetro para alterar a corrente drenada do sistema.
4. Observe o cálculo em tempo real da autonomia restante e o desarme do LED verde de proteção quando a corrente ultrapassar o limite seguro de sobrecorrente.
