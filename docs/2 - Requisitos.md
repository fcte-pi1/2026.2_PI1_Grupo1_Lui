# Documento de Requisitos do Projeto — Micromouse (PI1 2026.2 - Grupo 1)

---

## 1. Visão Geral e Escopo

### 1.1 Objetivo do Documento
Este documento estabelece a especificação formal de engenharia dos requisitos do robô móvel autônomo **Micromouse (Grupo 1)** e de seu sistema de suporte e telemetria para a disciplina Projeto Integrador 1 (semestre 2026.2, FCTE/UnB, Prof. Lui). O documento define as capacidades operacionais esperadas do produto, as restrições regulamentares de competição e os atributos mensuráveis de qualidade, distribuídos de forma integrada entre as quatro áreas de engenharia: **Estruturas (Aeroespacial e Automotiva)**, **Energia**, **Eletrônica** e **Software**.

* **Requisitos Funcionais (RF):** Descrevem o comportamento e as capacidades operacionais do sistema (*o que o produto deve fazer*), formulados em nível de abstração comportamental (formato de Épico), sem prescrever prematuramente escolhas internas de implementação ou tecnologias específicas.
* **Requisitos Não-Funcionais (RNF):** Definem os critérios de qualidade, desempenho, restrições regulamentares do edital, segurança e limitações físicas de engenharia (*sob quais restrições o sistema opera*), formulados de forma estritamente **quantitativa e verificável**, associados a métodos diretos de ensaio laboratorial.

---

### 1.2 Metodologia de Priorização (MoSCoW)

A priorização dos requisitos segue o método **MoSCoW**, balanceando o núcleo mandatório da entrega com metas de otimização e delimitando com precisão as fronteiras do escopo:

| Categoria | Significado no Projeto | Critério de Decisão |
|:---:|---|---|
| **Must have** | Requisitos mandatórios e inegociáveis. | Sem eles, o robô não participa da prova, viola o edital ou não cumpre a missão básica. |
| **Should have** | Requisitos importantes de alta relevância técnica. | Agregam maturidade, eficiência e competitividade esperadas; passíveis de readequação caso haja contingência severa de prazo. |
| **Could have** | Requisitos desejáveis e diferenciais competitivos. | Recursos de valor agregado que serão implementados caso os requisitos *Must* e *Should* estejam concluídos e validados. |
| **Won't have** | Deliberações explícitas de itens fora de escopo. | Capacidades deliberadamente excluídas do projeto neste semestre para preservação do foco e viabilidade temporal e orçamentária. |

---

## 2. Requisitos por Subsistema

### 2.1 Estruturas (EST)

O subsistema de Estruturas compreende o chassi, suportes para sensores, arranjo mecânico de tração, rodagem e acomodação dos módulos embarcados.

#### Requisitos Funcionais (RF) — Estruturas

| ID | Nome do Requisito | Descrição Comportamental | Prioridade | Critério de Aceitação |
|:---:|---|---|:---:|---|
| **RF-EST01** | Acomodação e Fixação dos Componentes | A estrutura física do chassi deve posicionar e reter mecanicamente todos os subsistemas embarcados (bateria, atuadores, circuitos eletrônicos e cabeamento), prevenindo folgas, oscilações ou desacoplamentos durante acelerações, frenagens e manobras na pista. | Must have | Todos os módulos embarcados permanecem fixos e operacionais após 10 baterias de manobras dinâmicas contínuas em pista. |
| **RF-EST02** | Tração Mecânica e Transmissão de Movimento | O conjunto mecânico de tração e apoio deve converter o torque entregue pelos atuadores em deslocamento linear sobre o piso da pista, provendo atrito estático suficiente para arranques retos e desacelerações sem patinamento perceptível. | Must have | O robô acelera do repouso até sua velocidade de ensaio (≥ 0,15 m/s) em linha reta sem escorregamento lateral das rodas motrizes. |
| **RF-EST03** | Posicionamento Fixo e Desobstruído de Sensores | A estrutura mecânica deve fornecer pontos de ancoragem rígidos e orientados para o sensoriamento de paredes (visada frontal e laterais ortogonais), mantendo o campo óptico desobstruído em relação ao chassi e às rodas. | Must have | Os sensores mantêm alinhamento angular estável (variação < 2°) em relação aos eixos principais do chassi após testes de colisão branda com anteparos. |

