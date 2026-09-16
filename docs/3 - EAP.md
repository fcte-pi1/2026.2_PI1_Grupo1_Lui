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
        N2 --> N2_1["2.1 Chassi e Mecânica do Robô"]:::estH
        N2 --> N2_2["2.2 Labirinto"]:::estH
        N2_1 --> N2_1_1["2.1.1 Especificação de Materiais, Balanço de Massa e Interfaces"]:::est
        N2_1 --> N2_1_2["2.1.2 Modelagem 3D do Chassi, Para-choque e Acesso Rápido"]:::est
        N2_1 --> N2_1_3["2.1.3 Sistema de Tração Mecânica e Rodagem"]:::est
        N2_1 --> N2_1_4["2.1.4 Simulação Numérica Estrutural (FEA)"]:::est
        N2_1 --> N2_1_5["2.1.5 Fabricação dos Componentes Estruturais"]:::est
        N2_1 --> N2_1_6["2.1.6 Montagem e Integração Mecânica do Chassi"]:::est
        N2_1 --> N2_1_7["2.1.7 Ensaios Físicos Estruturais"]:::est
        N2_2 --> N2_2_1["2.2.1 Design e Planta Executiva"]:::est
        N2_2 --> N2_2_2["2.2.2 Fabricação"]:::est
        N2_2 --> N2_2_3["2.2.3 Validação Dimensional e Ensaios da Pista"]:::est

        %% 3. Firmware / Eletrônica (Verde)
        N3 --> N3_1["3.1 Hardware"]:::eleH
        N3 --> N3_2["3.2 Software Embarcado"]:::eleH
        N3_1 --> N3_1_1["3.1.1 Sensoriamento"]:::ele
        N3_1 --> N3_1_2["3.1.2 Atuação e Acionamento de Potência"]:::ele
        N3_1 --> N3_1_3["3.1.3 Esquemático Elétrico e Placa de Circuito Impresso (PCB)"]:::ele
        N3_1 --> N3_1_4["3.1.4 Interface de Usuário e Sinalização (IHM)"]:::ele
        N3_1 --> N3_1_5["3.1.5 Testes de Hardware"]:::ele
        N3_2 --> N3_2_1["3.2.1 Telemetria"]:::ele
        N3_2 --> N3_2_2["3.2.2 Microcontrolador"]:::ele

        %% 4. Sistema Energético (Vermelho)
        N4 --> N4_1["4.1 Circuito Elétrico"]:::ene
        N4 --> N4_2["4.2 Bateria"]:::ene
        N4 --> N4_3["4.3 Dispositivo de Chaveamento Geral"]:::ene
        N4 --> N4_4["4.4 Motor Elétrico"]:::ene
        N4 --> N4_5["4.5 Conexão e Cabeamento dos Motores"]:::ene
        N4 --> N4_6["4.6 Sistema Térmico"]:::ene
        N4 --> N4_7["4.7 Testes de Energia"]:::ene

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
        N2 --> N2_1["2.1 Chassi e Mecânica do Robô"]:::estH
        N2 --> N2_2["2.2 Labirinto"]:::estH

        N2_1 --> N2_1_1["2.1.1 Especificação de Materiais, Balanço de Massa e Interfaces"]:::est
        N2_1 --> N2_1_2["2.1.2 Modelagem 3D do Chassi, Para-choque e Acesso Rápido"]:::est
        N2_1 --> N2_1_3["2.1.3 Sistema de Tração Mecânica e Rodagem"]:::est
        N2_1 --> N2_1_4["2.1.4 Simulação Numérica Estrutural (FEA)"]:::est
        N2_1 --> N2_1_5["2.1.5 Fabricação dos Componentes Estruturais"]:::est
        N2_1 --> N2_1_6["2.1.6 Montagem e Integração Mecânica do Chassi"]:::est
        N2_1 --> N2_1_7["2.1.7 Ensaios Físicos Estruturais"]:::est

        N2_2 --> N2_2_1["2.2.1 Design e Planta Executiva"]:::est
        N2_2 --> N2_2_2["2.2.2 Fabricação"]:::est
        N2_2 --> N2_2_3["2.2.3 Validação Dimensional e Ensaios da Pista"]:::est
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
        N3_1 --> N3_1_2["3.1.2 Atuação e Acionamento de Potência"]:::ele
        N3_1 --> N3_1_3["3.1.3 Esquemático Elétrico e Placa de Circuito Impresso (PCB)"]:::ele
        N3_1 --> N3_1_4["3.1.4 Interface de Usuário e Sinalização (IHM)"]:::ele
        N3_1 --> N3_1_5["3.1.5 Testes de Hardware"]:::ele

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
        N4 --> N4_5["4.5 Conexão e Cabeamento dos Motores"]:::ene
        N4 --> N4_6["4.6 Sistema Térmico"]:::ene
        N4 --> N4_7["4.7 Testes de Energia"]:::ene
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

