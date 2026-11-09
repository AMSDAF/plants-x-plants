/* Valida ataques e trocas e controla turnos, nocautes e vitória. */
#include "battle.h"

#include <stdio.h>
#include <stdlib.h>

static int has_reserve(const Team *team)
{
    for (int i = 0; i < team->count; i++) {
        if (i != team->active_index && Plant_IsAlive(&team->plants[i])) return 1;
    }
    return 0;
}

static int has_low_hp(const Team *team)
{
    if (team->active_index < 0 || team->active_index >= team->count) return 0;
    const Plant *plant = &team->plants[team->active_index];
    return Plant_IsAlive(plant) && plant->current_hp <= plant->max_hp / 2;
}

static int evaluate_condition(LogicCondition condition, const Team *own, const Team *enemy)
{
    switch (condition) {
        case LOGIC_OWN_LOW_HP: return has_low_hp(own);
        case LOGIC_ENEMY_RESERVE: return has_reserve(enemy);
        case LOGIC_ENEMY_LOW_HP: return has_low_hp(enemy);
        case LOGIC_OWN_RESERVE: return has_reserve(own);
        default: return 0;
    }
}

LogicEvaluation Battle_EvaluateFormula(const BattleState *battle, int side, LogicFormula formula)
{
    if (formula.left < 0 || formula.left >= LOGIC_CONDITION_COUNT ||
        formula.right < 0 || formula.right >= LOGIC_CONDITION_COUNT ||
        formula.left == formula.right)
        return (LogicEvaluation){0};

    const Team *own = side == 0 ? &battle->player_team : &battle->enemy_team;
    const Team *enemy = side == 0 ? &battle->enemy_team : &battle->player_team;
    LogicEvaluation value = {
        .left = evaluate_condition(formula.left, own, enemy),
        .right = evaluate_condition(formula.right, own, enemy)
    };
    switch (formula.operator) {
        case LOGIC_AND: value.result = value.left && value.right; break;
        case LOGIC_OR: value.result = value.left || value.right; break;
        case LOGIC_XOR: value.result = value.left != value.right; break;
        default: break;
    }
    return value;
}

const Move *Battle_LogicMove(LogicOperator operator)
{
    static const Move moves[LOGIC_OPERATOR_COUNT] = {
        [LOGIC_AND] = {"Golpe Lógico", 60, 100},
        [LOGIC_OR] = {"Golpe Lógico", 45, 100},
        [LOGIC_XOR] = {"Golpe Lógico", 55, 100}
    };
    if (operator < 0 || operator >= LOGIC_OPERATOR_COUNT) return NULL;
    return &moves[operator];
}

const Move *Battle_GetAvailableLogicMove(const BattleState *battle)
{
    if (battle->state != BATTLE_STATE_ONGOING) return NULL;
    LogicFormula formula = battle->formulas[battle->current_turn];
    if (!Battle_EvaluateFormula(battle, battle->current_turn, formula).result) return NULL;
    return Battle_LogicMove(formula.operator);
}

void Battle_Init(BattleState *battle, Team player_team, Team enemy_team)
{
    *battle = (BattleState){
        .player_team = player_team,
        .enemy_team = enemy_team,
        .last_message = "A batalha começou!"
    };
    Battle_CheckConditions(battle);
}

int Battle_ExecuteMove(BattleState *battle, int move_index)
{
    if (battle->state != BATTLE_STATE_ONGOING || move_index < 0 || move_index > BATTLE_LOGIC_MOVE_INDEX)
        return 0;

    Team *attacker_team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    Team *defender_team = battle->current_turn == 0 ? &battle->enemy_team : &battle->player_team;
    Plant *attacker = Team_GetActivePlant(attacker_team);
    Plant *defender = Team_GetActivePlant(defender_team);

    if (!attacker || !defender || !Plant_IsAlive(attacker) || !Plant_IsAlive(defender))
        return 0;

    const Move *move = move_index == BATTLE_LOGIC_MOVE_INDEX
        ? Battle_GetAvailableLogicMove(battle) : &attacker->moves[move_index];
    if (!move) return 0;
    if (move->accuracy < 100 && rand() % 100 >= move->accuracy) {
        snprintf(battle->last_message, sizeof(battle->last_message),
                 "%s errou %s!", attacker->name_pt, move->name);
        battle->current_turn = 1 - battle->current_turn;
        return 1;
    }

    int raw_damage = move->power + attacker->attack - defender->defense / 2;
    if (raw_damage < 5) raw_damage = 5;

    int previous_hp = defender->current_hp;
    Plant_TakeDamage(defender, raw_damage);
    int damage = previous_hp - defender->current_hp;
    int knocked_out = !Plant_IsAlive(defender);
    snprintf(battle->last_message, sizeof(battle->last_message),
             "%s usou %s: %d de dano.%s", attacker->name_pt, move->name,
             damage, knocked_out ? " Planta derrotada!" : "");

    Battle_CheckConditions(battle);
    if (battle->state == BATTLE_STATE_ONGOING) {
        battle->current_turn = 1 - battle->current_turn;
        if (knocked_out) battle->state = BATTLE_STATE_REPLACEMENT;
    }

    return 1;
}

int Battle_CanSwitch(const BattleState *battle, int plant_index)
{
    if (battle->state != BATTLE_STATE_ONGOING && battle->state != BATTLE_STATE_REPLACEMENT)
        return 0;

    const Team *team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    return plant_index >= 0 && plant_index < team->count &&
           plant_index != team->active_index && Plant_IsAlive(&team->plants[plant_index]);
}

int Battle_SwitchPlant(BattleState *battle, int plant_index)
{
    if (!Battle_CanSwitch(battle, plant_index)) return 0;

    Team *team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    team->active_index = plant_index;
    snprintf(battle->last_message, sizeof(battle->last_message),
             "%s entrou em campo!", team->plants[plant_index].name_pt);

    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        battle->state = BATTLE_STATE_ONGOING;
    } else {
        battle->current_turn = 1 - battle->current_turn;
    }
    return 1;
}

void Battle_CheckConditions(BattleState *battle)
{
    if (!Team_HasLivingPlants(&battle->enemy_team)) {
        battle->state = BATTLE_STATE_PLAYER_WIN;
    } else if (!Team_HasLivingPlants(&battle->player_team)) {
        battle->state = BATTLE_STATE_ENEMY_WIN;
    }
}
