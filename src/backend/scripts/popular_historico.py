"""Popula o banco com corridas fictícias para medir o histórico (RNF-SFT04).

Uso, a partir de src/backend:
    python -m scripts.popular_historico --url sqlite:///./historico.db --corridas 500
    python -m scripts.popular_historico --url mysql+pymysql://usuario:senha@host/armazena

Apaga as corridas e leituras que já existirem no banco indicado antes de gravar,
por isso nunca aponte para o banco de produção. As corridas alternam entre as três
geometrias, começam a cada 3 horas a partir de 01/09/2026 e cada uma recebe
algumas leituras, para o banco ter o volume de uma base real.
"""
import argparse
from datetime import datetime, timedelta

from sqlalchemy import delete

from app.banco import (
    COLUNAS,
    LABIRINTOS,
    ORIENTACOES,
    Corrida,
    Leitura,
    criar_banco,
    iniciar_esquema,
)

INICIO = datetime(2026, 9, 1, 8, 0, 0)
STATUS = ("CONCLUIDO", "CONCLUIDO", "TRAVADO", "COLISAO", "INTERROMPIDA")


def popular(url: str, total: int = 500, leituras_por_corrida: int = 10) -> int:
    engine, sessoes = criar_banco(url)
    try:
        iniciar_esquema(engine, sessoes)
        with sessoes.begin() as sessao:
            sessao.execute(delete(Leitura))
            sessao.execute(delete(Corrida))
            for i in range(total):
                labirinto = LABIRINTOS[i % len(LABIRINTOS)]
                status = STATUS[i % len(STATUS)]
                duracao_ms = 60_000 + (i % 7) * 15_000
                inicio = INICIO + timedelta(hours=3 * i)
                # Ids explícitos (o banco acabou de ser esvaziado) permitem gravar
                # tudo em lote, sem uma ida ao banco por corrida.
                sessao.add(Corrida(
                    id=i + 1,
                    inicio=inicio,
                    fim=inicio + timedelta(milliseconds=duracao_ms),
                    tempo_total_ms=duracao_ms,
                    labirinto=labirinto,
                    velocidade_media=round(15 + (i % 9) * 0.5, 1),
                    bateria_inicial_pct=95 - i % 10,
                    bateria_final_pct=80 - i % 10,
                    status_final=status,
                    desafio_cumprido=status == "CONCLUIDO",
                    modo="SPEED_RUN" if i % 4 == 3 else "EXPLORACAO",
                    celulas_visitadas=leituras_por_corrida,
                ))
                sessao.add_all(
                    Leitura(
                        timestamp_ms=1000 * (n + 1),
                        labirinto=labirinto,
                        desafio_cumprido=False,
                        bateria_pct=90,
                        bateria_volts=7.9,
                        corrida_id=i + 1,
                        velocidade_media=17.0,
                        coluna=COLUNAS[n % len(COLUNAS)],
                        linha=1 + n % 4,
                        orientacao=ORIENTACOES[n % len(ORIENTACOES)],
                        status="MAPEANDO",
                        velocidade_atual=18.0,
                        parede_norte=n % 2 == 0,
                        parede_sul=n % 3 == 0,
                        parede_leste=False,
                        parede_oeste=True,
                        seq=n + 1,
                        rssi=-60,
                    )
                    for n in range(leituras_por_corrida)
                )
    finally:
        engine.dispose()
    return total


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--url", required=True, help="URL SQLAlchemy do banco de teste")
    parser.add_argument("--corridas", type=int, default=500)
    parser.add_argument("--leituras", type=int, default=10, help="leituras por corrida")
    args = parser.parse_args()
    total = popular(args.url, args.corridas, args.leituras)
    print(f"{total} corridas gravadas")


if __name__ == "__main__":
    main()
