/* Desenha a arena provisória e encaminha comandos ao motor de combate. */
#include "battle_screen.h"
#include "ui_theme.h"

#include "ai.h"
#include "plant_data.h"
#include "raylib.h"
#include <math.h>

static const Color ink = {28, 58, 46, 255};
static const Color paper = {253, 245, 216, 255};

static const int move_keys[MAX_MOVES] = {KEY_ONE, KEY_TWO, KEY_THREE};
static const int switch_keys[MAX_TEAM_SIZE] = {KEY_Q, KEY_W, KEY_E, KEY_R};
static const char *const switch_labels[MAX_TEAM_SIZE] = {"Q", "W", "E", "R"};

static const Rectangle return_button = {64, 565, 265, 54};
static const Rectangle setup_left = {64, 242, 400, 58};
static const Rectangle setup_operator = {480, 242, 200, 58};
static const Rectangle setup_right = {696, 242, 520, 58};
static const Rectangle setup_plant_buttons[MAX_TEAM_SIZE] = {
    {64, 142, 276, 38}, {354, 142, 276, 38},
    {644, 142, 276, 38}, {934, 142, 282, 38}
};
static const Rectangle setup_move_buttons[MAX_MOVES] = {
    {64, 535, 370, 48}, {450, 535, 370, 48}, {836, 535, 380, 48}
};
static const Rectangle setup_confirm = {64, 602, 370, 54};
static const Rectangle remove_reserve = {453, 602, 430, 54};
enum { ROSTER_COLUMNS = 4 };
static const float pending_action_seconds = 2.0f;

static const Color species_colors[SPECIES_COUNT] = {
    [SPECIES_CACTUS] = {56, 123, 78, 255},
    [SPECIES_SUNFLOWER] = {225, 154, 39, 255},
    [SPECIES_ALOE] = {65, 162, 132, 255},
    [SPECIES_POISON_IVY] = {89, 125, 51, 255},
    [SPECIES_PEPPER] = {208, 61, 83, 255},
    [SPECIES_ORCHID] = {194, 80, 167, 255},
    [SPECIES_CLOVER] = {70, 150, 61, 255},
    [SPECIES_CARNIVOROUS_PLANT] = {74, 124, 67, 255}
};

static const char *const conditions[LOGIC_CONDITION_COUNT] = {
    "Esta planta tem até metade do HP",
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

static const char *side_name(int side)
{
    return side == 0 ? "Jogador" : "Guardião";
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
    return screen->battle.current_turn == 1 && !battle_over(&screen->battle);
}

static Rectangle move_button(int index)
{
    return (Rectangle){64 + 386 * index, 567, 370, 48};
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
    screen->roster_cursor = (cursor + PLAYABLE_SPECIES_COUNT) % PLAYABLE_SPECIES_COUNT;

    int selected = -1;
    for (int i = 0; i < PLAYABLE_SPECIES_COUNT; i++) {
        if (activate(species_card(i), KEY_NULL)) {
            screen->roster_cursor = i;
            const SpeciesData *species = PlantData_GetPlayableSpecies(i);
            selected = species ? species->id : -1;
            break;
        }
    }
    if (selected < 0 && IsKeyPressed(KEY_SPACE)) {
        const SpeciesData *species = PlantData_GetPlayableSpecies(screen->roster_cursor);
        selected = species ? species->id : -1;
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

static int valid_logic_formula(LogicFormula formula)
{
    return formula.left >= 0 && formula.left < LOGIC_CONDITION_COUNT &&
           formula.right >= 0 && formula.right < LOGIC_CONDITION_COUNT &&
           formula.left != formula.right && formula.operator >= 0 &&
           formula.operator < LOGIC_OPERATOR_COUNT;
}

static void update_setup(BattleScreen *screen)
{
    LogicFormula *draft = &screen->draft;
    Team *team = screen->setup_side == 0
        ? &screen->battle.player_team : &screen->battle.enemy_team;
    if (team->count <= 0) return;
    if (team->count > MAX_TEAM_SIZE) team->count = MAX_TEAM_SIZE;
    if (screen->draft_plant_index < 0 || screen->draft_plant_index >= team->count)
        screen->draft_plant_index = 0;
    if (!valid_logic_formula(*draft))
        *draft = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_RESERVE, LOGIC_AND};
    int previous_plant = screen->draft_plant_index;
    int selected_plant = previous_plant;
    if (team->count > 0 && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))) {
        int direction = IsKeyPressed(KEY_RIGHT) ? 1 : -1;
        selected_plant = (previous_plant + direction + team->count) % team->count;
    } else {
        for (int i = 0; i < team->count; i++) {
            if (activate(setup_plant_buttons[i], KEY_NULL)) {
                selected_plant = i;
                break;
            }
        }
    }
    if (selected_plant != previous_plant) {
        screen->battle.formulas[screen->setup_side][previous_plant] = screen->draft;
        screen->draft_plant_index = selected_plant;
        screen->draft = screen->battle.formulas[screen->setup_side][selected_plant];
        screen->draft_move_index = screen->battle.boosted_move_indices
            [screen->setup_side][selected_plant];
    }
    int plant_index = screen->draft_plant_index;
    if (activate(setup_left, KEY_Q)) {
        draft->left = next_condition(draft->left, draft->right);
    } else if (activate(setup_right, KEY_W)) {
        draft->right = next_condition(draft->right, draft->left);
    } else if (activate(setup_operator, KEY_E)) {
        draft->operator = (draft->operator + 1) % LOGIC_OPERATOR_COUNT;
    } else if (activate(setup_move_buttons[0], KEY_ONE) &&
               team->plants[plant_index].moves[0].effect == MOVE_EFFECT_DAMAGE) {
        screen->draft_move_index = 0;
        screen->battle.boosted_move_indices[screen->setup_side][plant_index] = 0;
    } else if (activate(setup_move_buttons[1], KEY_TWO) &&
               team->plants[plant_index].moves[1].effect == MOVE_EFFECT_DAMAGE) {
        screen->draft_move_index = 1;
        screen->battle.boosted_move_indices[screen->setup_side][plant_index] = 1;
    } else if (activate(setup_move_buttons[2], KEY_THREE) &&
               team->plants[plant_index].moves[2].effect == MOVE_EFFECT_DAMAGE) {
        screen->draft_move_index = 2;
        screen->battle.boosted_move_indices[screen->setup_side][plant_index] = 2;
    } else if (activate(setup_confirm, KEY_ENTER)) {
        screen->battle.formulas[screen->setup_side][plant_index] = *draft;
        screen->battle.boosted_move_indices[screen->setup_side][plant_index] = screen->draft_move_index;
        screen->setup_side = -1;
    }
}

