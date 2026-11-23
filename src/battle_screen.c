/* Compõe a arena botânica e encaminha ações validadas ao motor de combate. */
#include "battle_screen.h"
#include "ui_theme.h"

#include "ai.h"
#include "plant_data.h"
#include "raylib.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const Color ink = {28, 58, 46, 255};
static const Color paper = {253, 245, 216, 255};

static const int move_keys[MAX_MOVES] = {KEY_ONE, KEY_TWO, KEY_THREE};
static const int switch_keys[MAX_TEAM_SIZE] = {KEY_Q, KEY_W, KEY_E, KEY_R};
static const char *const switch_labels[MAX_TEAM_SIZE] = {"Q", "W", "E", "R"};

static const Rectangle return_button = {568, 518, 647, 60};
static const Rectangle back_button = {390, 520, 120, 28};
static const Rectangle setup_left = {64, 242, 400, 58};
static const Rectangle setup_operator = {480, 242, 200, 58};
static const Rectangle setup_right = {696, 242, 520, 58};
static const float action_prepare_seconds = 0.15f;
static const float message_read_seconds = 1.5f;
static const Rectangle setup_plant_buttons[MAX_TEAM_SIZE] = {
    {64,142,276,38}, {354,142,276,38}, {644,142,276,38}, {934,142,282,38}
};
static const Rectangle setup_move_buttons[MAX_MOVES] = {
    {64, 535, 370, 48}, {450, 535, 370, 48}, {836, 535, 380, 48}
};
static const Rectangle setup_confirm = {64, 602, 370, 54};
static const Rectangle remove_reserve = {453, 602, 430, 54};
enum { ROSTER_COLUMNS = 4 };

static const Color species_colors[SPECIES_COUNT] = {
    [SPECIES_CACTUS] = {56, 123, 78, 255},
    [SPECIES_SUNFLOWER] = {225, 154, 39, 255},
    [SPECIES_ALOE] = {65, 162, 132, 255},
    [SPECIES_POISON_IVY] = {89, 125, 51, 255},
    [SPECIES_PEPPER] = {208, 61, 43, 255},
    [SPECIES_CARNIVOROUS_PLANT] = {101, 152, 56, 255},
    [SPECIES_ORCHID] = {194, 80, 167, 255},
    [SPECIES_CLOVER] = {70, 150, 61, 255}
};

static const char *const conditions[LOGIC_CONDITION_COUNT] = {
    "Esta planta tem até metade do HP",
    "Rival com status",
    "Ativa rival tem até 50% de HP",
    "Rival mais rápido",
    "Você tem mais ATQ",
    "Você tem mais DEF",
    "Você tem reserva viva"
};
static const char *const operators[LOGIC_OPERATOR_COUNT] = {"E", "OU", "XOR"};
static const char *const operator_rules[LOGIC_OPERATOR_COUNT] = {
    "As duas condições precisam ser verdadeiras.",
    "Pelo menos uma condição precisa ser verdadeira.",
    "Exatamente uma condição precisa ser verdadeira."
};

static bool battle_over(const BattleState *battle)
{
    return battle->state == BATTLE_STATE_PLAYER_WIN || battle->state == BATTLE_STATE_ENEMY_WIN;
}

static bool enemy_replacement(const BattleState *battle)
{
    return battle->state == BATTLE_STATE_REPLACEMENT && battle->current_turn == 1;
}

static Rectangle move_button(int index)
{
    return (Rectangle){568, 522 + index*54, 647, 50};
}

static Rectangle switch_button(int index)
{
    return (Rectangle){568 + (index%2)*328, 518 + (index/2)*69, 319, 60};
}

static Rectangle root_button(int index) { return switch_button(index); }

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
            selected = species ? (int)species->id : -1;
            break;
        }
    }
    if (selected < 0 && IsKeyPressed(KEY_SPACE)) {
        const SpeciesData *species = PlantData_GetPlayableSpecies(screen->roster_cursor);
        selected = species ? (int)species->id : -1;
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
        *draft = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_STATUS, LOGIC_AND};
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

static void clear_round(BattleScreen *screen)
{
    screen->round_active = false;
    screen->round = (BattleRound){0};
    screen->round_result_message[0] = '\0';
    screen->round_message_count = 0;
    screen->round_message_index = 0;
    screen->animation_side = -1;
    screen->animation_plant_index = -1;
    screen->animation_status = false;
}

static void clear_pending(BattleScreen *screen)
{
    screen->pending_action = BATTLE_PENDING_NONE;
    screen->action_timer = 0;
    screen->pending_move_index = -1;
    screen->pending_plant_index = -1;
    screen->pending_cpu_action = AI_ACTION_NONE;
    screen->pending_cpu_index = -1;
    screen->pending_player_action = (BattleAction){BATTLE_ACTION_NONE, -1};
    screen->pending_enemy_action = (BattleAction){BATTLE_ACTION_NONE, -1};
}

static void queue_action(BattleScreen *screen, BattlePendingAction action, int value)
{
    clear_pending(screen);
    clear_round(screen);
    screen->pending_action = action;
    screen->action_timer = action_prepare_seconds;
    screen->pending_move_index = action == BATTLE_PENDING_MOVE ? value : -1;
    screen->pending_plant_index = action == BATTLE_PENDING_SWITCH ? value : -1;
}

/* Define qual planta anima no resultado: físico avança, status pula, à distância fica parado. */
static void set_action_animation(BattleScreen *screen, int step)
{
    screen->animation_side = -1;
    screen->animation_plant_index = -1;
    screen->animation_status = false;
    if (!screen->round_active || step < 0 || step >= 2) return;
    const BattleRound *round = &screen->round;
    int side = round->sides[step];
    BattleAction action = round->actions[side];
    if (action.type != BATTLE_ACTION_MOVE || action.index < 0 || action.index >= MAX_MOVES) return;
    const Team *team = side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
    int index = round->acted_indices[side];
    if (index < 0 || index >= team->count) return;
    MoveCategory category = team->plants[index].moves[action.index].category;
    if (category != MOVE_CATEGORY_PHYSICAL && category != MOVE_CATEGORY_STATUS) return;
    screen->animation_side = side;
    screen->animation_plant_index = index;
    screen->animation_status = category == MOVE_CATEGORY_STATUS;
}

