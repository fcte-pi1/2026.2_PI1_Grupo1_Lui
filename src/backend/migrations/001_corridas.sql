-- Schema inicial RF-SFT07, compatível com app/banco.py.
CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET utf8mb4;
USE `mydb`;

CREATE TABLE IF NOT EXISTS `Corrida` (
  `idCorrida` INT NOT NULL AUTO_INCREMENT,
  `inicio` DATETIME(6) NOT NULL,
  `fim` DATETIME(6) NULL,
  `tempo_total_ms` INT NOT NULL DEFAULT 0,
  `labirinto` ENUM('4x4', '8x4', '12x4') NOT NULL,
  `Velocidade_media` FLOAT NOT NULL DEFAULT 0,
  `bateria_inicial_pct` INT NOT NULL DEFAULT 0,
  `bateria_final_pct` INT NOT NULL DEFAULT 0,
  `status_final` ENUM('CONCLUIDO', 'COLISAO', 'TIMEOUT', 'TRAVADO', 'INTERROMPIDA') NULL,
  `desafio_cumprido` TINYINT NOT NULL DEFAULT 0,
  `tentativa` INT NULL,
  `modo` ENUM('EXPLORACAO', 'SPEED_RUN') NOT NULL DEFAULT 'EXPLORACAO',
  `celulas_visitadas` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`idCorrida`),
  INDEX `ix_Corrida_labirinto` (`labirinto`),
  INDEX `ix_Corrida_status_final` (`status_final`)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `Leitura` (
  `idLeitura` INT NOT NULL AUTO_INCREMENT,
  `timestamp_ms` INT NOT NULL,
  `Tamanho_Mapa` ENUM('4x4', '8x4', '12x4') NOT NULL,
  `Desafio_cumprido` TINYINT NOT NULL,
  `bateria_pct` INT NOT NULL,
  `bateria_volts` FLOAT NOT NULL,
  `Corrida_idCorrida` INT NOT NULL,
  `velocidade_media` FLOAT NOT NULL,
  `coluna` ENUM('A', 'B', 'C', 'D') NOT NULL,
  `linha` INT NOT NULL,
  `orientacao` ENUM('N', 'S', 'L', 'O') NOT NULL,
  `status` ENUM('MAPEANDO', 'SPEED_RUN', 'CONCLUIDO', 'COLISAO', 'TIMEOUT', 'TRAVADO') NOT NULL,
  `velocidade_atual` FLOAT NOT NULL,
  `parede_norte` TINYINT NOT NULL,
  `Parede_sul` TINYINT NOT NULL,
  `parede_leste` TINYINT NOT NULL,
  `parede_oeste` TINYINT NOT NULL,
  `seq` BIGINT NULL,
  `rssi` SMALLINT NULL,
  PRIMARY KEY (`idLeitura`),
  INDEX `ix_Leitura_corrida` (`Corrida_idCorrida`),
  CONSTRAINT `fk_Leitura_Corrida1` FOREIGN KEY (`Corrida_idCorrida`)
    REFERENCES `Corrida` (`idCorrida`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `Celula` (
  `idCelula` INT NOT NULL AUTO_INCREMENT,
  `Corrida_idCorrida` INT NOT NULL,
  `coluna` ENUM('A', 'B', 'C', 'D') NOT NULL,
  `linha` INT NOT NULL,
  `parede_norte` TINYINT NOT NULL,
  `Parede_sul` TINYINT NOT NULL,
  `parede_leste` TINYINT NOT NULL,
  `parede_oeste` TINYINT NOT NULL,
  `visitada_em_ms` INT NULL,
  PRIMARY KEY (`idCelula`),
  INDEX `ix_Celula_corrida` (`Corrida_idCorrida`),
  CONSTRAINT `fk_Celula_Corrida1` FOREIGN KEY (`Corrida_idCorrida`)
    REFERENCES `Corrida` (`idCorrida`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB;
