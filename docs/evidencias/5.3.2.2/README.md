# Evidências: EAP 5.3.2.2 (Relatório de testes de software, doc 7.4)

Prints gerados em 21/10/2026, sobre o *commit* `60a364a` da `main`, usados no [doc 7.4](../../7.4%20-%20Testes%20de%20software.md).

## Cobertura dos testes unitários e de integração

| Evidência | Arquivo | Resultado |
|:---|:---|:---|
| Relatório gcovr do firmware do robô | [cobertura_firmware_robo.png](cobertura_firmware_robo.png) | 176 testes; 96,5% das linhas, 100% das funções, 90,4% dos ramos |
| Relatório gcovr do firmware do gateway | [cobertura_firmware_gateway.png](cobertura_firmware_gateway.png) | 6 testes; 97,4% das linhas, 100% das funções, 76,8% dos ramos |
| Relatório pytest-cov do backend | [cobertura_backend.png](cobertura_backend.png) | 222 testes; 99% de cobertura total |
| Relatório Vitest do frontend, por pasta | [cobertura_frontend.png](cobertura_frontend.png) | 107 testes; 87,4% das linhas, 82,4% dos ramos, 76,1% das funções |
| Relatório Vitest dos componentes do frontend | [cobertura_frontend_componentes.png](cobertura_frontend_componentes.png) | `Painel.jsx` e indicadores sem teste de componente (0%) |

## Integração contínua (GitHub Actions)

Prints sem login no GitHub: mostram o resultado, os *jobs* e os artefatos de cobertura, mas não a tabela de cobertura do resumo, visível só para usuários autenticados.

| Evidência | Arquivo | Resultado |
|:---|:---|:---|
| Firmware CI, execução #21 (`e79af19`) | [ci_execucao_firmware.png](ci_execucao_firmware.png) | Aprovado |
| Backend CI (`4f9570d`) | [ci_execucao_backend.png](ci_execucao_backend.png) | Aprovado |
| Frontend CI, execução #13 (`d194ed2`) | [ci_execucao_frontend.png](ci_execucao_frontend.png) | Aprovado |

## Painel com o simulador

Backend com `TELEMETRIA_FONTE=simulador` e `SIMULADOR_VELOCIDADE=4`, painel no navegador; print ao fim de cada corrida.

| Evidência | Arquivo | Resultado |
|:---|:---|:---|
| Pista 4x4, cenário `completa` | [painel_simulador_4x4.png](painel_simulador_4x4.png) | Concluído em D4, 00:14 |
| Pista 8x4, cenário `completa` | [painel_simulador_8x4.png](painel_simulador_8x4.png) | Concluído em D8, 00:14 |
| Pista 12x4, cenário `completa` | [painel_simulador_12x4.png](painel_simulador_12x4.png) | Concluído em D12, 00:30 |
| Pista 4x4, cenário `travado` | [painel_simulador_travado_4x4.png](painel_simulador_travado_4x4.png) | Travado em D3, 00:08 |
| Pista 8x4, cenário `travado` | [painel_simulador_travado_8x4.png](painel_simulador_travado_8x4.png) | Travado em B3, 00:08 |
| Pista 12x4, cenário `travado` | [painel_simulador_travado_12x4.png](painel_simulador_travado_12x4.png) | Travado em C8, 00:16 |

## Pendente

- Prints e relatório dos testes E2E com Playwright (issue #143).
- Prints da tabela de cobertura no resumo dos workflows de CI, feitos com login no GitHub.