static void queue_round(BattleScreen *screen, BattleAction player_action, BattleAction enemy_action)
{
    BattleRound prepared;
    if (!Battle_PrepareRound(&screen->battle, player_action, enemy_action, &prepared)) return;
    clear_pending(screen);
    clear_round(screen);
    screen->round = prepared;
    screen->round_active = true;
    screen->pending_player_action = player_action;
    screen->pending_enemy_action = enemy_action;
    screen->pending_action = BATTLE_PENDING_ROUND;
    screen->action_timer = action_prepare_seconds;
}

static BattleAction plan_enemy_action(const BattleState *battle)
{
    BattleAction action = {BATTLE_ACTION_NONE, -1};
    int index = -1;
    AIActionType type = AI_GetActionForSide(battle, 1, &index);
    if (type == AI_ACTION_MOVE) action = (BattleAction){BATTLE_ACTION_MOVE, index};
    else if (type == AI_ACTION_SWITCH) action = (BattleAction){BATTLE_ACTION_SWITCH, index};
    return action;
}

static void append_round_message(BattleScreen *screen, const char *message)
{
    if (!message || !message[0] || screen->round_message_count >= BATTLE_ROUND_MESSAGES) return;
    int offset = 0;
    if (screen->round_message_count > 0) {
        int last = screen->round_message_offsets[screen->round_message_count-1];
        offset = last+(int)strlen(screen->round_result_message+last)+1;
    }
    if ((size_t)offset+strlen(message)+1 > sizeof(screen->round_result_message)) return;
    screen->round_message_offsets[screen->round_message_count++] = offset;
    snprintf(screen->round_result_message+offset,sizeof(screen->round_result_message)-(size_t)offset,"%s",message);
}

static int stat_value(const Plant *plant, MoveStat stat)
{
    return stat == MOVE_STAT_ATTACK ? plant->attack : stat == MOVE_STAT_DEFENSE ? plant->defense : plant->speed;
}

/* Ataque e resultado são páginas curtas; os efeitos extras preservam a ordem do motor. */
static void capture_round_messages(BattleScreen *screen, const BattleState *before, const char *fallback)
{
    screen->round_result_message[0] = '\0';
    screen->round_message_count = 0;
    screen->round_message_index = 0;
    screen->message_armed = false;
    screen->message_elapsed = 0;
    const BattleMoveFeedback *event = &screen->battle.feedback;
    bool move_event = event->sequence != before->feedback.sequence;
    if (move_event) {
        const Team *acting = event->side == 0 ? &before->player_team : &before->enemy_team;
        const Team *old_target = event->target_side == 0 ? &before->player_team : &before->enemy_team;
        const Team *new_target = event->target_side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
        const Plant *plant = &acting->plants[event->plant_index];
        const Move *move = &plant->moves[event->move_index];
        const Plant *target = &old_target->plants[old_target->active_index];
        char page[192];
        snprintf(page,sizeof(page),"%s usou %s%s.",plant->name_pt,move->name,event->boosted ? " fortalecido" : "");
        append_round_message(screen,page);
        if (move->effect == MOVE_EFFECT_ATTRIBUTE) {
            const char *stat = move->stat == MOVE_STAT_ATTACK ? "Ataque" : move->stat == MOVE_STAT_DEFENSE ? "Defesa" : "Velocidade";
            snprintf(page,sizeof(page),"%s de %s: %d para %d.",stat,target->name_pt,
                stat_value(target,move->stat),stat_value(&new_target->plants[old_target->active_index],move->stat));
        } else if (event->hit) {
            snprintf(page,sizeof(page),"%s recebeu %d de dano.%s",target->name_pt,event->damage,
                event->critical ? " Acerto crítico!" : "");
        } else snprintf(page,sizeof(page),"O golpe não acertou %s.",target->name_pt);
        append_round_message(screen,page);
    }
    char extra[192];
    snprintf(extra,sizeof(extra),"%s",fallback ? fallback : screen->battle.last_message);
    int start = 0, sentence = 0;
    size_t length = strlen(extra);
    for (size_t i = 0; i <= length; i++) {
        if (extra[i] != '\0' && !((extra[i] == '.' || extra[i] == '!') && extra[i+1] == ' ')) continue;
        bool last = extra[i] == '\0';
        if (!last) extra[i+1] = '\0';
        if (!move_event || sentence > 0) append_round_message(screen,extra+start);
        sentence++;
        if (last) break;
        start = (int)i+2;
        while (extra[start] == ' ') start++;
        i = (size_t)start-1;
    }
    if (!screen->round_message_count) append_round_message(screen,"A ação terminou.");
}

static void show_next_round_message(BattleScreen *screen)
{
    screen->message_elapsed = 0;
    screen->round_message_index++;
    if (screen->round_message_index < screen->round_message_count) {
        screen->pending_action = BATTLE_PENDING_ROUND_RESULT;
        screen->action_timer = 0;
        screen->message_armed = false;
        return;
    }
    if (screen->round_active && screen->round.step < 2 && !battle_over(&screen->battle)) {
        screen->pending_action = BATTLE_PENDING_ROUND;
        screen->action_timer = action_prepare_seconds;
        return;
    }
    if (screen->round_active) Battle_FinishRound(&screen->battle, &screen->round);
    clear_round(screen);
}