Compreende a arquitetura mecânica física do robô (chassi, suportes com tolerâncias angulares controladas, sistema de tração/rodagem, para-choque perimetral, mecanismo de acesso rápido à bateria) e a infraestrutura física de testes (labirinto modular e validação dimensional rigorosa da pista).

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **2.1** | Chassi e Mecânica do Robô | Conjunto estrutural completo do robô móvel responsável pela integridade física, rigidez, absorção de impactos, sustentação e fixação de todos os componentes embarcados. | Chassi estrutural integrado, leve e balanceado, pronto para operação dinâmica em pista. |
| **2.1.1** | Especificação de Materiais, Balanço de Massa e Interfaces | Seleção e caracterização técnica dos materiais poliméricos estruturais (PLA/PETG) e processos de conformação do projeto; elaboração do balanço de propriedades mássicas (*mass balance*) e determinação analítica do centro de gravidade (CG) e momentos de inércia do veículo (meta de massa total ≤ 500 g); e definição formal da interface física com a Eletrônica (RF-EST03), incluindo matriz de tolerâncias geométricas para alinhamento angular dos suportes de sensores de distância (variação < 2°). | Matriz técnica de especificação de materiais estruturais, planilha de balanço de massa com localização do centro de gravidade (CG) e documento de especificação de interface física de fixação com limites de tolerância angular (< 2°). |
| **2.1.2** | Modelagem 3D do Chassi, Para-choque e Acesso Rápido | Modelagem computacional CAD 3D paramétrica da base estrutural, suportes dedicados de fixação de componentes, para-choque com cantos arredondados (raio de curvatura perimetral ≥ 1,0 mm) para limitar forças dinâmicas de impacto nas paredes a < 2,0 N (RNF-EST03), e mecanismo ergonômico de encaixe e liberação rápida da bateria para substituição em tempo ≤ 60 s sem desmontagem de PCB ou sensores (RNF-EST05). | Conjunto de modelos CAD 3D parametrizados (.STEP / .STL) e desenhos técnicos cotados contemplando o berço com trava rápida de bateria, para-choque com raio R ≥ 1,0 mm e suportes internos. |
| **2.1.3** | Sistema de Tração Mecânica e Rodagem | Concepção, seleção e dimensionamento mecânico dos elementos físicos de rodagem e apoio (rodas motrizes, pneus de alta aderência, acoplamentos de eixos dos motores, mancais de rolamento e rodas bobas de baixo atrito) para conversão eficiente de torque em deslocamento retilíneo sem patinamento (RF-EST02), com geometria desobstruída para inspeção e limpeza rápida dos pneus em tempo ≤ 120 s (RNF-EST06). | Conjunto de tração física dimensionado e detalhado em CAD, com especificação de compostos de pneu de alto atrito e berço de eixos com acesso direto para limpeza e manutenção. |
| **2.1.4** | Simulação Numérica Estrutural (FEA) | Modelagem computacional e análise numérica estrutural por elementos finitos (FEA) no ambiente CAD/CAE pré-fabricação: avaliação de rigidez estática e torcional do chassi, distribuição de tensões sob aceleração máxima e frenagem, identificação de concentradores de tensão e cálculo dos fatores de segurança estruturais ($FS \ge 1{,}5$), incluindo ciclo de otimização topológica e refinamento do modelo 3D antes da liberação para fabricação física. | Relatório técnico de simulação numérica por elementos finitos (FEA) com mapas de tensões de von Mises, fatores de segurança comprovados e modelo CAD aprovado virtualmente para fabricação. |
| **2.1.5** | Fabricação dos Componentes Estruturais | Processamento e conformação física das peças projetadas (chassi, suportes internos, berço de bateria e para-choque) por manufatura aditiva (impressão 3D FDM) ou usinagem, controle de parâmetros de fabricação (infill, orientação de camadas e espessura de casca), pós-processamento, remoção de suportes e acabamento superficial. | Lote completo de peças estruturais físicas fabricadas, desbastadas e inspecionadas individualmente quanto à ausência de rebarbas e defeitos de manufatura. |
| **2.1.6** | Montagem e Integração Mecânica do Chassi | Procedimento de união física e assentamento mecânico de todos os componentes estruturais fabricados, abrangendo ensaio preliminar de encaixe a seco (*fit test*) entre peças, parafusamento e inserção de fixadores roscados, assentamento dos mancais e do conjunto de rodagem, e validação funcional do mecanismo de retenção e liberação rápida da bateria. | Chassi mecânico completamente montado em ordem de marcha, com alinhamento das partes e acoplamento rígido sem folgas estruturais. |
| **2.1.7** | Ensaios Físicos Estruturais | Protocolo de ensaios laboratoriais e validação experimental de conformidade física do robô construído, conforme o documento [7.1 - Testes de estrutura.md](7.1%20-%20Testes%20de%20estrutura.md): pesagem real em balança de precisão contra a meta de massa, medições com paquímetro do envelope externo (≤ 165 x 165 mm e diagonal ≤ 160 mm) e vão livre (≥ 2,0 mm), verificação física de raios perimetrais (≥ 1,0 mm), ensaio de força dinâmica de contato em bancada dinamométrica (< 2,0 N), cronometragem de troca de bateria (≤ 60 s) e inspeção do alinhamento angular dos sensores pós-ensaio (< 2°). | Relatório técnico de ensaios físicos laboratoriais com evidências fotográficas das peças isoladas e carro montado, registros de medições e laudo formal de conformidade com os requisitos RF-EST01/02/03 e RNF-EST01 a RNF-EST06. |
| **2.2** | Labirinto | Infraestrutura física modular da arena oficial de testes para calibração sensorial, ensaios de tração e homologação de corridas autônomas. | Pista física montada com módulos de piso plano e paredes ortogonais intercambiáveis nas configurações 4x4, 8x4 e 12x4 células. |
| **2.2.1** | Design e Planta Executiva | Projeto dimensional completo e detalhamento executivo da modulação de células (180 x 180 mm), encaixes de pinos, espessura e altura de paredes (12 x 50 mm) e largura livre de corredores (168 mm). | Planta executiva técnica cotada em CAD com especificações de tolerâncias e desenhos de montagem para pistas 4x4, 8x4 e 12x4. |
| **2.2.2** | Fabricação | Construção física, corte e acabamento superficial dos módulos de piso (preto fosco antirreflexo) e paredes (brancas com topo vermelho) respeitando as diretrizes oficiais de ensaio. | Conjunto físico modular de pisos, paredes e postes de sustentação usinados e pintados para composição flexível da arena. |
| **2.2.3** | Validação Dimensional e Ensaios da Pista | Controle metrológico e inspeção rigorosa das dimensões do labirinto montado em relação à planta executiva, validando a largura livre crítica do corredor de 168 ± 2 mm (parâmetro compartilhado vital para RNF-EST01, RNF-ELE02 e RNF-SFT02), a ortogonalidade dos cruzamentos e a planicidade do solo. | Relatório de controle metrológico da pista com mapa de medições nas três configurações oficiais (4x4, 8x4 e 12x4), atestando a conformidade dos corredores para operação segura do robô e dos sensores. |

