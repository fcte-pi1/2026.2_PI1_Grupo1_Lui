# Documento de Requisitos do Projeto - Micromouse (PI1 2026/2)

## Objetivo
Definir e documentar os requisitos do projeto. Requisitos definem o que um sistema deve fazer e sob quais restrições.

* **Requisitos Funcionais (RF):** Relacionados com "o que um sistema deve fazer" (suas funcionalidades). Serão descritos em alto nível (Épico).
* **Requisitos Não-Funcionais (RNF):** Relacionados com "sob que restrições". Devem ser descritos de forma **objetiva** (ex: "a página deve carregar em até 5s quando em conexão 4G"). Evitar descrições subjetivas (mais facilmente, mais rapidamente, de fácil uso).

> **Priorização:** Ao descrever os requisitos (RF e RNF), utilizar a classificação **MoSCoW** (*Must have, Should have, Could have*) para auxiliar a priorização.
>
> **Áreas Envolvidas:** O projeto integra as quatro áreas de engenharia da FCTE: Estruturas, Energia, Eletrônica e Software.

---

## 1. Estruturas

### Requisitos Funcionais (RF) - Estruturas

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RF-EST01** | Acomodação e Fixação Estrutural | O chassi deve abrigar, fixar e proteger contra vibrações e impactos todos os módulos eletrônicos, motores, sensores e bateria, mantendo o centro de massa rebaixado para estabilidade. | Must have | Estruturas | # |
| **RF-EST02** | Sistema de Tração e Rodagem Mecânica | O conjunto mecânico de tração e apoio deve converter o torque dos motores em deslocamento linear com aderência suficiente para evitar derrapagens nas curvas e pistas. | Must have | Estruturas | # |
| **RF-EST03** | Suportes Direcionais para Sensores | A estrutura física deve fornecer suportes angulados para fixação e alinhamento dos sensores de distância (frontais, diagonais e laterais), livres de obstruções visuais. | Must have | Estruturas | # |
| **RF-EST04** | Acesso Rápido para Manutenção e Limpeza | A estrutura mecânica deve permitir acesso direto e desmontagem rápida para substituição de baterias e limpeza de rodas sem exigir a desmontagem das placas eletrônicas. | Should have | Estruturas | # |
| **RF-EST05** | Construção da Pista de Testes Modular (4x4) | A equipe deve projetar e fabricar uma pista física modular simplificada (4x4 células) com paredes removíveis, postes de travamento e acabamento análogo ao padrão oficial da competição. | Must have | Estruturas | # |

### Requisitos Não-Funcionais (RNF) - Estruturas

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RNF-EST01** | Dimensões Físicas Máximas | O Micromouse não deve exceder 165 mm de comprimento por 165 mm de largura em qualquer configuração operacional (sem restrição de altura). | Must have | Estruturas | # |
| **RNF-EST02** | Restrição de Locomoção Terrestre | A locomoção deve ser exclusivamente terrestre por contato mecânico de rodas ou esteiras com o solo, sendo proibido voar, saltar, escalar paredes ou usar propulsão por combustão/foguete. | Must have | Estruturas | # |
| **RNF-EST03** | Preservação da Integridade da Pista | O robô não deve aplicar forças de impacto excessivas nem possuir componentes pontiagudos ou abrasivos capazes de marcar, trincar ou danificar as paredes ou piso do labirinto. | Must have | Estruturas | # |
| **RNF-EST04** | Altura Livre do Solo (Ground Clearance) | A base inferior do chassi deve garantir uma folga vertical mínima de 2,5 mm em relação ao piso para transpor pequenos desníveis nas junções dos módulos da pista. | Must have | Estruturas | # |

---

## 2. Energia

