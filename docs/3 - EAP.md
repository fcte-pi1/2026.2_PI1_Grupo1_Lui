# Estrutura Analítica do Projeto (EAP)

A **Estrutura Analítica do Projeto (EAP)**, ou *Work Breakdown Structure (WBS)*, é a ferramenta central de decomposição hierárquica do escopo global do projeto **Micromouse Autônomo (Grupo 1 - PI1 2026.2, FCTE/UnB)**. 

A sua finalidade é desmembrar o trabalho total da equipe multidisciplinar (integrando as engenharias de Software, Eletrônica, Energia, Aeroespacial e Automotiva) em componentes menores e gerenciáveis, denominados **pacotes de trabalho e entregas**, assegurando a rastreabilidade direta com os objetivos do projeto e o cumprimento integral da **Regra dos 100%**.

---

## 1. Diagrama Hierárquico da EAP

A representação gráfica a seguir consolida a estrutura de decomposição do sistema, abrangendo desde a documentação de gerência até os subsistemas físicos, lógicos e de validação em pista.

=== "🌳 Visão Geral (Todas as Branches)"

    ```mermaid
    flowchart LR
        %% Hues Temáticos por Branch
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef docH fill:#bae6fd,stroke:#0284c7,stroke-width:2.5px,color:#0f172a;
        classDef doc fill:#e0f2fe,stroke:#0284c7,stroke-width:1.5px,color:#0f172a;
        classDef estH fill:#fed7aa,stroke:#ea580c,stroke-width:2.5px,color:#0f172a;
        classDef est fill:#ffedd5,stroke:#ea580c,stroke-width:1.5px,color:#0f172a;
        classDef eleH fill:#bbf7d0,stroke:#16a34a,stroke-width:2.5px,color:#0f172a;
        classDef ele fill:#dcfce7,stroke:#16a34a,stroke-width:1.5px,color:#0f172a;
        classDef eneH fill:#fecdd3,stroke:#e11d48,stroke-width:2.5px,color:#0f172a;
        classDef ene fill:#ffe4e6,stroke:#e11d48,stroke-width:1.5px,color:#0f172a;
        classDef logH fill:#e9d5ff,stroke:#9333ea,stroke-width:2.5px,color:#0f172a;
        classDef log fill:#f3e8ff,stroke:#9333ea,stroke-width:1.5px,color:#0f172a;
        classDef valH fill:#99f6e4,stroke:#0d9488,stroke-width:2.5px,color:#0f172a;
        classDef val fill:#ccfbf1,stroke:#0d9488,stroke-width:1.5px,color:#0f172a;

        %% Raiz
        ROOT["Micromouse Autônomo"]:::root

        %% Macro-Pacotes
        ROOT --> N1["1. Documentação"]:::docH
        ROOT --> N2["2. Estrutura"]:::estH
        ROOT --> N3["3. Firmware / Eletrônica"]:::eleH
        ROOT --> N4["4. Sistema Energético"]:::eneH
        ROOT --> N5["5. Sistemas de Lógica e Processamento"]:::logH
        ROOT --> N6["6. Validação"]:::valH

        %% 1. Documentação (Azul)
        N1 --> N1_1["1.1 Relatório"]:::doc
        N1 --> N1_2["1.2 Cronograma"]:::doc
        N1 --> N1_3["1.3 Elicitação"]:::doc
        N1 --> N1_4["1.4 Escopo"]:::doc
        N1 --> N1_5["1.5 Orçamento"]:::doc

        %% 2. Estrutura (Laranja)
        N2 --> N2_1["2.1 Corpo"]:::estH
        N2 --> N2_2["2.2 Labirinto"]:::estH
        N2_1 --> N2_1_1["2.1.1 Montagem"]:::est
        N2_1 --> N2_1_2["2.1.2 Fabricação"]:::est
        N2_1 --> N2_1_3["2.1.3 Modelo 3D do chassis"]:::est
        N2_1 --> N2_1_4["2.1.4 Modelo 3D dos suportes internos"]:::est
        N2_1 --> N2_1_5["2.1.5 Simulações de resistência mecânica"]:::est
        N2_2 --> N2_2_1["2.2.1 Design"]:::est
        N2_2 --> N2_2_2["2.2.2 Fabricação"]:::est

        %% 3. Firmware / Eletrônica (Verde)
        N3 --> N3_1["3.1 Hardware"]:::eleH
        N3 --> N3_2["3.2 Software Embarcado"]:::eleH
        N3_1 --> N3_1_1["3.1.1 Sensoriamento"]:::ele
        N3_1 --> N3_1_2["3.1.2 Sistema Locomotor"]:::ele
        N3_1 --> N3_1_3["3.1.3 Placa de Circuito Impresso (PCB)"]:::ele
        N3_2 --> N3_2_1["3.2.1 Telemetria"]:::ele
        N3_2 --> N3_2_2["3.2.2 Microcontrolador"]:::ele

        %% 4. Sistema Energético (Vermelho)
        N4 --> N4_1["4.1 Circuito Elétrico"]:::ene
        N4 --> N4_2["4.2 Bateria"]:::ene
        N4 --> N4_3["4.3 Dispositivo de Chaveamento Geral"]:::ene
        N4 --> N4_4["4.4 Motor Elétrico"]:::ene
        N4 --> N4_5["4.5 Montagem"]:::ene
        N4 --> N4_6["4.6 Sistema Térmico"]:::ene

        %% 5. Sistemas de Lógica e Processamento (Roxo)
        N5 --> N5_1["5.1 Release 1 - Interface Web"]:::logH
        N5 --> N5_2["5.2 Release 2 - Algoritmo Central e Software Embarcado"]:::logH
        N5 --> N5_3["5.3 Release 3 - Integração e Testes de Software"]:::logH
        N5_1 --> N5_1_1["5.1.1 Dashboard de Monitoramento em Tempo Real"]:::log
        N5_1 --> N5_1_2["5.1.2 Histórico e Consulta de Corridas"]:::log
        N5_1 --> N5_1_3["5.1.3 Simulador do Labirinto"]:::log
        N5_2 --> N5_2_1["5.2.1 Inicialização e Energização do Sistema"]:::log
        N5_2 --> N5_2_2["5.2.2 Navegação Autônoma"]:::log
        N5_2 --> N5_2_3["5.2.3 Desempenho e Otimização"]:::log
        N5_3 --> N5_3_1["5.3.1 Integração do Sistema"]:::log
        N5_3 --> N5_3_2["5.3.2 Testes de Software"]:::log

        %% 6. Validação (Teal)
        N6 --> N6_1["6.1 Teste Integrado"]:::val
        N6 --> N6_2["6.2 Teste em Labirinto"]:::val
    ```

