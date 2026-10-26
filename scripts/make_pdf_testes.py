#!/usr/bin/env python3
"""
Script de geração de PDFs acadêmicos para:
- 7.2 - Testes de energia.md -> 7.2 - Testes de energia.pdf
- 7.4 - Testes de software.md -> 7.4 - Testes de software.pdf

Padrão Universidade de Brasília (UnB/FCTE), com diagramação ABNT,
suporte a páginas em paisagem para tabelas densas e resolução de imagens.
"""

import os
import re
import markdown
import weasyprint

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DOCS_DIR = os.path.join(BASE_DIR, "docs")

CSS_BASE = '''
/* Layout e Páginas A4 */
@page {
  size: A4 portrait;
  margin: 22mm 18mm 22mm 18mm;

  @top-left {
    content: "Universidade de Brasília • PI1 2026.2";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #475569;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }

  @top-right {
    content: "%DOC_TITLE_HEADER%";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }

  @bottom-left {
    content: "Faculdade de Ciências e Tecnologias em Engenharia - FCTE";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #64748b;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }

  @bottom-right {
    content: "Página " counter(page) " de " counter(pages);
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }
}

/* Capa: sem cabeçalho e sem rodapé */
@page:first {
  margin: 0;
  @top-left { content: none; }
  @top-right { content: none; }
  @bottom-left { content: none; }
  @bottom-right { content: none; }
}

/* Página Paisagem para Tabelas Densas */
@page landscape_page {
  size: A4 landscape;
  margin: 16mm 15mm 16mm 15mm;

  @top-left {
    content: "Universidade de Brasília • FCTE - Projeto Integrador 1";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #475569;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }

  @top-right {
    content: "%DOC_TITLE_HEADER% (Tabelas de Referência)";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-bottom: 0.5pt solid #cbd5e1;
    padding-bottom: 3px;
  }

  @bottom-left {
    content: "Micromouse - Grupo 1 (2026.2)";
    font-size: 8pt;
    font-family: Arial, sans-serif;
    color: #64748b;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }

  @bottom-right {
    content: "Página " counter(page) " de " counter(pages);
    font-size: 8pt;
    font-family: Arial, sans-serif;
    font-weight: bold;
    color: #0f172a;
    border-top: 0.5pt solid #e2e8f0;
    padding-top: 3px;
  }
}

.page-landscape {
  page: landscape_page;
  page-break-before: always;
  page-break-after: always;
}

body {
  font-family: "Liberation Sans", Arial, Helvetica, sans-serif;
  font-size: 9.2pt;
  line-height: 1.45;
  color: #1e293b;
  margin: 0;
  padding: 0;
}

/* Capa ABNT UnB */
.abnt-cover {
  height: 297mm;
  padding: 30mm 25mm 25mm 25mm;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  text-align: center;
  background-color: #ffffff;
  page-break-after: always;
}

.cover-inst-header {
  text-transform: uppercase;
  font-weight: bold;
  line-height: 1.4;
  color: #000000;
}

.cover-inst-name {
  font-size: 13pt;
  letter-spacing: 0.5px;
}

.cover-faculty-name {
  font-size: 11pt;
  color: #334155;
  margin-top: 4px;
}

.cover-discipline-name {
  font-size: 10pt;
  color: #475569;
  margin-top: 4px;
}

.cover-authors {
  font-size: 9.5pt;
  line-height: 1.6;
  color: #1e293b;
  margin-top: 25mm;
}

.cover-title-area {
  margin-top: 25mm;
}

.cover-doc-title {
  font-size: 17pt;
  font-weight: bold;
  text-transform: uppercase;
  color: #000000;
  margin-bottom: 8px;
}

.cover-doc-subtitle {
  font-size: 11pt;
  font-style: italic;
  color: #334155;
}

.cover-natureza {
  margin-top: 25mm;
  margin-left: 42%;
  text-align: justify;
  font-size: 8.5pt;
  line-height: 1.4;
  color: #334155;
}

.cover-location-year {
  font-size: 10.5pt;
  font-weight: bold;
  text-transform: uppercase;
  color: #000000;
}

/* Tipografia */
h1 {
  font-size: 14pt;
  font-weight: bold;
  color: #0f172a;
  border-bottom: 1.5pt solid #0f172a;
  padding-bottom: 4px;
  margin: 18px 0 10px 0;
}

h2 {
  font-size: 11.5pt;
  font-weight: bold;
  color: #0f3b6c;
  border-bottom: 0.8pt solid #cbd5e1;
  padding-bottom: 3px;
  margin: 18px 0 8px 0;
  page-break-after: avoid;
}

h3 {
  font-size: 10pt;
  font-weight: bold;
  color: #1e293b;
  margin: 14px 0 6px 0;
  page-break-after: avoid;
}

h4 {
  font-size: 9pt;
  font-weight: bold;
  color: #334155;
  margin: 10px 0 4px 0;
  page-break-after: avoid;
}

p {
  margin: 0 0 8px 0;
  text-align: justify;
}

/* Tabelas */
table {
  width: 100%;
  border-collapse: collapse;
  margin: 8px 0 12px 0;
  font-size: 7.5pt;
  page-break-inside: auto;
}

tr {
  page-break-inside: avoid;
}

th {
  background-color: #0f3b6c;
  color: #ffffff;
  font-weight: bold;
  text-align: left;
  padding: 5px 6px;
  border: 0.5pt solid #0f3b6c;
  font-size: 7.5pt;
}

td {
  padding: 4px 6px;
  border: 0.5pt solid #cbd5e1;
  vertical-align: top;
}

tr:nth-child(even) td {
  background-color: #f8fafc;
}

.status-pass {
  color: #15803d;
  font-weight: bold;
}

.status-warn {
  color: #b45309;
  font-weight: bold;
}

.status-pend {
  color: #64748b;
  font-weight: bold;
}

/* Caixas de Destaque / Admonitions */
.admonition {
  margin: 8px 0;
  padding: 7px 10px;
  border-radius: 2px;
  background-color: #f8fafc;
  border-left: 3pt solid #64748b;
  page-break-inside: avoid;
}

.admonition.quote {
  border-left-color: #0f3b6c;
  background-color: #f1f5f9;
}

.admonition.info {
  border-left-color: #0284c7;
  background-color: #f0f9ff;
}

.admonition.success {
  border-left-color: #16a34a;
  background-color: #f0fdf4;
}

.admonition.warning {
  border-left-color: #f59e0b;
  background-color: #fffbeb;
}

.admonition.note {
  border-left-color: #6366f1;
  background-color: #eef2ff;
}

.admonition-title {
  font-weight: bold;
  font-size: 8.5pt;
  color: #0f172a;
  margin-bottom: 4px;
}

.admonition p:last-child {
  margin-bottom: 0;
}

/* Códigos e Terminais */
pre {
  background-color: #0f172a;
  color: #f1f5f9;
  padding: 7px 9px;
  border-radius: 3px;
  font-family: "Liberation Mono", Consolas, monospace;
  font-size: 7pt;
  line-height: 1.35;
  page-break-inside: avoid;
  margin: 6px 0;
}

code {
  font-family: "Liberation Mono", Consolas, monospace;
  font-size: 7.8pt;
  background-color: #f1f5f9;
  color: #0f172a;
  padding: 1px 3px;
  border-radius: 2px;
}

pre code {
  background-color: transparent;
  color: inherit;
  padding: 0;
}

/* Imagens */
img {
  display: block;
  margin: 8px auto;
  max-width: 88%;
  max-height: 220px;
  height: auto;
  border: 0.5pt solid #cbd5e1;
  page-break-inside: avoid;
}

em {
  display: block;
  text-align: center;
  font-size: 7.8pt;
  color: #64748b;
  margin-top: 3px;
  margin-bottom: 10px;
}

ul, ol {
  margin: 3px 0 8px 0;
  padding-left: 20px;
}

li {
  margin-bottom: 2px;
}

.math-display {
  text-align: center;
  font-family: "Liberation Serif", Times, serif;
  font-size: 9.5pt;
  margin: 8px 0;
  font-style: italic;
}

hr {
  border: none;
  border-top: 0.5pt solid #cbd5e1;
  margin: 12px 0;
}
'''