### Requisitos Funcionais (RF) - Energia

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RF-ENE01** | Condicionamento e Distribuição de Potência | O subsistema de energia deve converter e distribuir tensões reguladas estáveis para a eletrônica lógica/controle (3.3V/5V) e para o circuito de potência dos motores. | Must have | Energia | # |
| **RF-ENE02** | Monitoramento de Nível e Consumo de Bateria | O circuito de energia deve medir continuamente a tensão total do banco de baterias e a corrente consumida, gerando os dados de telemetria energética. | Must have | Energia | # |
| **RF-ENE03** | Proteção e Corte por Subtensão (Undervoltage Protection) | O sistema deve cessar automaticamente a alimentação dos motores caso a tensão das células caia abaixo do limiar seguro de descarga para evitar degradação química da bateria. | Must have | Energia | # |
| **RF-ENE04** | Chaveamento Geral e Conector de Recarga | O circuito deve possuir chave mecânica liga/desliga para isolamento elétrico total e conector padronizado para recarga externa com balanceamento de células. | Must have | Energia | # |

### Requisitos Não-Funcionais (RNF) - Energia

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RNF-ENE01** | Autonomia do Subsistema de Alimentação | O banco de baterias embarcado deve fornecer energia contínua para no mínimo 20 minutos de navegação autônoma nominal sem necessidade de troca ou recarga. | Must have | Energia | # |
| **RNF-ENE02** | Regulação e Estabilidade de Tensão | Os reguladores de tensão devem manter a variação de tensão das linhas lógicas dentro de mais ou menos 5%, mesmo durante picos de corrente exigidos pelos motores em aceleração. | Must have | Energia | # |
| **RNF-ENE03** | Tempo Máximo de Recarga | O sistema de baterias deve permitir recarga completa de 0% a 100% da sua capacidade em tempo não superior a 90 minutos através de carregador balanceador dedicado. | Should have | Energia | # |

---

## 3. Eletrônica

### Requisitos Funcionais (RF) - Eletrônica

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RF-ELE01** | Sensoriamento de Proximidade e Paredes | O hardware deve realizar aquisição de dados de distância das paredes por meio de sensores ópticos (ToF/infravermelho) ou ultrassônicos posicionados estrategicamente. | Must have | Eletrônica | # |
| **RF-ELE02** | Odometria e Sensoriamento Inercial | O circuito deve capturar pulsos de encoders acoplados às rodas e/ou medições de uma unidade inercial (IMU) para estimativa de deslocamento e orientação angular. | Must have | Eletrônica | # |
| **RF-ELE03** | Acionamento e Controle Bidirecional de Motores | A placa deve conter drivers de potência ou pontes H para amplificar sinais PWM do microcontrolador, viabilizando o controle bidirecional e frenagem dos motores. | Must have | Eletrônica | # |
| **RF-ELE04** | Placa de Circuito Impresso Integrada (PCB) | O hardware deve consolidar o microcontrolador principal e circuitos de interface em uma PCB dedicada, evitando conexões soltas ou fiações suspensas. | Must have | Eletrônica | # |
| **RF-ELE05** | Interface de Comunicação Sem Fio | A placa eletrônica deve integrar transceptor de rádio frequência (Wi-Fi, Bluetooth, ESP-NOW ou RF 2.4 GHz) para transmissão de telemetria em tempo real. | Must have | Eletrônica | # |
| **RF-ELE06** | Sinalização Visual e Sonora de Estado | O hardware deve disponibilizar LEDs indicadores e buzzer sonoro para comunicar modos operacionais do robô (calibração, falha, prontidão e objetivo alcançado). | Should have | Eletrônica | # |

### Requisitos Não-Funcionais (RNF) - Eletrônica

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RNF-ELE01** | Frequência de Amostragem dos Sensores | O conjunto de sensoriamento de paredes deve operar com taxa de amostragem mínima de 20 Hz (intervalo máximo de 50 ms entre leituras consecutivas). | Must have | Eletrônica | # |
| **RNF-ELE02** | Alcance e Precisão de Detecção | Os sensores de distância devem detectar obstáculos no intervalo de 20 mm a 180 mm com erro de medição não superior a mais ou menos 5 mm nas condições de teste. | Must have | Eletrônica | # |
| **RNF-ELE03** | Alcance da Comunicação Sem Fio | O enlace de rádio sem fio deve manter conexão estável e sem perda excessiva de pacotes a uma distância mínima de 10 metros em visada direta da pista. | Must have | Eletrônica | # |

