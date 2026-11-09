"""Mapeamento SQLAlchemy do schema MySQL Corrida/Leitura.

Espelha migrations/armazena.sql coluna a coluna (tipos, nulidade, padrões e
índices). No MySQL as colunas usam os tipos nativos do script (ENUM, TINYINT,
DATETIME(6)); no SQLite dos testes os ENUMs viram VARCHAR com CHECK, para que um
valor fora do contrato seja recusado também ali. O teste
tests/test_banco_mysql.py compara este mapeamento com o banco criado pelo script.
"""
import os
from datetime import datetime, timezone

from sqlalchemy import (
    BigInteger,
    DateTime,
    Enum,
    Float,
    ForeignKey,
    Index,
    Integer,
    SmallInteger,
    create_engine,
    select,
    text,
)
from sqlalchemy.dialects import mysql
from sqlalchemy.orm import DeclarativeBase, Mapped, mapped_column, sessionmaker
from sqlalchemy.pool import StaticPool

# Valores dos ENUMs de armazena.sql (os mesmos do contrato de telemetria).
LABIRINTOS = ("4x4", "8x4", "12x4")
STATUS_FINAIS = ("CONCLUIDO", "COLISAO", "TIMEOUT", "TRAVADO", "INTERROMPIDA")
MODOS = ("EXPLORACAO", "SPEED_RUN")
COLUNAS = ("A", "B", "C", "D")
ORIENTACOES = ("NORTE", "SUL", "LESTE", "OESTE")
STATUS_LEITURA = ("CONCLUIDO", "COLISAO", "TIMEOUT", "SPEED_RUN", "TRAVADO", "MAPEANDO")

# DATETIME(6) e TINYINT só existem no MySQL; nos outros bancos valem os genéricos.
DATAHORA = DateTime().with_variant(mysql.DATETIME(fsp=6), "mysql")
TINYINT = Integer().with_variant(mysql.TINYINT(), "mysql")


def _enum(*valores: str) -> Enum:
    # Um tipo novo por coluna: o CHECK do SQLite pertence à tabela da coluna.
    return Enum(*valores, native_enum=True, create_constraint=True)


class Base(DeclarativeBase):
    pass


class Corrida(Base):
    __tablename__ = "Corrida"

    id: Mapped[int] = mapped_column("idCorrida", Integer, primary_key=True, autoincrement=True)
    inicio: Mapped[datetime | None] = mapped_column(DATAHORA, nullable=True)
    fim: Mapped[datetime | None] = mapped_column(DATAHORA, nullable=True)
    tempo_total_ms: Mapped[int] = mapped_column(
        Integer, nullable=False, default=0, server_default=text("0")
    )
    labirinto: Mapped[str] = mapped_column(_enum(*LABIRINTOS), nullable=False, index=True)
    velocidade_media: Mapped[float] = mapped_column(
        "Velocidade_media", Float, nullable=False, default=0, server_default=text("0")
    )
    bateria_inicial_pct: Mapped[int] = mapped_column(
        Integer, nullable=False, default=0, server_default=text("0")
    )
    bateria_final_pct: Mapped[int] = mapped_column(
        Integer, nullable=False, default=0, server_default=text("0")
    )
    status_final: Mapped[str | None] = mapped_column(
        _enum(*STATUS_FINAIS), nullable=True, index=True
    )
    desafio_cumprido: Mapped[bool] = mapped_column(
        TINYINT, nullable=False, default=False, server_default=text("0")
    )
    tentativa: Mapped[int | None] = mapped_column(Integer, nullable=True)
    modo: Mapped[str] = mapped_column(
        _enum(*MODOS), nullable=False, default="EXPLORACAO", server_default="EXPLORACAO"
    )
    celulas_visitadas: Mapped[int] = mapped_column(
        Integer, nullable=False, default=0, server_default=text("0")
    )


class Leitura(Base):
    __tablename__ = "Leitura"
    __table_args__ = (Index("fk_Leitura_Corrida1_idx", "Corrida_idCorrida"),)

    id: Mapped[int] = mapped_column("idLeitura", Integer, primary_key=True, autoincrement=True)
    timestamp_ms: Mapped[int] = mapped_column(Integer, nullable=False)
    labirinto: Mapped[str] = mapped_column("Tamanho_Mapa", _enum(*LABIRINTOS), nullable=False)
    desafio_cumprido: Mapped[bool] = mapped_column("Desafio_cumprido", TINYINT, nullable=False)
    bateria_pct: Mapped[int] = mapped_column(Integer, nullable=False)
    bateria_volts: Mapped[float] = mapped_column(Float, nullable=False)
    corrida_id: Mapped[int] = mapped_column(
        "Corrida_idCorrida",
        Integer,
        ForeignKey("Corrida.idCorrida", name="fk_Leitura_Corrida1"),
        nullable=False,
    )
    velocidade_media: Mapped[float] = mapped_column(Float, nullable=False)
    coluna: Mapped[str] = mapped_column(_enum(*COLUNAS), nullable=False)
    linha: Mapped[int] = mapped_column(Integer, nullable=False)
    parede_norte: Mapped[bool | None] = mapped_column(TINYINT, nullable=True)
    parede_sul: Mapped[bool | None] = mapped_column("Parede_sul", TINYINT, nullable=True)
    parede_leste: Mapped[bool | None] = mapped_column(TINYINT, nullable=True)
    parede_oeste: Mapped[bool | None] = mapped_column(TINYINT, nullable=True)
    seq: Mapped[int | None] = mapped_column(BigInteger, nullable=True)
    rssi: Mapped[int | None] = mapped_column(SmallInteger, nullable=True)
    orientacao: Mapped[str] = mapped_column(_enum(*ORIENTACOES), nullable=False)
    status: Mapped[str] = mapped_column(_enum(*STATUS_LEITURA), nullable=False)
    velocidade_atual: Mapped[float] = mapped_column(Float, nullable=False)


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
