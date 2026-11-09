"""O mapeamento de app/banco.py descreve exatamente o schema de migrations/armazena.sql.

Roda só com MYSQL_TEST_URL apontando para um MySQL em que o armazena.sql foi
aplicado (job "Persistência no MySQL" do Backend CI). Foi uma divergência entre
os dois (orientação abreviada contra o ENUM do script) que causou a #142.
"""
import pytest
from sqlalchemy import create_engine, inspect

from app.banco import Base
from tests.conftest import MYSQL_TEST_URL, sem_mysql

pytestmark = [pytest.mark.mysql, sem_mysql]

TABELAS = [tabela.name for tabela in Base.metadata.sorted_tables]


@pytest.fixture(scope="module")
def engine():
    engine = create_engine(MYSQL_TEST_URL)
    yield engine
    engine.dispose()


def _padrao(valor) -> str | None:
    if valor is None:
        return None
    texto = getattr(valor, "arg", valor)
    texto = getattr(texto, "text", texto)
    return str(texto).strip("'")


def _colunas_do_mapeamento(tabela, dialeto):
    return {
        coluna.name: (
            coluna.type.dialect_impl(dialeto).compile(dialect=dialeto),
            coluna.nullable,
            _padrao(coluna.server_default),
        )
        for coluna in Base.metadata.tables[tabela].columns
    }


def _colunas_do_banco(inspetor, tabela, dialeto):
    return {
        coluna["name"]: (
            coluna["type"].compile(dialect=dialeto),
            coluna["nullable"],
            _padrao(coluna["default"]),
        )
        for coluna in inspetor.get_columns(tabela)
    }


@pytest.mark.parametrize("tabela", TABELAS)
def test_colunas_tipos_nulidade_e_padroes_iguais(engine, tabela):
    inspetor = inspect(engine)
    assert _colunas_do_banco(inspetor, tabela, engine.dialect) == _colunas_do_mapeamento(
        tabela, engine.dialect
    )


@pytest.mark.parametrize("tabela", TABELAS)
def test_chaves_e_indices_iguais(engine, tabela):
    inspetor = inspect(engine)
    mapeada = Base.metadata.tables[tabela]
    chave = [coluna.name for coluna in mapeada.primary_key.columns]
    assert inspetor.get_pk_constraint(tabela)["constrained_columns"] == chave

    # O armazena.sql repete a chave primária num UNIQUE INDEX (idMapasP_UNIQUE),
    # que não muda nada na estrutura; só esse caso é desconsiderado.
    indices_banco = {
        (indice["name"], tuple(indice["column_names"]), bool(indice["unique"]))
        for indice in inspetor.get_indexes(tabela)
        if not (indice["unique"] and indice["column_names"] == chave)
    }
    indices_mapeados = {
        (indice.name, tuple(coluna.name for coluna in indice.columns), bool(indice.unique))
        for indice in mapeada.indexes
    }
    assert indices_banco == indices_mapeados

    fks_banco = {
        (fk["name"], tuple(fk["constrained_columns"]), fk["referred_table"],
         tuple(fk["referred_columns"]))
        for fk in inspetor.get_foreign_keys(tabela)
    }
    fks_mapeadas = {
        (fk.name, tuple(c.name for c in fk.columns), fk.referred_table.name,
         tuple(e.column.name for e in fk.elements))
        for fk in mapeada.foreign_key_constraints
    }
    assert fks_banco == fks_mapeadas