=== "📋 1. Documentação"

    ```mermaid
    flowchart LR
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef docH fill:#bae6fd,stroke:#0284c7,stroke-width:2.5px,color:#0f172a;
        classDef doc fill:#e0f2fe,stroke:#0284c7,stroke-width:1.5px,color:#0f172a;

        ROOT["Micromouse Autônomo"]:::root
        ROOT --> N1["1. Documentação"]:::docH
        N1 --> N1_1["1.1 Relatório"]:::doc
        N1 --> N1_2["1.2 Cronograma"]:::doc
        N1 --> N1_3["1.3 Elicitação"]:::doc
        N1 --> N1_4["1.4 Escopo"]:::doc
        N1 --> N1_5["1.5 Orçamento"]:::doc
    ```

=== "🔧 2. Estrutura"

    ```mermaid
    flowchart LR
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef estH fill:#fed7aa,stroke:#ea580c,stroke-width:2.5px,color:#0f172a;
        classDef est fill:#ffedd5,stroke:#ea580c,stroke-width:1.5px,color:#0f172a;

        ROOT["Micromouse Autônomo"]:::root
        ROOT --> N2["2. Estrutura"]:::estH
        N2 --> N2_1["2.1 Corpo"]:::estH
        N2 --> N2_2["2.2 Labirinto"]:::estH

        N2_1 --> N2_1_1["2.1.1 Montagem"]:::est
        N2_1 --> N2_1_2["2.1.2 Fabricação"]:::est
        N2_1 --> N2_1_3["2.1.3 Modelo 3D do chassis"]:::est
        N2_1 --> N2_1_4["2.1.4 Modelo 3D dos suportes internos"]:::est
        N2_1 --> N2_1_5["2.1.5 Simulações de resistência mecânica"]:::est

        N2_2 --> N2_2_1["2.2.1 Design"]:::est
        N2_2 --> N2_2_2["2.2.2 Fabricação"]:::est
    ```