static void queue_action(BattleScreen *screen, BattlePendingAction action, int value)
{
    screen->pending_action = action;
    screen->action_timer = pending_action_seconds;
    screen->pending_move_index = action == BATTLE_PENDING_MOVE ? value : -1;
    screen->pending_plant_index = action == BATTLE_PENDING_SWITCH ? value : -1;
    screen->pending_cpu_action = AI_ACTION_NONE;
    screen->pending_cpu_index = -1;
}

static void resolve_pending_action(BattleScreen *screen)
{
    BattlePendingAction action = screen->pending_action;
    int value = action == BATTLE_PENDING_MOVE ? screen->pending_move_index
              : action == BATTLE_PENDING_SWITCH ? screen->pending_plant_index : -1;
    screen->pending_action = BATTLE_PENDING_NONE;
    screen->action_timer = 0;
    screen->pending_move_index = -1;
    screen->pending_plant_index = -1;

    if (action == BATTLE_PENDING_CPU) {
        if (screen->pending_cpu_action == AI_ACTION_MOVE)
            Battle_ExecuteMove(&screen->battle, screen->pending_cpu_index);
        else if (screen->pending_cpu_action == AI_ACTION_SWITCH)
            Battle_SwitchPlant(&screen->battle, screen->pending_cpu_index);
    }
    else if (action == BATTLE_PENDING_MOVE) Battle_ExecuteMove(&screen->battle, value);
    else if (action == BATTLE_PENDING_SWITCH) Battle_SwitchPlant(&screen->battle, value);
    else if (action == BATTLE_PENDING_PARALYSIS)
        Battle_SkipParalyzedTurn(&screen->battle);
}

BattleScreenAction battle_screen_update(BattleScreen *screen, float dt)
{
    BattleState *battle = &screen->battle;
    if (IsKeyPressed(KEY_ESCAPE)) return BATTLE_SCREEN_RETURN;

    if (screen->setup_side >= 0) {
        if (screen->choosing_team) {
            if (update_team_selection(screen)) return BATTLE_SCREEN_TEAM_READY;
        } else {
            update_setup(screen);
        }
        return BATTLE_SCREEN_NONE;
    }

    if (IsKeyPressed(KEY_TAB)) screen->show_statuses = !screen->show_statuses;

    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    if (screen->pending_action != BATTLE_PENDING_NONE) {
        screen->action_timer -= dt;
        if (screen->action_timer <= 0) resolve_pending_action(screen);
        return BATTLE_SCREEN_NONE;
    }

    if (battle_over(battle)) {
        if (activate(return_button, KEY_ENTER)) return BATTLE_SCREEN_RETURN;
        return BATTLE_SCREEN_NONE;
    }

    const Team *team = turn_team(battle);
    const Plant *active = team->active_index >= 0 && team->active_index < team->count
        ? &team->plants[team->active_index] : NULL;
    if (battle->state == BATTLE_STATE_ONGOING && active &&
        Plant_HasStatus(active, PLANT_STATUS_PARALYZED)) {
        queue_action(screen, BATTLE_PENDING_PARALYSIS, -1);
        return BATTLE_SCREEN_NONE;
    }

    if (cpu_turn(screen)) {
        queue_action(screen, BATTLE_PENDING_CPU, -1);
        screen->pending_cpu_action = AI_GetPlannedAction(battle, &screen->pending_cpu_index);
        return BATTLE_SCREEN_NONE;
    }

    if (battle->state == BATTLE_STATE_ONGOING) {
        for (int i = 0; i < MAX_MOVES; i++) {
            if (Battle_CanUseMove(battle, battle->current_turn,
                                  battle->current_turn == 0
                                      ? battle->player_team.active_index
                                      : battle->enemy_team.active_index,
                                  i) && activate(move_button(i), move_keys[i])) {
                queue_action(screen, BATTLE_PENDING_MOVE, i);
                return BATTLE_SCREEN_NONE;
            }
        }
    }

    for (int i = 0; i < turn_team(battle)->count; i++) {
        if (activate(switch_button(i), switch_keys[i])) {
            if (Battle_CanSwitch(battle, i)) queue_action(screen, BATTLE_PENDING_SWITCH, i);
            break;
        }
    }
    return BATTLE_SCREEN_NONE;
}