def clean_latex_and_symbols(text):
    text = text.replace("—", "-")
    """Substitui expressões LaTeX comuns e ícones por equivalentes limpos em texto/HTML."""
    substitutions = [
        (r'$\approx 0\text{ A}$', '≈ 0 A'),
        (r'$0,00\text{ A}$', '0,00 A'),
        (r'$14,8\text{ nA}$', '14,8 nA'),
        (r'$7,4\text{ nA}$', '7,4 nA'),
        (r'$120\text{ mA}$', '120 mA'),
        (r'$148\text{ mA}$', '148 mA'),
        (r'$296\text{ mA}$', '296 mA'),
        (r'$80\text{ mA}$', '80 mA'),
        (r'$40\text{ mA}$', '40 mA'),
        (r'$150\text{ mA}$', '150 mA'),
        (r'$220\text{ mA}$', '220 mA'),
        (r'$230\text{ mA}$', '230 mA'),
        (r'$300\text{ mA}$', '300 mA'),
        (r'$359\text{ mA}$', '359 mA'),
        (r'$0,55\text{ A}$', '0,55 A'),
        (r'$1,10\text{ A}$', '1,10 A'),
        (r'$1,46\text{ A}$', '1,46 A'),
        (r'$1,6\text{ A}$', '1,6 A'),
        (r'$3,95\text{ A}$', '3,95 A'),
        (r'$0\text{ A}$', '0 A'),
        (r'$0\text{ mA}$', '0 mA'),
        (r'$1500\text{ mAh}$', '1500 mAh'),
        (r'$7,4\text{ V}$', '7,4 V'),
        (r'$7,58\text{ V}$', '7,58 V'),
        (r'$7,59\text{ V}$', '7,59 V'),
        (r'$7,60\text{ V}$', '7,60 V'),
        (r'$6\text{ V}$', '6 V'),
        (r'$3,3\text{ V}$', '3,3 V'),
        (r'$5,0\text{ V}$', '5,0 V'),
        (r'$6,6\text{ V}$', '6,6 V'),
        (r'$1\text{ G}\Omega$', '1 GΩ'),
        (r'$50\ \Omega$', '50 Ω'),
        (r'$4,93\ \Omega$', '4,93 Ω'),
        (r'$1,2\text{ s}$', '1,2 s'),
        (r'$\ge 15\text{ min}$', '≥ 15 min'),
        (r'$\ge 30\text{ min}$', '≥ 30 min'),
        (r'$\le 200\text{ ms}$', '≤ 200 ms'),
        (r'$\le 500\text{ ms}$', '≤ 500 ms'),
        (r'$\le 90\text{ min}$', '≤ 90 min'),
        (r'$\approx 391\text{ min}$', '≈ 391 min'),
        (r'$\approx 304\text{ min}$', '≈ 304 min'),
        (r'$\approx 300\text{ min}$', '≈ 300 min'),
        (r'$\approx 62\text{ min}$', '≈ 62 min'),
        (r'$\approx 6,5\text{ h}$', '≈ 6,5 h'),
        (r'$\pm 5\%$', '±5%'),
        (r'$1,5\%$', '1,5%'),
        (r'$7,4\text{ V} \div 1\text{ G}\Omega$', '7,4 V / 1 GΩ'),
        (r'$V_1 = 7,4\text{ V}$', 'V1 = 7,4 V'),
    ]
    for old, new in substitutions:
        text = text.replace(old, new)

    text = text.replace(':white_check_mark:', '<span class="status-pass">Conforme</span>')
    text = text.replace(':warning:', '<span class="status-warn">Parcial</span>')
    text = text.replace(':hourglass_flowing_sand:', '<span class="status-pend">Pendente</span>')

    # Remove atributos Markdown do tipo { width="..." }
    text = re.sub(r'\{[ ]*width=[^}]+\}', '', text)

    # Substitui equações de bloco $$...$$ por divs formatadas
    def repl_block_math(match):
        eq = match.group(1).strip()
        eq = eq.replace(r'\text{Autonomia}', 'Autonomia')
        eq = eq.replace(r'\text{Autonomia simulada}', 'Autonomia simulada')
        eq = eq.replace(r'\text{ mAh}', ' mAh')
        eq = eq.replace(r'\text{ mA}', ' mA')
        eq = eq.replace(r'\text{ min}', ' min')
        eq = eq.replace(r'\text{ h}', ' h')
        eq = eq.replace(r'\times', '×')
        eq = eq.replace(r'\approx', '≈')
        # Limpa frações básicas \frac{A}{B}
        eq = re.sub(r'\\frac\{([^}]+)\}\{([^}]+)\}', r'(\1 / \2)', eq)
        return f'<div class="math-display">{eq}</div>'

    text = re.sub(r'\$\$(.*?)\$\$', repl_block_math, text, flags=re.DOTALL)

    return text

