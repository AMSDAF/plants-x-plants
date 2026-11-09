/* Desenha a arena provisória e encaminha comandos ao motor de combate. */
#include "battle_screen.h"

#include "ai.h"
#include "plant_data.h"
#include "raylib.h"

static const Color ink = {28, 58, 46, 255};
static const Color paper = {253, 245, 216, 255};

static const int move_keys[MAX_MOVES] = {KEY_ONE, KEY_TWO};
static const int switch_keys[MAX_TEAM_SIZE] = {KEY_Q, KEY_W, KEY_E, KEY_R};
static const char *const switch_labels[MAX_TEAM_SIZE] = {"Q", "W", "E", "R"};

static const Rectangle restart_button = {64, 565, 265, 54};
static const Rectangle menu_button = {347, 565, 265, 54};
static const Rectangle logic_button = {634, 567, 582, 48};
static const Rectangle setup_left = {64, 225, 400, 58};
static const Rectangle setup_operator = {480, 225, 200, 58};
static const Rectangle setup_right = {696, 225, 520, 58};
static const Rectangle setup_confirm = {64, 602, 370, 54};
static const Rectangle remove_reserve = {453, 602, 430, 54};
enum { ROSTER_COLUMNS = 4 };

static const Color species_colors[SPECIES_COUNT] = {
    [SPECIES_CACTUS] = {56, 123, 78, 255},
    [SPECIES_SUNFLOWER] = {225, 154, 39, 255},
    [SPECIES_ALOE] = {65, 162, 132, 255},
    [SPECIES_ROSE] = {208, 61, 83, 255},
    [SPECIES_IVY] = {89, 125, 51, 255},
    [SPECIES_BAMBOO] = {144, 170, 53, 255},
    [SPECIES_FERN] = {45, 142, 84, 255},
    [SPECIES_LAVENDER] = {146, 109, 184, 255},
    [SPECIES_ORCHID] = {194, 80, 167, 255},
    [SPECIES_LOTUS] = {231, 133, 157, 255},
    [SPECIES_MINT] = {72, 177, 118, 255},
    [SPECIES_CLOVER] = {70, 150, 61, 255}
};

static const char *const conditions[LOGIC_CONDITION_COUNT] = {
    "Sua ativa tem até metade do HP",
    "O adversário tem reserva viva",
    "Ativa rival tem até 50% de HP",
    "Você tem reserva viva"
};
static const char *const operators[LOGIC_OPERATOR_COUNT] = {"E", "OU", "XOR"};
static const char *const operator_rules[LOGIC_OPERATOR_COUNT] = {
    "As duas condições precisam ser verdadeiras.",
    "Pelo menos uma condição precisa ser verdadeira.",
    "Exatamente uma condição precisa ser verdadeira."
};

static const char *side_name(const BattleScreen *screen, int side)
{
    if (side == 0) return "Jogador 1";
    return screen->versus_cpu ? "Computador" : "Jogador 2";
}

static const Team *turn_team(const BattleState *battle)
{
    return battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
}

static bool battle_over(const BattleState *battle)
{
    return battle->state == BATTLE_STATE_PLAYER_WIN || battle->state == BATTLE_STATE_ENEMY_WIN;
}

static bool cpu_turn(const BattleScreen *screen)
{
    return screen->versus_cpu && screen->battle.current_turn == 1 && !battle_over(&screen->battle);
}

static Rectangle move_button(int index)
{
    return (Rectangle){64 + 285 * index, 567, 269, 48};
}

static Rectangle switch_button(int index)
{
    return (Rectangle){64 + 285 * index, 635, 269, 48};
}

static bool activate(Rectangle bounds, int key)
{
    return IsKeyPressed(key) ||
           (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), bounds));
}

static Rectangle species_card(int index)
{
    return (Rectangle){64 + 290 * (index % ROSTER_COLUMNS),
                       192 + 113 * (index / ROSTER_COLUMNS), 276, 96};
}

