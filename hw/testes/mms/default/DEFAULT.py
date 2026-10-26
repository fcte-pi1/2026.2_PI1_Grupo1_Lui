import API
import sys

def log(string):
    """Escreve mensagens no console do simulador (stderr)."""
    sys.stderr.write(f"{string}\n")
    sys.stderr.flush()

# ==============================================================================
# CONFIGURAÇÕES DO ALGORITMO E CRITÉRIOS DE SAÍDA
# ==============================================================================
# EXIT_MODE define como o robô identifica que encontrou a saída:
#
# 1. "auto" (Recomendado):
#    - Detecta automaticamente aberturas nas bordas externas do labirinto (perímetro aberto).
#    - Se as bordas forem todas fechadas:
#        * Em labirintos 16x16 (padrão Micromouse): identifica o centro {(7,7), (7,8), (8,7), (8,8)}.
#        * Em outros tamanhos (ex: 4x4, 8x4, 12x4): se TARGET_COORDS estiver definido, usa ele;
#          caso contrário, considera o canto oposto (width - 1, height - 1) ou explora todo o labirinto.
#
# 2. "borda":
#    - Considera saída estritamente uma abertura para fora do perímetro do labirinto.
#
# 3. "coordenada":
#    - Considera saída ao alcançar qualquer uma das coordenadas definidas em TARGET_COORDS.
# ==============================================================================

EXIT_MODE = "auto"

# Coordenada(s) manual(is) de saída, caso sejam conhecidas previamente.
# Exemplo para labirinto 4x4: TARGET_COORDS = {(3, 3)}
# Exemplo para labirinto 16x16: TARGET_COORDS = {(15, 15)}
# Deixe None para detecção automática.
TARGET_COORDS = None

# ==============================================================================
# CONSTANTES DE DIREÇÃO
# ==============================================================================
# 0: Norte (+Y), 1: Leste (+X), 2: Sul (-Y), 3: Oeste (-X)
DIR_CHARS = ['n', 'e', 's', 'w']
DIR_NAMES = ['Norte', 'Leste', 'Sul', 'Oeste']
DX = [0, 1, 0, -1]
DY = [1, 0, -1, 0]


def turn_to(current_dir, target_dir):
    """
    Gira o robô da orientação atual para a direção alvo com o menor número de curvas.
    Retorna a nova direção.
    """
    diff = (target_dir - current_dir) % 4
    if diff == 1:
        API.turnRight()
    elif diff == 2:
        API.turnRight()
        API.turnRight()
    elif diff == 3:
        API.turnLeft()
    return target_dir


def update_walls(x, y, current_dir, width, height, known_walls):
    """
    Lê os sensores da célula atual (frente, esquerda, direita) e atualiza
    tanto a memória do robô quanto a renderização gráfica no simulador.
    As paredes são registradas simetricamente para a célula atual e sua vizinha.
    """
    f_dir = current_dir
    l_dir = (current_dir - 1) % 4
    r_dir = (current_dir + 1) % 4

    sensor_checks = [
        (API.wallFront(), f_dir),
        (API.wallLeft(), l_dir),
        (API.wallRight(), r_dir),
    ]

    for has_wall, direction in sensor_checks:
        d_char = DIR_CHARS[direction]
        if has_wall:
            known_walls[(x, y)].add(d_char)
            API.setWall(x, y, d_char)

            # Parede simétrica na célula vizinha
            nx = x + DX[direction]
            ny = y + DY[direction]
            if 0 <= nx < width and 0 <= ny < height:
                opp_char = DIR_CHARS[(direction + 2) % 4]
                known_walls[(nx, ny)].add(opp_char)
                API.setWall(nx, ny, opp_char)


def is_exit(x, y, width, height, known_walls, mode="auto", target_coords=None):
    """
    Avalia se a célula (x, y) é a saída com base no modo configurado.
    """
    # 1. Coordenada específica configurada
    if target_coords and (x, y) in target_coords:
        return True

    # 2. Saída por abertura na borda externa do labirinto
    if mode in ("auto", "borda"):
        # Ignora a célula de largada (0, 0) para as paredes sul e oeste
        if x == 0 and 'w' not in known_walls.get((x, y), set()) and (x, y) != (0, 0):
            return True
        if x == width - 1 and 'e' not in known_walls.get((x, y), set()):
            return True
        if y == height - 1 and 'n' not in known_walls.get((x, y), set()):
            return True
        if y == 0 and 's' not in known_walls.get((x, y), set()) and (x, y) != (0, 0):
            return True

    # 3. Modo automático em labirinto clássico de Micromouse (16x16 com bordas fechadas)
    if mode == "auto":
        if width == 16 and height == 16:
            center_cells = {(7, 7), (7, 8), (8, 7), (8, 8)}
            if (x, y) in center_cells:
                return True
        elif target_coords is None:
            # Se for outra geometria e não tiver alvo definido nem borda aberta,
            # considera o canto oposto à largada
            if (x, y) == (width - 1, height - 1):
                return True

    return False