#### Requisitos Não-Funcionais (RNF) — Estruturas

| ID | Nome do Requisito | Descrição e Métrica Quantitativa | Prioridade | Método de Verificação Laboratorial |
|:---:|---|---|:---:|---|
| **RNF-EST01** | Dimensões Físicas e Envelope de Giro | O robô montado em ordem de marcha deve possuir comprimento total ≤ 165 mm, largura total ≤ 165 mm e diâmetro de rotação sobre o próprio eixo (diagonal máxima) < 160 mm. | Must have | Medição direta com paquímetro e execução de giro livre de 360° no próprio eixo dentro de corredor de 168 mm sem tocar as paredes. |
| **RNF-EST02** | Locomoção Exclusivamente Terrestre | A locomoção do robô deve ser realizada exclusivamente por contato mecânico de tração das rodas com a superfície do solo da pista, sendo expressamente vedado saltar, voar, planar, escalar paredes ou utilizar qualquer propulsão por combustão ou reação química. | Must have | Inspeção visual e funcional do mecanismo de tração durante operação estática e dinâmica na pista de testes. |
| **RNF-EST03** | Preservação da Integridade da Pista | O robô não deve arranhar, trincar, deslocar, manchar ou degradar as paredes ou o piso da pista; deve apresentar extremidades perimetrais sem cantos vivos (raio de curvatura mínimo de 1,0 mm) e força dinâmica máxima de contato nas paredes inferior a 2,0 N sob velocidade nominal. | Must have | Inspeção com gabarito de raio de curvatura, medição dinamométrica de impacto e exame visual minucioso da pista após 5 baterias de corridas completas. |
| **RNF-EST04** | Altura Livre do Solo (*Ground Clearance*) | A face inferior do chassi deve manter uma folga vertical livre mínima de 2,0 mm em relação ao piso plano da pista em todos os seus pontos de varredura. | Must have | Deslocamento do robô sobre lâmina de gabarito plana de 2,0 mm sem atrito ou contato mecânico da base. |
| **RNF-EST05** | Tempo e Facilidade de Substituição da Bateria | A estrutura física deve permitir o desacoplamento e substituição do banco de baterias em tempo não superior a 60 segundos, sem desmontagem das placas eletrônicas principais ou dos suportes de sensores. | Should have | Cronometragem de 5 procedimentos de troca de bateria realizados por operadores distintos, registrando tempo médio e desvio padrão. |
| **RNF-EST06** | Acessibilidade para Limpeza de Rodagem | O mecanismo de rodagem e apoio deve permitir inspeção visual direta e limpeza superficial dos pneus/apoios sem necessidade de ferramentas especiais ou desmontagem estrutural do chassi. | Should have | Inspeção e limpeza superficial das bandas de rodagem realizadas em bancada em menos de 120 segundos. |

---

### 2.2 Energia (ENE)

O subsistema de Energia é responsável pelo armazenamento eletroquímico, conversão e regulação de potência, proteções elétricas e monitoramento do estado de carga.

#### Requisitos Funcionais (RF) — Energia

| ID | Nome do Requisito | Descrição Comportamental | Prioridade | Critério de Aceitação |
|:---:|---|---|:---:|---|
| **RF-ENE01** | Conversão e Distribuição de Potência | O circuito de energia deve converter a tensão primária da bateria em barramentos elétricos dedicados e desacoplados: linha de potência para os atuadores de tração e linha estável de baixa tensão para a lógica de controle e sensores. | Must have | Fornecimento contínuo de tensão regulada para a eletrônica de controle independentemente das variações de carga dos motores. |
| **RF-ENE02** | Monitoramento de Tensão de Alimentação | O subsistema deve medir continuamente a tensão total do banco de baterias e disponibilizar essa leitura em formato digital para a lógica de processamento e telemetria. | Must have | Leitura de tensão enviada ao sistema de processamento com taxa de amostragem mínima de 1 Hz e erro absoluto ≤ 0,1 V em relação a multímetro calibrado. |
| **RF-ENE03** | Corte Automático por Subtensão | O circuito de monitoramento deve identificar a queda de tensão abaixo do limiar crítico de segurança eletroquímica das células e ordenar o desligamento imediato dos atuadores de tração. | Must have | Motores de tração são desativados em menos de 200 ms assim que a tensão atinge o limiar programado (ex.: 3,3 V/célula), protegendo a bateria contra descarga profunda. |
| **RF-ENE04** | Chaveamento Geral Mecânico | O circuito deve conter dispositivo físico de acionamento mecânico (chave liga/desliga) com acesso externo direto, permitindo o isolamento galvânico instantâneo de todas as cargas da bateria. | Must have | Abertura mecânica da chave interrompe o fluxo de corrente do circuito para 0 mA instantaneamente. |

