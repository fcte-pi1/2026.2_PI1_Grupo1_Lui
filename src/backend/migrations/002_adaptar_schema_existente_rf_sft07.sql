-- Execute uma vez sobre o schema original enviado pelo grupo.
-- Converte as geometrias para o contrato do firmware (linhas x colunas).
ALTER TABLE `Corrida`
  MODIFY `labirinto` ENUM('4x4', '4x8', '4x12', '8x4', '12x4') NOT NULL;
UPDATE `Corrida` SET `labirinto` = '8x4' WHERE `labirinto` = '4x8';
UPDATE `Corrida` SET `labirinto` = '12x4' WHERE `labirinto` = '4x12';
ALTER TABLE `Corrida`
  MODIFY `labirinto` ENUM('4x4', '8x4', '12x4') NOT NULL,
  MODIFY `inicio` DATETIME(6) NULL,
  MODIFY `fim` DATETIME(6) NULL,
  MODIFY `tempo_total_ms` INT NOT NULL DEFAULT 0,
  MODIFY `Velocidade_media` FLOAT NOT NULL DEFAULT 0,
  MODIFY `bateria_inicial_pct` INT NOT NULL DEFAULT 0,
  MODIFY `bateria_final_pct` INT NOT NULL DEFAULT 0,
  MODIFY `status_final` ENUM('CONCLUIDO', 'COLISAO', 'TIMEOUT', 'TRAVADO', 'INTERROMPIDA') NULL,
  MODIFY `desafio_cumprido` TINYINT NOT NULL DEFAULT 0,
  MODIFY `tentativa` INT NULL,
  MODIFY `modo` ENUM('EXPLORACAO', 'SPEED_RUN') NOT NULL DEFAULT 'EXPLORACAO',
  MODIFY `celulas_visitadas` INT NOT NULL DEFAULT 0;

ALTER TABLE `Leitura`
  MODIFY `Tamanho_Mapa` ENUM('4x4', '4x8', '4x12', '8x4', '12x4') NOT NULL;
UPDATE `Leitura` SET `Tamanho_Mapa` = '8x4' WHERE `Tamanho_Mapa` = '4x8';
UPDATE `Leitura` SET `Tamanho_Mapa` = '12x4' WHERE `Tamanho_Mapa` = '4x12';
ALTER TABLE `Leitura`
  MODIFY `idLeitura` INT NOT NULL AUTO_INCREMENT,
  MODIFY `Tamanho_Mapa` ENUM('4x4', '8x4', '12x4') NOT NULL,
  MODIFY `status` ENUM('MAPEANDO', 'SPEED_RUN', 'CONCLUIDO', 'COLISAO', 'TIMEOUT', 'TRAVADO') NOT NULL,
  ADD COLUMN `parede_norte` TINYINT NULL,
  ADD COLUMN `Parede_sul` TINYINT NULL,
  ADD COLUMN `parede_leste` TINYINT NULL,
  ADD COLUMN `parede_oeste` TINYINT NULL,
  ADD COLUMN `seq` BIGINT NULL,
  ADD COLUMN `rssi` SMALLINT NULL;

-- Leituras antigas não tinham paredes armazenadas; os valores nulos significam
-- desconhecido. Novas leituras recebem as quatro paredes vindas no JSON.
