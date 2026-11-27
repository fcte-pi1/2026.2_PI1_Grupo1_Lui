#ifndef PISTAS_SLIDES_H
#define PISTAS_SLIDES_H

// As três pistas de exemplo dos slides do tema do PI1 (slides 3, 4 e 5), copiadas sem
// mudança de src/backend/app/simulador/pistas.txt, para o robô virtual percorrer no
// ambiente nativo as mesmas pistas que o simulador do backend mostra no painel.
// Formato e leitor: LeitorPistas.h. As figuras do 8x4 e do 12x4 têm regiões fechadas,
// sem acesso a partir da largada (7 células no 8x4 e 8 no 12x4).

const char* const PISTA_SLIDES_4X4 = R"(
+---+---+---+---+
| L             |
+---+---+   +---+
|       |   |   |
+   +   +   +   +
|               |
+   +---+---+---+
|             C |
+---+---+---+---+
)";

const char* const PISTA_SLIDES_8X4 = R"(
+---+---+---+---+
| L             |
+---+   +---+---+
|   |   |       |
+   +   +   +   +
|   |   |       |
+   +   +---+   +
|           |   |
+   +   +---+   +
|       |       |
+---+   +---+---+
|   |           |
+   +---+---+   +
|               |
+   +---+---+   +
|             C |
+---+---+---+---+
)";

const char* const PISTA_SLIDES_12X4 = R"(
+---+---+---+---+
| L             |
+---+   +---+---+
|   |           |
+   +---+   +   +
|       |       |
+   +   +---+   +
|           |   |
+   +   +   +   +
|           |   |
+---+   +---+   +
|   |           |
+   +---+   +   +
|       |       |
+   +---+   +---+
|   |       |   |
+---+   +---+   +
|       |       |
+   +   +---+   +
|           |   |
+   +---+   +---+
|   |   |       |
+   +   +---+---+
|             C |
+---+---+---+---+
)";

#endif