static void resolve_pending_action(BattleScreen *screen)
{
    BattleState before = screen->battle;
    BattlePendingAction action = screen->pending_action;
    AIActionType cpu_action = screen->pending_cpu_action;
    int cpu_index = screen->pending_cpu_index;
    int value = action == BATTLE_PENDING_MOVE ? screen->pending_move_index
              : action == BATTLE_PENDING_SWITCH ? screen->pending_plant_index : -1;
    clear_pending(screen);

    if (action == BATTLE_PENDING_CPU) {
        if (cpu_action == AI_ACTION_MOVE) Battle_ExecuteMove(&screen->battle, cpu_index);
        else if (cpu_action == AI_ACTION_SWITCH) Battle_SwitchPlant(&screen->battle, cpu_index);
    }
    else if (action == BATTLE_PENDING_MOVE) Battle_ExecuteMove(&screen->battle, value);
    else if (action == BATTLE_PENDING_SWITCH) Battle_SwitchPlant(&screen->battle, value);
    else if (action == BATTLE_PENDING_ROUND || action == BATTLE_PENDING_PARALYSIS) {
        int step = screen->round.step;
        int executed = Battle_ExecuteRoundStep(&screen->battle, &screen->round);
        capture_round_messages(screen,&before,executed == 1 ? NULL : "A ação seguinte foi cancelada.");
        if (executed == 1) set_action_animation(screen, step);
        else set_action_animation(screen, -1);
        screen->pending_action = BATTLE_PENDING_ROUND_RESULT;
        screen->action_timer = 0;
    }
    else if (action == BATTLE_PENDING_ROUND_RESULT) show_next_round_message(screen);
    if (action == BATTLE_PENDING_CPU || action == BATTLE_PENDING_MOVE || action == BATTLE_PENDING_SWITCH) {
        capture_round_messages(screen,&before,NULL);
        screen->pending_action = BATTLE_PENDING_ROUND_RESULT;
        screen->action_timer = 0;
    }
    battle_fx_begin(&screen->fx,&before,&screen->battle);
}

BattleScreenAction battle_screen_update(BattleScreen *screen, float dt)
{
    dt = fminf(0.1f,fmaxf(0,dt));
    battle_fx_update(&screen->fx,dt);
    BattleState *battle = &screen->battle;
    if (battle_over(battle) && screen->fx.active) return BATTLE_SCREEN_NONE;
    bool forced = battle->state == BATTLE_STATE_REPLACEMENT && battle->current_turn == 0;
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (screen->show_statuses) { screen->show_statuses = false; return BATTLE_SCREEN_NONE; }
        if (screen->pending_action != BATTLE_PENDING_NONE) return BATTLE_SCREEN_NONE;
        if (screen->setup_side < 0 && screen->menu != BATTLE_MENU_ROOT && !forced && !battle_over(battle)) {
            screen->menu = BATTLE_MENU_ROOT;
            screen->menu_cursor = 0;
            return BATTLE_SCREEN_NONE;
        }
        return BATTLE_SCREEN_RETURN;
    }

    if (screen->setup_side >= 0) {
        if (screen->choosing_team) {
            if (update_team_selection(screen)) return BATTLE_SCREEN_TEAM_READY;
        } else {
            update_setup(screen);
        }
        return BATTLE_SCREEN_NONE;
    }

    if (IsKeyPressed(KEY_TAB)) screen->show_statuses = !screen->show_statuses;

    if (screen->pending_action != BATTLE_PENDING_NONE) {
        if (screen->pending_action == BATTLE_PENDING_ROUND_RESULT) {
            bool held = IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if (screen->fx.active || screen->show_statuses) { screen->message_armed = false; return BATTLE_SCREEN_NONE; }
            screen->message_elapsed += dt;
            if (screen->message_elapsed >= message_read_seconds) {
                resolve_pending_action(screen);
                return BATTLE_SCREEN_NONE;
            }
            if (!screen->message_armed) {
                if (!held) screen->message_armed = true;
                return BATTLE_SCREEN_NONE;
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                resolve_pending_action(screen);
        } else {
            screen->action_timer -= dt;
            if (screen->action_timer <= 0) {
                resolve_pending_action(screen);
                screen->menu = BATTLE_MENU_ROOT;
                screen->menu_cursor = 0;
            }
        }
        return BATTLE_SCREEN_NONE;
    }

    if (battle_over(battle)) {
        if (screen->round_active) Battle_FinishRound(battle,&screen->round);
        clear_pending(screen); clear_round(screen);
        screen->menu = BATTLE_MENU_ROOT; screen->menu_cursor = 0;
        if (activate(return_button,KEY_ENTER)) return BATTLE_SCREEN_RETURN;
        return BATTLE_SCREEN_NONE;
    }

    const Team *team = &battle->player_team;
    const Plant *active = team->active_index >= 0 && team->active_index < team->count
        ? &team->plants[team->active_index] : NULL;
    if (enemy_replacement(battle)) {
        queue_action(screen, BATTLE_PENDING_CPU, -1);
        screen->pending_cpu_action = AI_GetPlannedAction(battle, &screen->pending_cpu_index);
        return BATTLE_SCREEN_NONE;
    }
    if (battle->state == BATTLE_STATE_ONGOING && active &&
        Plant_HasStatus(active, PLANT_STATUS_PARALYZED)) {
        queue_round(screen, (BattleAction){BATTLE_ACTION_NONE, -1}, plan_enemy_action(battle));
        screen->pending_action = BATTLE_PENDING_PARALYSIS;
        return BATTLE_SCREEN_NONE;
    }

    if (forced) screen->menu = BATTLE_MENU_TEAM;
    if (!forced && screen->menu != BATTLE_MENU_ROOT &&
        (activate(back_button, KEY_BACKSPACE))) {
        screen->menu = BATTLE_MENU_ROOT;
        screen->menu_cursor = 0;
        return BATTLE_SCREEN_NONE;
    }

    int cursor = screen->menu_cursor;
    int count = screen->menu == BATTLE_MENU_MOVES ? MAX_MOVES : MAX_TEAM_SIZE;
    if (IsKeyPressed(KEY_LEFT)) cursor--;
    if (IsKeyPressed(KEY_RIGHT)) cursor++;
    if (IsKeyPressed(KEY_UP)) cursor -= screen->menu == BATTLE_MENU_MOVES ? 1 : 2;
    if (IsKeyPressed(KEY_DOWN)) cursor += screen->menu == BATTLE_MENU_MOVES ? 1 : 2;
    screen->menu_cursor = (cursor + count) % count;
    Vector2 motion = GetMouseDelta();
    if (motion.x != 0 || motion.y != 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int i = 0; i < count; i++) {
            Rectangle bounds = screen->menu == BATTLE_MENU_MOVES ? move_button(i) : switch_button(i);
            if (CheckCollisionPointRec(GetMousePosition(),bounds)) screen->menu_cursor = i;
        }
    }
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (screen->menu == BATTLE_MENU_ROOT) {
        for (int i = 0; i < 4; i++) {
            if (activate(root_button(i), KEY_NULL) || (confirm && screen->menu_cursor == i)) {
                if (i < 2) {
                    screen->menu = i == 0 ? BATTLE_MENU_MOVES : BATTLE_MENU_TEAM;
                    screen->menu_cursor = 0;
                }
                return BATTLE_SCREEN_NONE;
            }
        }
    }

    /* Os atalhos antigos continuam disponíveis, sem criar regras paralelas. */
    if (battle->state == BATTLE_STATE_ONGOING) {
        for (int i = 0; i < MAX_MOVES; i++) {
            bool chosen = IsKeyPressed(move_keys[i]) ||
                (screen->menu == BATTLE_MENU_MOVES &&
                 (activate(move_button(i), KEY_NULL) || (confirm && screen->menu_cursor == i)));
            if (chosen) {
                if (Battle_CanUseMove(battle,0,battle->player_team.active_index,i)) {
                    queue_round(screen, (BattleAction){BATTLE_ACTION_MOVE, i},
                                plan_enemy_action(battle));
                    screen->menu = BATTLE_MENU_ROOT;
                    screen->menu_cursor = 0;
                }
                return BATTLE_SCREEN_NONE;
            }
        }
    }

    for (int i = 0; i < team->count; i++) {
        bool chosen = IsKeyPressed(switch_keys[i]) ||
            (screen->menu == BATTLE_MENU_TEAM &&
             (activate(switch_button(i), KEY_NULL) || (confirm && screen->menu_cursor == i)));
        if (chosen) {
            if (forced) {
                if (Battle_CanSwitch(battle, i)) {
                    queue_action(screen, BATTLE_PENDING_SWITCH, i);
                    screen->menu = BATTLE_MENU_ROOT;
                    screen->menu_cursor = 0;
                }
            } else if (battle->state == BATTLE_STATE_ONGOING &&
                       Battle_CanSideSwitch(battle, 0, i)) {
                queue_round(screen, (BattleAction){BATTLE_ACTION_SWITCH, i},
                            plan_enemy_action(battle));
                screen->menu = BATTLE_MENU_ROOT;
                screen->menu_cursor = 0;
            }
            break;
        }
    }
    return BATTLE_SCREEN_NONE;
}