static void draw_button(const UiTheme *ui, Rectangle bounds, const char *label, bool enabled)
{
    bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRec(bounds, enabled ? (hovered ? GOLD : paper) : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, ink);
    ui_text(ui, label, (int)bounds.x + 12, (int)bounds.y + ((int)bounds.height - 20) / 2,
             20, enabled ? ink : DARKGRAY);
}

static void draw_move_button(const UiTheme *ui, Rectangle bounds, const Plant *attacker,
                             const Move *move,
                             int move_index, int effective_power, int potential_power,
                             bool selected_for_buff, bool enabled)
{
    bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
    Color text_color = enabled ? ink : DARKGRAY;
    DrawRectangleRec(bounds, enabled ? (hovered ? GOLD : paper) : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, ink);
    if (move->effect == MOVE_EFFECT_ATTRIBUTE) {
        const char *stat_name = move->stat == MOVE_STAT_ATTACK ? "ATQ"
                              : move->stat == MOVE_STAT_DEFENSE ? "DEF" : "VEL";
        const char *target_name = move->target == MOVE_TARGET_SELF ? "próprio" : "rival";
        const char *sign = move->stat_change_mode == MOVE_STAT_CHANGE_RATIO_DEBUFF ? "-" : "+";
        ui_text(ui, TextFormat("%d  %s · %s %s %s%.1fx", move_index + 1, move->name,
                               target_name, stat_name, sign, move->effect_step),
                (int)bounds.x + 12, (int)bounds.y + 5, 15, text_color);
        ui_text(ui, TextFormat("Modo %s · Limite %.1fx do atributo inicial",
                               move->stat_change_mode == MOVE_STAT_CHANGE_RATIO_DEBUFF
                                   ? "redução proporcional" : "aumento acumulado",
                               move->effect_cap),
                (int)bounds.x + 12, (int)bounds.y + 27, 12, text_color);
        return;
    }
    if (selected_for_buff && potential_power > move->power) {
        ui_text(ui, TextFormat("%d  %s · %d → %d", move_index + 1, move->name,
                               move->power, potential_power),
                (int)bounds.x + 12, (int)bounds.y + 5, 16, text_color);
    } else {
        ui_text(ui, TextFormat("%d  %s · Poder %d (normal)", move_index + 1, move->name,
                               effective_power),
                (int)bounds.x + 12, (int)bounds.y + 5, 16, text_color);
    }
    int critical_chance = Battle_GetCriticalChance(attacker, move);
    if (critical_chance != move->critical_chance) {
        ui_text(ui, TextFormat("Crítico %d%% (base %d%%)", critical_chance,
                               move->critical_chance),
                (int)bounds.x + 12, (int)bounds.y + 27, 12, text_color);
    } else {
        ui_text(ui, TextFormat("Crítico %d%%", critical_chance),
                (int)bounds.x + 12, (int)bounds.y + 27, 12, text_color);
    }
    const char *category = move->category == MOVE_CATEGORY_PHYSICAL ? "Físico"
                         : move->category == MOVE_CATEGORY_RANGED ? "À distância" : "Atributo";
    ui_text(ui, TextFormat("%s  ·  Precisão %d%%  ·  Crítico %d%%  ·  Prioridade %d",
                           category, move->accuracy, move->critical_chance, move->priority),
            (int)bounds.x + 12, (int)bounds.y + 26, 12, text_color);
}