---

## 4. Software

### Requisitos Funcionais (RF) - Software

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RF-SFT01** | Controle Cinemático em Baixo Nível (PID) | O firmware embarcado deve implementar malha de controle PID para manter o robô centralizado no corredor e realizar rotações de 90° e 180° com precisão. | Must have | Software | # |
| **RF-SFT02** | Mapeamento e Estimativa de Posição | O software deve rastrear a célula atual do robô e atualizar dinamicamente a representação matricial do labirinto conforme novas paredes forem descobertas. | Must have | Software | # |
| **RF-SFT03** | Algoritmo de Resolução Autônoma de Labirinto | O firmware deve executar algoritmo autônomo de exploração e tomada de decisão (ex: Floodfill ou Tremaux) para navegar até o objetivo sem intervenção externa. | Must have | Software | # |
| **RF-SFT04** | Reconhecimento de Objetivo e Conclusão de Prova | O software deve detectar o ingresso na célula objetivo central, paralisar a locomoção e registrar o tempo de conclusão da corrida. | Must have | Software | # |
| **RF-SFT05** | Cálculo de Rota Otimizada (Speed Run) | O software deve computar a trajetória mais curta com base no mapa registrado na exploração e guiar o robô em velocidade máxima numa corrida subsequente. | Should have | Software | # |
| **RF-SFT06** | Despacho Contínuo de Telemetria | O firmware deve empacotar e despachar os dados operacionais (posição, velocidade, consumo de bateria, tempo decorrido e status do desafio) para a estação base. | Must have | Software | # |
| **RF-SFT07** | Dashboard Web em Tempo Real | A aplicação web deve renderizar visualmente a grade do labirinto, o trajeto percorrido pelo robô em tempo real, consumo de bateria, velocidade média e status da prova. | Must have | Software | # |
| **RF-SFT08** | Persistência e Consulta Histórica em Banco de Dados | O sistema web deve armazenar os registros das corridas em banco de dados e permitir consulta e filtragem por labirinto específico ou visão consolidada de todos os labirintos. | Must have | Software | # |

### Requisitos Não-Funcionais (RNF) - Software

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
|:---:|---|---|:---:|---|---|
| **RNF-SFT01** | Latência de Atualização da Telemetria | A latência total entre a emissão do dado de telemetria no robô e a sua atualização no painel web deve ser inferior a 1,0 segundo em rede local. | Should have | Software | # |
| **RNF-SFT02** | Imutabilidade e Não-Intervenção Humana | O firmware deve bloquear qualquer envio de comando de movimentação externa ou carregamento de mapas prévios após o acionamento da largada na pista. | Must have | Software | # |
| **RNF-SFT03** | Tempo de Resposta para Consultas Históricas | A interface web deve carregar e exibir dados históricos de corridas do banco de dados em tempo não superior a 2,0 segundos para bases com até 1.000 registros. | Should have | Software | # |
| **RNF-SFT04** | Responsividade e Compatibilidade Web | O sistema web deve operar de forma responsiva nas resoluções mínimas a partir de 1280x720 pixels nos navegadores Chrome e Firefox em suas versões atualizadas. | Should have | Software | # |
| **RNF-SFT05** | Padrão de Governança no GitHub | Todo o código-fonte, esquemáticos e documentação devem seguir a estrutura de diretórios do template oficial da disciplina e issues com no máximo 2 responsáveis. | Must have | Software | # |

---

## Critérios de Aceite
- [x] Todos os requisitos funcionais descritos como Épicos.
- [x] Todos os requisitos não-funcionais descritos de forma objetiva e mensurável.
- [x] Classificação MoSCoW aplicada em todos os itens.
- [x] Requisitos categorizados e distribuídos por área de engenharia (Estruturas, Energia, Eletrônica e Software).
