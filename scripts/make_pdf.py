#!/usr/bin/env python3
"""
Script de geração de PDF acadêmico para '7.3 - Testes de hardware.md'.
Utiliza WeasyPrint para gerar um documento formal no padrão da Universidade de Brasília (UnB/FCTE),
com suporte a páginas em paisagem para tabelas densas, conversão das abas do Wokwi em cartões
com diagramas de circuito renderizados e diagramas vetoriais SVG integrados.
"""

import os
import re
import urllib.parse
import markdown
import weasyprint

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
MD_PATH = os.path.join(DOCS_DIR, "7.3 - Testes de hardware.md")
PDF_OUTPUT_ROOT = os.path.join(BASE_DIR, "7.3 - Testes de hardware.pdf")
PDF_OUTPUT_DOCS = os.path.join(DOCS_DIR, "7.3 - Testes de hardware.pdf")
FIGS_DIR = os.path.join(DOCS_DIR, "figs", "testes_hardware")

# Diagrama Vetorial SVG: Acionamento de Motores e Odometria
SVG_MOTOR_DIAGRAM = '''
<div class="diagram-wrapper">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 740 230" class="vector-diagram" style="width: 100%; max-width: 680px; height: auto;">
  <defs>
    <marker id="arrow-blue" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 1.5 L 8 5 L 0 8.5 z" fill="#0f3b6c" />
    </marker>
    <marker id="arrow-green" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 1.5 L 8 5 L 0 8.5 z" fill="#15803d" />
    </marker>
    <marker id="arrow-orange" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 1.5 L 8 5 L 0 8.5 z" fill="#b45309" />
    </marker>
  </defs>

  <!-- ESP32 -->
  <rect x="15" y="20" width="180" height="195" rx="3" fill="#ffffff" stroke="#334155" stroke-width="1.2" />
  <rect x="15" y="20" width="180" height="28" rx="3" fill="#0f3b6c" />
  <text x="105" y="39" fill="#ffffff" font-family="Arial, sans-serif" font-size="11" font-weight="bold" text-anchor="middle">ESP32 DevKit V1</text>
  <text x="28" y="72" fill="#334155" font-family="monospace" font-size="9">GPIO 16 (PWM A)</text>
  <text x="28" y="93" fill="#334155" font-family="monospace" font-size="9">GPIO 18/19 (AIN)</text>
  <text x="28" y="118" fill="#334155" font-family="monospace" font-size="9">GPIO 17 (PWM B)</text>
  <text x="28" y="139" fill="#334155" font-family="monospace" font-size="9">GPIO 23/5 (BIN)</text>
  <text x="28" y="172" fill="#15803d" font-family="monospace" font-size="9" font-weight="bold">GPIO 27/26 (Enc L)</text>
  <text x="28" y="193" fill="#15803d" font-family="monospace" font-size="9" font-weight="bold">GPIO 25/33 (Enc R)</text>

  <!-- TB6612FNG -->
  <rect x="285" y="38" width="180" height="155" rx="3" fill="#ffffff" stroke="#334155" stroke-width="1.2" />
  <rect x="285" y="38" width="180" height="28" rx="3" fill="#0f766e" />
  <text x="375" y="57" fill="#ffffff" font-family="Arial, sans-serif" font-size="11" font-weight="bold" text-anchor="middle">Driver TB6612FNG</text>
  <text x="298" y="90" fill="#334155" font-family="Arial, sans-serif" font-size="9.5">PWM 20 kHz (Hardware)</text>
  <text x="298" y="112" fill="#334155" font-family="Arial, sans-serif" font-size="9.5">MOSFETs de baixa perda</text>
  <text x="298" y="134" fill="#334155" font-family="Arial, sans-serif" font-size="9.5">Corrente: 1.2 A contínua</text>
  <text x="298" y="162" fill="#b45309" font-family="Arial, sans-serif" font-size="9.5" font-weight="bold">Alimentação VM: 7.4 V</text>

  <!-- Motores -->
  <!-- Motor Esquerdo -->
  <rect x="545" y="20" width="180" height="80" rx="3" fill="#ffffff" stroke="#334155" stroke-width="1.2" />
  <rect x="545" y="20" width="180" height="24" rx="3" fill="#475569" />
  <text x="635" y="36" fill="#ffffff" font-family="Arial, sans-serif" font-size="10.5" font-weight="bold" text-anchor="middle">Motor DC Esquerdo</text>
  <text x="558" y="62" fill="#334155" font-family="Arial, sans-serif" font-size="9">N20 30:1 + Encoder Magnético</text>
  <text x="558" y="83" fill="#15803d" font-family="monospace" font-size="9">360 pulsos / volta roda</text>

  <!-- Motor Direito -->
  <rect x="545" y="130" width="180" height="80" rx="3" fill="#ffffff" stroke="#334155" stroke-width="1.2" />
  <rect x="545" y="130" width="180" height="24" rx="3" fill="#475569" />
  <text x="635" y="146" fill="#ffffff" font-family="Arial, sans-serif" font-size="10.5" font-weight="bold" text-anchor="middle">Motor DC Direito</text>
  <text x="558" y="172" fill="#334155" font-family="Arial, sans-serif" font-size="9">N20 30:1 + Encoder Magnético</text>
  <text x="558" y="193" fill="#15803d" font-family="monospace" font-size="9">360 pulsos / volta roda</text>

  <!-- Conexões ESP32 -> TB6612 -->
  <path d="M 195 72 L 285 72" stroke="#0f3b6c" stroke-width="1.4" marker-end="url(#arrow-blue)" />
  <path d="M 195 93 L 285 93" stroke="#0f3b6c" stroke-width="1.4" marker-end="url(#arrow-blue)" />
  <path d="M 195 118 L 285 118" stroke="#0f3b6c" stroke-width="1.4" marker-end="url(#arrow-blue)" />
  <path d="M 195 139 L 285 139" stroke="#0f3b6c" stroke-width="1.4" marker-end="url(#arrow-blue)" />

  <!-- Conexões TB6612 -> Motores -->
  <path d="M 465 76 L 545 60" stroke="#b45309" stroke-width="1.6" marker-end="url(#arrow-orange)" />
  <path d="M 465 152 L 545 170" stroke="#b45309" stroke-width="1.6" marker-end="url(#arrow-orange)" />

  <!-- Retorno dos Encoders -->
  <path d="M 545 88 C 500 88, 480 172, 195 172" stroke="#15803d" stroke-width="1.4" stroke-dasharray="3,3" marker-end="url(#arrow-green)" fill="none" />
  <path d="M 545 198 L 195 198" stroke="#15803d" stroke-width="1.4" stroke-dasharray="3,3" marker-end="url(#arrow-green)" fill="none" />
</svg>
<div class="diagram-caption">Figura: Diagrama esquemático do acionamento dos micromotores via PWM e leitura de pulsos de odometria.</div>
</div>
'''