static Rectangle team_slot_button(int index)
{
    return (Rectangle){64 + 290 * index, 536, 276, 48};
}

static bool update_team_selection(BattleScreen *screen)
{
    Team *team = screen->setup_side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
    if (activate(setup_confirm, KEY_ENTER)) {
        screen->choosing_team = false;
        return true;
    }

    for (int i = 0; i < MAX_TEAM_SIZE && i <= team->count; i++) {
        if (activate(team_slot_button(i), switch_keys[i])) screen->team_slot = i;
    }
    if (activate(remove_reserve, KEY_BACKSPACE)) {
        Team_RemoveReserve(team, screen->team_slot);
        return false;
    }

    int cursor = screen->roster_cursor;
    if (IsKeyPressed(KEY_LEFT)) cursor--;
    else if (IsKeyPressed(KEY_RIGHT)) cursor++;
    else if (IsKeyPressed(KEY_UP)) cursor -= ROSTER_COLUMNS;
    else if (IsKeyPressed(KEY_DOWN)) cursor += ROSTER_COLUMNS;
    screen->roster_cursor = (cursor + SPECIES_COUNT) % SPECIES_COUNT;

    int selected = IsKeyPressed(KEY_SPACE) ? screen->roster_cursor : -1;
    for (int i = 0; i < SPECIES_COUNT; i++) {
        if (activate(species_card(i), KEY_NULL)) {
            screen->roster_cursor = i;
            selected = i;
            break;
        }
    }
    if (selected >= 0 && Team_SelectSpecies(team, screen->team_slot, selected))
        screen->team_slot = (screen->team_slot + 1) % team->count;
    return false;
}

static LogicCondition next_condition(LogicCondition current, LogicCondition other)
{
    LogicCondition next = (current + 1) % LOGIC_CONDITION_COUNT;
    if (next == other) next = (next + 1) % LOGIC_CONDITION_COUNT;
    return next;
}

static void update_setup(BattleScreen *screen)
{
    LogicFormula *draft = &screen->draft;
    if (activate(setup_left, KEY_Q)) {
        draft->left = next_condition(draft->left, draft->right);
    } else if (activate(setup_right, KEY_W)) {
        draft->right = next_condition(draft->right, draft->left);
    } else if (activate(setup_operator, KEY_E)) {
        draft->operator = (draft->operator + 1) % LOGIC_OPERATOR_COUNT;
    } else if (activate(setup_confirm, KEY_ENTER)) {
        screen->battle.formulas[screen->setup_side] = *draft;
        if (screen->setup_side == 0 && !screen->versus_cpu) {
            screen->setup_side = 1;
            screen->choosing_team = true;
            screen->roster_cursor = 0;
            screen->team_slot = 0;
            *draft = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_RESERVE, LOGIC_AND};
        } else {
            screen->setup_side = -1;
        }
    }
}

BattleScreenAction battle_screen_update(BattleScreen *screen, float dt)
{
    BattleState *battle = &screen->battle;
    if (IsKeyPressed(KEY_ESCAPE)) return BATTLE_SCREEN_MENU;

    if (screen->setup_side >= 0) {
        if (screen->choosing_team) {
            if (update_team_selection(screen)) return BATTLE_SCREEN_TEAM_READY;
        } else {
            update_setup(screen);
        }
        return BATTLE_SCREEN_NONE;
    }

    if (battle_over(battle)) {
        if (activate(restart_button, KEY_ENTER)) return BATTLE_SCREEN_RESTART;
        if (activate(menu_button, KEY_M)) return BATTLE_SCREEN_MENU;
        return BATTLE_SCREEN_NONE;
    }

    if (cpu_turn(screen)) {
        screen->cpu_wait += dt > 0.1f ? 0.1f : dt;
        if (screen->cpu_wait >= 0.65f) {
            AI_PlayTurn(battle);
            screen->cpu_wait = 0;
        }
        return BATTLE_SCREEN_NONE;
    }

    if (battle->state == BATTLE_STATE_ONGOING) {
        if (activate(logic_button, KEY_THREE)) {
            Battle_ExecuteMove(battle, BATTLE_LOGIC_MOVE_INDEX);
            return BATTLE_SCREEN_NONE;
        }
        for (int i = 0; i < MAX_MOVES; i++) {
            if (activate(move_button(i), move_keys[i])) {
                Battle_ExecuteMove(battle, i);
                return BATTLE_SCREEN_NONE;
            }
        }
    }

    for (int i = 0; i < turn_team(battle)->count; i++) {
        if (activate(switch_button(i), switch_keys[i])) {
            Battle_SwitchPlant(battle, i);
            break;
        }
    }
    return BATTLE_SCREEN_NONE;
}