#### Requisitos Não-Funcionais (RNF) — Energia

| ID | Nome do Requisito | Descrição e Métrica Quantitativa | Prioridade | Método de Verificação Laboratorial |
|:---:|---|---|:---:|---|
| **RNF-ENE01** | Autonomia Operacional Contínua | O sistema de alimentação deve sustentar o funcionamento ininterrupto do robô por no mínimo 15 minutos em regime de navegação ativa contínua (velocidade média ≥ 0,15 m/s, sensores e rádio transmitindo a 1 Hz). | Must have | Cronometragem de ensaio contínuo de movimentação em bancada/pista com carga ativa nominal até o limiar de proteção de subtensão. |
| **RNF-ENE02** | Estabilidade da Tensão Lógica sob Transitórios | A linha de alimentação lógica deve manter sua tensão dentro de uma tolerância estrita de ±5% da tensão nominal (ex.: 3,3 V ± 0,165 V), mesmo sob transitórios severos de partida e reversão dos motores. | Must have | Monitoramento osciloscópico da linha de alimentação lógica durante reversão instantânea de sentido de giro dos motores em bancada de testes. |
| **RNF-ENE03** | Proteção por Sobrecorrente | O barramento de alimentação primário deve conter elemento de proteção por sobrecorrente (fusível) dimensionado para interromper a corrente em menos de 500 ms sob condição de curto-circuito antes que ocorram danos térmicos às células ou fiação. | Must have | Ensaio em bancada sob carga controlada e conferência das curvas de corrente/tempo das especificações do fusível integrado. |
| **RNF-ENE04** | Polarização Segura de Alimentação | O conector de acoplamento da bateria à placa de alimentação deve dispor de guia ou trava mecânica assimétrica que torne fisicamente impossível a conexão invertida de polaridade. | Must have | Teste físico de inserção reversa manual com força de até 20 N sem estabelecer contato elétrico entre os terminais. |
| **RNF-ENE05** | Tempo de Recarga da Bateria | O sistema deve permitir a recarga de 100% da capacidade útil da bateria descarregada em tempo inferior a 90 minutos através de carregador/balanceador externo apropriado. | Should have | Cronometragem do ciclo de carga a partir do estado de corte de subtensão até a sinalização de término pelo carregador externo. |

---

### 2.3 Eletrônica (ELE)

O subsistema de Eletrônica engloba a unidade de controle embarcada, os módulos de sensoriamento de obstáculos e odometria, a interface de potência para acionamento motriz e as comunicações de rádio e sinalizações.

#### Requisitos Funcionais (RF) — Eletrônica

