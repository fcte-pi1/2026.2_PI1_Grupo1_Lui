# Mecânica e Estruturas — Micromouse (Grupo 1)

Esta pasta reúne os arquivos do projeto estrutural do Micromouse: modelagem CAD, arquivos de impressão 3D e imagens.

O detalhamento das decisões de projeto (chassi, tração e rodagem, materiais, verificação de requisitos e rastreabilidade) está no documento **4.1 — Projeto Conceitual de Estruturas** (AP5): **[inserir link]**.

---

## Estrutura da pasta

```
mecanica-estruturas/
├── README.md
├── cad/            # arquivos CATIA (.CATPart, .CATProduct)
├── impressao3d/    # arquivos .stl
└── imagens/        # imagens rasterizadas (.png, .jpg)
```

---

## Arquivos CAD (CATIA)

| Arquivo | Descrição |
| --- | --- |
| `[nome].CATProduct` | Montagem completa do robô |
| `[nome].CATPart` | [peça] |
| `[nome].CATPart` | [peça] |

> Os arquivos nativos do CATIA exigem licença do software para abrir.

---

## Arquivos de impressão 3D

| Arquivo | Peça | Material |
| --- | --- | --- |
| `[nome].stl` | [peça] | [PLA/PETG] |
| `[nome].stl` | [peça] | [PLA/PETG] |

---

## Imagens

| Arquivo | Descrição |
| --- | --- |
| `micromouse_chassi_vista_superior.png` | Vista superior do chassi |
| `micromouse_chassi_vista_lateral.png` | Vista lateral do chassi |
| `micromouse_chassi_vista_frontal.png` | Vista frontal do chassi |
| `visao_isometrica_catia.jpg` | Vista isométrica no CATIA |

![Vista isométrica do chassi no CATIA](imagens/visao_isometrica_catia.jpg)