static void draw_button(Rectangle bounds, const char *label, bool enabled)
{
    bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRec(bounds, enabled ? (hovered ? GOLD : paper) : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, ink);
    DrawText(label, (int)bounds.x + 12, (int)bounds.y + ((int)bounds.height - 20) / 2,
             20, enabled ? ink : DARKGRAY);
}

static void draw_team_selection(const BattleScreen *screen)
{
    const Team *team = screen->setup_side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
    DrawText("Monte sua equipe", 64, 42, 40, ink);
    DrawText("ESC · Menu", 1092, 43, 20, ink);
    DrawText(TextFormat("%s  ·  %d/%d plantas no grupo", side_name(screen, screen->setup_side),
                        team->count, MAX_TEAM_SIZE),
             64, 104, 26, ink);
    DrawText("Escolha uma vaga abaixo, depois clique numa planta ou use as setas e Espaço.",
             64, 149, 20, ink);

    for (int i = 0; i < SPECIES_COUNT; i++) {
        const SpeciesData *species = PlantData_GetSpecies((SpeciesId)i);
        Rectangle card = species_card(i);
        int x = (int)card.x;
        int y = (int)card.y;
        bool focused = i == screen->roster_cursor || CheckCollisionPointRec(GetMousePosition(), card);
        DrawRectangleRec(card, focused ? GOLD : paper);
        DrawRectangleLinesEx(card, focused ? 3 : 2, ink);
        DrawCircle(x + 22, y + 25, 9, species_colors[i]);
        DrawText(species->name_pt, x + 39, y + 14, 22, ink);
        DrawText(TextFormat("HP %d  ATQ %d  DEF %d", species->base_hp, species->base_attack,
                            species->base_defense), x + 14, y + 48, 17, ink);
        for (int slot = 0; slot < team->count; slot++) {
            if (team->plants[slot].id != i) continue;
            const char *role = slot == 0 ? "ATIVA" : TextFormat("RESERVA %d", slot);
            DrawText(role, x + 14, y + 72, 15, ink);
        }
    }
    for (int i = 0; i < MAX_TEAM_SIZE; i++) {
        const char *name = i < team->count ? team->plants[i].name_pt : "Vaga livre";
        Rectangle bounds = team_slot_button(i);
        draw_button(bounds, TextFormat("%s  %s", switch_labels[i], name), i <= team->count);
        if (i == screen->team_slot) DrawRectangleLinesEx(bounds, 4, ORANGE);
    }
    draw_button(setup_confirm, "ENTER  Confirmar equipe", true);
    draw_button(remove_reserve, "BACKSPACE  Remover reserva",
                team->count > MIN_VERSUS_TEAM_SIZE && screen->team_slot > 0 && screen->team_slot < team->count);
    const char *hint = screen->team_slot == team->count
        ? "Escolha uma espécie que ainda não está no grupo."
        : "A primeira planta começa ativa. As demais ficam na reserva.";
    DrawText(hint, 64, 676, 19, ink);
}

static const char *truth(int value)
{
    return value ? "V" : "F";
}