/* Recursos emprestados são somente de leitura durante a composição. */
typedef struct {
    const UiTheme *ui;
    const SelectionUiAssets *fonts;
    const BattleScreenAssets *assets;
    const Texture2D *plants;
    const Rectangle *sources;
} BattleView;

static void text(const BattleView *view, const char *value, float x, float y, int size, Color color)
{
    selection_ui_pixel_text(view->ui, view->fonts, value, x, y, size, color);
}

static void text_box(const BattleView *view, const char *value, Rectangle bounds, int size, Color color)
{
    selection_ui_pixel_box(view->ui, view->fonts, value, bounds, size, color);
}

static void text_wrap(const BattleView *view, const char *value, Rectangle bounds, int size, Color color)
{
    float draw_size;
    Font font = selection_ui_font(view->ui, view->fonts, size, &draw_size);
    ui_text_wrap_reveal_pixels(font, value, bounds, draw_size, color, 10000, view->fonts->font_scale);
}

static void cell(Rectangle bounds, bool enabled, bool selected)
{
    selection_ui_panel(bounds, 3, enabled ? ink : (Color){130,130,111,255});
    Color fill = !enabled ? (Color){214,210,195,255} : selected ? (Color){214,229,170,255} : paper;
    selection_ui_panel((Rectangle){bounds.x+2,bounds.y+2,bounds.width-4,bounds.height-4},3,fill);
}

static void pointer(const BattleView *view, Rectangle bounds)
{
    selection_ui_skin(view->fonts, SELECTION_POINTER,
                       (Rectangle){bounds.x+16,bounds.y+bounds.height/2-12,18,25},1);
}

static Texture2D plant_art(const BattleView *view, int id, Rectangle *source)
{
    if (id == SPECIES_ALOE || id == SPECIES_SUNFLOWER) {
        int index = id == SPECIES_ALOE ? 0 : 1;
        *source = view->assets->boss_sources[index];
        return view->assets->bosses[index];
    }
    if (id >= 0 && id < SPECIES_COUNT) {
        *source = view->sources[id];
        return view->plants[id];
    }
    *source = (Rectangle){0};
    return (Texture2D){0};
}

static void plant_sprite(const BattleView *view, const Plant *plant, Rectangle bounds, bool mirrored, Color effect_tint)
{
    Rectangle source;
    Texture2D texture = plant_art(view, plant->id, &source);
    Color tint = effect_tint;
    if (texture.id && source.width > 0 && source.height > 0) {
        float scale = fminf(bounds.width/source.width,bounds.height/source.height);
        Rectangle dest = {roundf(bounds.x+(bounds.width-source.width*scale)/2),
                          roundf(bounds.y+bounds.height-source.height*scale),
                          source.width*scale,source.height*scale};
        if (mirrored) source.width = -source.width;
        DrawTexturePro(texture,source,dest,(Vector2){0},0,tint);
    } else {
        float x = bounds.x+bounds.width/2, base = bounds.y+bounds.height;
        DrawRectangle((int)x-4,(int)(base-bounds.height*0.5f),8,(int)(bounds.height*0.5f),DARKGREEN);
        DrawEllipse((int)x,(int)(base-bounds.height*0.55f),bounds.width*0.28f,bounds.height*0.25f,
                     plant->id >= 0 && plant->id < SPECIES_COUNT ? species_colors[plant->id] : GREEN);
    }
}

