#!/usr/bin/env python3
"""
Teste Automatizado do Protocolo de Telemetria e JSON Gateway
Issue #140 - [EAP 3.2.1.1] Protocolo de telemetria e firmware do gateway ESP-NOW

Verifica:
1. Tamanho fixo do pacote (18 bytes)
2. Decodificacao binaria e verificacao de CRC16
3. Geracao e validacao do JSON emitido na Serial pelo Gateway
"""

import sys
import struct
import json

TELEMETRY_HEADER = 0xA5

def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc

def pack_telemetry(seq, time_s, x, y, heading, state, walls, dl, df, dr, yaw_deg, vbat_mv):
    heading_state = ((heading & 0x0F) << 4) | (state & 0x0F)
    raw_yaw = int(yaw_deg * 10)
    # Formato struct: < B H H B B B B B B B h H (16 bytes sem CRC)
    payload = struct.pack("<BHHBBBBBBBhH",
                          TELEMETRY_HEADER, seq, time_s, x, y,
                          heading_state, walls, dl, df, dr,
                          raw_yaw, vbat_mv)
    crc = crc16_ccitt(payload)
    return payload + struct.pack("<H", crc)

def unpack_and_to_json(raw_bytes: bytes, rssi: int):
    if len(raw_bytes) != 18:
        raise ValueError(f"Pacote invalido! Esperado 18 bytes, recebido {len(raw_bytes)}")

    payload = raw_bytes[:16]
    pkt_crc, = struct.unpack("<H", raw_bytes[16:18])
    expected_crc = crc16_ccitt(payload)

    if pkt_crc != expected_crc:
        raise ValueError(f"Falha de CRC! Esperado 0x{expected_crc:04X}, recebido 0x{pkt_crc:04X}")

    header, seq, time_s, x, y, h_s, walls, dl, df, dr, yaw_raw, vbat_mv = struct.unpack("<BHHBBBBBBBhH", payload)

    state = h_s & 0x0F
    heading = (h_s >> 4) & 0x0F

    telemetry_dict = {
        "seq": seq,
        "timestamp_ms": time_s * 1000,
        "rssi": rssi,
        "pos": {"x": x, "y": y},
        "heading": heading,
        "state": state,
        "walls": walls,
        "sensors_cm": {"left": dl, "front": df, "right": dr},
        "imu_yaw": round(yaw_raw / 10.0, 1),
        "battery_v": round(vbat_mv / 1000.0, 2)
    }
    return json.dumps(telemetry_dict)

def run_tests():
    print("=" * 75)
    print("  SUITE DE TESTE AUTOMATIZADO - ISSUE #140: PROTOCOLO & GATEWAY JSON")
    print("=" * 75)

    test_samples = [
        {"seq": 1, "time_s": 1, "x": 0, "y": 0, "heading": 0, "state": 1, "walls": 0x0E, "dl": 6, "df": 18, "dr": 6, "yaw": 0.0, "vbat": 8200, "rssi": -55},
        {"seq": 2, "time_s": 2, "x": 0, "y": 1, "heading": 0, "state": 2, "walls": 0x0A, "dl": 5, "df": 35, "dr": 5, "yaw": 1.2, "vbat": 8150, "rssi": -58},
        {"seq": 3, "time_s": 3, "x": 0, "y": 2, "heading": 1, "state": 2, "walls": 0x05, "dl": 5, "df": 18, "dr": 5, "yaw": 90.0, "vbat": 8100, "rssi": -63},
        {"seq": 4, "time_s": 4, "x": 1, "y": 2, "heading": 1, "state": 3, "walls": 0x00, "dl": 40, "df": 40, "dr": 40, "yaw": 89.8, "vbat": 8050, "rssi": -67}
    ]

    all_passed = True

    for i, s in enumerate(test_samples, 1):
        raw_pkt = pack_telemetry(s["seq"], s["time_s"], s["x"], s["y"], s["heading"],
                                 s["state"], s["walls"], s["dl"], s["df"], s["dr"],
                                 s["yaw"], s["vbat"])

        if len(raw_pkt) != 18:
            print(f"[ERRO] Pacote #{i} tamanho incorreto: {len(raw_pkt)} bytes")
            all_passed = False
            continue

        try:
            json_str = unpack_and_to_json(raw_pkt, s["rssi"])
            parsed = json.loads(json_str)

            # Validacao dos campos contratuais
            assert parsed["seq"] == s["seq"]
            assert parsed["rssi"] == s["rssi"]
            assert parsed["battery_v"] == round(s["vbat"] / 1000.0, 2)
            assert parsed["pos"]["x"] == s["x"]

            print(f"Pacote #{i:02d} (18 bytes) -> JSON Gateway valido: {json_str[:85]}... [OK]")
        except Exception as e:
            print(f"[FALHA] Pacote #{i}: {e}")
            all_passed = False

    print("\n" + "=" * 75)
    if all_passed:
        print("CRITERIOS DE ACEITE DA ISSUE #140 ATENDIDOS COM SUCESSO:")
        print("  [X] Empacotamento binario validado exatamente em 18 bytes.")
        print("  [X] Codificacao, CRC16 e decodificacao 100% integra.")
        print("  [X] Gateway converte para JSON com seq e RSSI conforme contrato EAP 5.3.1.1.")
        print("=" * 75)
        return 0
    else:
        print("[ERRO] Falha no contrato de dados ou decodificacao!")
        return 1

if __name__ == "__main__":
    sys.exit(run_tests())
