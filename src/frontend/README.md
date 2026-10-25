# _Frontend_

Esta pasta deverá armazenar arquivos referentes a:

- Código-fonte da interface: componentes, páginas, estilos e lógica de apresentação, organizados conforme o framework utilizado ([React](https://react.dev/), [Vue](https://vuejs.org/), [Angular](https://angular.io/) etc.).
- Arquivos de marcação e estilo estáticos: `index.html`, arquivos `.css`, `.scss` ou `.sass` de estilização global.
- Arquivos de definição de dependências: `package.json` e `package-lock.json` (ou `yarn.lock`) com todas as bibliotecas utilizadas.
- Arquivos de configuração do bundler/toolchain: `vite.config.js`, `webpack.config.js`, `tsconfig.json` etc.
- Arquivos de configuração de ambiente: `.env.example` com as variáveis de ambiente públicas necessárias (ex.: URL base da API).
- Arquivos de containerização: `Dockerfile` e `docker-compose.yml`, caso a aplicação seja servida via contêiner.

## Como Executar

Para instalar as dependências e executar a aplicação localmente:

```bash
# Instalar dependências
npm install

# Iniciar o servidor de desenvolvimento
npm run dev

# Executar os testes unitários
npm test

# Executar os testes uma vez, com relatório de cobertura em coverage/
npm run test:coverage

# Verificar o código com o ESLint
npm run lint
```

O workflow [`frontend-ci.yml`](../../.github/workflows/frontend-ci.yml) roda lint, testes com cobertura e build em todo PR que altere `src/frontend` ou `src/backend` e publica a tabela de cobertura no resumo da execução. Um segundo job roda os testes E2E e publica o relatório HTML do Playwright no artefato `relatorio-e2e`. O formato das mensagens recebidas pelo WebSocket está no [contrato de dados](../README.md#contrato-de-dados).

## Testes E2E (Playwright)

Os testes de `e2e/` abrem o painel no Chromium e conferem os casos do roteiro de testes (doc 4.4, seção 7) que rodam sem o robô: CT-09 nas três geometrias (com as partes de CT-04 e CT-07 que o painel mostra), CT-15, CT-08 parcial e o recomeço do mapa quando a corrida se repete. O Playwright sobe o Vite na porta 4173, e cada teste sobe o seu próprio backend com o simulador acelerado, numa porta livre. Não é preciso deixar nada rodando antes.

Pré-requisito: o ambiente virtual do backend criado em `src\backend\.venv` (veja o [README do backend](../backend/README.md#como-executar)).

No PowerShell, a partir de `src\frontend`:

```powershell
npm install                           # só na primeira vez
npx playwright install chromium       # só na primeira vez: baixa o navegador
npm run test:e2e                      # roda os testes (cerca de 40 s)
npx playwright show-report            # abre o relatório HTML da última execução
npm run test:e2e:evidencias           # roda e salva prints e vídeo em docs\evidencias\5.3.2.1
```

Para rodar um teste só, filtre pelo nome: `npx playwright test -g "CT-15"`. Para ver o navegador durante o teste, acrescente `--headed`. Por padrão os testes usam o Python de `src\backend\.venv`; outro Python pode ser indicado com `$env:E2E_PYTHON = "C:\caminho\python.exe"`.

Os resultados ficam em `playwright-report/` e `test-results/`, que não são versionados.

Evite incluir:

- Dependências instaladas: a pasta `node_modules/` deve ser gerada localmente via `npm install` ou equivalente e nunca incluída no repositório.
- Artefatos de build: diretórios como `dist/`, `build/` ou `.next/` são gerados pelo processo de compilação e não devem ser versionados.
- Arquivos de configuração pessoal: arquivos como `.directory` (Linux/KDE) ou configurações locais de editor (ex.: `.vscode/settings.json`), salvo configurações compartilhadas como `.editorconfig`.
- Credenciais e segredos: arquivos `.env` com valores reais nunca devem ser versionados.

> [!WARNING]
> **Não acrescente arquivos referentes ao _backend_ nesta pasta.** Eles deverão ser armazenados na pasta [backend](https://github.com/fcte-pi1/template/tree/main/src/backend) deste repositório.
