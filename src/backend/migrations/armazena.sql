SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';
-- Schema armazena
-- Schema armazena
CREATE SCHEMA IF NOT EXISTS armazena DEFAULT CHARACTER SET utf8 ;
USE armazena ;
-- Table armazena.`Corrida`
CREATE TABLE IF NOT EXISTS armazena.`Corrida` (
  idCorrida INT NOT NULL AUTO_INCREMENT,
  inicio DATETIME(6) NULL,
  fim DATETIME(6) NULL,
  tempo_total_ms INT NOT NULL DEFAULT 0,
  labirinto ENUM('4x4', '8x4', '12x4') NOT NULL,
  Velocidade_media FLOAT NOT NULL DEFAULT 0,
  bateria_inicial_pct INT NOT NULL DEFAULT 0, -- é alterada apos recebero o primeiro pacote
  bateria_final_pct INT NOT NULL DEFAULT 0,
  status_final ENUM('CONCLUIDO', 'COLISAO', 'TIMEOUT', 'TRAVADO', 'INTERROMPIDA') NULL,
  desafio_cumprido TINYINT NOT NULL DEFAULT 0,
  tentativa INT NULL,
  modo ENUM('EXPLORACAO', 'SPEED_RUN') NOT NULL DEFAULT 'EXPLORACAO',
  celulas_visitadas INT NOT NULL DEFAULT 0,
  PRIMARY KEY (idCorrida),
  UNIQUE INDEX idMapasP_UNIQUE (idCorrida ASC) VISIBLE,
  INDEX ix_Corrida_labirinto (labirinto ASC) VISIBLE, -- filtro por geometria do histórico (HU-13)
  INDEX ix_Corrida_status_final (status_final ASC) VISIBLE)
ENGINE = InnoDB;
-- Table armazena.`Leitura`
CREATE TABLE IF NOT EXISTS armazena.`Leitura` (
  idLeitura INT NOT NULL AUTO_INCREMENT,
  timestamp_ms INT NOT NULL,
  Tamanho_Mapa ENUM('4x4', '8x4', '12x4') NOT NULL,
  Desafio_cumprido TINYINT NOT NULL,
  bateria_pct INT NOT NULL,
  bateria_volts FLOAT NOT NULL,
  Corrida_idCorrida INT NOT NULL,
  velocidade_media FLOAT NOT NULL,
  coluna ENUM('A', 'B', 'C', 'D') NOT NULL,
  linha INT NOT NULL,
  parede_norte TINYINT NULL,
  Parede_sul TINYINT NULL,
  parede_leste TINYINT NULL,
  parede_oeste TINYINT NULL,
  seq BIGINT NULL,
  rssi SMALLINT NULL,
  orientacao ENUM('NORTE', 'SUL', 'LESTE', 'OESTE') NOT NULL,
  status ENUM('CONCLUIDO', 'COLISAO', 'TIMEOUT', 'SPEED_RUN', 'TRAVADO', 'MAPEANDO') NOT NULL,
  velocidade_atual FLOAT NOT NULL,
  PRIMARY KEY (idLeitura),
  INDEX fk_Leitura_Corrida1_idx (Corrida_idCorrida ASC) VISIBLE,
  CONSTRAINT fk_Leitura_Corrida1
    FOREIGN KEY (Corrida_idCorrida)
    REFERENCES armazena.`Corrida` (idCorrida)
    ON DELETE NO ACTION
    ON UPDATE NO ACTION)
ENGINE = InnoDB;

SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;