| ID | Nome do Requisito | Descrição Comportamental | Prioridade | Critério de Aceitação |
|:---:|---|---|:---:|---|
| **RF-ELE01** | Sensoriamento de Obstáculos e Paredes | O hardware sensorial deve medir a proximidade de superfícies sólidas à frente e nas duas laterais ortogonais do veículo, fornecendo leituras numéricas de distância ao processador central. | Must have | Identificação correta do estado de presença ou ausência de paredes nas três direções (frontal, esquerda e direita) em 100% dos testes estáticos na célula da pista. |
| **RF-ELE02** | Odometria de Deslocamento das Rodas | O circuito eletrônico deve capturar os sinais angulares de rotação gerados pelo movimento de cada roda motriz de forma independente, permitindo o cálculo do deslocamento linear e da velocidade angular. | Must have | Contagem determinística de pulsos por volta sem perda de passos em velocidades lineares de até 0,5 m/s. |
| **RF-ELE03** | Modulação e Acionamento de Potência Motriz | A interface de potência deve receber comandos lógicos do controlador central e convertê-los em acionamento elétrico reversível com modulação contínua de velocidade para os atuadores de tração. | Must have | Resposta linear na velocidade angular das rodas proporcional ao comando de modulação aplicado, em ambos os sentidos de giro. |
| **RF-ELE04** | Transmissão Sem Fio de Telemetria | O subsistema eletrônico deve dispor de canal de comunicação sem fio via radiofrequência capaz de transmitir pacotes de telemetria da corrida para a estação receptora externa em tempo real. | Must have | Recepção íntegra dos pacotes de telemetria no receptor externo durante toda a trajetória do robô na pista de testes. |
| **RF-ELE05** | Sinalização Visual de Modos Operacionais | O circuito deve conter indicadores visuais discretos para sinalizar externamente o estado operacional atual do robô (calibração, pronto, navegando, objetivo alcançado e falha/bateria fraca). | Should have | Ativação dos padrões visuais correspondentes a cada um dos 5 modos operacionais sob comando do controlador. |
| **RF-ELE06** | Sinalização Acústica de Eventos e Alertas | O circuito deve disponibilizar dispositivo sonoro para emissão de bipes acústicos em eventos específicos do sistema (conclusão de inicialização, largada, chegada ao objetivo e alarme de bateria). | Could have | Emissão de alerta audível (nível de pressão sonora ≥ 60 dB a 1 metro) sincronizado com os eventos programados. |

#### Requisitos Não-Funcionais (RNF) — Eletrônica

| ID | Nome do Requisito | Descrição e Métrica Quantitativa | Prioridade | Método de Verificação Laboratorial |
|:---:|---|---|:---:|---|
| **RNF-ELE01** | Taxa de Amostragem do Sensoriamento | A frequência de amostragem e atualização das leituras de distância dos sensores periféricos deve ser igual ou superior a 20 Hz (intervalo máximo entre medições de 50 ms). | Must have | Medição via osciloscópio ou registro temporal em log de amostragem durante varredura contínua. |
| **RNF-ELE02** | Faixa Útil e Tolerância de Detecção | O sistema de sensoriamento de paredes deve operar na faixa útil de 30 mm a 180 mm (cobrindo a folga do corredor de 168 mm) com erro máximo de medição ≤ ±10 mm em condições de iluminação padrão de laboratório (300 a 500 lux). | Must have | Posicionamento estático do robô diante de anteparo plano em distâncias aferidas de 50 mm, 100 mm e 150 mm com régua de precisão. |
| **RNF-ELE03** | Desempenho do Enlace Sem Fio | O enlace sem fio deve manter comunicação estável a uma distância em visada direta de no mínimo 8 metros da pista, com taxa de perda de pacotes inferior a 2,0% e nível de sinal RSSI não inferior a -75 dBm. | Must have | Transmissão contínua de 500 pacotes de telemetria com o robô posicionado no ponto mais distante da pista em relação à estação receptora. |
| **RNF-ELE04** | Construtibilidade e Conexão Estruturada (PCB/Shield) | O hardware final deve ser integralmente integrado em placa de circuito impresso (PCB dedicada ou shield industrial estruturado com trilhas e conectores soldados), sendo estritamente vedado o uso de matrizes de contato (*protoboards*) ou fiações soltas no robô final. | Must have | Inspeção visual direta de engenharia e ensaio de vibração mecânica branda sem desconexões elétricas. |
| **RNF-ELE05** | Imunidade a Ruído Indutivo de Motores | O circuito de controle deve dispor de desacoplamento capacitivo e proteção contra transientes indutivos nos terminais dos atuadores, garantindo zero reinicializações espúrias (*resets*) do processador durante manobras de chaveamento abrupto. | Must have | Execução de 20 reversões instantâneas de sentido dos atuadores em rotação máxima com monitoramento contínuo da linha de reset do microcontrolador. |

---

### 2.4 Software (SFT)

O subsistema de Software é dividido em **Firmware Embarcado** (responsável pelo controle cinemático, mapeamento, algoritmo de navegação e despacho de telemetria) e **Aplicação Web** (responsável pelo recebimento, exibição em tempo real, persistência e consulta do histórico de provas).

