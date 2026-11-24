# Medidas Físicas do Robô (padronizadas em mm)

> Ao preencher novas medições abaixo, atualize a fonte única em
> [`src/firmware/lib/calibracao/src/CalibracaoRobo.h`](../src/firmware/lib/calibracao/src/CalibracaoRobo.h).

## Chassi e Roda Boba (conferidos)
- `CHASSI_LARGURA_MM = 83.5` (8,35 cm)
- `CHASSI_COMPRIMENTO_MM = 113.5` (11,35 cm)
- `CIRCUNFERENCIA_RODA_BOBA_MM = 50.0` (5 cm)

## Fase 0 — Medidas para Calibração (ROTEIRO_TESTES_BANCADA.md)
- `CAL_RODA_DIAMETRO_MM = 22.0`
- `CAL_BITOLA_MM = 111.0` (referência estimada: `110.0`)
- `LARGURA_EXTERNA_TOTAL_MM = _____` (limite regulamento: `<= 165.0`)
- `X_TOF_FRONTAL_EIXO_MM = _____` (para `CAL_DIST_ENCOSTO_FRONTAL_MM`)
- `DIST_ENTRE_TOFS_LATERAIS_MM = _____` (para `CAL_DIST_IDEAL_PAREDE_LATERAL_MM`)
- `CAL_COMPENSACAO_LARGADA_MM = _____` (referência: `0.0` se eixo centrado em A1)