=== "⚡ 3. Firmware / Eletrônica"

    ```mermaid
    flowchart LR
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef eleH fill:#bbf7d0,stroke:#16a34a,stroke-width:2.5px,color:#0f172a;
        classDef ele fill:#dcfce7,stroke:#16a34a,stroke-width:1.5px,color:#0f172a;

        ROOT["Micromouse Autônomo"]:::root
        ROOT --> N3["3. Firmware / Eletrônica"]:::eleH
        N3 --> N3_1["3.1 Hardware"]:::eleH
        N3 --> N3_2["3.2 Software Embarcado"]:::eleH

        N3_1 --> N3_1_1["3.1.1 Sensoriamento"]:::ele
        N3_1 --> N3_1_2["3.1.2 Sistema Locomotor"]:::ele
        N3_1 --> N3_1_3["3.1.3 Placa de Circuito Impresso (PCB)"]:::ele

        N3_2 --> N3_2_1["3.2.1 Telemetria"]:::ele
        N3_2 --> N3_2_2["3.2.2 Microcontrolador"]:::ele
    ```

=== "🔋 4. Sistema Energético"

    ```mermaid
    flowchart LR
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef eneH fill:#fecdd3,stroke:#e11d48,stroke-width:2.5px,color:#0f172a;
        classDef ene fill:#ffe4e6,stroke:#e11d48,stroke-width:1.5px,color:#0f172a;

        ROOT["Micromouse Autônomo"]:::root
        ROOT --> N4["4. Sistema Energético"]:::eneH
        N4 --> N4_1["4.1 Circuito Elétrico"]:::ene
        N4 --> N4_2["4.2 Bateria"]:::ene
        N4 --> N4_3["4.3 Dispositivo de Chaveamento Geral"]:::ene
        N4 --> N4_4["4.4 Motor Elétrico"]:::ene
        N4 --> N4_5["4.5 Montagem"]:::ene
        N4 --> N4_6["4.6 Sistema Térmico"]:::ene
    ```

=== "💻 5. Sistemas de Lógica e Processamento"

    ```mermaid
    flowchart LR
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef logH fill:#e9d5ff,stroke:#9333ea,stroke-width:2.5px,color:#0f172a;
        classDef log fill:#f3e8ff,stroke:#9333ea,stroke-width:1.5px,color:#0f172a;

        ROOT["Micromouse Autônomo"]:::root
        ROOT --> N5["5. Sistemas de Lógica e Processamento"]:::logH
        N5 --> N5_1["5.1 Release 1 - Interface Web"]:::logH
        N5 --> N5_2["5.2 Release 2 - Algoritmo Central e Software Embarcado"]:::logH
        N5 --> N5_3["5.3 Release 3 - Integração e Testes de Software"]:::logH

        N5_1 --> N5_1_1["5.1.1 Dashboard de Monitoramento em Tempo Real"]:::log
        N5_1 --> N5_1_2["5.1.2 Histórico e Consulta de Corridas"]:::log
        N5_1 --> N5_1_3["5.1.3 Simulador do Labirinto"]:::log

        N5_2 --> N5_2_1["5.2.1 Inicialização e Energização do Sistema"]:::log
        N5_2 --> N5_2_2["5.2.2 Navegação Autônoma"]:::log
        N5_2 --> N5_2_3["5.2.3 Desempenho e Otimização"]:::log

        N5_3 --> N5_3_1["5.3.1 Integração do Sistema"]:::log
        N5_3 --> N5_3_2["5.3.2 Testes de Software"]:::log
    ```

