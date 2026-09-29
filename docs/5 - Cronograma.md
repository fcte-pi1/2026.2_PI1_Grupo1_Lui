# Cronograma

O cronograma abaixo reflete o planejamento executivo da equipe para o desenvolvimento do Micromouse, desde a concepção até a competição final. Os dados foram estruturados mapeando as dependências lógicas de construção e testes, garantindo o cumprimento de todos os *milestones* (APs) da disciplina.

| **ID** | **Fase** | **Entrega** | **Tarefa** | **Data de Início** | **Data de Fim** | **Responsável** | **Predecessor** | **% de Execução** | **Status** | **_Milestone_** |
|:------:|:------|:--------|:-----------|:-----------------|:----------------|:--------------------|:----------------|:-----------------:|:------------|:--------------|
| **1** | **Concepção** | **AP1 a AP5** | **Fase de Projeto Conceitual e Arquitetura** | **10/08/2026** | **20/09/2026** | **Equipe** | **-** | **100%** | Concluído | **TAP e Arq.** |
| 1.1 | Concepção | AP2 | Elaboração do Termo de Abertura do Projeto (TAP) | 10/08/2026 | 20/08/2026 | Antônio Lucas | - | 100% | Concluído | AP2 |
| 1.2 | Concepção | AP5 | Definição da Arquitetura e Requisitos (Docs 4.1 a 4.4) | 21/08/2026 | 20/09/2026 | Sub-gerentes | 1.1 | 100% | Concluído | AP5 |
| **2** | **Planejamento** | **AP6** | **Planejamento Financeiro e Cronograma** | **21/09/2026** | **30/09/2026** | **Antônio Lucas** | **1.2** | **100%** | Concluído | **AP6** |
| 2.1 | Planejamento | AP6 | Levantamento de custos, BOM e Orçamento final | 21/09/2026 | 29/09/2026 | Antônio Lucas | 1.2 | 100% | Concluído | AP6 |
| 2.2 | Planejamento | AP6 | Estruturação do Cronograma e exportação de tarefas | 28/09/2026 | 30/09/2026 | Antônio Lucas | 2.1 | 100% | Concluído | AP6 |
| **3** | **Manufatura** | **Pré-AP12** | **Aquisição de Materiais e Fabricação** | **01/10/2026** | **15/10/2026** | **Equipe** | **2.1** | **0%** | A Fazer | **-** |
| 3.1 | Manufatura | Interna | Arrecadação de fundos e compra de eletrônicos/bateria | 01/10/2026 | 05/10/2026 | Antônio Lucas | 2.1 | 0% | A Fazer | - |
| 3.2 | Manufatura | Interna | Impressão 3D do chassi e suportes mecânicos | 05/10/2026 | 12/10/2026 | Humberto / João Vitor | 3.1 | 0% | A Fazer | - |
| 3.3 | Manufatura | Interna | Corte e pintura do Labirinto Oficial 4x4 em MDF | 10/10/2026 | 15/10/2026 | Mariana / Carlos | 3.1 | 0% | A Fazer | - |
| **4** | **Validação** | **AP12** | **Testes Unitários de Subsistemas** | **10/10/2026** | **26/10/2026** | **Sub-gerentes** | **3.3** | **0%** | A Fazer | **AP12** |
| 4.1 | Validação | AP12 | Testes de Estrutura (Aferição dimensional e Tração) | 13/10/2026 | 18/10/2026 | Carlos Henrique | 3.2 | 0% | A Fazer | AP12 |
| 4.2 | Validação | AP12 | Testes de Energia (Consumo de corrente e autonomia) | 13/10/2026 | 18/10/2026 | Rafaela Trajano | 3.1 | 0% | A Fazer | AP12 |
| 4.3 | Validação | AP12 | Testes de Hardware (Calibração 3x VL53L0X e ESP-NOW) | 13/10/2026 | 20/10/2026 | Leonardo Augusto | 3.1, 3.3 | 0% | A Fazer | AP12 |
| 4.4 | Validação | AP12 | Testes de Software (Simulador Mock e Cobertura >80%) | 10/10/2026 | 20/10/2026 | Pedro Ian | 1.2 | 0% | A Fazer | AP12 |
| 4.5 | Validação | AP12 | Redação dos Relatórios 7.1 a 7.4 e PR para main | 21/10/2026 | 26/10/2026 | Sub-gerentes | 4.1 a 4.4 | 0% | A Fazer | AP12 |
| **5** | **Integração** | **AP18** | **Montagem Final e Testes em Malha Fechada** | **27/10/2026** | **23/11/2026** | **Equipe** | **4.5** | **0%** | A Fazer | **AP18** |
| 5.1 | Integração | Interna | Acoplamento mecânico e soldagem final no chassi | 27/10/2026 | 02/11/2026 | Rafaela / Carlos | 4.1, 4.2 | 0% | A Fazer | - |
| 5.2 | Integração | Interna | Calibração do PID (Velocidade e controle direcional) | 03/11/2026 | 10/11/2026 | Pedro I. / Leonardo A. | 5.1 | 0% | A Fazer | - |
| 5.3 | Integração | Interna | Integração do algoritmo de labirinto com a telemetria | 11/11/2026 | 18/11/2026 | Pedro Ian | 5.2, 4.4 | 0% | A Fazer | - |
| 5.4 | Integração | AP18 | Teste da movimentação autônoma na Pista 4x4 | 18/11/2026 | 21/11/2026 | Antônio Lucas | 5.3, 3.3 | 0% | A Fazer | AP18 |
| 5.5 | Integração | AP18 | Documentação de Integração e Pull Request final | 21/11/2026 | 23/11/2026 | Antônio Lucas | 5.4 | 0% | A Fazer | AP18 |
| **6** | **Encerramento**| **Final** | **Ajustes de Competição e Apresentação** | **24/11/2026** | **10/12/2026** | **Equipe** | **5.5** | **0%** | A Fazer | **Final** |
| 6.1 | Encerramento | Interna | Refinamento do Speed Run e resolução de bugs finais | 24/11/2026 | 05/12/2026 | Pedro Ian | 5.4 | 0% | A Fazer | - |
| 6.2 | Encerramento | Final | Competição Oficial do Micromouse e Defesa do Projeto | 06/12/2026 | 10/12/2026 | Todos | 6.1 | 0% | A Fazer | Final |