static void draw_team_selection(const UiTheme *ui, const BattleScreen *screen)
{
    const Team *team = screen->setup_side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
    ui_text(ui, "Monte sua equipe", 64, 42, 40, ink);
    ui_text(ui, "ESC · Estufa", 1060, 43, 20, ink);
    ui_text(ui, TextFormat("%s  ·  %d/%d plantas no grupo", side_name(screen->setup_side),
                        team->count, MAX_TEAM_SIZE),
             64, 104, 26, ink);
    ui_text(ui, "Escolha uma vaga abaixo, depois clique numa planta ou use as setas e Espaço.",
             64, 149, 20, ink);

    for (int i = 0; i < PLAYABLE_SPECIES_COUNT; i++) {
        const SpeciesData *species = PlantData_GetPlayableSpecies(i);
        Rectangle card = species_card(i);
        int x = (int)card.x;
        int y = (int)card.y;
        bool focused = i == screen->roster_cursor || CheckCollisionPointRec(GetMousePosition(), card);
        DrawRectangleRec(card, focused ? GOLD : paper);
        DrawRectangleLinesEx(card, focused ? 3 : 2, ink);
        DrawCircle(x + 22, y + 25, 9, species_colors[species->id]);
        ui_text(ui, species->name_pt, x + 39, y + 14, 22, ink);
        ui_text(ui, TextFormat("HP %d  ATQ %d  DEF %d  VEL %d", species->base_hp, species->base_attack,
                            species->base_defense, species->base_speed), x + 14, y + 48, 17, ink);
        for (int slot = 0; slot < team->count; slot++) {
            if (team->plants[slot].id != species->id) continue;
            const char *role = slot == 0 ? "ATIVA" : TextFormat("RESERVA %d", slot);
            ui_text(ui, role, x + 14, y + 72, 15, ink);
        }
    }
    for (int i = 0; i < MAX_TEAM_SIZE; i++) {
        const char *name = i < team->count ? team->plants[i].name_pt : "Vaga livre";
        Rectangle bounds = team_slot_button(i);
        draw_button(ui, bounds, TextFormat("%s  %s", switch_labels[i], name), i <= team->count);
        if (i == screen->team_slot) DrawRectangleLinesEx(bounds, 4, ORANGE);
    }
    draw_button(ui, setup_confirm, "ENTER  Confirmar equipe", true);
    draw_button(ui, remove_reserve, "BACKSPACE  Remover reserva",
                team->count > MIN_TEAM_SIZE && screen->team_slot > 0 && screen->team_slot < team->count);
    const char *hint = screen->team_slot == team->count
        ? "Escolha uma espécie que ainda não está no grupo."
        : "A primeira planta começa ativa. As demais ficam na reserva.";
    ui_text(ui, hint, 64, 676, 19, ink);
}

static const char *truth(int value)
{
    return value ? "V" : "F";
}