=== "✅ 6. Validação"

    ```mermaid
    flowchart LR
        classDef root fill:#f1f5f9,stroke:#475569,stroke-width:2.5px,color:#0f172a;
        classDef valH fill:#99f6e4,stroke:#0d9488,stroke-width:2.5px,color:#0f172a;
        classDef val fill:#ccfbf1,stroke:#0d9488,stroke-width:1.5px,color:#0f172a;

        ROOT["Micromouse Autônomo"]:::root
        ROOT --> N6["6. Validação"]:::valH
        N6 --> N6_1["6.1 Teste Integrado"]:::val
        N6 --> N6_2["6.2 Teste em Labirinto"]:::val
    ```

---

## 2. Dicionário da EAP (Work Breakdown Structure Dictionary)

O Dicionário da EAP descreve formalmente o escopo e o entregável tangível de cada pacote de trabalho da árvore, garantindo a compreensão inequívoca das entregas de engenharia por todos os membros da equipe e avaliadores.

### 1. Documentação

Gerencia e consolida todo o corpo documental formal do projeto exigido pelas diretrizes da disciplina e boas práticas do PMBOK.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **1.1** | Relatório | Elaboração dos relatórios técnicos e científicos periódicos (parciais e final) do projeto. | Documentos de Relatório Parcial e Relatório Final de Encerramento (formato PDF/Markdown). |
| **1.2** | Cronograma | Planejamento temporal, sequenciamento de atividades, marcos de entrega (*milestones*) e alocação de responsabilidades. | Cronograma estruturado no GitHub Projects e exportado na documentação do repositório. |
| **1.3** | Elicitação | Levantamento e especificação formal dos requisitos funcionais (RF) e não-funcionais (RNF) com todas as áreas de engenharia. | Documento formal de Requisitos com priorização MoSCoW e critérios mensuráveis de aceitação. |
| **1.4** | Escopo | Delimitação das fronteiras do produto, declaração de escopo e exclusões explícitas (*Won't Have*). | Termo de Abertura do Projeto (TAP) e declaração de escopo aprovada. |
| **1.5** | Orçamento | Levantamento detalhado de custos de aquisição de componentes, materiais, fretes e ferramentas, respeitando o teto orçamentário. | Planilha orçamentária detalhada com estimativa de custos versus despesas reais consolidadas. |

---

### 2. Estrutura

Compreende a arquitetura mecânica física do robô (chassi, suportes, rodagem e acoplamentos) e a infraestrutura física de testes (labirinto).

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **2.1** | Corpo | Conjunto estrutural completo do robô móvel responsável pela integridade física e fixação de todos os componentes embarcados. | Chassi estrutural montado com sensores, atuadores e eletrônica acoplados. |
| **2.1.1** | Montagem | Procedimento e ferramental de união mecânica, parafusamento, colagem e acomodação de hardware e bateria no chassi. | Robô fisicamente montado em ordem de marcha sem folgas ou interferências estruturais. |
| **2.1.2** | Fabricação | Processamento e conformação das peças físicas via manufatura aditiva (impressão 3D) ou usinagem/corte laser. | Lote de componentes físicos estruturais fabricados com tolerâncias dimensionais nominais. |
| **2.1.3** | Modelo 3D do chassis | Modelagem computacional em software CAD da base estrutural de sustentação do robô. | Arquivos de modelagem CAD parametrizados (.STEP / .STL) e desenhos técnicos cotados. |
| **2.1.4** | Modelo 3D dos suportes internos | Modelagem computacional dos suportes específicos de sensores ópticos/ultrassônicos, baterias e mancais de motores. | Modelos CAD 3D (.STEP / .STL) dos suportes dedicados de fixação. |
| **2.1.5** | Simulações de resistência mecânica | Análise numérica por elementos finitos (FEA) de esforços mecânicos, vibrações e impactos toleráveis pelo chassi. | Relatório de simulação estrutural com mapas de tensão de von Mises e fatores de segurança. |
| **2.2** | Labirinto | Infraestrutura de pista física modular para ensaios, calibração de sensores e provas do robô. | Pista física de testes com piso plano e paredes ortogonais destacáveis. |
| **2.2.1** | Design | Projeto dimensional, plantas executivas e modulação das células, paredes e pinos de junção do labirinto. | Desenho técnico executivo cotado das células, paredes e traçados (4x4, 8x4 e 12x4). |
| **2.2.2** | Fabricação | Construção e pintura da base do piso (preto fosco com acabamento não reflexivo) e paredes (brancas com topo vermelho). | Conjunto físico completo de piso e paredes modulares para montagem de pistas 4x4, 8x4 e 12x4. |

---

### 3. Firmware / Eletrônica

Engloba a instrumentação sensorial (distância e orientação inercial), atuadores, placa de circuito impresso e firmware embarcado de controle e telemetria.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **3.1** | Hardware | Conjunto de subsistemas eletroeletrônicos físicos integrados para sensoriamento, locomoção e processamento embarcado. | Circuito eletrônico integrado com sensores, controladores de motor e microcontrolador em funcionamento. |
| **3.1.1** | Sensoriamento | Módulos e transdutores de medição de distância às paredes (sensores ópticos/ToF frontais e laterais) e sensor inercial/giroscópio para auxílio no controle de orientação angular e prevenção de colisão. | Conjunto integrado e calibrado de sensores de distância e giroscópio (IMU) com cablagem, fixação e condicionamento de sinais validados. |
| **3.1.2** | Sistema Locomotor | Subsistema eletromecânico e de potência responsável pela tração controlada, integrando drivers de potência (ponte H com acionamento PWM), malha fechada de controle de velocidade e encoders de odometria angular. | Conjunto integrado de acionamento composto por drivers de potência, motores DC com redução acoplados, encoders de contagem de pulsos e malha de controle de velocidade operacional. |
| **3.1.3** | Placa de Circuito Impresso (PCB) | Placa de circuito impresso dedicada para interconexão segura e desacoplada dos componentes, englobando o layout de trilhas de potência e lógica, além da montagem e soldagem de circuitos integrados e componentes auxiliares. | Placa física industrial fabricada, montada e montada com componentes soldados e validados nos pontos de teste elétrico (test points). |
| **3.2** | Software Embarcado | Camada de software de baixo nível responsável pela interface direta com o hardware, tratamento de interrupções, temporizadores e despacho de telemetria. | Código-fonte C/C++ dos drivers de baixo nível (*Board Support Package*) e rotinas de temporização e amostragem de dados. |
| **3.2.1** | Telemetria | Protocolo de comunicação serial/sem fio e rotinas de empacotamento determinístico dos parâmetros operacionais do robô para envio em tempo real. | Pacote de firmware transmissor de telemetria via canal sem fio a 1 Hz com estrutura de dados validada. |
| **3.2.2** | Microcontrolador | Configuração de relógio, temporizadores, interrupções (ISR), conversores analógico-digitais (ADC) e interfaces de comunicação serial/barramentos do chip. | Código de inicialização (*BSP*) e rotinas de configuração de registradores e periféricos do chip operacional. |

---

### 4. Sistema Energético

Responsável pelo armazenamento de energia, conversão e regulação elétrica, chaveamento de segurança, fixação de atuadores e dissipação térmica do robô.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **4.1** | Circuito Elétrico | Arquitetura elétrica global de distribuição, filtragem, desacoplamento de barramentos de potência e lógica, e proteções elétricas do sistema. | Diagrama unifilar e esquemático da distribuição de potência com fusíveis, barramentos regulados e circuito de distribuição montado e testado. |
| **4.2** | Bateria | Conjunto de células eletroquímicas (LiPo/Li-Ion) dimensionadas para fornecer a corrente de partida e autonomia exigida pelo veículo. | Banco de baterias com conector polarizado assimétrico, sistema de carregamento balanceado com monitoramento de tensão integrado. |
| **4.3** | Dispositivo de Chaveamento Geral | Acionamento e interrupção da alimentação geral do robô. | Chave geral instalada, integrada ao circuito e capaz de interromper ou liberar a alimentação elétrica do robô. |
| **4.4** | Motor Elétrico | Unidades de conversão eletromecânica de potência para propulsão do micromouse. | Par de motores DC com redução devidamente selecionados, testados e caracterizados. |
| **4.5** | Montagem | Fixação física dos motores no chassi, acoplamento aos eixos das rodas motrizes e isolamento elétrico. | Conjunto motor-redutor fixado mecanicamente ao chassi e com fiação conectada à placa de potência. |
| **4.6** | Sistema Térmico | Análise do perfil de aquecimento e estratégias de dissipação passiva de calor dos componentes de potência. | Sistema de arrefecimento passivo (dissipadores térmicos e convecção pelo chassi) e cálculo de transferência térmica. |

---

### 5. Sistemas de Lógica e Processamento

Engloba a inteligência computacional do sistema: a plataforma web de telemetria e análise, o simulador virtual do labirinto, os algoritmos autônomos de navegação embarcada e as suítes de testes de software automatizados.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **5.1** | Release 1 - Interface Web | Primeira versão funcional da plataforma web integrada com exibição visual dos dados de corrida, módulo de histórico e ferramenta de simulação virtual. | Aplicação web responsiva com servidor de recepção de telemetria e módulo simulador funcional. |
| **5.1.1** | Dashboard de Monitoramento em Tempo Real | Interface gráfica interativa para recepção e renderização em tempo real da telemetria (grade do labirinto com paredes descobertas, posição instantânea do robô, rastro percorrido, velocidade, cronômetro e nível de bateria). | Painel web frontend com componentes visuais reativos sincronizados em tempo real (taxa ≥ 1 Hz) com os dados recebidos da corrida. |
| **5.1.2** | Histórico e Consulta de Corridas | Módulo de persistência em banco de dados das séries temporais de corridas e interface gráfica com filtros de busca por data, dimensões do labirinto (4x4, 8x4, 12x4) e métricas comparativas. | Módulo de banco de dados e tela de consulta histórica com tabelas e gráficos comparativos de desempenho entre execuções. |
| **5.1.3** | Simulador do Labirinto | Ambiente virtual bidimensional do micromouse e da arena para emulação de sensores e validação precoce dos algoritmos de navegação, permitindo testes sem depender da estrutura física e da pista estarem prontas. | Simulador computacional interativo capaz de carregar labirintos virtuais (4x4, 8x4 e 12x4) e validar trajetórias de busca e resolução de forma autônoma. |
| **5.2** | Release 2 - Algoritmo Central e Software Embarcado | Núcleo de lógica decisória embarcada para exploração, mapeamento matricial e resolução autônoma do labirinto. | Firmware de navegação autônoma compilado e gravado no microcontrolador do robô. |
| **5.2.1** | Inicialização e Energização do Sistema | Rotinas de autoteste (*power-on self-test*), calibração estática dos sensores ópticos/inerciais e confirmação de largada. | Sequência de inicialização e rotina de calibração automática de sensores no firmware com confirmação de prontidão. |
| **5.2.2** | Navegação Autônoma | Implementação do algoritmo de busca e resolução de labirintos (ex.: *Flood Fill* ou seguidor inteligente de paredes com mapas de distância matriciais). | Módulo algorítmico de tomada de decisão, atualização matricial de paredes em memória e cálculo dinâmico de rota. |
| **5.2.3** | Desempenho e Otimização | Algoritmo de rota rápida (*Speed Run*), suavização de curvas ortogonais em movimento contínuo e otimização de tempo de travessia. | Módulo algorítmico de cálculo de trajetória ótima (*Speed Run*) computado após a exploração inicial, reduzindo paradas e tempos de curva. |
| **5.3** | Release 3 - Integração e Testes de Software | Consolidação ponta a ponta do ecossistema de software (embarcado, comunicação sem fio e dashboard web) e validação formal por suítes de testes automatizados. | Versão estável e integrada do software do sistema (embarcado + web) com cobertura de testes documentada e homologada. |
| **5.3.1** | Integração do Sistema | Integração completa do firmware de navegação autônoma e do protocolo de telemetria sem fio com os serviços de ingestão e visualização do dashboard web. | Fluxo de dados ponta a ponta plenamente funcional entre o microcontrolador do robô e a aplicação web. |
| **5.3.2** | Testes de Software | Planejamento, codificação e execução de suítes de testes automatizados em nível unitário, integração lógica e testes ponta a ponta (E2E) para as regras de navegação, simulação e componentes da interface web. | Suíte de testes automatizados executável no pipeline (Jest/pytest/Playwright) com relatório de cobertura de código-fonte ≥ 80%, atendendo ao documento de Testes de Software. |

---

### 6. Validação

Conjunto de protocolos, ensaios laboratoriais em bancada e testes em pista real para comprovação do cumprimento dos requisitos multidisciplinares do edital.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **6.1** | Teste Integrado | Ensaios de bancada e em ambiente controlado para validação de interoperabilidade entre mecânica, eletrônica, energia e software embarcado. | Relatório técnico de ensaios integrados com registros osciloscópicos, telemetria e logs de comunicação e erro. |
| **6.2** | Teste em Labirinto | Baterias formais de corridas autônomas nas pistas físicas oficiais (4x4, 8x4 e 12x4 células). | Relatório de validação em pista com vídeos de evidência, tempos cronometrados e mapas reconstruídos. |

---

## 3. Matriz de Rastreabilidade com os Requisitos

A tabela abaixo sintetiza a correspondência entre os macro-pacotes da EAP e os requisitos de engenharia definidos no documento de requisitos:

| Macro-Pacote da EAP | Subsistemas Atendidos | Requisitos Funcionais (RF) | Requisitos Não-Funcionais (RNF) |
|---|---|---|---|
| **1. Documentação** | Gerência / Todos | Governança de escopo e qualidade | Cumprimento de prazos, orçamento e edital |
| **2. Estrutura** | Estruturas / Labirinto | RF-EST01, RF-EST02, RF-EST03 | RNF-EST01, RNF-EST02, RNF-EST03, RNF-EST04, RNF-EST05, RNF-EST06 |
| **3. Firmware / Eletrônica** | Eletrônica / Embarcado | RF-ELE01, RF-ELE02, RF-ELE03, RF-ELE04, RF-ELE05, RF-ELE06 | RNF-ELE01, RNF-ELE02, RNF-ELE03, RNF-ELE04, RNF-ELE05 |
| **4. Sistema Energético** | Energia | RF-ENE01, RF-ENE02, RF-ENE03, RF-ENE04 | RNF-ENE01, RNF-ENE02, RNF-ENE03, RNF-ENE04, RNF-ENE05 |
| **5. Sistemas de Lógica e Processamento** | Software (Embarcado, Simulação e Web) | RF-SFT01, RF-SFT02, RF-SFT03, RF-SFT04, RF-SFT05, RF-SFT06, RF-SFT07, RF-SFT08, RF-SFT09, RF-SFT10, RF-SFT11 | RNF-SFT01, RNF-SFT02, RNF-SFT03, RNF-SFT04, RNF-SFT05, RNF-SFT06 |
| **6. Validação** | Todos os Subsistemas | Verificação de missão e desafio cumprido | Cumprimento integral dos requisitos de engenharia e edital |