#### Requisitos Funcionais (RF) — Software

| ID | Nome do Requisito | Descrição Comportamental | Prioridade | Critério de Aceitação |
|:---:|---|---|:---:|---|
| **RF-SFT01** | Controle e Alinhamento de Trajetória | O software embarcado deve regular dinamicamente a velocidade dos atuadores de tração para manter o robô centralizado no corredor durante retas e realizar rotações ortogonais com erro angular acumulado ≤ ±3°. | Must have | Robô percorre reta de 3 células consecutivas mantendo desvio lateral inferior a 15 mm em relação à linha de centro sem colidir nas laterais. |
| **RF-SFT02** | Mapeamento Topológico das Geometrias Oficiais | O firmware deve registrar dinamicamente na memória a representação matricial do labirinto, atualizando a cada célula visitada as paredes detectadas e as passagens livres com suporte configurável para as três geometrias oficiais do edital: 4x4, 8x4 e 12x4 células. | Must have | A matriz lógica gerada na memória ao final da exploração é 100% idêntica à configuração física real da pista de teste utilizada. |
| **RF-SFT03** | Navegação e Resolução Autônoma de Labirinto | O firmware deve computar trajetórias viáveis a partir do estado atual da exploração e decidir autonomamente os comandos de avanço e curva para conduzir o veículo da célula inicial (0,0) até a área de chegada. | Must have | Conclusão autônoma da corrida com chegada ao objetivo em menos de 3 minutos em pista 4x4 sem intervenção externa. |
| **RF-SFT04** | Reconhecimento de Objetivo e Parada Automática | O sistema deve reconhecer o ingresso do robô na célula ou área objetivo pré-configurada, cessar imediatamente o comando de tração e congelar a contagem do cronômetro da corrida. | Must have | Interrupção da movimentação mecânica em menos de 200 ms após cruzar o limiar de entrada da célula de chegada. |
| **RF-SFT05** | Despacho Contínuo dos Parâmetros de Telemetria | O firmware deve empacotar e transmitir via canal sem fio os 6 parâmetros mandatórios exigidos pelo edital: (1) tipo do labirinto, (2) trajeto percorrido, (3) nível/consumo da bateria, (4) velocidade média, (5) tempo decorrido e (6) status de desafio cumprido. | Must have | Envio periódico dos 6 parâmetros com taxa de despacho de 1 Hz e validação de recebimento na estação receptora. |
| **RF-SFT06** | Dashboard Web de Monitoramento em Tempo Real | A aplicação web deve exibir em interface gráfica interativa a grade do labirinto com paredes reveladas em tempo real, a posição instantânea do robô, o rastro percorrido, a velocidade, a bateria e o cronômetro da prova. | Must have | A interface gráfica reflete a movimentação e atualização das paredes na tela de forma sincronizada com a progressão física do robô na pista. |
| **RF-SFT07** | Persistência Histórica de Corridas | O sistema web deve persistir em banco de dados os dados consolidados e a série temporal de cada corrida concluída ou interrompida, identificada por carimbo temporal e geometria da pista. | Should have | Registro completo da corrida consultável no banco de dados imediatamente após o encerramento da prova. |
| **RF-SFT08** | Consulta e Filtragem Histórica Web | A aplicação web deve fornecer filtros de busca na interface gráfica para consulta e recuperação dos dados de corridas passadas por tipo de labirinto (4x4, 8x4, 12x4) e data de execução. | Should have | Exibição correta da lista e detalhes de telemetria das corridas selecionadas pelos filtros em menos de 2,0 segundos. |
| **RF-SFT09** | Execução de Trajetória Otimizada (*Speed Run*) | Após a fase inicial de exploração, o software deve ser capaz de computar a rota ótima (menor número de células/distância) até o objetivo e percorrê-la em corrida rápida subsequente com velocidade superior à de mapeamento. | Should have | Tempo de percurso na segunda corrida (*speed run*) inferior em pelo menos 20% em relação ao tempo da primeira volta de exploração. |
| **RF-SFT10** | Suavização Contínua de Trajetória em Curva | O algoritmo de trajetória do speed run deve calcular curvas em arco contínuo sem necessidade de frenagem total para rotação estática de 90°, reduzindo a perda de velocidade. | Could have | Execução de curva de 90° em movimento contínuo reduzindo em no mínimo 300 ms o tempo de transição de célula em relação ao giro parado. |
| **RF-SFT11** | Detecção e Tratamento de Travamento Mecânico (*Stall*) | O firmware deve monitorar a correlação entre o comando de tração e a leitura de odometria; se houver comando de motor ativo sem rotação das rodas por mais de 1,0 segundo, o software deve cortar os motores para prevenir danos. | Must have | Corte de potência dos atuadores em ensaio de bloqueio físico voluntário das rodas motrizes em no máximo 1,2 segundo. |