static void draw_setup(const UiTheme *ui, const BattleScreen *screen)
{
    LogicFormula formula = screen->draft;
    const Team *team = screen->setup_side == 0
        ? &screen->battle.player_team : &screen->battle.enemy_team;
    if (team->count <= 0) return;
    const int plant_count = team->count < MAX_TEAM_SIZE ? team->count : MAX_TEAM_SIZE;
    const int plant_index = screen->draft_plant_index >= 0 && screen->draft_plant_index < plant_count
        ? screen->draft_plant_index : 0;
    if (!valid_logic_formula(formula))
        formula = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_RESERVE, LOGIC_AND};
    LogicEvaluation value = Battle_EvaluatePlantFormula(&screen->battle, screen->setup_side,
        plant_index, formula);
    const Plant *active = &team->plants[plant_index];
    int move_index = screen->draft_move_index >= 0 && screen->draft_move_index < MAX_MOVES &&
                     active->moves[screen->draft_move_index].effect == MOVE_EFFECT_DAMAGE
        ? screen->draft_move_index : 0;
    int boosted_power = active->moves[move_index].power +
                        Battle_GetFormulaPowerBonus(formula.operator);

    ui_text(ui, "Configure o buff de cada planta", 64, 42, 36, ink);
    ui_text(ui, "ESC · Estufa", 1060, 43, 20, ink);
    ui_text(ui, TextFormat("Montagem de %s", side_name(screen->setup_side)), 64, 110, 28, ink);
    ui_text(ui, "Selecione a planta para ajustar suas condições, operador e golpe.", 64, 184, 20, ink);
    for (int i = 0; i < plant_count; i++) {
        Rectangle bounds = setup_plant_buttons[i];
        const LogicFormula plant_formula = i == plant_index
            ? formula : screen->battle.formulas[screen->setup_side][i];
        draw_button(ui, bounds, TextFormat("%s · G%d · %s", team->plants[i].name_pt,
            screen->battle.boosted_move_indices[screen->setup_side][i] + 1,
            operators[plant_formula.operator >= 0 && plant_formula.operator < LOGIC_OPERATOR_COUNT
                ? plant_formula.operator : LOGIC_AND]), true);
        if (i == plant_index) DrawRectangleLinesEx(bounds, 4, ORANGE);
    }
    ui_text(ui, "Condição P", 64, 218, 20, ink);
    ui_text(ui, "Operador", 480, 218, 20, ink);
    ui_text(ui, "Condição Q", 696, 218, 20, ink);
    draw_button(ui, setup_left, TextFormat("Q  %s", conditions[formula.left]), true);
    draw_button(ui, setup_operator, TextFormat("[E]  %s", operators[formula.operator]), true);
    draw_button(ui, setup_right, TextFormat("W  %s", conditions[formula.right]), true);

    DrawRectangle(64, 319, 1152, 197, paper);
    ui_text(ui, TextFormat("P %s Q", operators[formula.operator]), 86, 341, 33, ink);
    ui_text(ui, operator_rules[formula.operator], 86, 391, 24, ink);
    ui_text(ui, TextFormat("Bônus do operador: +%d de poder quando a fórmula for verdadeira",
                           Battle_GetFormulaPowerBonus(formula.operator)), 86, 433, 22, ink);
    ui_text(ui, TextFormat("Com as equipes no início: %s %s %s = %s",
                        truth(value.left), operators[formula.operator], truth(value.right),
                        truth(value.result)), 86, 471, 22, ink);
    for (int i = 0; i < MAX_MOVES; i++) {
        const Move *candidate = &active->moves[i];
        if (candidate->effect == MOVE_EFFECT_ATTRIBUTE) {
            const char *stat_name = candidate->stat == MOVE_STAT_ATTACK ? "ATQ"
                                  : candidate->stat == MOVE_STAT_DEFENSE ? "DEF" : "VEL";
            const char *sign = candidate->stat_change_mode == MOVE_STAT_CHANGE_RATIO_DEBUFF
                ? "-" : "+";
            draw_button(ui, setup_move_buttons[i],
                TextFormat("[%d] %s · %s %s%.1fx (máx %.1fx)", i + 1,
                           candidate->name, stat_name, sign, candidate->effect_step,
                           candidate->effect_cap), false);
        } else if (i == move_index) {
            draw_button(ui, setup_move_buttons[i],
                        TextFormat("[%d]  %s  ·  Poder %d → %d (+%d)", i + 1,
                                   candidate->name, candidate->power, boosted_power,
                                   Battle_GetFormulaPowerBonus(formula.operator)), true);
        } else {
            draw_button(ui, setup_move_buttons[i],
                        TextFormat("[%d]  %s  ·  Poder %d", i + 1,
                                   candidate->name, candidate->power), true);
        }
    }
    ui_text(ui, "←/→ ou clique seleciona; 1/2/3 escolhe o golpe. Buffs de atributo não recebem bônus lógico.",
            64, 583, 16, ink);
    draw_button(ui, setup_confirm, "ENTER  Confirmar fórmula", true);
}

static void draw_plant(const UiTheme *ui, const Plant *plant, int x, int y,
                       Texture2D aloe_guard, Texture2D sunflower_guard)
{
    DrawEllipse(x, y + 56, 73, 14, Fade(ink, 0.25f));
    if (!Plant_IsAlive(plant)) {
        ui_text(ui, "Derrotada", x - 49, y + 17, 20, ink);
        return;
    }

    Texture2D botanical_boss = (Texture2D){0};
    if (plant->id == SPECIES_ALOE) botanical_boss = aloe_guard;
    if (plant->id == SPECIES_SUNFLOWER) botanical_boss = sunflower_guard;
    if (botanical_boss.id) {
        float time = (float)GetTime();
        float bob = plant->id == SPECIES_SUNFLOWER ? 0.0f : sinf(time * 2.4f) * 2.0f;
        float sway = sinf(time * 1.7f) * 1.2f;
        const float height = plant->id == SPECIES_SUNFLOWER ? 220.0f : 165.0f;
        const float width = height * (float)botanical_boss.width / (float)botanical_boss.height;
        Rectangle source = {0, 0, (float)botanical_boss.width, (float)botanical_boss.height};
        Rectangle destination = {x, y + 56 + bob, width, height};
        DrawTexturePro(botanical_boss, source, destination,
                       (Vector2){width / 2, height}, sway, WHITE);
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
        if (plant->id == SPECIES_ORCHID) {
            DrawEllipse(x - 23, y - 23, 22, 20, color);
            DrawEllipse(x + 23, y - 23, 22, 20, color);
            DrawEllipse(x, y - 39, 27, 25, color);
            DrawCircle(x, y - 20, 18, paper);
        } else if (plant->id == SPECIES_PEPPER) {
            DrawEllipse(x, y - 31, 25, 36, color);
            DrawTriangle((Vector2){x - 19, y - 12}, (Vector2){x, y + 34},
                         (Vector2){x + 19, y - 12}, color);
        } else if (plant->id == SPECIES_CARNIVOROUS_PLANT) {
            DrawEllipse(x, y - 30, 42, 28, color);
            DrawEllipse(x, y - 27, 32, 12, MAROON);
            for (int i = 0; i < 5; i++) {
                DrawRectangle(x - 27 + i * 12, y - 39, 6, 10, paper);
                DrawRectangle(x - 27 + i * 12, y - 21, 6, 8, paper);
            }
        } else if (plant->id == SPECIES_CLOVER) {
            for (int i = 0; i < 4; i++)
                DrawEllipse(x - 18 + (i % 2) * 36, y - 54 + (i / 2) * 32, 22, 21, color);
        } else {
            DrawEllipse(x - 23, y - 18, 28, 17, color);
            DrawEllipse(x + 23, y - 26, 27, 18, color);
            DrawEllipse(x, y - 37, 15, 30, color);
        }
    }
}