static void hp_bar(Rectangle bounds, const Plant *plant, float shown_hp)
{
    float fraction = plant->max_hp > 0 ? shown_hp/plant->max_hp : 0;
    fraction = fminf(1,fmaxf(0,fraction));
    selection_ui_panel(bounds,3,ink);
    Rectangle fill = {bounds.x+2,bounds.y+2,bounds.width-4,bounds.height-4};
    selection_ui_panel(fill,2,(Color){44,74,25,255});
    fill.width = roundf(fill.width*fraction);
    if (fill.width > 0) {
        Color color = fraction > .5f ? (Color){115,220,22,255}
                       : fraction > .25f ? (Color){240,187,38,255} : (Color){203,69,44,255};
        DrawRectangleRec(fill,color);
        DrawRectangle((int)fill.x,(int)fill.y,(int)fill.width,2,Fade(paper,.5f));
    }
}

static void status(const BattleView *view, const BattleScreen *screen, int side, Rectangle bounds)
{
    const Team *team = side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
    const Plant *active = &team->plants[team->active_index];
    battle_screen_draw_art(view->ui,view->assets,BATTLE_ART_STATUS,bounds);
    text(view,active->name_pt,bounds.x+28,bounds.y+22,28,ink);
    const char *label = side == 1 ? "Guardião" : battle_over(&screen->battle) ? "Sua equipe"
                        : screen->pending_action != BATTLE_PENDING_NONE ? "Em combate" : "Sua vez";
    text(view,active->id == SPECIES_PEPPER ? TextFormat("%s · Frutos %d",label,active->fruit_count) : label,
         bounds.x+28,bounds.y+49,22,ink);
    float shown_hp = battle_fx_hp(&screen->fx,side,team->active_index,active->current_hp);
    hp_bar((Rectangle){bounds.x+24,bounds.y+84,bounds.width-132,14},active,shown_hp);
    const char *hp = TextFormat("%d/%d",(int)roundf(shown_hp),active->max_hp);
    text(view,hp,bounds.x+bounds.width-18-selection_ui_measure(view->ui,view->fonts,hp,22),
          bounds.y+78,22,ink);
}

static void arena(const BattleView *view, const BattleScreen *screen)
{
    battle_screen_draw_art(view->ui,view->assets,
        screen->scene == BATTLE_SCENE_NURSERY ? BATTLE_ART_NURSERY : BATTLE_ART_DEPOT,
        (Rectangle){0,0,1280,720});
    battle_screen_draw_art(view->ui,view->assets,BATTLE_ART_GROUND,(Rectangle){252,386,424,98});
    battle_screen_draw_art(view->ui,view->assets,BATTLE_ART_GROUND,(Rectangle){736,254,302,74});
    const Team *player = &screen->battle.player_team, *enemy = &screen->battle.enemy_team;
    plant_sprite(view,&player->plants[player->active_index],
        battle_fx_plant_bounds(&screen->fx,0,(Rectangle){329,185,250,267}),false,
        battle_fx_tint(&screen->fx,0,Plant_IsAlive(&player->plants[player->active_index]) || screen->fx.active
            ? WHITE : (Color){135,135,135,130}));
    plant_sprite(view,&enemy->plants[enemy->active_index],
        battle_fx_plant_bounds(&screen->fx,1,(Rectangle){783,91,210,211}),true,
        battle_fx_tint(&screen->fx,1,Plant_IsAlive(&enemy->plants[enemy->active_index]) || screen->fx.active
            ? WHITE : (Color){135,135,135,130}));
    battle_fx_draw(&screen->fx,&screen->battle);
    status(view,screen,1,(Rectangle){70,64,354,114});
    status(view,screen,0,(Rectangle){815,343,376,114});
    battle_screen_draw_art(view->ui,view->assets,BATTLE_ART_LOCATION,(Rectangle){518,2,244,54});
    text_box(view,screen->scene == BATTLE_SCENE_NURSERY ? "Viveiro" : "Depósito",
              (Rectangle){548,10,184,34},28,ink);
}

static void menu_root(const BattleView *view, const BattleScreen *screen, bool enabled)
{
    static const char *const labels[] = {"Atacar","Trocar","Mochila","Fugir"};
    for (int i = 0; i < 4; i++) {
        Rectangle box = root_button(i);
        bool available = enabled && i < 2;
        bool focused = available && screen->menu_cursor == i;
        cell(box,available,focused);
        if (focused) pointer(view,box);
        Color color = available ? ink : (Color){124,127,112,255};
        text_box(view,labels[i],(Rectangle){box.x+48,box.y+8,i<2 ? box.width-66 : 161,box.height-16},
                  i<2 ? 36 : 30,color);
        if (i >= 2) {
            Rectangle badge = {box.x+211,box.y+18,96,26};
            selection_ui_panel(badge,3,(Color){146,146,125,255});
            text_box(view,"Em breve",badge,20,paper);
        }
    }
    text_box(view,"Vamos implementar em breve",(Rectangle){600,652,600,24},20,(Color){124,127,112,255});
}

static void back_control(const BattleView *view)
{
    cell(back_button,true,CheckCollisionPointRec(GetMousePosition(),back_button));
    text_box(view,"Voltar",back_button,22,ink);
}