#### Requisitos Não-Funcionais (RNF) — Software

| ID | Nome do Requisito | Descrição e Métrica Quantitativa | Prioridade | Método de Verificação Laboratorial |
|:---:|---|---|:---:|---|
| **RNF-SFT01** | Autonomia Operacional Plena e Não-Intervenção | Uma vez acionada a largada física na pista, o robô deve operar de forma 100% autônoma, sendo estritamente proibida qualquer intervenção humana externa, pilotagem remota, envio manual de instruções ou carregamento de mapas prévios do traçado do labirinto durante a execução da prova. | Must have | Demonstração prática de corrida autônoma sem recepção de sinais de controle externo e auditoria do código-fonte para comprovar ausência de mapas pré-carregados. |
| **RNF-SFT02** | Compatibilidade com as Geometrias Oficiais do Labirinto | O software embarcado e o sistema de telemetria devem ser plenamente compatíveis e capazes de explorar, mapear e exibir os dados para as três geometrias oficiais estipuladas pelo edital: labirintos de 4x4, 8x4 e 12x4 células (células de 180x180 mm, paredes de 12x50 mm e corredores de 168 mm). | Must have | Execução de baterias de testes de navegação autônoma e exibição correta no painel web nas três configurações oficiais (4x4, 8x4 e 12x4). |
| **RNF-SFT03** | Taxa de Atualização e Latência da Telemetria Web | A aplicação web deve receber dados e atualizar os componentes do painel em tempo real com taxa mínima de 1 Hz e latência de exibição ponta a ponta inferior a 1,0 segundo em rede local. | Must have | Comparação por gravação em vídeo de alta taxa de quadros entre a movimentação física do robô na pista e a atualização do componente gráfico no painel web. |
| **RNF-SFT04** | Tempo de Resposta de Consultas ao Histórico | As consultas e recuperação de corridas armazenadas no banco de dados devem apresentar tempo de renderização na interface web inferior a 2,0 segundos para bases com até 500 registros. | Should have | Medição do tempo de resposta (*Time to First Byte* e renderização completa) no console de desenvolvimento (DevTools) do navegador. |
| **RNF-SFT05** | Compatibilidade e Responsividade Desktop | A aplicação web deve ser plenamente funcional e sem distorções visuais nos principais navegadores modernos (Google Chrome e Mozilla Firefox) em resoluções a partir de 1366 x 768 pixels. | Should have | Testes automatizados ou manuais de layout e redimensionamento de janela nos navegadores em resolução 1366x768 e 1920x1080. |
| **RNF-SFT06** | Responsividade para Dispositivos Móveis | A interface do dashboard web deve adaptar seus componentes principais (status, cronômetro e telemetria essencial) para visualização legível em telas verticais de smartphones (largura a partir de 360 px). | Could have | Emulação de viewport móvel (360x640 px a 412x915 px) com conferência de legibilidade e ausência de barra de rolagem horizontal desnecessária. |

---

## 3. Delimitação Explícita de Fora de Escopo (Won't Have)

A tabela abaixo delimita formalmente o que **não faz parte** do escopo do produto neste ciclo de desenvolvimento, prevenindo desvios de escopo (*scope creep*) e assegurando o foco nos objetivos mandatórios da disciplina:

| ID | Item Fora de Escopo | Justificativa e Delimitação Técnica |
|:---:|---|---|
| **WH-01** | Pilotagem Manual ou Teleoperação durante a Prova | O sistema não disponibilizará interface de controle remoto por joystick, teclado ou aplicativo móvel para movimentar o robô na pista. O robô opera com autonomia estrita conforme o requisito mandatório **RNF-SFT01**. |
| **WH-02** | Resolução de Labirinto Completo Padrão IEEE (16x16 células) | O produto é dimensionado e focado nas configurações oficiais do edital (4x4, 8x4 e 12x4 células). O suporte a labirintos 16x16 células está fora do escopo deste semestre. |
| **WH-03** | Sucção Aerodinâmica Ativa (*Fan / Downforce*) | Não será desenvolvido sistema de turbina de sucção para criar efeito solo artificial. A tração apoia-se puramente na física de contato dos pneus com o solo. |
| **WH-04** | Visão Computacional e Transmissão de Imagem/Vídeo | O robô não transportará câmera de vídeo nem realizará processamento visual de imagens. Todo o sensoriamento é pontual e o enlace de rádio é reservado para telemetria numérica. |
| **WH-05** | Recarga sem Fio por Indução (*Wireless Charging*) | A recarga da bateria será feita exclusivamente por desconexão física através do conector polarizado e uso de carregador de bancada externo. |

---

## 4 Matriz de Rastreabilidade de Dependências

A tabela a seguir explicita a cadeia de rastreabilidade entre requisitos com relação de precedência técnica direta:

| Requisito de Origem | Subsistema | Requisito(s) Dependente(s) | Subsistema Dependente | Natureza e Descrição da Dependência |
|:---:|:---:|:---:|:---:|---|
| **RF-EST01** | Estruturas | RF-ELE01, RF-ELE03 | Eletrônica | A estabilidade mecânica do chassi é pré-requisito para alinhamento dos sensores e fixação dos drivers. |
| **RF-EST02** | Estruturas | RF-ELE02, RF-SFT01 | Eletrônica / Software | A tração e o atrito dos pneus condicionam a acurácia da odometria e o controle cinemático. |
| **RF-EST03** | Estruturas | RF-ELE01, RF-SFT02 | Eletrônica / Software | A ancoragem rígida dos suportes de sensores assegura leitura correta das paredes pelo mapeamento. |
| **RF-ENE01** | Energia | RF-ELE01, RF-ELE03 | Eletrônica | A entrega de barramentos regulados de potência e lógica alimenta todos os sensores e atuadores. |
| **RF-ENE02** | Energia | RF-SFT05, RF-SFT06 | Software | A leitura analógica/digital da bateria é empacotada na telemetria e exibida no painel web. |
| **RF-ENE03** | Energia | RF-ELE03 | Eletrônica | O circuito de corte comanda o bloqueio físico imediato dos acionadores dos motores. |
| **RF-ELE01** | Eletrônica | RF-SFT01, RF-SFT02 | Software | Os dados de distância alimentam a correção de curso nas retas e a matriz de paredes. |
| **RF-ELE02** | Eletrônica | RF-SFT01, RF-SFT11 | Software | A odometria fornece o deslocamento linear e angular e permite detectar travamento (*stall*). |
| **RF-ELE03** | Eletrônica | RF-EST02, RF-SFT01 | Estruturas / Software | A conversão de sinais lógicos em potência mecânica movimenta as rodas motrizes. |
| **RF-ELE04** | Eletrônica | RF-SFT06 | Software | O canal de rádio é o meio físico de transmissão dos pacotes para a interface web. |
| **RF-SFT02** | Software | RF-SFT03, RF-SFT09 | Software | O mapa de paredes descobertas é a entrada essencial dos algoritmos de busca e de speed run. |
| **RF-SFT03** | Software | RF-SFT04, RF-SFT05 | Software | A condução autônoma gera o trajeto percorrido e comanda a parada ao atingir o objetivo. |
| **RF-SFT05** | Software | RF-ELE04, RF-SFT06 | Eletrônica / Software | O empacotamento dos 6 parâmetros do edital alimenta a transmissão de rádio e a tela web. |
| **RF-SFT06** | Software | RF-SFT07 | Software | Os dados exibidos na tela são estruturados para consolidação no banco de dados. |
| **RF-SFT07** | Software | RF-SFT08 | Software | O banco de dados persistido viabiliza a execução de consultas e filtros históricos. |