static void draw_team(const UiTheme *ui, const Team *team, const char *name, int x,
                      bool has_turn, Texture2D aloe_guard, Texture2D sunflower_guard)
{
    const Plant *active = &team->plants[team->active_index];
    float hp_fraction = (float)active->current_hp / active->max_hp;
    Rectangle panel = {x, 96, 544, 337};
    DrawRectangleRec(panel, paper);
    DrawRectangleLinesEx(panel, has_turn ? 5 : 2, has_turn ? ORANGE : ink);
    ui_text(ui, name, x + 20, 115, 25, ink);
    ui_text(ui, TextFormat("%s  |  %d/%d HP", active->name_pt, active->current_hp, active->max_hp),
             x + 20, 151, 20, ink);
    if (active->id == SPECIES_PEPPER)
        ui_text(ui, TextFormat("Frutos: %d", active->fruit_count), x + 380, 151, 18, ink);
    DrawRectangle(x + 20, 182, 504, 12, LIGHTGRAY);
    DrawRectangle(x + 20, 182, (int)(504 * hp_fraction), 12, hp_fraction > 0.25f ? DARKGREEN : RED);
    ui_text(ui, TextFormat("ATQ %d  ·  DEF %d  ·  VEL %d",
                           active->attack, active->defense, active->speed),
            x + 20, 202, 15, ink);
    draw_plant(ui, active, x + 272, 284, aloe_guard, sunflower_guard);

    for (int i = 0; i < team->count; i++) {
        const Plant *plant = &team->plants[i];
        int slot_x = x + 20 + i * 126;
        ui_text_wrap(ui, plant->name_pt, (Rectangle){slot_x, 371, 118, 42}, 16, ink, false);
        ui_text(ui, TextFormat("%d/%d%s", plant->current_hp, plant->max_hp,
                            i == team->active_index ? " · ativa" : ""),
                 slot_x, 414, 16, ink);
    }
}

static const char *status_name(PlantStatusType type)
{
    switch (type) {
        case PLANT_STATUS_POISON: return "Veneno";
        case PLANT_STATUS_BURN: return "Queimadura";
        case PLANT_STATUS_LUCK: return "Sorte";
        case PLANT_STATUS_PARALYZED: return "Paralisado";
        case PLANT_STATUS_HOST: return "Hospedeiro";
        case PLANT_STATUS_SPEED_BOOST: return "Velocidade";
        case PLANT_STATUS_ATTACK_BOOST: return "Ataque";
        case PLANT_STATUS_DEFENSE_BOOST: return "Defesa";
        case PLANT_STATUS_ACCURACY_BOOST: return "Precisão";
        case PLANT_STATUS_ATTACK_DOWN: return "Ataque reduzido";
        case PLANT_STATUS_DEFENSE_DOWN: return "Defesa reduzida";
        case PLANT_STATUS_SPEED_DOWN: return "Velocidade reduzida";
        default: return NULL;
    }
}

static Color status_color(PlantStatusType type)
{
    switch (type) {
        case PLANT_STATUS_POISON: return (Color){64, 123, 67, 255};
        case PLANT_STATUS_BURN: return (Color){194, 77, 43, 255};
        case PLANT_STATUS_LUCK: return (Color){176, 132, 34, 255};
        case PLANT_STATUS_PARALYZED: return (Color){112, 80, 150, 255};
        case PLANT_STATUS_HOST: return (Color){154, 76, 133, 255};
        default: return (Color){56, 117, 139, 255};
    }
}

static const char *status_label(const PlantStatus *status)
{
    const char *name = status_name(status->type);
    if (!name) return "Status desconhecido";
    if (status->type == PLANT_STATUS_PARALYZED)
        return TextFormat("%s · próxima ação", name);
    if (status->type == PLANT_STATUS_LUCK ||
        status->type == PLANT_STATUS_SPEED_BOOST ||
        status->type == PLANT_STATUS_ATTACK_BOOST ||
        status->type == PLANT_STATUS_DEFENSE_BOOST ||
        status->type == PLANT_STATUS_ACCURACY_BOOST ||
        status->type == PLANT_STATUS_ATTACK_DOWN ||
        status->type == PLANT_STATUS_DEFENSE_DOWN ||
        status->type == PLANT_STATUS_SPEED_DOWN)
        return TextFormat("%s ×%.1f", name, status->value);
    if (status->remaining_turns > 0)
        return TextFormat("%s · %d turno%s", name, status->remaining_turns,
                          status->remaining_turns == 1 ? "" : "s");
    return name;
}