# Diagrama Vetorial SVG: Protocolo ESP-NOW
SVG_ESPNOW_DIAGRAM = '''
<div class="diagram-wrapper">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 700 220" class="vector-diagram" style="width: 100%; max-width: 640px; height: auto;">
  <defs>
    <marker id="arr-blue" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 1.5 L 8 5 L 0 8.5 z" fill="#0f3b6c" />
    </marker>
    <marker id="arr-teal" viewBox="0 0 10 10" refX="6" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 1.5 L 8 5 L 0 8.5 z" fill="#0f766e" />
    </marker>
  </defs>

  <!-- Nós -->
  <rect x="30" y="15" width="160" height="30" rx="3" fill="#0f3b6c" />
  <text x="110" y="34" fill="#ffffff" font-family="Arial, sans-serif" font-size="10.5" font-weight="bold" text-anchor="middle">Robô Micromouse</text>
  <line x1="110" y1="45" x2="110" y2="205" stroke="#94a3b8" stroke-width="1" stroke-dasharray="3,3" />

  <rect x="270" y="15" width="160" height="30" rx="3" fill="#0f766e" />
  <text x="350" y="34" fill="#ffffff" font-family="Arial, sans-serif" font-size="10.5" font-weight="bold" text-anchor="middle">Gateway de Solo (ESP32)</text>
  <line x1="350" y1="45" x2="350" y2="205" stroke="#94a3b8" stroke-width="1" stroke-dasharray="3,3" />

  <rect x="500" y="15" width="170" height="30" rx="3" fill="#475569" />
  <text x="585" y="34" fill="#ffffff" font-family="Arial, sans-serif" font-size="10.5" font-weight="bold" text-anchor="middle">Dashboard / Computador</text>
  <line x1="585" y1="45" x2="585" y2="205" stroke="#94a3b8" stroke-width="1" stroke-dasharray="3,3" />

  <!-- Faixa de rádio -->
  <rect x="50" y="58" width="600" height="22" rx="2" fill="#f8fafc" stroke="#cbd5e1" stroke-width="1" />
  <text x="350" y="73" fill="#475569" font-family="Arial, sans-serif" font-size="9.5" font-weight="bold" text-anchor="middle">Canal sem fio ESP-NOW em 2.4 GHz (Canal 1 fixo, comunicação direta por MAC)</text>

  <!-- Mensagem 1 -->
  <path d="M 110 110 L 350 110" stroke="#0f3b6c" stroke-width="1.5" marker-end="url(#arr-blue)" />
  <rect x="150" y="96" width="160" height="16" rx="2" fill="#ffffff" stroke="#93c5fd" />
  <text x="230" y="108" fill="#0f3b6c" font-family="Arial, sans-serif" font-size="8.5" font-weight="bold" text-anchor="middle">Frame binário 18 bytes (1 Hz)</text>

  <!-- Processamento -->
  <rect x="300" y="128" width="100" height="26" rx="2" fill="#f0fdfa" stroke="#0f766e" />
  <text x="350" y="140" fill="#0f766e" font-family="Arial, sans-serif" font-size="7.5" text-anchor="middle">Valida CRC16</text>
  <text x="350" y="150" fill="#0f766e" font-family="Arial, sans-serif" font-size="7.5" text-anchor="middle">+ Mede RSSI do enlace</text>

  <!-- Mensagem 2 -->
  <path d="M 350 178 L 585 178" stroke="#0f766e" stroke-width="1.5" marker-end="url(#arr-teal)" />
  <rect x="385" y="164" width="165" height="16" rx="2" fill="#ffffff" stroke="#5eead4" />
  <text x="467" y="176" fill="#0f766e" font-family="Arial, sans-serif" font-size="8.5" font-weight="bold" text-anchor="middle">Serial USB JSON (115200 bps)</text>
</svg>
<div class="diagram-caption">Figura: Fluxo de dados da telemetria via rádio ESP-NOW e conversão serial USB para formato JSON.</div>
</div>
'''

