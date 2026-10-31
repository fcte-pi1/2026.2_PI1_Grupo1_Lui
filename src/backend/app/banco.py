"""Mapeamento SQLAlchemy do schema MySQL Corrida/Leitura."""
import os
from datetime import datetime, timezone

from sqlalchemy import DateTime, ForeignKey, Integer, String, create_engine, select
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column, sessionmaker
from sqlalchemy.pool import StaticPool


class Base(DeclarativeBase):
    pass


class Corrida(Base):
    __tablename__ = "Corrida"

    id: Mapped[int] = mapped_column("idCorrida", Integer, primary_key=True, autoincrement=True)
    inicio: Mapped[datetime | None] = mapped_column(DateTime, nullable=True)
    fim: Mapped[datetime | None] = mapped_column(DateTime, nullable=True)
    tempo_total_ms: Mapped[int] = mapped_column(Integer, default=0, nullable=False)
    labirinto: Mapped[str] = mapped_column(String(8), nullable=False, index=True)
    velocidade_media: Mapped[float] = mapped_column("Velocidade_media", nullable=False, default=0)
    bateria_inicial_pct: Mapped[int] = mapped_column(Integer, nullable=False, default=0)
    bateria_final_pct: Mapped[int] = mapped_column(Integer, nullable=False, default=0)
    status_final: Mapped[str | None] = mapped_column(String(20), nullable=True, index=True)
    desafio_cumprido: Mapped[bool] = mapped_column(Integer, nullable=False, default=False)
    tentativa: Mapped[int | None] = mapped_column(Integer, nullable=True)
    modo: Mapped[str] = mapped_column(String(20), nullable=False, default="EXPLORACAO")
    celulas_visitadas: Mapped[int] = mapped_column(Integer, nullable=False, default=0)


class Leitura(Base):
    __tablename__ = "Leitura"

    id: Mapped[int] = mapped_column("idLeitura", Integer, primary_key=True, autoincrement=True)
    timestamp_ms: Mapped[int] = mapped_column(Integer, nullable=False)
    labirinto: Mapped[str] = mapped_column("Tamanho_Mapa", String(8), nullable=False)
    desafio_cumprido: Mapped[bool] = mapped_column("Desafio_cumprido", Integer, nullable=False)
    bateria_pct: Mapped[int] = mapped_column(Integer, nullable=False)
    bateria_volts: Mapped[float] = mapped_column(nullable=False)
    corrida_id: Mapped[int] = mapped_column(
        "Corrida_idCorrida", ForeignKey("Corrida.idCorrida"), nullable=False, index=True
    )
    velocidade_media: Mapped[float] = mapped_column(nullable=False)
    coluna: Mapped[str] = mapped_column(String(1), nullable=False)
    linha: Mapped[int] = mapped_column(Integer, nullable=False)
    orientacao: Mapped[str] = mapped_column(String(8), nullable=False)
    status: Mapped[str] = mapped_column(String(20), nullable=False)
    velocidade_atual: Mapped[float] = mapped_column(nullable=False)
    parede_norte: Mapped[bool | None] = mapped_column(Integer, nullable=True)
    parede_sul: Mapped[bool | None] = mapped_column("Parede_sul", Integer, nullable=True)
    parede_leste: Mapped[bool | None] = mapped_column(Integer, nullable=True)
    parede_oeste: Mapped[bool | None] = mapped_column(Integer, nullable=True)
    seq: Mapped[int | None] = mapped_column(Integer, nullable=True)
    rssi: Mapped[int | None] = mapped_column(Integer, nullable=True)


def agora_utc() -> datetime:
    return datetime.now(timezone.utc).replace(tzinfo=None)


def criar_banco(url: str | None = None):
    url = url or os.getenv("DATABASE_URL", "sqlite:///./corridas.db")
    options = {"check_same_thread": False} if url.startswith("sqlite") else {}
    pool = StaticPool if url in ("sqlite://", "sqlite:///:memory:") else None
    engine = create_engine(url, connect_args=options, pool_pre_ping=True, poolclass=pool)
    return engine, sessionmaker(engine, expire_on_commit=False)


def iniciar_esquema(engine, sessoes):
    # create_all prepara o banco vazio; alterações em bancos existentes são feitas
    # pelas migrações versionadas, nunca por DDL implícito em produção.
    Base.metadata.create_all(engine)
    agora = agora_utc()
    with sessoes.begin() as sessao:
        abertas = sessao.scalars(select(Corrida).where(Corrida.fim.is_(None))).all()
        for corrida in abertas:
            corrida.status_final = "INTERROMPIDA"
            corrida.fim = agora
