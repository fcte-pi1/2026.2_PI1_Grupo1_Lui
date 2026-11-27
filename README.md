
# Projeto Integrador 1 - 2026.2 | Grupo 01 | Prof. Lui

<div align="center">
  <img src="docs/images.png" alt="Logo UnB" width="120" />

  # Projeto Integrador 1 — Micromouse (Grupo 1)
  **Semestre 2026.2 | Universidade de Brasília (UnB) | Prof. Lui**

  [![Documentação](https://img.shields.io/badge/Site-Documentação_Oficial-blue?style=for-the-badge&logo=markdown)](https://fcte-pi1.github.io/2026.2_PI1_Grupo1_Lui/)
</div>

---

> **Aviso ao Professor:** Este é o repositório oficial do Grupo 01 utilizado para o desenvolvimento do projeto de PI1 (Micromouse) durante o semestre 2026.2. Todo o histórico de commits, código-fonte e detalhamentos estão centralizados aqui.

## 🤖 Sobre o Projeto
O projeto consiste no desenvolvimento completo de um robô autônomo (Micromouse) capaz de navegar e resolver um labirinto, integrando conhecimentos de quatro grandes áreas da engenharia: **Software, Eletrônica, Energia e Estruturas**.

Toda a documentação gerencial e conceitual (TAP, Requisitos, EAP, Cronograma, Testes, etc.) está centralizada de forma interativa no nosso **[Site de Documentação Oficial](https://fcte-pi1.github.io/2026.2_PI1_Grupo1_Lui/)**.

## 🗂️ Estrutura do Repositório
Para manter o padrão e a facilidade de busca, o repositório está dividido nas seguintes pastas:

* 📂 **`docs/`** — Arquivos Markdown (`.md`) e ativos que alimentam a página web.
* 📂 **`hw/`** — Arquitetura de hardware, esquemáticos e arquivos de simulação de circuitos.
* 📂 **`mec/`** — Modelagens 3D, arquivos CAD e projetos de estrutura.
* 📂 **`src/`** — Código-fonte e algoritmos de navegação/controle do robô.

*(A estrutura de pastas não isola as equipes. O projeto é multidisciplinar e os membros devem trabalhar nas diferentes pastas conforme a necessidade das tarefas).*

## 👥 Equipe
O time é formado por estudantes de engenharia e dividido em subáreas para facilitar o gerenciamento técnico, atuando sob a liderança geral de **Antônio Lucas** e apoio dos subgerentes:

* 💻 **Software:** Pedro Ian (Subgerente)
* ⚡ **Energia:** Rafaela Trajano (Subgerente)
* 🔌 **Eletrônica:** Leonardo Augusto (Subgerente)
* 🏗️ **Estruturas:** Carlos Henrique (Subgerente)

<details>
<summary><b>Clique aqui para ver a lista completa com os 19 integrantes</b></summary>

* Humberto Alencar (Aeroespacial)
* João Vitor Sales Ibiapina (Software)
* Eduardo Lobo Moreira (Software)
* Pedro Ian (Software)
* Breno Teixeira (Software)
* Rafaela Trajano Ruas (Energia)
* Lucas Oliveira de Paula (Software)
* Carlos Henrique de Paiva Muniz (Software)
* Mariana Solano de Brito Elias (Aeroespacial)
* Antônio José Magalhães Leão Júnior (Software)
* João Pedro Jaime (Software)
* Lucas Peixoto Rodrigues (Software)
* Daniel Almeida Frota (Software)
* Antônio Lucas De Araújo Lima (Software)
* Pedro Franco Cavalcanti Rêgo (Software)
* Alexandre Henrique Almeida Valadares Sousa (Software)
* Davi Sakai (Software)
* Gustavo Antônio (Software)
* Leonardo Augusto (Eletrônica)

</details>

## 🚀 Para os Integrantes: Como Contribuir

A organização e a correta utilização do repositório serão consideradas na avaliação do grupo. Dessa forma, é fundamental que **todos** utilizem o Git de forma alinhada com a estratégia da equipe.

### Fluxo de Trabalho
1. **Clone o repositório:** Após obter acesso (via convite no e-mail ou notificação), abra o terminal e clone o projeto:
   ```bash
   git clone https://github.com/fcte-pi1/2026.2_PI1_Grupo1_Lui.git
   ```

---

## 🛠️ Gravação dos Firmwares (ESP32 e ESP8266) e Controle Remoto

> **Pré-requisito:** ter o PlatformIO instalado (`pio`) ou usar o ambiente virtual do projeto (`src/backend/.venv/bin/pio` no Linux/macOS ou `src\backend\.venv\Scripts\pio.exe` no Windows). Feche o Monitor Serial antes de gravar ou rodar os scripts Python na mesma porta USB.

### 1. Passar o firmware de bancada para o ESP32 (robô)
```bash
# Firmware completo de bancada (menu interativo + comandos sem fio ESP-NOW)
pio run -d src/firmware/bancada -e esp32dev -t upload

# (Opcional) Abrir monitor serial USB direto no ESP32 (115200 bps)
pio device monitor -d src/firmware/bancada -b 115200

# (Opcional) Gravar testes isolados por componente
pio run -d src/firmware/bancada -e teste_tof_mapa_xshut -t upload -t monitor
# Outros ambientes: teste_motor_esquerdo, teste_motor_direito, teste_encoder_esquerdo,
# teste_encoder_direito, teste_tof_esquerdo, teste_tof_frontal, teste_tof_direito,
# teste_tof_sequencia, teste_bateria, teste_botao, teste_led
```

Para gravar o **firmware oficial da prova autônoma** no ESP32 do robô:
```bash
pio run -d src/firmware/robo -e esp32dev -t upload
```

Para gravar o **firmware geral unificado** (junta o algoritmo da prova autônoma de `robo/` com o menu de testes de bancada no mesmo binário):
```bash
pio run -d src/firmware/geral -e esp32dev -t upload
```

### 2. Passar o firmware do gateway para o ESP8266 (NodeMCU / D1 mini)
```bash
# Conecte o ESP8266 no USB do computador e grave:
pio run -d src/firmware/gateway -e esp8266 -t upload

# (Opcional) Conferir os pacotes JSON chegando na serial (115200 bps)
pio device monitor -d src/firmware/gateway -b 115200
```
*(O robô e o gateway já usam canal Wi-Fi 1 em broadcast; não é necessário parear endereços MAC.)*

### 3. Como usar o Controle Remoto e Testes Sem Fio (`scripts/`)
Com o **firmware de bancada** gravado no ESP32 do robô e o **ESP8266 gateway** plugado no USB do computador (ex.: `/dev/ttyUSB0` no Linux ou `COM3`/`COM5` no Windows):

1. **Instale o `pyserial`** (ou use o Python da `.venv` de `src/backend/.venv`, que já possui `pyserial`):
   ```bash
   # Controle remoto completo (modos 1..9, avançar 1 célula, curvas 90°/180°, freio 's'/ESPAÇO)
   src/backend/.venv/bin/python scripts/controle_remoto.py /dev/ttyUSB0
   # Windows PowerShell:
   # .\src\backend\.venv\Scripts\python.exe scripts\controle_remoto.py COM3
   ```
2. **Teste rápido de motores sem fio** (envia tecla na hora sem precisar de `ENTER`: `e`/`E` esquerdo, `d`/`D` direito, `+`/`-` PWM, `s` freio):
   ```bash
   src/backend/.venv/bin/python scripts/teste_motores_remoto.py /dev/ttyUSB0
   ```
3. **Teste rápido dos 3 sensores ToF sem fio** (exibe `livre` / `PAREDE` a `< 110 mm` em tempo real):
   ```bash
   src/backend/.venv/bin/python scripts/teste_sensores_remoto.py /dev/ttyUSB0
   ```
4. **Apenas monitorar a telemetria JSON** (quando o robô está com o firmware oficial `robo` ou `geral`):
   ```bash
   src/backend/.venv/bin/python scripts/monitor_telemetria.py /dev/ttyUSB0
   ```
   *(Com `robo` ou `geral` gravado, você pode enviar `k` para iniciar a resolução do labirinto — ou usar o botão **Resolver** no painel Web — além de `s` para parada de emergência e `d` para diagnóstico rápido via Serial/ESP-NOW.)*

![Calendário 2026/2](https://github.com/fcte-pi1/template/blob/main/docs/figs/Calendario.png?raw=true)