def clean_latex_and_symbols(text):
    """Substitui sequências LaTeX e símbolos web por equivalentes limpos em texto/HTML."""
    # Elimina travessões longos
    text = text.replace("—", "-")

    substitutions = [
        (r'$\le 10\text{ mm}$', '≤ 10 mm'),
        (r'$\le \pm 10\text{ mm}$', '≤ ±10 mm'),
        (r'$\pm 10\text{ mm}$', '±10 mm'),
        (r'$\ge 20\text{ Hz}$', '≥ 20 Hz'),
        (r'$\le 50\text{ ms}$', '≤ 50 ms'),
        (r'$\ge 60\text{ dB}$', '≥ 60 dB'),
        (r'$\ge -75\text{ dBm}$', '≥ -75 dBm'),
        (r'$\approx 30,3\text{ Hz}$', '≈ 30,3 Hz'),
        (r'$\approx 30,3\text{ leituras por segundo}$', '≈ 30,3 leituras/s'),
        (r'$\pm 5\%$', '±5%'),
        (r'$\pm 1\%$', '±1%'),
        (r'$20,07\text{ mm}$', '20,07 mm'),
        (r'$39,96\text{ mm}$', '39,96 mm'),
        (r'$60,05\text{ mm}$', '60,05 mm'),
        (r'$79,85\text{ mm}$', '79,85 mm'),
        (r'$100,09\text{ mm}$', '100,09 mm'),
        (r'$149,98\text{ mm}$', '149,98 mm'),
        (r'$200,18\text{ mm}$', '200,18 mm'),
        (r'$0,08\text{ mm}$', '0,08 mm'),
        (r'$0,18\text{ mm}$', '0,18 mm'),
        (r'$0,55\text{ mm}$', '0,55 mm'),
        (r'$0,66\text{ mm}$', '0,66 mm'),
        (r'$0,47\text{ mm}$', '0,47 mm'),
        (r'$0,62\text{ mm}$', '0,62 mm'),
        (r'$0,60\text{ mm}$', '0,60 mm'),
        (r'$0,64\text{ mm}$', '0,64 mm'),
        (r'$0,07\text{ mm}$', '0,07 mm'),
        (r'$0,04\text{ mm}$', '0,04 mm'),
        (r'$0,05\text{ mm}$', '0,05 mm'),
        (r'$0,15\text{ mm}$', '0,15 mm'),
        (r'$0,09\text{ mm}$', '0,09 mm'),
        (r'$0,02\text{ mm}$', '0,02 mm'),
        (r'$0,25\text{ mm}$', '0,25 mm'),
        (r'$5\text{ mm}$', '5 mm'),
        (r'$100\text{ mm}$', '100 mm'),
        (r'$54\text{ mm}$', '54 mm'),
        (r'$50\text{ mm}$', '50 mm'),
        (r'$33\text{ ms}$', '33 ms'),
        (r'$45\text{ ms}$', '45 ms'),
        (r'$44,2\text{ ms}$', '44,2 ms'),
        (r'$42,8\text{ ms}$', '42,8 ms'),
        (r'$40\text{ ms}$', '40 ms'),
        (r'$15\text{ ms}$', '15 ms'),
        (r'$50\text{ ms}$', '50 ms'),
        (r'$100\text{ ms}$', '100 ms'),
        (r'$200\text{ ms}$', '200 ms'),
        (r'$500\text{ ms}$', '500 ms'),
        (r'$1\text{ segundo}$', '1 segundo'),
        (r'$1\text{ Hz}$', '1 Hz'),
        (r'$20\text{ kHz}$', '20 kHz'),
        (r'$100\text{ kHz}$', '100 kHz'),
        (r'$400\text{ kHz}$', '400 kHz'),
        (r'$240\text{ MHz}$', '240 MHz'),
        (r'$2,4\text{ GHz}$', '2,4 GHz'),
        (r'$2412\text{ MHz}$', '2412 MHz'),
        (r'$3,3\text{ V}$', '3,3 V'),
        (r'$5,0\text{ V}$', '5,0 V'),
        (r'$7,4\text{ V}$', '7,4 V'),
        (r'$7,40\text{ V}$', '7,40 V'),
        (r'$8,40\text{ V}$', '8,40 V'),
        (r'$7,00\text{ V}$', '7,00 V'),
        (r'$6,60\text{ V}$', '6,60 V'),
        (r'$6,6\text{ V}$', '6,6 V'),
        (r'$6,0\text{ V}$', '6,0 V'),
        (r'$8,4\text{ V}$', '8,4 V'),
        (r'$1,85\text{ V}$', '1,85 V'),
        (r'$3,70\text{ V}$', '3,70 V'),
        (r'$5,55\text{ V}$', '5,55 V'),
        (r'$-3,70\text{ V}$', '-3,70 V'),
        (r'$2,68\text{ V}$', '2,68 V'),
        (r'$2,36\text{ V}$', '2,36 V'),
        (r'$2,23\text{ V}$', '2,23 V'),
        (r'$2,11\text{ V}$', '2,11 V'),
        (r'$3,25\text{ V}$', '3,25 V'),
        (r'$3,31\text{ V}$', '3,31 V'),
        (r'$4,88\text{ V}$', '4,88 V'),
        (r'$5,02\text{ V}$', '5,02 V'),
        (r'$1,4\text{ A}$', '1,4 A'),
        (r'$1,2\text{ A}$', '1,2 A'),
        (r'$4,7\text{ k}\Omega$', '4,7 kΩ'),
        (r'$100\text{ k}\Omega$', '100 kΩ'),
        (r'$47\text{ k}\Omega$', '47 kΩ'),
        (r'$-69,5\text{ dBm}$', '-69,5 dBm'),
        (r'$-75\text{ dBm}$', '-75 dBm'),
        (r'$-48,0\text{ dBm}$', '-48,0 dBm'),
        (r'$-56,1\text{ dBm}$', '-56,1 dBm'),
        (r'$-65,0\text{ dBm}$', '-65,0 dBm'),
        (r'$0,5\text{ m/s}$', '0,5 m/s'),
        (r'$1,65\text{ cm}$', '1,65 cm'),
        (r'$12\text{ cm}$', '12 cm'),
        (r'$120\text{ mm}$', '120 mm'),
        (r'$2\text{ metros}$', '2 metros'),
        (r'$4\text{ metros}$', '4 metros'),
        (r'$8\text{ metros}$', '8 metros'),
        (r'$1\text{ metro}$', '1 metro'),
        (r'$1000\text{ pacotes}$', '1000 pacotes'),
        (r'$360\text{ pulsos por volta}$', '360 pulsos/volta'),
        (r'$3600\text{ pulsos esperados}$', '3600 pulsos'),
        (r'$3600\text{ pulsos teóricos}$', '3600 pulsos'),
        (r'$> 400\text{ RPM}$', '> 400 RPM'),
        (r'$0,1\%$', '0,1%'),
        (r'$0,70\%$', '0,70%'),
        (r'$0,80\%$', '0,80%'),
        (r'$0,10\%$', '0,10%'),
        (r'$0,30\%$', '0,30%'),
        (r'$2,0\%$', '2,0%'),
        (r'$1,5\%$', '1,5%'),
        (r'$56,6\%$', '56,6%'),
        (r'$13,4\%$', '13,4%'),
        (r'$100\%$', '100%'),
        (r'$50\%$', '50%'),
        (r'$742\text{ kB}$', '742 kB'),
        (r'$1310\text{ kB}$', '1310 kB'),
        (r'$43,9\text{ kB}$', '43,9 kB'),
        (r'$327\text{ kB}$', '327 kB'),
        (r'$115200\text{ bps}$', '115200 bps'),
        (r'$+20\text{ dBm}$', '+20 dBm'),
        (r'$100\text{ mW}$', '100 mW'),
        (r'$33000\ \mu\text{s}$', '33000 µs'),
        (r'$d_{\text{ref}}$', 'd_ref'),
        (r'$\overline{d}$', 'd_médio'),
        (r'$\sigma$', 'σ (desvio)'),
        (r'$|\overline{d} - d_{\text{ref}}|$', 'Erro Abs.'),
        (r'$4\text{ Hz}$', '4 Hz'),
        (r'$4\times 4$', '4x4'),
        (r'$8\times 4$', '8x4'),
        (r'$12\times 4$', '12x4'),
        (r'$128\times 64$', '128x64'),
    ]
    for old, new in substitutions:
        text = text.replace(old, new)

    text = text.replace(':white_check_mark:', '<span class="status-pass">Conforme</span>')

    # Substitui equações de bloco $$...$$
    def repl_block_math(match):
        eq = match.group(1).strip()
        eq = eq.replace(r'\text{corrigida}', 'corrigida')
        eq = eq.replace(r'\text{Razão do divisor}', 'Razão do divisor')
        eq = eq.replace(r'\approx', '≈')
        eq = eq.replace(r'\cdot', '·')
        # Limpa frações básicas \frac{A}{B}
        eq = re.sub(r'\\frac\{([^}]+)\}\{([^}]+)\}', r'(\1 / \2)', eq)
        return f'<div class="math-display">{eq}</div>'

    text = re.sub(r'\$\$(.*?)\$\$', repl_block_math, text, flags=re.DOTALL)
    return text

