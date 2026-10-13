#include <unity.h>
#include <string.h>
#include <ArduinoJson.h>
#include "TelemetriaCodec.h"
#include "TelemetriaJson.h"
#include "TemporizadorPeriodico.h"

void setUp(void) {}
void tearDown(void) {}

// Pacote do exemplo da seção 6.4 do doc 4.4 (contrato de dados).
static TelemetriaPacket pacoteExemplo() {
    TelemetriaPacket p;
    memset(&p, 0, sizeof(p));
    p.timestamp_ms = 142000;
    p.labirinto_id = LAB_8X4;
    p.status_id = MAPEANDO;
    p.desafio_cumprido = 0;
    p.bateria_pct = 78;
    p.bateria_mv = 7420;
    p.velocidade_atual = 350;  // 35,0 cm/s
    p.velocidade_media = 285;  // 28,5 cm/s
    p.coluna = 1;              // B
    p.linha = 3;               // 4
    p.orientacao = 2;          // LESTE
    p.paredes = 0x06;          // sul + leste
    return p;
}

// ---------------------------------------------------------------- Tamanho / layout
void test_pacote_tem_18_bytes(void) {
    TEST_ASSERT_EQUAL(18, sizeof(TelemetriaPacket));
    TEST_ASSERT_EQUAL(TELEMETRIA_TAMANHO_BYTES, sizeof(TelemetriaPacket));
}

void test_codificar_layout_little_endian(void) {
    TelemetriaPacket p = pacoteExemplo();
    uint8_t buf[18];
    TEST_ASSERT_EQUAL(18, telemetriaCodificar(p, buf, sizeof(buf)));
    const uint8_t esperado[18] = {
        0xB0, 0x2A, 0x02, 0x00,  // 142000 = 0x00022AB0
        0x02,                    // labirinto 8x4
        0x01,                    // MAPEANDO
        0x00,                    // desafio nao cumprido
        78,                      // bateria %
        0xFC, 0x1C,              // 7420 = 0x1CFC
        0x5E, 0x01,              // 350 = 0x015E
        0x1D, 0x01,              // 285 = 0x011D
        1, 3, 2, 0x06};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(esperado, buf, 18);
}

void test_codificacao_igual_a_struct_na_memoria(void) {
    // Robô (ESP32) e PC de teste são little-endian: o codec deve casar com a struct packed.
    TelemetriaPacket p = pacoteExemplo();
    uint8_t buf[18];
    telemetriaCodificar(p, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)&p, buf, 18);
}

// ---------------------------------------------------------------- Ida e volta
void test_ida_e_volta_preserva_todos_os_campos(void) {
    TelemetriaPacket p = pacoteExemplo();
    uint8_t buf[18];
    telemetriaCodificar(p, buf, sizeof(buf));
    TelemetriaPacket q;
    memset(&q, 0xAA, sizeof(q));
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaDecodificar(buf, 18, q));
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)&p, (const uint8_t*)&q, 18);
}

void test_ida_e_volta_velocidades_negativas(void) {
    TelemetriaPacket p = pacoteExemplo();
    p.velocidade_atual = -120;
    p.velocidade_media = -32768;
    uint8_t buf[18];
    telemetriaCodificar(p, buf, sizeof(buf));
    TelemetriaPacket q;
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaDecodificar(buf, 18, q));
    TEST_ASSERT_EQUAL_INT16(-120, q.velocidade_atual);
    TEST_ASSERT_EQUAL_INT16(-32768, q.velocidade_media);
}

void test_ida_e_volta_valores_extremos_validos(void) {
    TelemetriaPacket p = pacoteExemplo();
    p.timestamp_ms = TELEMETRIA_TIMESTAMP_MAX_MS;
    p.labirinto_id = LAB_12X4;
    p.status_id = TRAVADO;
    p.desafio_cumprido = 1;
    p.bateria_pct = 100;
    p.bateria_mv = 65535;
    p.coluna = 3;
    p.linha = 11;
    p.orientacao = 3;
    p.paredes = 0x0F;
    uint8_t buf[18];
    telemetriaCodificar(p, buf, sizeof(buf));
    TelemetriaPacket q;
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaDecodificar(buf, 18, q));
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)&p, (const uint8_t*)&q, 18);
}