static void menu_moves(const BattleView *view, const BattleScreen *screen)
{
    const BattleState *battle = &screen->battle;
    int index = battle->player_team.active_index;
    const Plant *plant = &battle->player_team.plants[index];
    for (int i = 0; i < MAX_MOVES; i++) {
        const Move *move = &plant->moves[i];
        Rectangle box = move_button(i);
        bool enabled = Battle_CanUseMove(battle,0,index,i);
        bool focus = enabled && screen->menu_cursor == i;
        Color color = enabled ? ink : (Color){112,114,99,255};
        cell(box,enabled,focus);
        if (focus) pointer(view,box);
        bool logical = battle->boosted_move_indices[0][index] == i;
        if (logical) {
            LogicFormula formula = battle->formulas[0][index];
            LogicEvaluation value = Battle_EvaluatePlantFormula(battle,0,index,formula);
            const char *evaluation = TextFormat("%s %s %s = %s · %s",value.left ? "V" : "F",
                operators[formula.operator],value.right ? "V" : "F",value.result ? "V" : "F",
                value.result ? TextFormat("+%d",Battle_GetFormulaPowerBonus(formula.operator)) : "bloqueado");
            float width = selection_ui_measure(view->ui,view->fonts,evaluation,20)+12;
            text(view,TextFormat("%d · %s · lógico",i+1,move->name),box.x+48,box.y+2,24,color);
            text(view,evaluation,box.x+box.width-width,box.y+6,20,
                value.result ? ink : (Color){143,66,42,255});
        } else {
            text(view,TextFormat("%d · %s%s",i+1,move->name,
                enabled ? "" : " · indisponível"),box.x+48,box.y+2,24,color);
        }
        if (move->effect == MOVE_EFFECT_ATTRIBUTE) {
            const char *stat = move->stat == MOVE_STAT_ATTACK ? "ATQ" : move->stat == MOVE_STAT_DEFENSE ? "DEF" : "VEL";
            text(view,TextFormat("%s %s · passo %.1fx · limite %.1fx",move->target == MOVE_TARGET_SELF ? "Próprio" : "Rival",
                stat,move->effect_step,move->effect_cap),box.x+48,box.y+28,20,color);
        } else {
            text(view,TextFormat("Poder %d · Precisão %d%% · Crítico %d%% · Prioridade %d",
                Battle_GetPlantMovePower(battle,0,index,i),move->accuracy,
                Battle_GetCriticalChance(plant,move),move->priority),box.x+48,box.y+28,20,color);
        }
    }
    back_control(view);
}

static void menu_team(const BattleView *view, const BattleScreen *screen, bool forced)
{
    const Team *team = &screen->battle.player_team;
    for (int i = 0; i < MAX_TEAM_SIZE; i++) {
        Rectangle box = switch_button(i);
        bool available = Battle_CanSwitch(&screen->battle,i);
        bool focus = available && screen->menu_cursor == i;
        cell(box,available,focus);
        Color color = available ? ink : (Color){112,114,99,255};
        if (i >= team->count) { text_box(view,"Vaga livre",box,22,color); continue; }
        const Plant *plant = &team->plants[i];
        plant_sprite(view,plant,(Rectangle){box.x+9,box.y+7,38,46},false,Plant_IsAlive(plant) ? WHITE : GRAY);
        const char *name = TextFormat("%s · %s",switch_labels[i],plant->name_pt);
        text_box(view,name,(Rectangle){box.x+52,box.y+5,box.width-62,26},22,color);
        const char *state = i == team->active_index ? (Plant_IsAlive(plant) ? " · ativa" : " · derrotada")
                           : !Plant_IsAlive(plant) ? " · derrotada" : "";
        text(view,TextFormat("%d/%d HP%s",plant->current_hp,plant->max_hp,state),box.x+56,box.y+34,20,color);
        if (focus) DrawRectangleLinesEx((Rectangle){box.x+3,box.y+3,box.width-6,box.height-6},2,ORANGE);
    }
    if (!forced) back_control(view);
    text(view,forced ? "Escolha uma planta viva." : "Q/W/E/R · escolher reserva",575,654,20,ink);
}

static void setup_button(const BattleView *view, Rectangle bounds, const char *label, bool enabled)
{
    cell(bounds,enabled,enabled && CheckCollisionPointRec(GetMousePosition(),bounds));
    text_box(view,label,(Rectangle){bounds.x+12,bounds.y+6,bounds.width-24,bounds.height-12},22,
              enabled ? ink : GRAY);
}