def process_wokwi_tabs_to_html(content):
    """
    Converte blocos de abas do Material for MkDocs (=== "Título")
    em caixas visuais independentes (wokwi-box), compilando o Markdown interno
    para HTML para que as tags de imagem sejam geradas perfeitamente.
    """
    lines = content.split('\n')
    results = []
    current_title = None
    current_body = []

    def flush():
        nonlocal current_title, current_body
        if current_title:
            raw_body = '\n'.join(current_body)
            docs_figs_url = "file://" + FIGS_DIR + "/"
            raw_body = re.sub(r'\((figs/testes_hardware/([^)]+))\)', r'(' + docs_figs_url + r'\2)', raw_body)
            body_html = markdown.markdown(raw_body, extensions=['markdown.extensions.tables'])
            card_html = f'''
<div class="wokwi-box">
  <div class="wokwi-box-header">{current_title}</div>
  <div class="wokwi-box-content">
    {body_html}
  </div>
</div>
'''
            results.append(card_html)
            current_title = None
            current_body = []

    for line in lines:
        m = re.match(r'^===\s+"([^"]+)"', line)
        if m:
            flush()
            current_title = m.group(1)
            continue
        if current_title:
            if line.startswith('    '):
                current_body.append(line[4:])
            elif line.startswith('\t'):
                current_body.append(line[1:])
            elif line.strip() == '':
                current_body.append('')
            else:
                flush()
                results.append(line)
        else:
            results.append(line)

    flush()
    return '\n'.join(results)