static void draw_setup(const BattleScreen *screen)
{
    LogicFormula formula = screen->draft;
    LogicEvaluation value = Battle_EvaluateFormula(&screen->battle, screen->setup_side, formula);
    const Move *move = Battle_LogicMove(formula.operator);

    DrawText("Monte seu próprio golpe", 64, 42, 40, ink);
    DrawText("ESC · Menu", 1092, 43, 20, ink);
    DrawText(TextFormat("Montagem de %s", side_name(screen, screen->setup_side)), 64, 110, 28, ink);
    DrawText("Escolha duas condições diferentes e combine com um operador.", 64, 158, 22, ink);
    DrawText("Condição P", 64, 196, 20, ink);
    DrawText("Operador", 480, 196, 20, ink);
    DrawText("Condição Q", 696, 196, 20, ink);
    draw_button(setup_left, TextFormat("Q  %s", conditions[formula.left]), true);
    draw_button(setup_operator, TextFormat("[E]  %s", operators[formula.operator]), true);
    draw_button(setup_right, TextFormat("W  %s", conditions[formula.right]), true);

    DrawRectangle(64, 319, 1152, 197, paper);
    DrawText(TextFormat("P %s Q", operators[formula.operator]), 86, 341, 33, ink);
    DrawText(operator_rules[formula.operator], 86, 391, 24, ink);
    DrawText(TextFormat("Poder %d  ·  Precisão %d%%", move->power, move->accuracy), 86, 433, 22, ink);
    DrawText(TextFormat("Com as equipes no início: %s %s %s = %s",
                        truth(value.left), operators[formula.operator], truth(value.right),
                        truth(value.result)), 86, 471, 22, ink);
    draw_button(setup_confirm, "ENTER  Confirmar fórmula", true);
}

static void draw_plant(const Plant *plant, int x, int y)
{
    DrawEllipse(x, y + 56, 73, 14, Fade(ink, 0.25f));
    if (!Plant_IsAlive(plant)) {
        DrawText("Derrotada", x - 49, y + 17, 20, ink);
        return;
    }

    if (plant->id == SPECIES_CACTUS) {
        DrawRectangleRounded((Rectangle){x - 23, y - 57, 46, 112}, 0.65f, 8, DARKGREEN);
        DrawRectangleRounded((Rectangle){x - 47, y - 19, 94, 32}, 0.70f, 8, GREEN);
        DrawRectangle(x - 12, y - 35, 5, 10, paper);
        DrawRectangle(x + 9, y - 35, 5, 10, paper);
    } else if (plant->id == SPECIES_SUNFLOWER) {
        DrawRectangle(x - 5, y - 11, 10, 66, DARKGREEN);
        DrawEllipse(x + 19, y + 33, 22, 10, GREEN);
        DrawCircle(x, y - 24, 40, ORANGE);
        DrawCircle(x, y - 24, 25, BROWN);
        DrawCircle(x - 8, y - 29, 3, paper);
        DrawCircle(x + 8, y - 29, 3, paper);
    } else {
        Color color = species_colors[plant->id];
        DrawRectangle(x - 5, y - 20, 10, 75, DARKGREEN);
        DrawEllipse(x - 17, y + 31, 23, 11, DARKGREEN);
        DrawEllipse(x + 19, y + 19, 23, 10, GREEN);
        if (plant->id == SPECIES_ROSE || plant->id == SPECIES_ORCHID ||
            plant->id == SPECIES_LAVENDER || plant->id == SPECIES_LOTUS) {
            DrawEllipse(x - 23, y - 23, 22, 20, color);
            DrawEllipse(x + 23, y - 23, 22, 20, color);
            DrawEllipse(x, y - 39, 27, 25, color);
            DrawCircle(x, y - 20, 18, paper);
        } else {
            DrawEllipse(x - 23, y - 18, 28, 17, color);
            DrawEllipse(x + 23, y - 26, 27, 18, color);
            DrawEllipse(x, y - 37, 15, 30, color);
        }
    }
}