static void draw_plant_statuses(const UiTheme *ui, const Plant *plant, Rectangle bounds)
{
    if (plant->status_count <= 0) {
        ui_text(ui, "Sem status aplicado", (int)bounds.x, (int)bounds.y, 14, DARKGRAY);
        return;
    }

    Font font = ui_font(ui, false);
    const float font_size = 14;
    const float chip_height = 19;
    const int count = plant->status_count > PLANT_STATUS_COUNT
        ? PLANT_STATUS_COUNT : plant->status_count;
    float x = bounds.x;
    float y = bounds.y;
    for (int i = 0; i < count; i++) {
        const PlantStatus *status = &plant->statuses[i];
        const char *label = status_label(status);
        float width = MeasureTextEx(font, label, font_size, 0).x + 16;
        if (x + width > bounds.x + bounds.width) {
            x = bounds.x;
            y += chip_height + 3;
        }
        Color color = status_color(status->type);
        Rectangle chip = {x, y, width, chip_height};
        DrawRectangleRec(chip, Fade(color, 0.15f));
        DrawRectangleLinesEx(chip, 1, color);
        DrawTextEx(font, label, (Vector2){x + 8, y + 1}, font_size, 0, ink);
        x += width + 5;
    }
}

static void draw_status_overlay(const UiTheme *ui, const BattleState *battle)
{
    DrawRectangle(0, 0, 1280, 720, Fade(ink, 0.72f));
    Rectangle panel = {72, 78, 1136, 564};
    DrawRectangleRec(panel, paper);
    DrawRectangleLinesEx(panel, 4, ink);
    ui_text(ui, "Status da batalha", 96, 94, 30, ink);
    ui_text(ui, "TAB · Fechar", 1050, 103, 18, ink);

    const Team *teams[2] = {&battle->player_team, &battle->enemy_team};
    const int column_x[2] = {96, 656};
    for (int side = 0; side < 2; side++) {
        ui_text(ui, side_name(side), column_x[side], 139, 21, ink);
        const Team *team = teams[side];
        int count = team->count > MAX_TEAM_SIZE ? MAX_TEAM_SIZE : team->count;
        for (int i = 0; i < count; i++) {
            const Plant *plant = &team->plants[i];
            float y = 168 + i * 112;
            Rectangle card = {column_x[side], y, 528, 102};
            DrawRectangleRec(card, (Color){247, 239, 212, 255});
            DrawRectangleLinesEx(card, i == team->active_index ? 3 : 1,
                                 i == team->active_index ? ORANGE : (Color){130, 144, 115, 255});
            ui_text(ui, plant->name_pt, (int)card.x + 12, (int)card.y + 7, 18, ink);
            ui_text(ui, TextFormat("HP %d/%d · ATQ %d · DEF %d · VEL %d",
                                   plant->current_hp, plant->max_hp, plant->attack,
                                   plant->defense, plant->speed),
                    (int)card.x + 168, (int)card.y + 9, 14, ink);
            draw_plant_statuses(ui, plant,
                (Rectangle){card.x + 12, card.y + 35, card.width - 24, card.height - 40});
        }
    }
}