def main():
    log("==================================================")
    log("Iniciando Micromouse - Algoritmo de Busca de Saída")
    log("Algoritmo: DFS com Retrocesso (Busca de Trémaux)")
    log("==================================================")

    # Limpeza visual inicial
    API.clearAllColor()
    API.clearAllText()

    # Dimensões do labirinto informadas pelo simulador
    try:
        width = API.mazeWidth()
        height = API.mazeHeight()
    except Exception:
        width, height = 16, 16

    log(f"Dimensões detectadas: {width}x{height}")
    log(f"Modo de Saída: '{EXIT_MODE}'")

    # Estado de localização do robô
    cur_x, cur_y = 0, 0
    cur_dir = 0  # 0 = Norte

    # Estruturas de memória do robô (Zero informação prévia)
    visited = {(cur_x, cur_y)}
    path_stack = [(cur_x, cur_y)]  # Pilha de células para retrocesso (backtracking)
    known_walls = { (x, y): set() for x in range(width) for y in range(height) }

    # Marcação da célula de largada
    API.setColor(0, 0, "G")
    API.setText(0, 0, "S")

    step_count = 0
    exit_found = False

    while path_stack:
        # Passo 1: Detecta e mapeia as paredes da célula atual
        update_walls(cur_x, cur_y, cur_dir, width, height, known_walls)

        # Passo 2: Verifica se a célula atual é a saída
        if is_exit(cur_x, cur_y, width, height, known_walls, mode=EXIT_MODE, target_coords=TARGET_COORDS):
            log(f"-> SUCESSO: Saída encontrada em ({cur_x}, {cur_y})!")
            log(f"-> Total de passos dados na exploração: {step_count}")
            exit_found = True
            break

        # Passo 3: Escolhe o próximo vizinho não visitado e acessível
        # Ordem de preferência: Frente (0 curvas) > Esquerda (1 curva) > Direita (1 curva) > Trás (2 curvas)
        next_step = None
        for rel in [0, 3, 1, 2]:
            cand_dir = (cur_dir + rel) % 4
            c_char = DIR_CHARS[cand_dir]
            if c_char not in known_walls[(cur_x, cur_y)]:
                nx = cur_x + DX[cand_dir]
                ny = cur_y + DY[cand_dir]
                if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in visited:
                    next_step = (cand_dir, nx, ny)
                    break

        if next_step is not None:
            # Avança para nova célula
            target_dir, nx, ny = next_step
            cur_dir = turn_to(cur_dir, target_dir)
            API.moveForward()
            step_count += 1
            cur_x, cur_y = nx, ny
            visited.add((cur_x, cur_y))
            path_stack.append((cur_x, cur_y))

            # Atualiza visual no simulador (Azul para caminho em exploração)
            API.setColor(cur_x, cur_y, "B")
            API.setText(cur_x, cur_y, str(len(path_stack) - 1))
        else:
            # Beco sem saída: faz retrocesso (backtracking) pelo caminho de onde veio
            API.setColor(cur_x, cur_y, "D")  # Cor cinza escura para becos explorados
            path_stack.pop()
            if not path_stack:
                log("Todo o labirinto acessível foi explorado.")
                break

            target_x, target_y = path_stack[-1]
            back_dir = None
            for d in range(4):
                if cur_x + DX[d] == target_x and cur_y + DY[d] == target_y:
                    back_dir = d
                    break

            if back_dir is not None:
                cur_dir = turn_to(cur_dir, back_dir)
                API.moveForward()
                step_count += 1
                cur_x, cur_y = target_x, target_y

    # Ao finalizar, destaca a rota final se encontrou a saída
    if exit_found:
        log(f"-> Rota direta da largada até a saída: {len(path_stack)} células.")
        for px, py in path_stack:
            API.setColor(px, py, "G")  # Pinta de verde a rota ótima descoberta
            API.setText(px, py, "*")
        API.setText(path_stack[0][0], path_stack[0][1], "S")
        API.setText(path_stack[-1][0], path_stack[-1][1], "FIM")
    else:
        log("Varredura concluída. Nenhuma saída adicional encontrada.")


if __name__ == "__main__":
    main()
