# Firmware Geral Unificado (`src/firmware/geral`)

Combina em um único binário para o ESP32 do robô:
- **Prova autônoma completa** (`CicloProva` + `Navegador` Flood Fill + `ExecutorMovimentoRobo`, idênticos a [`src/firmware/robo`](../robo));
- **Comissionamento e testes de bancada** (sensores ToF, motores manuais, encoders, avanço 1 célula, curvas 90°/180°, bateria, scan I²C/XSHUT e diagnóstico).

## Gravação

```bash
src/backend/.venv/bin/pio run -d src/firmware/geral -e esp32dev -t upload
```

## Comandos (Serial 115200 bps ou ESP-NOW via Gateway / Site)

- **Prova autônoma:**
  - Botão físico (`GPIO 15`): 3 toques trocam a pista (`4x4`/`8x4`/`12x4`); segurar ≥ 2 s e soltar inicia a prova; após concluir a exploração, 2 toques curtos armam o **Speed Run** (`SPEED_RUN` com curvas contínuas pela rota mais curta).
  - `k`: inicia o labirinto remotamente (usado também pelo botão **Resolver** no painel Web e por `scripts/controle_remoto.py`).
  - `f`: arma o **Speed Run** mantendo o mapa conhecido e a rota calculada após a exploração (depois basta enviar `k` ou segurar o botão ≥ 2 s na largada).
  - `g`: alterna a geometria (`4x4` → `8x4` → `12x4`) quando em `AGUARDANDO`.
  - `r`: reinicia para `AGUARDANDO` após estado terminal (`CONCLUIDO`/`TRAVADO`/`COLISAO`/`TIMEOUT`).
  - `s` ou espaço: parada de emergência (freia e trava os motores em prova, ou cancela o teste ativo).
- **Bancada (disponíveis quando fora de corrida ativa):**
  - `1`: leitura contínua dos 3 ToF em mm (`x` sai)
  - `2`: controle manual de motores (`e`/`E`, `d`/`D`, `a`/`A`, `+`/`-`, `s`, `x` sai — compatível com `scripts/teste_motores_remoto.py`)
  - `3`: leitura contínua dos encoders (`z` zera, `x` sai)
  - `4` / `5` / `6` / `7`: avança 1 célula / curva 90° dir / curva 90° esq / meia-volta 180° usando `ExecutorMovimentoRobo`
  - `8`: avanço lento com parada anti-colisão (`<= 100 mm`)
  - `9`: relatório da bateria
  - `0`: telemetria resumida a 10 Hz (`x` sai)
  - `i`: varredura individual dos XSHUT no I²C e reinicialização dos ToF
  - `d`: diagnóstico rápido em 1 linha
  - `w`: dispara pacote ESP-NOW imediatamente
  - `m` ou `h`: reexibe o menu