static void setup_view(const BattleView *view, const BattleScreen *screen)
{
    /* Mantém os mesmos controles e limites da montagem anterior. */
    battle_screen_draw_art(view->ui,view->assets,
        screen->scene == BATTLE_SCENE_NURSERY ? BATTLE_ART_NURSERY : BATTLE_ART_DEPOT,(Rectangle){0,0,1280,720});
    DrawRectangle(0,0,1280,720,Fade((Color){15,35,18,255},.72f));
    ui_botanical_panel(view->ui,(Rectangle){40,25,1200,657});
    const Team *team = screen->setup_side == 0 ? &screen->battle.player_team : &screen->battle.enemy_team;
    int plant_index = screen->draft_plant_index;
    if (plant_index < 0 || plant_index >= team->count) plant_index = 0;
    const Plant *active = &team->plants[plant_index];
    if (screen->choosing_team) {
        text(view,"Monte sua equipe",76,48,36,ink);
        text(view,"Escolha uma vaga e depois uma espécie.",76,106,24,ink);
        for (int i = 0; i < STARTER_SPECIES_COUNT; i++) {
            const SpeciesData *data = PlantData_GetStarter(i);
            Rectangle box = species_card(i);
            cell(box,true,screen->roster_cursor == i);
            Plant plant = {0};
            plant.id = data->id; plant.current_hp = 1;
            plant_sprite(view,&plant,(Rectangle){box.x+10,box.y+10,55,74},false,WHITE);
            text_box(view,data->name_pt,(Rectangle){box.x+70,box.y+14,box.width-82,33},22,ink);
            text(view,TextFormat("Vida %d · ATQ %d",data->base_hp,data->base_attack),box.x+72,box.y+57,20,ink);
        }
        for (int i = 0; i < MAX_TEAM_SIZE; i++) {
            Rectangle box = team_slot_button(i);
            setup_button(view,box,TextFormat("%s · %s",switch_labels[i],i<team->count ? team->plants[i].name_pt : "Vaga livre"),i<=team->count);
            if (i == screen->team_slot) DrawRectangleLinesEx(box,2,ORANGE);
        }
        setup_button(view,setup_confirm,"ENTER · Confirmar equipe",true);
        setup_button(view,remove_reserve,"BACKSPACE · Remover reserva",team->count>MIN_TEAM_SIZE && screen->team_slot>0 && screen->team_slot<team->count);
        return;
    }
    text(view,"Fortaleça um golpe",76,48,36,ink);
    text(view,"Configure cada planta com as setas ou os cartões abaixo.",76,108,24,ink);
    for (int i = 0; i < team->count; i++) {
        setup_button(view,setup_plant_buttons[i],team->plants[i].name_pt,true);
        if (i == plant_index) DrawRectangleLinesEx(setup_plant_buttons[i],2,ORANGE);
    }
    text(view,"Condição P",64,209,22,ink);
    text(view,"Operador",480,209,22,ink);
    text(view,"Condição Q",696,209,22,ink);
    setup_button(view,setup_left,TextFormat("Q · %s",conditions[screen->draft.left]),true);
    setup_button(view,setup_operator,TextFormat("E · %s",operators[screen->draft.operator]),true);
    setup_button(view,setup_right,TextFormat("W · %s",conditions[screen->draft.right]),true);
    LogicEvaluation value = Battle_EvaluatePlantFormula(&screen->battle,screen->setup_side,plant_index,screen->draft);
    cell((Rectangle){64,319,1152,197},true,false);
    text(view,TextFormat("P %s Q",operators[screen->draft.operator]),86,334,36,ink);
    text(view,operator_rules[screen->draft.operator],86,383,24,ink);
    text(view,TextFormat("Bônus de poder: +%d quando a fórmula for verdadeira.",
                          Battle_GetFormulaPowerBonus(screen->draft.operator)),86,422,24,ink);
    text(view,TextFormat("Agora: %s %s %s = %s · Falso bloqueia o golpe escolhido",value.left ? "V" : "F",operators[screen->draft.operator],
                          value.right ? "V" : "F",value.result ? "V" : "F"),86,465,24,ink);
    BattleState preview = screen->battle;
    preview.formulas[screen->setup_side][plant_index] = screen->draft;
    preview.boosted_move_indices[screen->setup_side][plant_index] = screen->draft_move_index;
    for (int i = 0; i < MAX_MOVES; i++) {
        Rectangle box = setup_move_buttons[i];
        bool enabled = active->moves[i].effect == MOVE_EFFECT_DAMAGE;
        cell(box,enabled,screen->draft_move_index == i);
        text_box(view,TextFormat("%d · %s",i+1,active->moves[i].name),
            (Rectangle){box.x+8,box.y+2,box.width-16,24},22,enabled ? ink : GRAY);
        const char *label = !enabled ? "Atributo · sem bônus de poder" :
            TextFormat("Poder %d%s",Battle_GetPlantMovePower(&preview,screen->setup_side,plant_index,i),
                screen->draft_move_index == i ? " · escolhido" : "");
        text_box(view,label,(Rectangle){box.x+8,box.y+27,box.width-16,20},20,enabled ? ink : GRAY);
        if (screen->draft_move_index == i) DrawRectangleLinesEx(box,2,ORANGE);
    }
    selection_ui_button(view->ui,view->fonts,setup_confirm,"ENTER · Confirmar fórmula",false,false,1);
}

static void status_overlay(const BattleView *view, const BattleState *battle)
{
    static const char *const names[PLANT_STATUS_COUNT] = {
        "Veneno", "Queimadura", "Sorte", "Paralisia", "Hospedeiro", "Velocidade aumentada",
        "Ataque aumentado", "Defesa aumentada", "Precisão aumentada", "Ataque reduzido",
        "Defesa reduzida", "Velocidade reduzida"
    };
    DrawRectangle(0,0,1280,720,Fade(ink,.72f));
    ui_botanical_panel(view->ui,(Rectangle){40,24,1200,672});
    text(view,"Efeitos em campo · TAB para fechar",76,48,30,ink);
    for (int side = 0; side < 2; side++) {
        const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
        float x = 76 + side*588;
        text(view,side ? "Guardião" : "Sua equipe",x,94,26,ink);
        for (int index = 0; index < team->count && index < MAX_TEAM_SIZE; index++) {
            const Plant *plant = &team->plants[index];
            Rectangle card = {x,133+index*132,548,125};
            cell(card,Plant_IsAlive(plant),index == team->active_index);
            text(view,TextFormat("%s%s",plant->name_pt,index == team->active_index ? " · ativa" : ""),x+12,card.y+5,24,ink);
            text(view,TextFormat("HP %d/%d · ATQ %d · DEF %d · VEL %d",plant->current_hp,plant->max_hp,
                plant->attack,plant->defense,plant->speed),x+12,card.y+32,20,ink);
            char effects[1024] = {0};
            int used = snprintf(effects,sizeof(effects),"Frutos %d",plant->fruit_count);
            for (int i = 0; i < plant->status_count && i < PLANT_STATUS_COUNT; i++) {
                const PlantStatus *effect = &plant->statuses[i];
                const char *name = effect->type >= 0 && effect->type < PLANT_STATUS_COUNT ? names[effect->type] : "Efeito";
                const char *duration = effect->remaining_turns ? TextFormat("%dt",effect->remaining_turns) : "permanente";
                int written = snprintf(effects+used,sizeof(effects)-(size_t)used," · %s %s (%.1fx)",name,duration,effect->value);
                if (written < 0 || (size_t)written >= sizeof(effects)-(size_t)used) break;
                used += written;
            }
            if (!plant->status_count) snprintf(effects+used,sizeof(effects)-(size_t)used," · Sem efeitos ativos");
            text_wrap(view,effects,(Rectangle){x+12,card.y+58,524,65},20,ink);
        }
    }
    text(view,"Hospedeiro: 1/16 da vida máxima do afetado por turno; esse dano cura a planta ativa adversária.",
         76,672,16,ink);
}

