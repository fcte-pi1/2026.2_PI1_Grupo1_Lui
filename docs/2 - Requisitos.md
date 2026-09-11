# Documento de Requisitos do Projeto - Micromouse (PI1 2026/2 - Lui - G1)

## Objetivo
Definir e documentar formalmente os requisitos do projeto do robô Micromouse e de seu sistema de suporte, estabelecendo o que o sistema deve realizar e sob quais restrições operacionais e regulamentares deve operar.

* **Requisitos Funcionais (RF):** Descrevem as funcionalidades e capacidades operacionais do sistema ("o que o sistema deve fazer"). São especificados em alto nível (formato de Épico), focando no comportamento esperado sem impor detalhes prematuros de implementação.
* **Requisitos Não-Funcionais (RNF):** Definem os critérios de qualidade, restrições físicas, regulamentares e de desempenho ("sob quais restrições o sistema opera"). São formulados de forma **objetiva e quantitativa**, evitando termos subjetivos (ex.: "rápido", "fácil", "eficiente") e acompanhados de métodos diretos e exequíveis de verificação laboratorial.
* **Priorização MoSCoW:** Cada requisito é classificado em:
    - **Must have:** Mandatório para o funcionamento básico e conformidade com o edital do projeto.
    - **Should have:** Importante para otimização e maturidade do produto, devendo ser implementado caso o cronograma e recursos permitam.
    - **Could have:** Desejável como diferencial competitivo, mas não essencial para a entrega básica.
    - **Won't have:** Itens de menor prioridade que ficam de fora do escopo atual, podendo ser reavaliados no futuro.
* **Áreas Integradas:** Abrange as quatro áreas de engenharia da FCTE/UnB: **Estruturas (Aeroespacial e Automotiva)**, **Energia**, **Eletrônica** e **Software**.

---

## 1. Estruturas

### Requisitos Funcionais (RF) - Estruturas

| ID | Nome do Requisito | Descrição | Prioridade |
|:---:|---|---|:---:|
| **RF-EST01** | Acomodação e Fixação Estrutural | O chassi deve abrigar e fixar rigidamente todos os módulos eletrônicos, atuadores, sensores e bateria, mantendo o centro de massa baixo e prevenindo desprendimento ou desalinhamento durante acelerações e manobras na pista. | Must have |
| **RF-EST02** | Sistema de Tração e Apoio Mecânico | O conjunto mecânico de tração diferencial e apoio (rodas motrizes com pneus aderentes e roda boba/esfera deslizante) deve converter o torque dos motores em deslocamento linear sobre o piso da pista com aderência suficiente para manobras de rotação e avanço em linha reta. | Must have |
| **RF-EST03** | Suportes Mecânicos para Sensores | A estrutura física deve disponibilizar suportes fixos e dedicados para a fixação e alinhamento dos sensores de distância (frontal e laterais), garantindo campo de visada frontal e lateral desobstruído para correta leitura das paredes. | Must have |
| **RF-EST04** | Acesso para Manutenção e Limpeza | A estrutura mecânica deve permitir acesso direto para substituição rápida da bateria e limpeza/inspeção das rodas em repouso, sem necessidade de desconectar ou desmontar as placas eletrônicas principais e suportes de sensores. | Should have |
| **RF-EST05** | Construção da Pista de Testes Modular (4x4) | A equipe deve projetar e construir uma pista física modular simplificada com 4x4 células (18 cm x 18 cm cada) e paredes removíveis de 5 cm de altura, com acabamento e materiais análogos ao padrão oficial de competição estabelecido no edital. | Must have |

### Requisitos Não-Funcionais (RNF) - Estruturas

| ID | Nome do Requisito | Descrição e Critério de Aceite | Prioridade | Método de Verificação |
|:---:|---|---|:---:|---|
| **RNF-EST01** | Dimensões Físicas e Envelope de Rotação | O Micromouse não deve exceder 16,5 cm (165 mm) de comprimento e 16,5 cm (165 mm) de largura (limite do edital), e deve possuir diâmetro de envelope de rotação (diagonal máxima) inferior a 160 mm, garantindo giro livre de 360° sobre o próprio eixo dentro do corredor de 168 mm sem colidir com as paredes. | Must have | Medição direta do comprimento, largura e da diagonal máxima do robô montado utilizando paquímetro ou régua metálica graduada. |
| **RNF-EST02** | Restrição de Locomoção Terrestre | A locomoção do robô deve ser exclusivamente terrestre por contato mecânico de rodas com o solo da pista, sendo vedado saltar, voar, escalar paredes ou utilizar propulsão por combustão/foguete. | Must have | Inspeção visual e funcional do mecanismo de tração durante operação estática e dinâmica na pista de testes. |
| **RNF-EST03** | Preservação da Integridade da Pista | O robô não deve apresentar cantos vivos cortantes, superfícies abrasivas ou forças de contato que possam riscar, marcar, trincar ou deslocar as paredes ou o piso do labirinto durante a navegação. | Must have | Inspeção tátil das bordas do robô e exame visual da pista de testes após 5 baterias de corridas completas. |
| **RNF-EST04** | Altura Livre do Solo (Ground Clearance) | O chassi montado com todos os componentes e bateria deve manter uma folga vertical livre mínima de 2,0 mm em relação ao solo plano, permitindo transpor pequenos desníveis nas junções dos módulos da pista sem prender. | Must have | Inserção de gabarito ou calibre de folga de 2,0 mm sob a face inferior do chassi com o robô apoiado sobre superfície plana. |