void battle_screen_draw(const UiTheme *ui, const BattleScreen *screen,
                        Texture2D aloe_guard, Texture2D sunflower_guard)
{
    const BattleState *battle = &screen->battle;
    ClearBackground((Color){210, 232, 195, 255});
    if (screen->setup_side >= 0) {
        if (screen->choosing_team) draw_team_selection(ui, screen);
        else draw_setup(ui, screen);
        return;
    }

    const Team *team = turn_team(battle);
    const Plant *active = &team->plants[team->active_index];
    bool ended = battle_over(battle);
    bool cpu = cpu_turn(screen);

    ui_text(ui, "Batalha na estufa", 64, 29, 38, ink);
    ui_text(ui, "TAB · Status", 890, 43, 18, ink);
    ui_text(ui, "ESC · Estufa", 1060, 43, 20, ink);

    draw_team(ui, &battle->player_team, side_name(0), 64,
              !ended && battle->current_turn == 0, aloe_guard, sunflower_guard);
    draw_team(ui, &battle->enemy_team, side_name(1), 672,
              !ended && battle->current_turn == 1, aloe_guard, sunflower_guard);

    const char *message = battle->last_message;
    if (screen->pending_action == BATTLE_PENDING_PARALYSIS) {
        message = TextFormat("%s está paralisada e perderá a ação...", active->name_pt);
    } else if (screen->pending_action == BATTLE_PENDING_CPU) {
        const Team *cpu_team = &battle->enemy_team;
        const Plant *cpu_active = cpu_team->active_index >= 0 &&
                                  cpu_team->active_index < cpu_team->count
            ? &cpu_team->plants[cpu_team->active_index] : NULL;
        if (screen->pending_cpu_action == AI_ACTION_MOVE && cpu_active &&
            screen->pending_cpu_index >= 0 && screen->pending_cpu_index < MAX_MOVES) {
            const Move *move = &cpu_active->moves[screen->pending_cpu_index];
            message = TextFormat("%s usou %s...", cpu_active->name_pt, move->name);
        } else if (screen->pending_cpu_action == AI_ACTION_SWITCH &&
                   screen->pending_cpu_index >= 0 &&
                   screen->pending_cpu_index < battle->enemy_team.count) {
            message = TextFormat("%s vai entrar em campo...",
                battle->enemy_team.plants[screen->pending_cpu_index].name_pt);
        } else {
            message = "O guardião não pode agir.";
        }
    } else if (screen->pending_action == BATTLE_PENDING_MOVE &&
               screen->pending_move_index >= 0 && screen->pending_move_index < MAX_MOVES) {
        const Move *move = &active->moves[screen->pending_move_index];
        message = TextFormat("%s usou %s...", active->name_pt, move->name);
    } else if (screen->pending_action == BATTLE_PENDING_SWITCH &&
               screen->pending_plant_index >= 0 && screen->pending_plant_index < team->count) {
        message = TextFormat("%s vai entrar em campo...",
                             team->plants[screen->pending_plant_index].name_pt);
    }
    DrawRectangle(64, 450, 1152, 42, ink);
    ui_text(ui, message, 80, 461, 20, paper);
    if (screen->pending_action != BATTLE_PENDING_NONE) {
        float elapsed = 1.0f - screen->action_timer / pending_action_seconds;
        DrawRectangle(64, 493, 1152, 4, LIGHTGRAY);
        DrawRectangle(64, 493, (int)(1152 * elapsed), 4, ORANGE);
    }

    if (ended) {
        int winner = battle->state == BATTLE_STATE_PLAYER_WIN ? 0 : 1;
        ui_text(ui, TextFormat("%s venceu!", side_name(winner)), 64, 511, 30, ink);
        draw_button(ui, return_button, "ENTER  Estufa", true);
        if (screen->show_statuses) draw_status_overlay(ui, battle);
        return;
    }

    const char *side = side_name(battle->current_turn);
    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        ui_text(ui, TextFormat("%s precisa escolher uma reserva.", side), 64, 511, 26, ink);
    } else {
        LogicFormula formula = battle->formulas[battle->current_turn][team->active_index];
        LogicEvaluation value = Battle_EvaluatePlantFormula(battle, battle->current_turn,
            team->active_index, formula);
        int boosted_index = battle->boosted_move_indices[battle->current_turn][team->active_index];
        const Move *boosted_move = &active->moves[boosted_index];
        int boosted_power = Battle_GetMovePower(battle, battle->current_turn, boosted_index);
        ui_text(ui, TextFormat("%s%s · %s %s %s = %s", side,
                            cpu ? " escolhendo..." : "",
                            conditions[formula.left], operators[formula.operator],
                            conditions[formula.right], truth(value.result)), 64, 510, 17, ink);
        if (value.result) {
            ui_text(ui, TextFormat("Golpe %d fortalecido: %s · poder %d → %d",
                                boosted_index + 1, boosted_move->name,
                                boosted_move->power, boosted_power), 64, 533, 17, ink);
        } else {
            ui_text(ui, TextFormat("Golpe escolhido: %s · poder base %d (aguardando fórmula)",
                                boosted_move->name, boosted_move->power), 64, 533, 17, ink);
        }
        for (int i = 0; i < MAX_MOVES; i++) {
            const Move *move = &active->moves[i];
            int selected_for_buff = i == boosted_index;
            int potential_power = selected_for_buff
                ? move->power + Battle_GetFormulaPowerBonus(formula.operator)
                : move->power;
            int can_use = Battle_CanUseMove(battle, battle->current_turn,
                                             team->active_index, i);
            draw_move_button(ui, move_button(i), active, move, i,
                             Battle_GetMovePower(battle, battle->current_turn, i),
                             potential_power, selected_for_buff,
                             !cpu && screen->pending_action == BATTLE_PENDING_NONE && can_use);
        }
        if (!value.result)
            ui_text(ui, "Golpe do buff bloqueado; os demais golpes seguem disponíveis com poder normal.",
                    64, 550, 15, RED);
    }

    for (int i = 0; i < team->count; i++) {
        draw_button(ui, switch_button(i),
                    TextFormat("%s  Trocar: %s", switch_labels[i], team->plants[i].name_pt),
                    !cpu && screen->pending_action == BATTLE_PENDING_NONE &&
                    Battle_CanSwitch(battle, i));
    }
    if (screen->show_statuses) draw_status_overlay(ui, battle);
}