static const char *pending_message(const BattleScreen *screen)
{
    const BattleState *battle = &screen->battle;
    if (screen->pending_action == BATTLE_PENDING_ROUND_RESULT) {
        if (screen->round_message_index < screen->round_message_count)
            return screen->round_result_message + screen->round_message_offsets[screen->round_message_index];
        return "A ação terminou...";
    }
    if (screen->pending_action == BATTLE_PENDING_PARALYSIS) {
        const Team *team = &battle->player_team;
        if (team->active_index >= 0 && team->active_index < team->count)
            return TextFormat("%s está paralisada...",team->plants[team->active_index].name_pt);
        return "A ação foi perdida...";
    }
    if (screen->pending_action == BATTLE_PENDING_ROUND && screen->round_active &&
        screen->round.step >= 0 && screen->round.step < 2) {
        int side = screen->round.sides[screen->round.step];
        const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
        int plant_index = screen->round.acted_indices[side];
        BattleAction action = screen->round.actions[side];
        if (plant_index < 0 || plant_index >= team->count) return "A ação foi escolhida...";
        const Plant *plant = &team->plants[plant_index];
        if (action.type == BATTLE_ACTION_MOVE && action.index >= 0 && action.index < MAX_MOVES)
            return TextFormat("%s vai usar %s...",plant->name_pt,plant->moves[action.index].name);
        if (action.type == BATTLE_ACTION_SWITCH && action.index >= 0 && action.index < team->count)
            return TextFormat("%s vai entrar em campo...",team->plants[action.index].name_pt);
        if (action.type == BATTLE_ACTION_NONE) return TextFormat("%s perde a ação...",plant->name_pt);
        return "A ação foi escolhida...";
    }
    const Team *team = screen->pending_action == BATTLE_PENDING_CPU
        ? &battle->enemy_team : &battle->player_team;
    const Plant *plant = team->active_index >= 0 && team->active_index < team->count
        ? &team->plants[team->active_index] : NULL;
    int move = screen->pending_action == BATTLE_PENDING_MOVE ? screen->pending_move_index
        : screen->pending_cpu_action == AI_ACTION_MOVE ? screen->pending_cpu_index : -1;
    int reserve = screen->pending_action == BATTLE_PENDING_SWITCH ? screen->pending_plant_index
        : screen->pending_cpu_action == AI_ACTION_SWITCH ? screen->pending_cpu_index : -1;
    if (move >= 0 && move < MAX_MOVES && plant)
        return TextFormat("%s prepara %s...",plant->name_pt,plant->moves[move].name);
    if (reserve >= 0 && reserve < team->count)
        return TextFormat("%s vai entrar em campo...",team->plants[reserve].name_pt);
    return "O guardião está escolhendo...";
}

void battle_screen_draw(const UiTheme *ui, const BattleScreen *screen,
                        const BattleScreenAssets *assets, const SelectionUiAssets *fonts,
                        const Texture2D plants[SPECIES_COUNT], const Rectangle sources[SPECIES_COUNT])
{
    BattleView view = {ui,fonts,assets,plants,sources};
    if (screen->setup_side >= 0) { setup_view(&view,screen); return; }
    arena(&view,screen);
    battle_screen_draw_art(ui,assets,BATTLE_ART_COMMANDS,(Rectangle){12,480,1256,226});
    const BattleState *battle = &screen->battle;
    bool ended = battle_over(battle), cpu = enemy_replacement(battle);
    bool pending = screen->pending_action != BATTLE_PENDING_NONE;
    bool forced = battle->state == BATTLE_STATE_REPLACEMENT && battle->current_turn == 0;
    const Plant *active = &battle->player_team.plants[battle->player_team.active_index];
    const char *question = pending ? "Rodada em andamento" : ended ? (battle->state == BATTLE_STATE_PLAYER_WIN ? "Você venceu!" : "Sua equipe foi derrotada.")
        : cpu ? "O guardião está escolhendo..." : forced ? "Escolha uma reserva para continuar."
        : screen->menu == BATTLE_MENU_TEAM ? "Qual planta vai entrar em campo?"
        : screen->menu == BATTLE_MENU_MOVES ? "Escolha seu golpe."
        : TextFormat("O que %s\nvai fazer?",active->name_pt);
    text_wrap(&view,question,(Rectangle){72,544,445,72},28,ink);
    text(&view,"TAB · efeitos",76,525,20,ink);
    char order[160] = {0};
    if (screen->round_active) {
        int first = screen->round.sides[0], second = screen->round.sides[1];
        const Team *a = first == 0 ? &battle->player_team : &battle->enemy_team;
        const Team *b = second == 0 ? &battle->player_team : &battle->enemy_team;
        int ai = screen->round.acted_indices[first], bi = screen->round.acted_indices[second];
        if (ai >= 0 && ai < a->count && bi >= 0 && bi < b->count)
            snprintf(order,sizeof(order),"Ordem desta rodada:\n1º: %s / 2º: %s",a->plants[ai].name_pt,b->plants[bi].name_pt);
    }
    const char *log = pending ? order : battle->last_message;
    text_wrap(&view,log,(Rectangle){72,612,445,74},20,ink);
    if (pending) {
        cell((Rectangle){568,518,647,108},true,false);
        text_wrap(&view,pending_message(screen),(Rectangle){594,532,595,82},28,ink);
        bool reading = screen->pending_action == BATTLE_PENDING_ROUND_RESULT && !screen->fx.active;
        Rectangle next = {568,638,647,36};
        cell(next,reading,reading);
        text_box(&view,reading ? "Avança sozinho · ENTER / clique: adiantar" : "Aguarde a animação...",next,24,ink);
    } else if (ended) {
        cell(return_button,true,true);
        text_box(&view,"ENTER · Voltar à estufa",return_button,30,ink);
    } else if (cpu || pending) {
        menu_root(&view,screen,false);
    } else if (forced || screen->menu == BATTLE_MENU_TEAM) {
        menu_team(&view,screen,forced);
    } else if (screen->menu == BATTLE_MENU_MOVES) {
        menu_moves(&view,screen);
    } else menu_root(&view,screen,true);
    if (screen->show_statuses) status_overlay(&view,battle);
}