def build_pdf():
    print(f"Lendo markdown em: {MD_PATH}")
    with open(MD_PATH, "r", encoding="utf-8") as f:
        md_content = f.read()

    # 1. Substituir diagramas Mermaid por SVGs vetoriais limpos
    md_content = re.sub(r'```mermaid\s*flowchart LR.*?```', lambda m: SVG_MOTOR_DIAGRAM, md_content, flags=re.DOTALL)
    md_content = re.sub(r'```mermaid\s*sequenceDiagram.*?```', lambda m: SVG_ESPNOW_DIAGRAM, md_content, flags=re.DOTALL)

    # 2. Processar e compilar as abas do Wokwi em cartões com imagens
    md_content = process_wokwi_tabs_to_html(md_content)

    # 3. Limpar fórmulas LaTeX e ícones
    md_content = clean_latex_and_symbols(md_content)

    # 4. Ajustar caminhos de todas as outras imagens para caminho absoluto file://
    docs_figs_url = "file://" + FIGS_DIR + "/"
    md_content = re.sub(r'\((figs/testes_hardware/([^)]+))\)', r'(' + docs_figs_url + r'\2)', md_content)

    # 5. Remover botões web irrelevantes para o PDF impresso
    md_content = re.sub(r'\[↑ Voltar ao Painel Geral de Testes\]\(#painel-geral-de-ensaios-de-hardware\)', '', md_content)

    # 6. Compilar markdown para HTML
    html_body = markdown.markdown(
        md_content,
        extensions=[
            'markdown.extensions.tables',
            'markdown.extensions.admonition',
            'markdown.extensions.fenced_code',
            'markdown.extensions.toc',
        ]
    )

    # 7. Envolver as tabelas densas em contêineres Paisagem (.page-landscape)
    html_body = re.sub(
        r'(<h2[^>]*>Painel Geral de Ensaios de Hardware</h2>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    html_body = re.sub(
        r'(<h2[^>]*>8\. Matriz de Homologação e Rastreabilidade de Requisitos</h2>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    # 8. Template HTML com Capa Acadêmica ABNT UnB
    html_full = f'''<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<title>Relatório de Testes de Hardware - Micromouse Grupo 1</title>
<style>
/* Layout e Páginas A4 */
@page {{
  size: A4 portrait;
  margin: 22mm 18mm 22mm 18mm;

  @top-left {{
    content: "Universidade de Brasília • PI1 2026.2";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #475569;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }}

  @top-right {{
    content: "7.3 - Testes de Hardware (Micromouse Grupo 1)";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }}

  @bottom-left {{
    content: "Faculdade de Ciências e Tecnologias em Engenharia - FCTE";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #64748b;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }}

  @bottom-right {{
    content: "Página " counter(page) " de " counter(pages);
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }}
}}

/* Capa: sem cabeçalho e sem rodapé */
@page:first {{
  margin: 0;
  @top-left {{ content: none; }}
  @top-right {{ content: none; }}
  @bottom-left {{ content: none; }}
  @bottom-right {{ content: none; }}
}}

/* Página Paisagem para Tabelas Densas (Painel Geral e Matriz) */
@page landscape_page {{
  size: A4 landscape;
  margin: 16mm 15mm 16mm 15mm;

  @top-left {{
    content: "Universidade de Brasília • FCTE - Projeto Integrador 1";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #475569;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }}

  @top-right {{
    content: "7.3 - Painel e Matriz de Homologação de Requisitos";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }}

  @bottom-left {{
    content: "Micromouse - Grupo 1 (2026.2)";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #64748b;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }}

  @bottom-right {{
    content: "Página " counter(page) " de " counter(pages);
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }}
}}

.page-landscape {{
  page: landscape_page;
  page-break-before: always;
  page-break-after: always;
}}

body {{
  font-family: "Liberation Sans", Arial, Helvetica, sans-serif;
  font-size: 9.5pt;
  line-height: 1.45;
  color: #1e293b;
  margin: 0;
  padding: 0;
}}

/* ==========================================================================
   CAPA OFICIAL PADRÃO ABNT / UNIVERSIDADE DE BRASÍLIA
   ========================================================================== */
.abnt-cover {{
  height: 297mm;
  padding: 30mm 25mm 25mm 25mm;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  text-align: center;
  background-color: #ffffff;
  page-break-after: always;
}}

.cover-inst-header {{
  text-transform: uppercase;
  font-weight: bold;
  line-height: 1.4;
  color: #000000;
}}

.cover-inst-name {{
  font-size: 13pt;
  letter-spacing: 0.5px;
}}

.cover-faculty-name {{
  font-size: 11pt;
  color: #334155;
  margin-top: 4px;
}}

.cover-discipline-name {{
  font-size: 10pt;
  color: #475569;
  margin-top: 4px;
}}

.cover-authors {{
  font-size: 9.5pt;
  line-height: 1.6;
  color: #1e293b;
  margin-top: 25mm;
}}

.cover-title-area {{
  margin-top: 25mm;
}}

.cover-doc-title {{
  font-size: 17pt;
  font-weight: bold;
  text-transform: uppercase;
  color: #000000;
  margin-bottom: 8px;
}}

.cover-doc-subtitle {{
  font-size: 11pt;
  font-style: italic;
  color: #334155;
}}

.cover-natureza {{
  margin-top: 25mm;
  margin-left: 42%;
  text-align: justify;
  font-size: 8.5pt;
  line-height: 1.4;
  color: #334155;
}}

.cover-location-year {{
  font-size: 10.5pt;
  font-weight: bold;
  text-transform: uppercase;
  color: #000000;
}}

/* ==========================================================================
   TIPOGRAFIA DO DOCUMENTO
   ========================================================================== */
h1 {{
  font-size: 15pt;
  font-weight: bold;
  color: #0f172a;
  border-bottom: 1.5pt solid #0f172a;
  padding-bottom: 4px;
  margin: 18px 0 10px 0;
}}

h2 {{
  font-size: 12pt;
  font-weight: bold;
  color: #0f3b6c;
  border-bottom: 0.8pt solid #cbd5e1;
  padding-bottom: 3px;
  margin: 20px 0 8px 0;
  page-break-after: avoid;
}}

h3 {{
  font-size: 10.5pt;
  font-weight: bold;
  color: #1e293b;
  margin: 14px 0 6px 0;
  page-break-after: avoid;
}}

h4 {{
  font-size: 9.5pt;
  font-weight: bold;
  color: #334155;
  margin: 10px 0 4px 0;
  page-break-after: avoid;
}}

p {{
  margin: 0 0 8px 0;
  text-align: justify;
}}

/* Tabelas */
table {{
  width: 100%;
  border-collapse: collapse;
  margin: 8px 0 12px 0;
  font-size: 7.5pt;
  page-break-inside: auto;
}}

tr {{
  page-break-inside: avoid;
}}

th {{
  background-color: #0f3b6c;
  color: #ffffff;
  font-weight: bold;
  text-align: left;
  padding: 5px 6px;
  border: 0.5pt solid #0f3b6c;
  font-size: 7.5pt;
}}

td {{
  padding: 4px 6px;
  border: 0.5pt solid #cbd5e1;
  vertical-align: top;
}}

tr:nth-child(even) td {{
  background-color: #f8fafc;
}}

.status-pass {{
  color: #15803d;
  font-weight: bold;
}}

/* Caixas de Destaque / Admonitions */
.admonition {{
  margin: 8px 0;
  padding: 8px 10px;
  border-radius: 2px;
  background-color: #f8fafc;
  border-left: 3pt solid #64748b;
  page-break-inside: avoid;
}}

.admonition.quote {{
  border-left-color: #0f3b6c;
  background-color: #f1f5f9;
}}

.admonition.info {{
  border-left-color: #0284c7;
  background-color: #f0f9ff;
}}

.admonition.success {{
  border-left-color: #16a34a;
  background-color: #f0fdf4;
}}

.admonition-title {{
  font-weight: bold;
  font-size: 9pt;
  color: #0f172a;
  margin-bottom: 4px;
}}

.admonition p:last-child {{
  margin-bottom: 0;
}}

/* Códigos e Terminais */
pre {{
  background-color: #0f172a;
  color: #f1f5f9;
  padding: 8px 10px;
  border-radius: 3px;
  font-family: "Liberation Mono", Consolas, monospace;
  font-size: 7.2pt;
  line-height: 1.35;
  page-break-inside: avoid;
  margin: 6px 0;
}}

code {{
  font-family: "Liberation Mono", Consolas, monospace;
  font-size: 8pt;
  background-color: #f1f5f9;
  color: #0f172a;
  padding: 1px 3px;
  border-radius: 2px;
}}

pre code {{
  background-color: transparent;
  color: inherit;
  padding: 0;
}}

/* Imagens */
img {{
  display: block;
  margin: 8px auto;
  max-width: 90%;
  max-height: 220px;
  height: auto;
  border: 0.5pt solid #cbd5e1;
  page-break-inside: avoid;
}}

em {{
  display: block;
  text-align: center;
  font-size: 7.8pt;
  color: #64748b;
  margin-top: 3px;
  margin-bottom: 10px;
}}

.diagram-wrapper {{
  margin: 12px 0;
  text-align: center;
  page-break-inside: avoid;
}}

.diagram-caption {{
  font-size: 7.8pt;
  color: #64748b;
  font-style: italic;
  margin-top: 4px;
}}

/* Cartões de Simulação Wokwi com Imagens Renderizadas */
.wokwi-box {{
  border: 0.8pt solid #94a3b8;
  border-radius: 3px;
  background-color: #ffffff;
  margin: 12px 0;
  page-break-inside: avoid;
}}

.wokwi-box-header {{
  background-color: #0f3b6c;
  color: #ffffff;
  font-weight: bold;
  font-size: 9.5pt;
  padding: 6px 8px;
  border-top-left-radius: 2px;
  border-top-right-radius: 2px;
}}

.wokwi-box-content {{
  padding: 8px 10px;
}}

.wokwi-box-content h4 {{
  margin-top: 2px;
  margin-bottom: 4px;
  color: #0f3b6c;
  font-size: 9pt;
}}

.wokwi-box-content img {{
  max-height: 180px;
  width: auto;
  margin: 6px auto;
}}

ul, ol {{
  margin: 3px 0 8px 0;
  padding-left: 20px;
}}

li {{
  margin-bottom: 2px;
}}

.math-display {{
  text-align: center;
  font-family: "Liberation Serif", Times, serif;
  font-size: 9.5pt;
  margin: 8px 0;
  font-style: italic;
}}

hr {{
  border: none;
  border-top: 0.5pt solid #cbd5e1;
  margin: 12px 0;
}}
</style>
</head>
<body>

<!-- CAPA OFICIAL PADRÃO ABNT / UNIVERSIDADE DE BRASÍLIA -->
<div class="abnt-cover">
  <div class="cover-inst-header">
    <div class="cover-inst-name">Universidade de Brasília</div>
    <div class="cover-faculty-name">Faculdade de Ciências e Tecnologias em Engenharia - FCTE</div>
    <div class="cover-discipline-name">Projeto Integrador de Engenharia 1 - Turma 2026.2</div>
  </div>

  <div class="cover-authors">
    Breno Teixeira da Silva &bull; Daniel Almeida &bull; Davi Sakai &bull; Eduardo Lobo de Miranda<br>
    João Pedro Jaime &bull; Leonardo Augusto &bull; Lucas Oliveira &bull; Pedro Franco &bull; Rafaela Trajano
  </div>

  <div class="cover-title-area">
    <div class="cover-doc-title">Relatório de Testes de Hardware</div>
    <div class="cover-doc-subtitle">Validação de Bancada e Simulação dos Subsistemas de Processamento, Sensores, Armazenamento, Atuadores e Comunicações</div>
  </div>

  <div class="cover-natureza">
    Relatório técnico de ensaios laboratoriais e simulações apresentado à disciplina de Projeto Integrador de Engenharia 1 da Universidade de Brasília (FCTE/UnB), como requisito de validação experimental do robô Micromouse (Grupo 1).<br><br>
    <strong>Orientador:</strong> Prof. Dr. Lui
  </div>

  <div class="cover-location-year">
    Gama - DF<br>
    2026
  </div>
</div>

<!-- CORPO DO DOCUMENTO -->
<div class="document-content">
{html_body}
</div>

</body>
</html>'''

    print("Gerando PDF com WeasyPrint...")
    html_obj = weasyprint.HTML(string=html_full, base_url=DOCS_DIR)
    
    html_obj.write_pdf(PDF_OUTPUT_ROOT)
    html_obj.write_pdf(PDF_OUTPUT_DOCS)

    size_kb = os.path.getsize(PDF_OUTPUT_ROOT) / 1024
    print(f"Sucesso! PDF gerado:")
    print(f" - {PDF_OUTPUT_ROOT} ({size_kb:.1f} kB)")
    print(f" - {PDF_OUTPUT_DOCS} ({size_kb:.1f} kB)")

if __name__ == "__main__":
    build_pdf()