---

## 2. Energia

### Requisitos Funcionais (RF) - Energia

| ID | Nome do Requisito | Descrição | Prioridade |
|:---:|---|---|:---:|
| **RF-ENE01** | Condicionamento e Distribuição de Potência | O subsistema de energia deve converter a tensão fornecida pela bateria e distribuir linhas reguladas e filtradas para a lógica de controle/sensores (ex.: 3,3 V / 5 V) e barramento dedicado de potência para os acionadores dos motores. | Must have |
| **RF-ENE02** | Monitoramento de Tensão de Bateria | O circuito de energia deve medir continuamente a tensão total do banco de baterias e disponibilizar essa leitura analógica/digital para o microcontrolador gerar os dados de telemetria energética. | Must have |
| **RF-ENE03** | Proteção contra Subtensão (Undervoltage Cutoff) | O sistema deve monitorar a tensão das células e cessar automaticamente a alimentação dos motores caso a tensão caia abaixo do limiar seguro de descarga (ex.: 3,3 V por célula LiPo), prevenindo degradação química da bateria. | Must have |
| **RF-ENE04** | Chaveamento Geral e Proteção por Fusível | O circuito de alimentação deve possuir chave mecânica liga/desliga de fácil acesso externo para corte total imediato, conector polarizado com trava mecânica contra inversão de polaridade (ex.: XT30 ou JST) e fusível de proteção contra sobrecorrente em série com a bateria. | Must have |

### Requisitos Não-Funcionais (RNF) - Energia

| ID | Nome do Requisito | Descrição e Critério de Aceite | Prioridade | Método de Verificação |
|:---:|---|---|:---:|---|
| **RNF-ENE01** | Autonomia Operacional Contínua | O banco de baterias embarcado deve fornecer energia contínua para no mínimo 15 minutos de navegação autônoma em perfil nominal de exploração (sensores, microcontrolador e motores ativos) sem necessidade de recarga ou troca. | Must have | Cronometragem do tempo de execução ininterrupta na pista de testes desde 100% de carga até o acionamento do corte por subtensão. |
| **RNF-ENE02** | Estabilidade da Tensão Lógica | O circuito regulador deve manter a linha de alimentação lógica (3,3 V / 5 V) dentro de uma tolerância de ±5% de sua tensão nominal, mesmo sob transitórios de corrente provocados pela aceleração máxima dos motores. | Must have | Medição da tensão no barramento lógico com multímetro/osciloscópio durante arranques e reversões dos motores em bancada. |
| **RNF-ENE03** | Tempo Máximo de Recarga | O sistema de baterias deve permitir recarga completa de sua capacidade útil em tempo não superior a 90 minutos utilizando carregador balanceador compatível com a química adotada. | Should have | Cronometragem do ciclo de recarga a partir do limiar de descarga segura até a indicação de carga completa pelo carregador. |
| **RNF-ENE04** | Proteção contra Sobrecorrente | O circuito de alimentação deve incorporar dispositivo de proteção por sobrecorrente (fusível de ação rápida entre 5A e 10A) dimensionado para interromper a corrente em caso de curto-circuito antes que ocorram danos térmicos às células LiPo ou aos condutores elétricos. | Must have | Inspeção visual da presença e especificação do fusível no circuito e teste de continuidade elétrica. |

---

## 3. Eletrônica

### Requisitos Funcionais (RF) - Eletrônica

