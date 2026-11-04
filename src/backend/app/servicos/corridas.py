"""Ciclo de vida de corrida e persistência de cada pacote recebido."""
from app.banco import Corrida, Leitura, agora_utc
from app.modelos.telemetria import TelemetriaMensagem

TERMINAIS = {"CONCLUIDO", "TRAVADO", "COLISAO", "TIMEOUT", "INTERROMPIDA"}
ORIENTACAO_BANCO = {"NORTE": "N", "SUL": "S", "LESTE": "L", "OESTE": "O"}


class ServicoCorridas:
    def __init__(self, sessoes):
        self.sessoes = sessoes
        self.corrida_ativa_id: int | None = None
        self.labirinto_ativo: str | None = None
        self.celulas_ativas: set[tuple[str, int]] = set()
        self.ultima_corrida_finalizada: tuple[int, str, int] | None = None

    @property
    def tem_corrida_ativa(self):
        return self.corrida_ativa_id is not None

    def processar(self, mensagem: TelemetriaMensagem):
        agora = agora_utc()
        with self.sessoes.begin() as sessao:
            # O firmware sinaliza a largada ao sair de AGUARDANDO para MAPEANDO.
            if mensagem.status == "AGUARDANDO":
                if self.corrida_ativa_id is not None:
                    self._encerrar(sessao, "INTERROMPIDA", agora, mensagem.timestamp_ms)
                self.ultima_corrida_finalizada = None
                return

            terminal_normal = mensagem.status in TERMINAIS - {"INTERROMPIDA"}
            if self.corrida_ativa_id is None and terminal_normal:
                finalizada = self.ultima_corrida_finalizada
                if finalizada and finalizada[1:] == (mensagem.status, mensagem.timestamp_ms):
                    self._adicionar_leitura(sessao, finalizada[0], mensagem, agora)
                    return

            if self.corrida_ativa_id is None or self.labirinto_ativo != mensagem.labirinto:
                if self.corrida_ativa_id is not None:
                    self._encerrar(sessao, "INTERROMPIDA", agora, mensagem.timestamp_ms)
                self.celulas_ativas = {(mensagem.posicao.coluna, mensagem.posicao.linha)}
                corrida = Corrida(
                    inicio=agora,
                    tempo_total_ms=0,
                    labirinto=mensagem.labirinto,
                    velocidade_media=0,
                    bateria_inicial_pct=mensagem.bateria_pct,
                    bateria_final_pct=mensagem.bateria_pct,
                    status_final=None,
                    desafio_cumprido=False,
                    tentativa=None,
                    modo="EXPLORACAO",
                    celulas_visitadas=1,
                )
                sessao.add(corrida)
                sessao.flush()
                self.corrida_ativa_id = corrida.id
                self.labirinto_ativo = mensagem.labirinto
                self.ultima_corrida_finalizada = None

            corrida = sessao.get(Corrida, self.corrida_ativa_id)
            if corrida is None:
                self._limpar_corrida_ativa()
                return
            self.celulas_ativas.add((mensagem.posicao.coluna, mensagem.posicao.linha))
            corrida.tempo_total_ms = max(corrida.tempo_total_ms, mensagem.timestamp_ms)
            corrida.velocidade_media = mensagem.velocidade_media
            corrida.bateria_final_pct = mensagem.bateria_pct
            corrida.desafio_cumprido = mensagem.desafio_cumprido
            corrida.celulas_visitadas = len(self.celulas_ativas)
            corrida.modo = "SPEED_RUN" if mensagem.status == "SPEED_RUN" else "EXPLORACAO"
            self._adicionar_leitura(sessao, corrida.id, mensagem, agora)

            if terminal_normal:
                corrida.status_final = mensagem.status
                corrida.fim = agora
                self.ultima_corrida_finalizada = (
                    corrida.id, mensagem.status, mensagem.timestamp_ms
                )
                self._limpar_corrida_ativa()

    @staticmethod
    def _adicionar_leitura(sessao, corrida_id, mensagem, agora):
        sessao.add(Leitura(
            timestamp_ms=mensagem.timestamp_ms,
            labirinto=mensagem.labirinto,
            desafio_cumprido=mensagem.desafio_cumprido,
            bateria_pct=mensagem.bateria_pct,
            bateria_volts=mensagem.bateria_volts,
            corrida_id=corrida_id,
            velocidade_media=mensagem.velocidade_media,
            coluna=mensagem.posicao.coluna,
            linha=mensagem.posicao.linha,
            orientacao=ORIENTACAO_BANCO[mensagem.orientacao],
            status=mensagem.status,
            velocidade_atual=mensagem.velocidade_atual,
            parede_norte=mensagem.paredes.norte,
            parede_sul=mensagem.paredes.sul,
            parede_leste=mensagem.paredes.leste,
            parede_oeste=mensagem.paredes.oeste,
            seq=mensagem.seq,
            rssi=mensagem.rssi,
        ))

    def interromper(self, status="INTERROMPIDA"):
        if self.corrida_ativa_id is None:
            return
        with self.sessoes.begin() as sessao:
            self._encerrar(sessao, status, agora_utc(), None)

    def _encerrar(self, sessao, status, agora, timestamp_ms):
        corrida = sessao.get(Corrida, self.corrida_ativa_id)
        if corrida is not None:
            corrida.status_final = status
            corrida.fim = agora
            if timestamp_ms is not None:
                corrida.tempo_total_ms = max(corrida.tempo_total_ms, timestamp_ms)
        self._limpar_corrida_ativa()

    def _limpar_corrida_ativa(self):
        self.corrida_ativa_id = None
        self.labirinto_ativo = None
        self.celulas_ativas.clear()
