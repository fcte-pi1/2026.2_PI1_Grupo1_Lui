# Backend

Servidor da telemetria em Python 3.11 com FastAPI. Recebe as linhas JSON do gateway pela serial, difunde cada pacote pelo WebSocket e persiste as corridas no MySQL (doc 4.4, seção 4).

O formato das mensagens é o [contrato de dados](../README.md#contrato-de-dados), que está congelado.

## Estado atual

| Parte | Situação |
|:---|:---|
| `GET /health` | Pronto |
| WebSocket `/ws/telemetria` com difusão para todos os clientes | Pronto |
| Validação da mensagem (`TelemetriaMensagem`) | Pronta |
| Ingestão serial e watchdog de enlace (`app/ingestao`) | A implementar |
| Ciclo de vida da corrida e persistência (`app/servicos`, `app/repositorios`) | A implementar (HU-12, #49) |
| API REST do histórico (`/api/corridas`) | A implementar (#142) |

## Estrutura

```text
backend/
├── requirements.txt       # Dependências de execução e de teste
├── pyproject.toml         # Configuração do pytest e da cobertura
├── app/
│   ├── main.py            # criar_app() e a instância usada pelo Uvicorn
│   ├── api/               # GET /health e WebSocket /ws/telemetria
│   ├── ingestao/          # Leitura da porta serial e watchdog
│   ├── servicos/          # Ciclo de vida da corrida
│   ├── repositorios/      # Acesso ao banco
│   └── modelos/           # Esquemas de validação (TelemetriaMensagem)
└── tests/
```

## Como executar

```bash
cd src/backend
python -m venv .venv
.venv\Scripts\activate          # Windows (PowerShell)
source .venv/bin/activate       # Linux e macOS
pip install -r requirements.txt
uvicorn app.main:app --reload
```

- Saúde: <http://127.0.0.1:8000/health>
- Documentação interativa da API: <http://127.0.0.1:8000/docs>
- Telemetria em tempo real: `ws://127.0.0.1:8000/ws/telemetria`

## Testes e cobertura

```bash
pytest --cov=app --cov-report=term-missing
```

O workflow [`backend-ci.yml`](../../.github/workflows/backend-ci.yml) roda esse comando em todo PR que altere `src/backend` e publica a tabela de cobertura no resumo da execução.

## Difundindo uma mensagem

A ingestão serial, quando implementada, deve validar cada linha e repassá-la ao gerenciador de conexões guardado em `app.state`:

```python
from pydantic import ValidationError

from app.modelos import TelemetriaMensagem

try:
    mensagem = TelemetriaMensagem.model_validate_json(linha)
except ValidationError:
    ...  # linha descartada (ruído, diagnóstico do gateway ou campo fora do contrato)
else:
    await app.state.gerenciador.difundir(mensagem)
```

Não versione `.env`, `.venv/`, `__pycache__/` nem os relatórios de cobertura; o `.gitignore` desta pasta já os ignora.