// ---------------------------------------------------------------- Erros de codificação
void test_codificar_buffer_pequeno_ou_nulo(void) {
    TelemetriaPacket p = pacoteExemplo();
    uint8_t buf[17];
    TEST_ASSERT_EQUAL(0, telemetriaCodificar(p, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL(0, telemetriaCodificar(p, nullptr, 18));
}

// ---------------------------------------------------------------- Erros de decodificação
void test_decodificar_tamanho_invalido(void) {
    uint8_t buf[19] = {0};
    TelemetriaPacket q;
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_TAMANHO, telemetriaDecodificar(buf, 17, q));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_TAMANHO, telemetriaDecodificar(buf, 19, q));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_TAMANHO, telemetriaDecodificar(buf, 0, q));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_PONTEIRO, telemetriaDecodificar(nullptr, 18, q));
}

// Altera um byte do pacote válido e confere o código de erro.
static TelemetriaResultado decodificaComByte(int indice, uint8_t valor) {
    TelemetriaPacket p = pacoteExemplo();
    uint8_t buf[18];
    telemetriaCodificar(p, buf, sizeof(buf));
    buf[indice] = valor;
    TelemetriaPacket q;
    return telemetriaDecodificar(buf, 18, q);
}

void test_decodificar_rejeita_campos_fora_do_dominio(void) {
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_LABIRINTO, decodificaComByte(4, 0));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_LABIRINTO, decodificaComByte(4, 4));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_STATUS, decodificaComByte(5, 7));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_DESAFIO, decodificaComByte(6, 2));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_BATERIA, decodificaComByte(7, 101));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_POSICAO, decodificaComByte(14, 4));   // coluna
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_POSICAO, decodificaComByte(15, 8));   // linha >= 8 em 8x4
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_ORIENTACAO, decodificaComByte(16, 4));
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_PAREDES, decodificaComByte(17, 0x10));
}

void test_decodificar_rejeita_timestamp_acima_de_10_min(void) {
    TelemetriaPacket p = pacoteExemplo();
    p.timestamp_ms = TELEMETRIA_TIMESTAMP_MAX_MS + 1;
    uint8_t buf[18];
    telemetriaCodificar(p, buf, sizeof(buf));
    TelemetriaPacket q;
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_TIMESTAMP, telemetriaDecodificar(buf, 18, q));
}

void test_linha_valida_depende_da_geometria(void) {
    TelemetriaPacket p = pacoteExemplo();
    p.labirinto_id = LAB_4X4;
    p.linha = 3;
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaValidar(p));
    p.linha = 4;
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_POSICAO, telemetriaValidar(p));
    p.labirinto_id = LAB_12X4;
    TEST_ASSERT_EQUAL(TELEMETRIA_OK, telemetriaValidar(p));
    p.linha = 12;
    TEST_ASSERT_EQUAL(TELEMETRIA_ERRO_POSICAO, telemetriaValidar(p));
}

void test_decodificar_erro_nao_altera_saida(void) {
    TelemetriaPacket q = pacoteExemplo();
    TelemetriaPacket antes = q;
    uint8_t buf[18] = {0};
    TEST_ASSERT_NOT_EQUAL(TELEMETRIA_OK, telemetriaDecodificar(buf, 18, q));
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t*)&antes, (const uint8_t*)&q, 18);
}