static void draw_team(const Team *team, const char *name, int x, bool has_turn)
{
    const Plant *active = &team->plants[team->active_index];
    float hp_fraction = (float)active->current_hp / active->max_hp;
    Rectangle panel = {x, 96, 544, 337};
    DrawRectangleRec(panel, paper);
    DrawRectangleLinesEx(panel, has_turn ? 5 : 2, has_turn ? ORANGE : ink);
    DrawText(name, x + 20, 115, 25, ink);
    DrawText(TextFormat("%s  |  %d/%d HP", active->name_pt, active->current_hp, active->max_hp),
             x + 20, 151, 20, ink);
    DrawRectangle(x + 20, 182, 504, 12, LIGHTGRAY);
    DrawRectangle(x + 20, 182, (int)(504 * hp_fraction), 12, hp_fraction > 0.25f ? DARKGREEN : RED);
    draw_plant(active, x + 272, 284);

    for (int i = 0; i < team->count; i++) {
        const Plant *plant = &team->plants[i];
        int slot_x = x + 20 + i * 126;
        DrawText(plant->name_pt, slot_x, 375, 18, ink);
        DrawText(TextFormat("%d/%d%s", plant->current_hp, plant->max_hp,
                            i == team->active_index ? " · ativa" : ""),
                 slot_x, 399, 16, ink);
    }
}

void battle_screen_draw(const BattleScreen *screen)
{
    const BattleState *battle = &screen->battle;
    ClearBackground((Color){210, 232, 195, 255});
    if (screen->setup_side >= 0) {
        if (screen->choosing_team) draw_team_selection(screen);
        else draw_setup(screen);
        return;
    }

    const Team *team = turn_team(battle);
    const Plant *active = &team->plants[team->active_index];
    bool ended = battle_over(battle);
    bool cpu = cpu_turn(screen);

    DrawText("Arena Versus", 64, 29, 38, ink);
    DrawText("ESC · Menu", 1092, 43, 20, ink);

    draw_team(&battle->player_team, side_name(screen, 0), 64, !ended && battle->current_turn == 0);
    draw_team(&battle->enemy_team, side_name(screen, 1), 672, !ended && battle->current_turn == 1);

    DrawRectangle(64, 450, 1152, 42, ink);
    DrawText(battle->last_message, 80, 461, 20, paper);

    if (ended) {
        int winner = battle->state == BATTLE_STATE_PLAYER_WIN ? 0 : 1;
        DrawText(TextFormat("%s venceu!", side_name(screen, winner)), 64, 511, 30, ink);
        draw_button(restart_button, "ENTER  Reiniciar", true);
        draw_button(menu_button, "M  Menu principal", true);
        return;
    }

    const char *side = side_name(screen, battle->current_turn);
    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        DrawText(TextFormat("%s precisa escolher uma reserva.", side), 64, 511, 26, ink);
    } else {
        DrawText(TextFormat("Turno de %s%s", side, cpu ? " · escolhendo..." : ""), 64, 511, 26, ink);
        for (int i = 0; i < MAX_MOVES; i++) {
            const Move *move = &active->moves[i];
            draw_button(move_button(i),
                        TextFormat("%d  %s  %d%%", i + 1, move->name, move->accuracy), !cpu);
        }
        LogicFormula formula = battle->formulas[battle->current_turn];
        LogicEvaluation value = Battle_EvaluateFormula(battle, battle->current_turn, formula);
        const Move *move = Battle_LogicMove(formula.operator);
        draw_button(logic_button,
                    TextFormat("3  %s  ·  Poder %d  ·  %s %s %s = %s", move->name, move->power,
                               truth(value.left), operators[formula.operator], truth(value.right),
                               truth(value.result)), !cpu && value.result);
        DrawText(TextFormat("P (%s): %s", truth(value.left), conditions[formula.left]), 634, 510, 18, ink);
        DrawText(TextFormat("Q (%s): %s", truth(value.right), conditions[formula.right]), 634, 534, 18, ink);
    }

    for (int i = 0; i < team->count; i++) {
        draw_button(switch_button(i),
                    TextFormat("%s  Trocar: %s", switch_labels[i], team->plants[i].name_pt),
                    !cpu && Battle_CanSwitch(battle, i));
    }
}