---

### 3. Firmware / Eletrônica

Engloba a instrumentação sensorial (distância e orientação inercial), atuadores, placa de circuito impresso e firmware embarcado de controle e telemetria.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **3.1** | Hardware | Conjunto de subsistemas eletroeletrônicos físicos integrados para sensoriamento, locomoção e processamento embarcado. | Circuito eletrônico integrado com sensores, controladores de motor e microcontrolador em funcionamento. |
| **3.1.1** | Sensoriamento | Módulos e transdutores de medição de distância às paredes (sensores ópticos/ToF frontais e laterais) e sensor inercial/giroscópio para auxílio no controle de orientação angular e prevenção de colisão. | Conjunto integrado e calibrado de sensores de distância e giroscópio (IMU) com cablagem, fixação e condicionamento de sinais validados. |
| **3.1.2** | Atuação e Acionamento de Potência | Subsistema eletroeletrônico de potência e sensoriamento de rotação, integrando os drivers de potência (ponte H com acionamento PWM), condicionamento elétrico para os motores DC, leitura determinística dos sinais dos encoders de odometria angular (RF-ELE02, RF-ELE03) e controle de velocidade em malha fechada. | Placa/circuito de drivers de potência com acionamento PWM bidirecional, encoders lendo quadratura sem perda de passos e malha de controle de velocidade operacional integrada ao firmware. |
| **3.1.3** | Esquemático Elétrico e Placa de Circuito Impresso (PCB) | Elaboração do diagrama de blocos funcional e esquemático elétrico com pinagens e simbologia padronizada (conforme documento [4.3 - Projeto conceitual de hardware.md](4.3%20-%20Projeto%20conceitual%20de%20hardware.md)); projeto CAD de layout e roteamento das trilhas de potência e lógica com desacoplamento capacitivo, planos de terra separados e área de isolamento da antena RF; geração de arquivos industriais Gerber; e fabricação física, montagem e soldagem de circuitos integrados e componentes auxiliares (RNF-ELE04, RNF-ELE05). | Diagrama de blocos, esquemático elétrico detalhado, arquivos de manufatura Gerber e placa física industrial montada com componentes soldados e validados nos pontos de teste elétrico (*test points*). |
| **3.1.4** | Interface de Usuário e Sinalização (IHM) | Concepção, montagem e integração dos circuitos de sinalização visual por LEDs discretos para os 5 estados operacionais (calibração, pronto, navegando, objetivo alcançado e falha/bateria fraca — RF-ELE05), dispositivo acústico de alerta (buzzer piezoelétrico ≥ 60 dB para bipes de evento — RF-ELE06) e botões táteis (*push buttons* / *dip switches*) de disparo de largada física e seleção de geometria da arena (4x4, 8x4, 12x4 — RNF-SFT01). | Circuito de interface homem-máquina (IHM) integrado à placa com LEDs indicadores de status, alarme sonoro buzzer e botões de comando físico testados e funcionais. |
| **3.1.5** | Testes de Hardware | Ensaios laboratoriais em bancada para validação funcional do microcontrolador (ESP32), calibração de sensores ópticos/inerciais (taxa ≥ 20 Hz e faixa 30-180 mm), resposta de acionamento dos atuadores, sinalização visual/sonora e comunicação de dados (documento [7.3 - Testes de hardware.md](7.3%20-%20Testes%20de%20hardware.md)). | Relatório de ensaios de hardware com registros de sinais osciloscópicos, validação de amostragem de sensores, resposta dos drivers e laudo de conformidade com os requisitos RF-ELE01 a RF-ELE06 e RNF-ELE01 a RNF-ELE05. |
| **3.2** | Software Embarcado | Camada de software de baixo nível responsável pela interface direta com o hardware, tratamento de interrupções, temporizadores e despacho de telemetria. | Código-fonte C/C++ dos drivers de baixo nível (*Board Support Package*) e rotinas de temporização e amostragem de dados. |
| **3.2.1** | Telemetria | Protocolo de comunicação serial/sem fio e rotinas de empacotamento determinístico dos parâmetros operacionais do robô para envio em tempo real. | Pacote de firmware transmissor de telemetria via canal sem fio a 1 Hz com estrutura de dados validada. |
| **3.2.2** | Microcontrolador | Configuração de relógio, temporizadores, interrupções (ISR), conversores analógico-digitais (ADC) e interfaces de comunicação serial/barramentos do chip. | Código de inicialização (*BSP*) e rotinas de configuração de registradores e periféricos do chip operacional. |