// ---------------------------------------------------------------- JSON (contrato EAP 5.3.1.1)
void test_json_exemplo_do_contrato_com_seq_e_rssi(void) {
    TelemetriaPacket p = pacoteExemplo();
    char json[512];
    size_t n = telemetriaParaJson(p, 42, -58, json, sizeof(json));
    TEST_ASSERT_GREATER_THAN(0, n);
    TEST_ASSERT_EQUAL(strlen(json), n);

    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);

    TEST_ASSERT_EQUAL_UINT32(42, doc["seq"].as<uint32_t>());
    TEST_ASSERT_EQUAL_INT(-58, doc["rssi"].as<int>());
    TEST_ASSERT_EQUAL_UINT32(142000, doc["timestamp_ms"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("8x4", doc["labirinto"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("MAPEANDO", doc["status"].as<const char*>());
    TEST_ASSERT_FALSE(doc["desafio_cumprido"].as<bool>());
    TEST_ASSERT_EQUAL_INT(78, doc["bateria_pct"].as<int>());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 7.42f, doc["bateria_volts"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 35.0f, doc["velocidade_atual"].as<float>());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 28.5f, doc["velocidade_media"].as<float>());
    TEST_ASSERT_EQUAL_STRING("B", doc["posicao"]["coluna"].as<const char*>());
    TEST_ASSERT_EQUAL_INT(4, doc["posicao"]["linha"].as<int>());
    TEST_ASSERT_EQUAL_STRING("LESTE", doc["orientacao"].as<const char*>());
    TEST_ASSERT_FALSE(doc["paredes"]["norte"].as<bool>());
    TEST_ASSERT_TRUE(doc["paredes"]["sul"].as<bool>());
    TEST_ASSERT_TRUE(doc["paredes"]["leste"].as<bool>());
    TEST_ASSERT_FALSE(doc["paredes"]["oeste"].as<bool>());
}

void test_json_contem_todos_os_campos_obrigatorios(void) {
    TelemetriaPacket p = pacoteExemplo();
    char json[512];
    TEST_ASSERT_GREATER_THAN(0, telemetriaParaJson(p, 1, -70, json, sizeof(json)));
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
    const char* campos[] = {"seq", "rssi", "timestamp_ms", "labirinto", "status",
                            "desafio_cumprido", "bateria_pct", "bateria_volts",
                            "velocidade_atual", "velocidade_media", "posicao",
                            "orientacao", "paredes"};
    for (size_t i = 0; i < sizeof(campos) / sizeof(campos[0]); i++) {
        TEST_ASSERT_TRUE_MESSAGE(doc[campos[i]].is<JsonVariant>() && !doc[campos[i]].isNull(), campos[i]);
    }
    TEST_ASSERT_FALSE(doc["posicao"]["coluna"].isNull());
    TEST_ASSERT_FALSE(doc["posicao"]["linha"].isNull());
    TEST_ASSERT_EQUAL(15, doc.size() + 2);  // 13 chaves de topo = contrato + seq + rssi
}

void test_json_conversoes_de_cantos(void) {
    TelemetriaPacket p = pacoteExemplo();
    p.labirinto_id = LAB_12X4;
    p.status_id = CONCLUIDO;
    p.desafio_cumprido = 1;
    p.coluna = 3;   // D
    p.linha = 11;   // 12
    p.orientacao = 3;  // OESTE
    p.paredes = 0x09;  // norte + oeste
    p.velocidade_atual = -50;  // -5,0 cm/s
    char json[512];
    TEST_ASSERT_GREATER_THAN(0, telemetriaParaJson(p, 7, -90, json, sizeof(json)));
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING("12x4", doc["labirinto"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("CONCLUIDO", doc["status"].as<const char*>());
    TEST_ASSERT_TRUE(doc["desafio_cumprido"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("D", doc["posicao"]["coluna"].as<const char*>());
    TEST_ASSERT_EQUAL_INT(12, doc["posicao"]["linha"].as<int>());
    TEST_ASSERT_EQUAL_STRING("OESTE", doc["orientacao"].as<const char*>());
    TEST_ASSERT_TRUE(doc["paredes"]["norte"].as<bool>());
    TEST_ASSERT_FALSE(doc["paredes"]["sul"].as<bool>());
    TEST_ASSERT_FALSE(doc["paredes"]["leste"].as<bool>());
    TEST_ASSERT_TRUE(doc["paredes"]["oeste"].as<bool>());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -5.0f, doc["velocidade_atual"].as<float>());
}

void test_json_todos_os_status_e_orientacoes(void) {
    const char* status[] = {"AGUARDANDO", "MAPEANDO", "SPEED_RUN", "COLISAO",
                            "CONCLUIDO", "TIMEOUT", "TRAVADO"};
    for (uint8_t s = 0; s <= 6; s++) TEST_ASSERT_EQUAL_STRING(status[s], telemetriaNomeStatus(s));
    TEST_ASSERT_NULL(telemetriaNomeStatus(7));
    const char* ori[] = {"NORTE", "SUL", "LESTE", "OESTE"};
    for (uint8_t o = 0; o <= 3; o++) TEST_ASSERT_EQUAL_STRING(ori[o], telemetriaNomeOrientacao(o));
    TEST_ASSERT_NULL(telemetriaNomeOrientacao(4));
    TEST_ASSERT_NULL(telemetriaNomeLabirinto(0));
    TEST_ASSERT_NULL(telemetriaLetraColuna(4));
    TEST_ASSERT_EQUAL_STRING("OK", telemetriaNomeResultado(TELEMETRIA_OK));
}

void test_json_rejeita_pacote_invalido_e_buffer_pequeno(void) {
    TelemetriaPacket p = pacoteExemplo();
    char json[512];
    char pequeno[16];
    TEST_ASSERT_EQUAL(0, telemetriaParaJson(p, 1, -60, pequeno, sizeof(pequeno)));
    TEST_ASSERT_EQUAL(0, telemetriaParaJson(p, 1, -60, nullptr, 512));
    TEST_ASSERT_EQUAL(0, telemetriaParaJson(p, 1, -60, json, 0));
    p.bateria_pct = 200;
    TEST_ASSERT_EQUAL(0, telemetriaParaJson(p, 1, -60, json, sizeof(json)));
}

// ---------------------------------------------------------------- Temporizador 1 Hz
void test_temporizador_dispara_a_1_hz(void) {
    TemporizadorPeriodico t(1000);
    TEST_ASSERT_TRUE(t.vencido(5000));    // primeiro disparo imediato
    TEST_ASSERT_FALSE(t.vencido(5500));
    TEST_ASSERT_FALSE(t.vencido(5999));
    TEST_ASSERT_TRUE(t.vencido(6000));
    TEST_ASSERT_FALSE(t.vencido(6001));
    TEST_ASSERT_TRUE(t.vencido(7000));
}

void test_temporizador_sem_deriva_com_atraso_pequeno(void) {
    TemporizadorPeriodico t(1000);
    t.vencido(0);
    TEST_ASSERT_TRUE(t.vencido(1010));   // atrasou 10 ms
    TEST_ASSERT_TRUE(t.vencido(2000));   // próximo continua alinhado em 2000
}

void test_temporizador_nao_dispara_em_rajada_apos_bloqueio(void) {
    TemporizadorPeriodico t(1000);
    t.vencido(0);
    TEST_ASSERT_TRUE(t.vencido(5000));   // laço ficou 5 s bloqueado
    TEST_ASSERT_FALSE(t.vencido(5001));  // sem disparos acumulados
    TEST_ASSERT_TRUE(t.vencido(6000));
}

void test_temporizador_overflow_de_millis(void) {
    TemporizadorPeriodico t(1000);
    TEST_ASSERT_TRUE(t.vencido(0xFFFFFC00UL));
    TEST_ASSERT_FALSE(t.vencido(0xFFFFFF00UL));
    TEST_ASSERT_TRUE(t.vencido((uint32_t)(0xFFFFFC00UL + 1000UL)));  // +1000 ms com wrap para 0
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pacote_tem_18_bytes);
    RUN_TEST(test_codificar_layout_little_endian);
    RUN_TEST(test_codificacao_igual_a_struct_na_memoria);
    RUN_TEST(test_ida_e_volta_preserva_todos_os_campos);
    RUN_TEST(test_ida_e_volta_velocidades_negativas);
    RUN_TEST(test_ida_e_volta_valores_extremos_validos);
    RUN_TEST(test_codificar_buffer_pequeno_ou_nulo);
    RUN_TEST(test_decodificar_tamanho_invalido);
    RUN_TEST(test_decodificar_rejeita_campos_fora_do_dominio);
    RUN_TEST(test_decodificar_rejeita_timestamp_acima_de_10_min);
    RUN_TEST(test_linha_valida_depende_da_geometria);
    RUN_TEST(test_decodificar_erro_nao_altera_saida);
    RUN_TEST(test_json_exemplo_do_contrato_com_seq_e_rssi);
    RUN_TEST(test_json_contem_todos_os_campos_obrigatorios);
    RUN_TEST(test_json_conversoes_de_cantos);
    RUN_TEST(test_json_todos_os_status_e_orientacoes);
    RUN_TEST(test_json_rejeita_pacote_invalido_e_buffer_pequeno);
    RUN_TEST(test_temporizador_dispara_a_1_hz);
    RUN_TEST(test_temporizador_sem_deriva_com_atraso_pequeno);
    RUN_TEST(test_temporizador_nao_dispara_em_rajada_apos_bloqueio);
    RUN_TEST(test_temporizador_overflow_de_millis);
    return UNITY_END();
}