| ID | Nome do Requisito | Descrição | Prioridade |
|:---:|---|---|:---:|
| **RF-ELE01** | Unidade Central de Processamento e Barramentos de I/O | O circuito eletrônico deve disponibilizar microcontrolador dedicado (ESP32) com barramento I2C para comunicação com sensores de distância, pinos com suporte a interrupção externa para leitura dos encoders e canais PWM de hardware para controle dos motores. | Must have |
| **RF-ELE02** | Sensoriamento de Paredes e Obstáculos | O hardware eletrônico deve realizar a leitura de proximidade das paredes frontais e laterais por meio de sensores de distância (ópticos laser ToF ou infravermelho), viabilizando a identificação de paredes livres ou bloqueadas em cada célula. | Must have |
| **RF-ELE03** | Odometria das Rodas | O circuito eletrônico deve capturar os pulsos de sinal gerados por encoders acoplados às rodas motrizes para viabilizar o cálculo do deslocamento linear percorrido e da velocidade angular pelo firmware. | Must have |
| **RF-ELE04** | Acionamento e Controle Bidirecional de Motores | A placa deve conter drivers de potência (ponte H) capazes de receber comandos de controle (PWM e sinais de sentido) do microcontrolador para acionamento bidirecional, variação suave de velocidade e frenagem dos motores DC. | Must have |
| **RF-ELE05** | Integração em Placa de Circuito Impresso (PCB / Shield) | O subsistema eletrônico deve consolidar o microcontrolador, drivers de potência, conectores de sensores e circuitos de filtragem em uma placa de circuito impresso (PCB dedicada ou shield estruturado), eliminando conexões soltas em protoboard. | Must have |
| **RF-ELE06** | Interface de Comunicação Sem Fio | O hardware de controle deve conter transceptor de rádio frequência integrado (Wi-Fi 2.4 GHz) para transmissão contínua de pacotes de dados de telemetria à estação receptora / sistema web. | Must have |
| **RF-ELE07** | Sinalização Audiovisual de Modos Operacionais | O circuito eletrônico deve conter indicadores visuais (LEDs de estado) e sonoro (buzzer piezoelétrico) para sinalizar os modos operacionais do robô (calibrando, pronto para largada, navegando, objetivo alcançado e falha/bateria baixa). | Should have |

### Requisitos Não-Funcionais (RNF) - Eletrônica

| ID | Nome do Requisito | Descrição e Critério de Aceite | Prioridade | Método de Verificação |
|:---:|---|---|:---:|---|
| **RNF-ELE01** | Taxa de Amostragem dos Sensores de Distância | O subsistema de sensoriamento de paredes deve operar com taxa de amostragem mínima de 20 Hz (intervalo máximo de 50 ms entre leituras consecutivas), assegurando dados suficientes para tomada de decisão em tempo hábil. | Must have | Medição do intervalo temporal médio entre leituras de sensores consecutivas através de saída serial ou timestamp em log. |
| **RNF-ELE02** | Faixa Útil e Tolerância de Detecção | Os sensores de distância devem detectar superfícies de parede no intervalo de 30 mm a 180 mm (abrangendo a largura do corredor de 168 mm) com tolerância de medição não superior a ±10 mm em condições de iluminação controlada. | Must have | Posicionamento do robô diante de anteparo plano em 3 distâncias fixas conhecidas (50 mm, 100 mm e 150 mm) aferidas com régua milimetrada. |
| **RNF-ELE03** | Alcance do Enlace Sem Fio | O enlace sem fio (Wi-Fi) deve manter comunicação contínua e transmissão estável de pacotes de telemetria a uma distância mínima de 8 metros em visada direta da pista de testes. | Must have | Teste de transmissão de pacotes entre o robô posicionado no ponto mais distante da pista e o computador receptor a 8 metros. |
| **RNF-ELE04** | Frequência de Chaveamento PWM dos Motores | O sinal de acionamento PWM fornecido pela eletrônica aos drivers dos motores deve operar em frequência na faixa de 1 kHz a 20 kHz, prevenindo ressonâncias mecânicas bruscas e assegurando controle proporcional estável. | Must have | Medição da frequência do sinal de controle nos pinos de saída PWM com multímetro (escala de frequência) ou osciloscópio de bancada. |
| **RNF-ELE05** | Imunidade a Ruídos e Isolamento de Sinais | O circuito eletrônico deve conter capacitores de desacoplamento e diodos de proteção contra transientes indutivos gerados pelos motores, garantindo que o microcontrolador não sofra reinicializações espúrias (*resets*) durante operação. | Must have | Teste de bancada submetendo os motores a reversões rápidas de sentido e travamentos momentâneos monitorando a linha de reset do ESP32. |

---

## 4. Software

### Requisitos Funcionais (RF) - Software

