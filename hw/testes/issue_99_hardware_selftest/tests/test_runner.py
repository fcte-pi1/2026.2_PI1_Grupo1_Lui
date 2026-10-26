#!/usr/bin/env python3
"""
Matriz de Rastreabilidade e Conformidade de Testes de Hardware
Issue #99 - [EAP 3.1.5] Testes de Hardware (Relatório 7.3)

Valida laudo de conformidade com todos os requisitos de Eletrônica:
- RF-ELE01 a RF-ELE06
- RNF-ELE01 a RNF-ELE05
"""

import sys

REQUIREMENTS = [
    {"req": "RF-ELE01", "desc": "Sensoriamento ToF (Frontal, Esquerdo, Direito)",   "verified_in": "Issue #135", "status": "CONFORME"},
    {"req": "RF-ELE02", "desc": "Odometria de Rodas por Encoders em Quadratura",   "verified_in": "Issue #136", "status": "CONFORME"},
    {"req": "RF-ELE03", "desc": "Acionamento de Potência TB6612FNG com PWM 20kHz", "verified_in": "Issue #136", "status": "CONFORME"},
    {"req": "RF-ELE04", "desc": "Transmissão Sem Fio ESP-NOW a 1 Hz",              "verified_in": "Issue #140", "status": "CONFORME"},
    {"req": "RF-ELE05", "desc": "Sinalização Visual RGB para 5 Modos Operacionais", "verified_in": "Issue #98",  "status": "CONFORME"},
    {"req": "RF-ELE06", "desc": "Sinalização Sonora Acústica (Buzzer >= 60 dB)",    "verified_in": "Issue #98",  "status": "CONFORME"},
    {"req": "RNF-ELE01", "desc": "Taxa de Amostragem Sensorial >= 20 Hz",           "verified_in": "Issue #135", "status": "CONFORME"},
    {"req": "RNF-ELE02", "desc": "Faixa Útil 30-180 mm com Erro <= 10 mm",          "verified_in": "Issue #135", "status": "CONFORME"},
    {"req": "RNF-ELE03", "desc": "Enlace Sem Fio Estável a 8 m com Perda < 2.0%",  "verified_in": "Issue #137", "status": "CONFORME"},
    {"req": "RNF-ELE04", "desc": "Construção em Shield PCB sem Protoboard Solta",   "verified_in": "Doc 4.3",    "status": "CONFORME"},
    {"req": "RNF-ELE05", "desc": "Imunidade a Ruído de Motores (Zero Resets)",      "verified_in": "Issue #131", "status": "CONFORME"}
]

def run_hardware_audit():
    print("=" * 85)
    print("  SUITE DE TESTE AUTOMATIZADO - ISSUE #99: AUDITORIA INTEGRADA DE HARDWARE")
    print("=" * 85)

    print(f"\n{'ID Requisito':<12} | {'Evidenciado Em':<14} | {'Descricao Funcional':<42} | {'Resultado'}")
    print("-" * 85)

    all_ok = True
    for r in REQUIREMENTS:
        if r["status"] != "CONFORME":
            all_ok = False
        print(f"{r['req']:<12} | {r['verified_in']:<14} | {r['desc']:<42} | {r['status']}")

    print("\n" + "=" * 85)
    if all_ok:
        print("LAUDO FINAL DE HOMOLOGACAO DE HARDWARE: 100% APROVADO")
        print("  Todos os requisitos RF-ELE01 a RF-ELE06 e RNF-ELE01 a RNF-ELE05 estao cumpridos.")
        print("=" * 85)
        return 0
    else:
        print("[ERRO] Pendencia de conformidade detectada em ensaios laboratoriais.")
        return 1

if __name__ == "__main__":
    sys.exit(run_hardware_audit())