---

### 4. Sistema Energético

Responsável pelo armazenamento de energia, conversão e regulação elétrica, chaveamento de segurança, alimentação de atuadores e dissipação térmica do robô.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **4.1** | Circuito Elétrico | Arquitetura elétrica global de distribuição, filtragem, desacoplamento de barramentos de potência e lógica, e proteções elétricas do sistema. | Diagrama unifilar e esquemático da distribuição de potência com fusíveis, barramentos regulados e circuito de distribuição montado e testado. |
| **4.2** | Bateria | Conjunto de células eletroquímicas (LiPo/Li-Ion) dimensionadas para fornecer a corrente de partida e autonomia exigida pelo veículo. | Banco de baterias com conector polarizado assimétrico, sistema de carregamento balanceado com monitoramento de tensão integrado. |
| **4.3** | Dispositivo de Chaveamento Geral | Acionamento e interrupção da alimentação geral do robô. | Chave geral instalada, integrada ao circuito e capaz de interromper ou liberar a alimentação elétrica do robô. |
| **4.4** | Motor Elétrico | Unidades de conversão eletromecânica de potência para propulsão do micromouse. | Par de motores DC com redução devidamente selecionados, testados e caracterizados eletricamente. |
| **4.5** | Conexão e Cabeamento dos Motores | Confecção do chicote elétrico de potência dos motores, dimensionamento de fiação e bitola (AWG), conectores de engate rápido com isolamento térmico e filtros de supressão de ruído indutivo. | Chicote elétrico de alimentação dos motores montado, testado quanto à continuidade e isolamento elétrico, e conectado com segurança à placa de potência. |
| **4.6** | Sistema Térmico | Análise do perfil de aquecimento e estratégias de dissipação passiva de calor dos componentes de potência. | Sistema de arrefecimento passivo (dissipadores térmicos e convecção pelo chassi) e cálculo de transferência térmica. |
| **4.7** | Testes de Energia | Ensaios de bancada e medições elétricas de consumo por subsistema, autonomia sob carga contínua, eficiência do chaveamento mecânico e comportamento térmico dos componentes (documento [7.2 - Testes de energia.md](7.2%20-%20Testes%20de%20energia.md)). | Relatório de ensaios energéticos com curvas de descarga da bateria, perfil de consumo de corrente e conformidade térmica. |

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
| **5.3.1** | Integração do Sistema | Integração lógica e digital entre o firmware de navegação autônoma/telemetria sem fio e a aplicação web de visualização, assegurando o tráfego contínuo dos pacotes de dados a 1 Hz. | Fluxo de dados digital ponta a ponta plenamente funcional entre o microcontrolador do robô e o painel web receptor. |
| **5.3.2** | Testes de Software | Planejamento, codificação e execução de suítes de testes automatizados em nível unitário, integração lógica e testes ponta a ponta (E2E) para as regras de navegação, simulação e componentes da interface web. | Suíte de testes automatizados executável no pipeline (Jest/pytest/Playwright) com relatório de cobertura de código-fonte ≥ 80%, atendendo ao documento de Testes de Software. |