| ID | Nome do Requisito | Descrição | Prioridade |
|:---:|---|---|:---:|
| **RF-SFT01** | Controle Cinemático, Alinhamento e Parada por Stall | O firmware embarcado deve implementar malha de controle em malha fechada (ex.: PID) utilizando as leituras dos sensores de distância e encoders para manter o robô centralizado nas retas, executar rotações controladas de 90° e 180°, e desativar os motores em caso de bloqueio mecânico prolongado (stall por colisão) detectado por ausência de pulsos de encoder sob PWM ativo. | Must have |
| **RF-SFT02** | Mapeamento Dinâmico das Geometrias Oficiais | O software deve rastrear as coordenadas da célula atual do robô na grade e atualizar dinamicamente a representação matricial do labirinto com suporte configurável para as três geometrias oficiais do edital (4x4, 8x4 e 12x4 células), registrando as paredes identificadas a cada avanço. | Must have |
| **RF-SFT03** | Algoritmo de Navegação e Resolução Autônoma | O firmware deve executar algoritmo autônomo de busca e tomada de decisão (ex: Floodfill, Tremaux ou algoritmo de parede) para conduzir o robô da célula inicial até a célula ou área objetivo sem qualquer intervenção humana externa. | Must have |
| **RF-SFT04** | Reconhecimento de Objetivo e Parada Automática | O software deve identificar quando o robô ingressou na área ou célula objetivo programada, cessar a locomoção de tração e registrar o tempo final de conclusão da corrida. | Must have |
| **RF-SFT05** | Despacho Contínuo de Telemetria | O firmware deve empacotar e despachar periodicamente via Wi-Fi os 6 parâmetros mandatórios estipulados pelo edital: (1) tipo de labirinto, (2) trajeto percorrido, (3) consumo de bateria, (4) velocidade média, (5) tempo de conclusão e (6) desafio cumprido (S/N). | Must have |
| **RF-SFT06** | Dashboard Web de Telemetria em Tempo Real | A aplicação web deve fornecer interface gráfica que exiba em tempo real a grade do labirinto com as paredes descobertas, o trajeto percorrido pelo robô, o nível/consumo de bateria, a velocidade média, o cronômetro da prova e o indicador de desafio cumprido. | Must have |
| **RF-SFT07** | Armazenamento e Consulta em Banco de Dados | O sistema web deve persistir os dados consolidados das corridas em banco de dados e disponibilizar filtros para consulta individual por labirinto específico ou exibição consolidada de todos os labirintos registrados. | Must have |
| **RF-SFT08** | Cálculo de Trajetória Otimizada (Speed Run) | Após a exploração do labirinto e identificação da rota viável, o software deve ser capaz de computar o caminho mais curto até o objetivo e executá-lo em uma corrida subsequente com velocidade superior à de mapeamento. | Should have |

### Requisitos Não-Funcionais (RNF) - Software

| ID | Nome do Requisito | Descrição e Critério de Aceite | Prioridade | Método de Verificação |
|:---:|---|---|:---:|---|
| **RNF-SFT01** | Autonomia e Imutabilidade Durante a Prova | O robô deve operar de forma 100% autônoma após o acionamento da largada na pista, sendo vedada qualquer intervenção manual de pilotagem externa, recarregamento ou alteração de código/memória sobre o labirinto durante a resolução. | Must have | Demonstração prática da corrida autônoma sem envio de comandos manuais e auditoria do código-fonte para confirmar ausência de mapas pré-gravados. |
| **RNF-SFT02** | Taxa e Latência da Telemetria Web | A aplicação web deve receber e atualizar as informações do dashboard com taxa mínima de 1 Hz (ao menos 1 envio por segundo) e latência perceptível inferior a 1,5 segundo em rede local Wi-Fi. | Must have | Comparação temporal entre o avanço físico do robô na pista de testes e o reflexo correspondente na interface do painel web. |
| **RNF-SFT03** | Tempo de Resposta para Consultas Históricas | A interface web deve recuperar e renderizar os dados de corridas anteriores armazenadas no banco de dados em tempo não superior a 2,0 segundos para bases com até 500 registros. | Should have | Medição do tempo de resposta da requisição de consulta no painel de rede (DevTools) do navegador web. |
| **RNF-SFT04** | Compatibilidade e Responsividade Web | O sistema web deve ser compatível com os navegadores modernos (Google Chrome e Mozilla Firefox) e renderizar a grade do labirinto e painéis de dados sem quebras de layout em resoluções mínimas a partir de 1366x768 pixels. | Should have | Teste visual de layout em ambos os navegadores sob resolução de 1366x768 pixels. |
| **RNF-SFT05** | Governança e Rastreabilidade no GitHub | O código-fonte, diagramas, esquemáticos e documentação técnica devem seguir estritamente o template oficial da disciplina no GitHub, com tarefas gerenciadas via GitHub Projects e issues atribuídas a no máximo 2 integrantes por item. | Must have | Auditoria direta do repositório no GitHub, estrutura de diretórios e quadro de issues no GitHub Projects. |