def fix_image_paths(text):
    """Transforma caminhos relativos de imagens em URLs file:// absolutas."""
    def repl_img(match):
        rel_path = match.group(1)
        abs_path = os.path.normpath(os.path.join(DOCS_DIR, rel_path))
        return f'(file://{abs_path})'

    return re.sub(r'\((figs/[^)]+|evidencias/[^)]+)\)', repl_img, text)

def build_pdf_energia():
    md_file = os.path.join(DOCS_DIR, "7.2 - Testes de energia.md")
    pdf_out_root = os.path.join(BASE_DIR, "7.2 - Testes de energia.pdf")
    pdf_out_docs = os.path.join(DOCS_DIR, "7.2 - Testes de energia.pdf")

    print(f"\n--- Gerando PDF de Energia de: {md_file} ---")
    with open(md_file, "r", encoding="utf-8") as f:
        content = f.read()

    content = clean_latex_and_symbols(content)
    content = fix_image_paths(content)
    content = re.sub(r'\[↑ Voltar ao Painel Geral de Testes\]\(#[^)]+\)', '', content)

    html_body = markdown.markdown(
        content,
        extensions=[
            'markdown.extensions.tables',
            'markdown.extensions.admonition',
            'markdown.extensions.fenced_code',
            'markdown.extensions.toc',
        ]
    )

    # Envolver Painel Geral e Matriz de Requisitos em Paisagem
    html_body = re.sub(
        r'(<h2[^>]*>Painel Geral de Ensaios de Energia</h2>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    html_body = re.sub(
        r'(<h2[^>]*>6\. Matriz de Rastreabilidade dos Requisitos de Energia</h2>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    css = CSS_BASE.replace("%DOC_TITLE_HEADER%", "7.2 - Testes de Energia (Micromouse Grupo 1)")

    html_full = f'''<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<title>Relatório de Testes de Energia - Micromouse Grupo 1</title>
<style>
{css}
</style>
</head>
<body>

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
    <div class="cover-doc-title">Relatório de Testes de Energia</div>
    <div class="cover-doc-subtitle">Simulações e Ensaios de Bancada de Consumo de Corrente da Tração, Autonomia e Comportamento Térmico</div>
  </div>

  <div class="cover-natureza">
    Relatório técnico de ensaios laboratoriais e simulações apresentado à disciplina de Projeto Integrador de Engenharia 1 da Universidade de Brasília (FCTE/UnB), como requisito de validação do subsistema de energia do robô Micromouse (Grupo 1).<br><br>
    <strong>Orientador:</strong> Prof. Dr. Lui
  </div>

  <div class="cover-location-year">
    Gama - DF<br>
    2026
  </div>
</div>

<div class="document-content">
{html_body}
</div>

</body>
</html>'''

    print("Renderizando com WeasyPrint...")
    html_obj = weasyprint.HTML(string=html_full, base_url=DOCS_DIR)
    html_obj.write_pdf(pdf_out_root)
    html_obj.write_pdf(pdf_out_docs)

    size_kb = os.path.getsize(pdf_out_root) / 1024
    print(f"Sucesso! PDF de Energia gerado:")
    print(f" - {pdf_out_root} ({size_kb:.1f} kB)")
    print(f" - {pdf_out_docs} ({size_kb:.1f} kB)")

def build_pdf_software():
    md_file = os.path.join(DOCS_DIR, "7.4 - Testes de software.md")
    pdf_out_root = os.path.join(BASE_DIR, "7.4 - Testes de software.pdf")
    pdf_out_docs = os.path.join(DOCS_DIR, "7.4 - Testes de software.pdf")

    print(f"\n--- Gerando PDF de Software de: {md_file} ---")
    with open(md_file, "r", encoding="utf-8") as f:
        content = f.read()

    content = clean_latex_and_symbols(content)
    content = fix_image_paths(content)
    content = re.sub(r'\[↑ Voltar[^\]]+\]\(#[^)]+\)', '', content)

    html_body = markdown.markdown(
        content,
        extensions=[
            'markdown.extensions.tables',
            'markdown.extensions.admonition',
            'markdown.extensions.fenced_code',
            'markdown.extensions.toc',
        ]
    )

    # Envolver as tabelas densas em Paisagem:
    # 1. Tabela de HUs implementadas (Seção 1)
    # 2. Tabela de Associação HUs x Casos de Teste x Automação (Seção 2)
    # 3. Tabela de Roteiro de Testes Atualizado (Seção 6)
    html_body = re.sub(
        r'(<h3[^>]*>1\. Histórias de usuário implementadas e protótipos</h3>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    html_body = re.sub(
        r'(<h3[^>]*>2\. Associação entre histórias de usuário, casos de teste e automação</h3>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    html_body = re.sub(
        r'(<h3[^>]*>6\. Roteiro de testes atualizado</h3>.*?</table>)',
        r'<div class="page-landscape">\1</div>',
        html_body,
        flags=re.DOTALL
    )

    css = CSS_BASE.replace("%DOC_TITLE_HEADER%", "7.4 - Testes de Software (Micromouse Grupo 1)")

    html_full = f'''<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<title>Relatório de Testes de Software - Micromouse Grupo 1</title>
<style>
{css}
</style>
</head>
<body>

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
    <div class="cover-doc-title">Relatório de Testes de Software</div>
    <div class="cover-doc-subtitle">Validação Funcional, Cobertura Automatizada (Firmware, Backend e Frontend) e Testes de Sistema E2E</div>
  </div>

  <div class="cover-natureza">
    Relatório técnico de testes de software apresentado à disciplina de Projeto Integrador de Engenharia 1 da Universidade de Brasília (FCTE/UnB), contemplando cobertura de testes automatizados unitários, de integração e ponta a ponta (E2E) do robô Micromouse (Grupo 1).<br><br>
    <strong>Orientador:</strong> Prof. Dr. Lui
  </div>

  <div class="cover-location-year">
    Gama - DF<br>
    2026
  </div>
</div>

<div class="document-content">
{html_body}
</div>

</body>
</html>'''

    print("Renderizando com WeasyPrint...")
    html_obj = weasyprint.HTML(string=html_full, base_url=DOCS_DIR)
    html_obj.write_pdf(pdf_out_root)
    html_obj.write_pdf(pdf_out_docs)

    size_kb = os.path.getsize(pdf_out_root) / 1024
    print(f"Sucesso! PDF de Software gerado:")
    print(f" - {pdf_out_root} ({size_kb:.1f} kB)")
    print(f" - {pdf_out_docs} ({size_kb:.1f} kB)")

if __name__ == "__main__":
    build_pdf_energia()
    build_pdf_software()