---

### 6. Validação

Conjunto de protocolos, ensaios laboratoriais em bancada e testes em pista real para comprovação do cumprimento dos requisitos multidisciplinares do edital.

| Código | Pacote de Trabalho / Entrega | Descrição do Escopo da Entrega | Entregável Tangível |
|:---:|---|---|---|
| **6.1** | Teste Integrado | Ensaios de bancada multidisciplinares em ambiente controlado para validação de interoperabilidade simultânea entre mecânica, eletrônica, energia e software embarcado sob regime de potência. | Relatório técnico de ensaios integrados com registros osciloscópicos, telemetria e logs de comunicação e erro. |
| **6.2** | Teste em Labirinto | Baterias formais de corridas autônomas nas pistas físicas oficiais (4x4, 8x4 e 12x4 células). | Relatório de validação em pista com vídeos de evidência, tempos cronometrados e mapas reconstruídos. |

---

## 3. Matriz de Rastreabilidade com os Requisitos

A tabela abaixo sintetiza a correspondência entre os macro-pacotes da EAP e os requisitos de engenharia definidos no documento de requisitos:

| Macro-Pacote da EAP | Subsistemas Atendidos | Requisitos Funcionais (RF) | Requisitos Não-Funcionais (RNF) |
|---|---|---|---|
| **1. Documentação** | Gerência / Todos | Governança de escopo e qualidade | Cumprimento de prazos, orçamento e edital |
| **2. Estrutura** | Estruturas / Labirinto | **RF-EST01** (Fixação/chassi: 2.1.1, 2.1.2, 2.1.6)<br>**RF-EST02** (Tração mecânica e rodagem: 2.1.3)<br>**RF-EST03** (Alinhamento de sensores < 2°: 2.1.1, 2.1.2, 2.1.7) | **RNF-EST01** (Envelope ≤ 160 mm: 2.1.2, 2.1.7, 2.2.3)<br>**RNF-EST02** (Locomoção terrestre: 2.1.3)<br>**RNF-EST03** (Raios ≥ 1 mm e impacto < 2 N: 2.1.2, 2.1.7)<br>**RNF-EST04** (Vão livre ≥ 2 mm: 2.1.2, 2.1.7)<br>**RNF-EST05** (Troca de bateria ≤ 60 s: 2.1.2, 2.1.6, 2.1.7)<br>**RNF-EST06** (Limpeza de rodagem ≤ 120 s: 2.1.3, 2.1.7) |
| **3. Firmware / Eletrônica** | Eletrônica / Embarcado | **RF-ELE01** (Sensoriamento: 3.1.1)<br>**RF-ELE02** (Odometria: 3.1.2)<br>**RF-ELE03** (Acionamento motores/potência: 3.1.2)<br>**RF-ELE04** (Telemetria sem fio: 3.2.1)<br>**RF-ELE05** (Sinalização visual: 3.1.4)<br>**RF-ELE06** (Alarme sonoro: 3.1.4) | **RNF-ELE01** (Amostragem ≥ 20 Hz: 3.1.1, 3.1.5)<br>**RNF-ELE02** (Faixa 30-180 mm: 3.1.1, 3.1.5)<br>**RNF-ELE03** (Alcance rádio ≥ 8 m: 3.2.1)<br>**RNF-ELE04** (Conexão estruturada PCB: 3.1.3)<br>**RNF-ELE05** (Imunidade indutiva: 3.1.2, 3.1.3) |
| **4. Sistema Energético** | Energia | **RF-ENE01** (Distribuição/conversão: 4.1)<br>**RF-ENE02** (Monitoramento tensão: 4.2)<br>**RF-ENE03** (Corte subtensão: 4.1, 4.2)<br>**RF-ENE04** (Chaveamento mecânico: 4.3) | **RNF-ENE01** (Autonomia ≥ 15 min: 4.2, 4.7)<br>**RNF-ENE02** (Tolerância ±5% lógica: 4.1, 4.7)<br>**RNF-ENE03** (Fusível < 500 ms: 4.1)<br>**RNF-ENE04** (Conector assimétrico: 4.2)<br>**RNF-ENE05** (Recarga ≤ 90 min: 4.2) |
| **5. Sistemas de Lógica e Processamento** | Software (Embarcado, Simulação e Web) | **RF-SFT01** a **RF-SFT04** (Navegação autônoma e parada: 5.2.1 a 5.2.3)<br>**RF-SFT05** e **RF-SFT06** (Telemetria e Dashboard: 5.1.1)<br>**RF-SFT07** e **RF-SFT08** (Histórico e Consulta: 5.1.2)<br>**RF-SFT09** e **RF-SFT10** (Speed run e curvas: 5.2.3)<br>**RF-SFT11** (Detecção de stall: 5.2.2)<br>*(Simulador do labirinto: 5.1.3 / Testes automatizados: 5.3.2)* | **RNF-SFT01** (Autonomia plena: 5.2.2)<br>**RNF-SFT02** (Geometrias oficiais 4x4, 8x4, 12x4: 5.1.3, 5.2.2)<br>**RNF-SFT03** (Taxa telemetria ≥ 1 Hz: 5.1.1)<br>**RNF-SFT04** (Tempo consulta < 2 s: 5.1.2)<br>**RNF-SFT05/06** (Responsividade web: 5.1.1, 5.1.2) |
| **6. Validação** | Todos os Subsistemas | Verificação de missão e desafio cumprido (6.1 Teste Integrado e 6.2 Teste em Labirinto) | Cumprimento integral dos critérios mensuráveis do edital e homologação do robô em pista oficial |
