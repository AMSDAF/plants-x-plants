/* Valida ataques e trocas e controla turnos, nocautes e vitória. */
#include "battle.h"
#include "plant_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CARNIVOROUS_PARALYSIS_CHANCE 30

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

static const Plant *active_plant(const Team *team)
{
    if (!team || team->active_index < 0 || team->active_index >= team->count) return NULL;
    return &team->plants[team->active_index];
}

/* Statuses que prejudicam a planta ativa e podem satisfazer a condição lógica. */
static int has_damaging_status(const Plant *plant)
{
    if (!plant || !Plant_IsAlive(plant)) return 0;
    return Plant_HasStatus(plant, PLANT_STATUS_POISON) ||
           Plant_HasStatus(plant, PLANT_STATUS_BURN) ||
           Plant_HasStatus(plant, PLANT_STATUS_HOST) ||
           Plant_HasStatus(plant, PLANT_STATUS_PARALYZED);
}

static int enemy_is_faster(const Team *own, const Team *enemy, int own_plant_index)
{
    if (own_plant_index < 0 || own_plant_index >= own->count) return 0;
    const Plant *own_plant = &own->plants[own_plant_index];
    const Plant *enemy_plant = active_plant(enemy);
    if (!Plant_IsAlive(own_plant) || !enemy_plant || !Plant_IsAlive(enemy_plant)) return 0;
    return enemy_plant->speed > own_plant->speed;
}

/* O buff lógico pertence somente ao jogador; o guardião usa golpes normais. */
static int logic_buff_enabled(int side)
{
    return side == 0;
}

static Team *team_for_side(BattleState *battle, int side)
{
    return side == 0 ? &battle->player_team : &battle->enemy_team;
}

static const Team *const_team_for_side(const BattleState *battle, int side)
{
    return side == 0 ? &battle->player_team : &battle->enemy_team;
}

static int can_switch_side(const BattleState *battle, int side, int plant_index)
{
    if (!battle || (side != 0 && side != 1)) return 0;
    if (battle->state != BATTLE_STATE_ONGOING && battle->state != BATTLE_STATE_REPLACEMENT)
        return 0;
    const Team *team = const_team_for_side(battle, side);
    return plant_index >= 0 && plant_index < team->count &&
           plant_index != team->active_index && Plant_IsAlive(&team->plants[plant_index]);
}

static void append_status_message(BattleState *battle, const char *message)
{
    size_t length = strlen(battle->last_message);
    if (length >= sizeof(battle->last_message) - 1) return;
    snprintf(battle->last_message + length, sizeof(battle->last_message) - length,
             " %s", message);
}

static int status_damage(const Plant *plant, int denominator)
{
    int damage = plant->max_hp / denominator;
    return damage > 0 ? damage : 1;
}

/* Aplica o dano e devolve o HP efetivamente perdido para mensagens e recordes. */
static int damage_plant(BattleState *battle, int side, Plant *plant, int requested)
{
    int previous_hp = plant->current_hp;
    Plant_TakeDamage(plant, requested);
    int damage = previous_hp - plant->current_hp;
    if (side == 0) battle->damage_taken += damage;
    return damage;
}

static int heal_plant(Plant *plant, int percent)
{
    if (!plant || !Plant_IsAlive(plant) || percent <= 0) return 0;
    int amount = (plant->max_hp * percent + 99) / 100;
    if (amount < 1) amount = 1;
    int old_hp = plant->current_hp;
    plant->current_hp += amount;
    if (plant->current_hp > plant->max_hp) plant->current_hp = plant->max_hp;
    return plant->current_hp - old_hp;
}

static int rounded_stat_amount(int base_stat, float multiplier)
{
    int amount = (int)(base_stat * multiplier + 0.5f);
    return amount > 0 ? amount : 1;
}

static int get_plant_stat(const Plant *plant, MoveStat stat)
{
    if (stat == MOVE_STAT_ATTACK) return plant->attack;
    if (stat == MOVE_STAT_DEFENSE) return plant->defense;
    if (stat == MOVE_STAT_SPEED) return plant->speed;
    return 0;
}

/* Compara o atributo atual da planta do buff com o da planta ativa rival. */
static int own_stat_higher(const Team *own, const Team *enemy, int own_plant_index, MoveStat stat)
{
    if (own_plant_index < 0 || own_plant_index >= own->count) return 0;
    const Plant *own_plant = &own->plants[own_plant_index];
    const Plant *enemy_plant = active_plant(enemy);
    if (!Plant_IsAlive(own_plant) || !enemy_plant || !Plant_IsAlive(enemy_plant)) return 0;
    return get_plant_stat(own_plant, stat) > get_plant_stat(enemy_plant, stat);
}

static int get_plant_base_stat(const Plant *plant, MoveStat stat)
{
    if (stat == MOVE_STAT_ATTACK) return plant->base_attack;
    if (stat == MOVE_STAT_DEFENSE) return plant->base_defense;
    if (stat == MOVE_STAT_SPEED) return plant->base_speed;
    return 0;
}

static void set_plant_stat(Plant *plant, MoveStat stat, int value)
{
    if (stat == MOVE_STAT_ATTACK) plant->attack = value;
    else if (stat == MOVE_STAT_DEFENSE) plant->defense = value;
    else if (stat == MOVE_STAT_SPEED) plant->speed = value;
}

static PlantStatusType stat_boost_status(MoveStat stat)
{
    if (stat == MOVE_STAT_ATTACK) return PLANT_STATUS_ATTACK_BOOST;
    if (stat == MOVE_STAT_DEFENSE) return PLANT_STATUS_DEFENSE_BOOST;
    return PLANT_STATUS_SPEED_BOOST;
}

static PlantStatusType stat_debuff_status(MoveStat stat)
{
    if (stat == MOVE_STAT_ATTACK) return PLANT_STATUS_ATTACK_DOWN;
    if (stat == MOVE_STAT_DEFENSE) return PLANT_STATUS_DEFENSE_DOWN;
    return PLANT_STATUS_SPEED_DOWN;
}

static void set_stat_status(Plant *plant, PlantStatusType type, float multiplier)
{
    for (int i = 0; i < plant->status_count; i++) {
        if (plant->statuses[i].type == type) {
            plant->statuses[i].value = multiplier;
            return;
        }
    }
    Plant_AddStatus(plant, type, 0, multiplier);
}

static void clear_stat_statuses(Plant *plant)
{
    static const PlantStatusType temporary_statuses[] = {
        PLANT_STATUS_ATTACK_BOOST, PLANT_STATUS_DEFENSE_BOOST,
        PLANT_STATUS_SPEED_BOOST, PLANT_STATUS_ACCURACY_BOOST,
        PLANT_STATUS_ATTACK_DOWN, PLANT_STATUS_DEFENSE_DOWN,
        PLANT_STATUS_SPEED_DOWN
    };
    for (size_t i = 0; i < sizeof(temporary_statuses) / sizeof(temporary_statuses[0]); i++)
        Plant_RemoveStatus(plant, temporary_statuses[i]);
}

static void restore_temporary_stats(Plant *plant)
{
    if (!plant) return;
    plant->attack = plant->base_attack;
    plant->defense = plant->base_defense;
    plant->speed = plant->base_speed;
    memset(plant->stat_debuff_amount, 0, sizeof(plant->stat_debuff_amount));
    clear_stat_statuses(plant);
}

static void apply_turn_statuses(BattleState *battle, int side, int plant_index)
{
    Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
    Team *opponent = side == 0 ? &battle->enemy_team : &battle->player_team;
    if (plant_index < 0 || plant_index >= team->count) return;

    Plant *plant = &team->plants[plant_index];
    if (!Plant_IsAlive(plant)) return;

    for (int i = 0; i < plant->status_count; i++) {
        PlantStatusType type = plant->statuses[i].type;
        int denominator = type == PLANT_STATUS_POISON ? 8
                        : type == PLANT_STATUS_BURN ? 16
                        : type == PLANT_STATUS_HOST ? 16 : 0;
        if (denominator == 0) continue;

        int requested_damage = status_damage(plant, denominator);
        int damage = damage_plant(battle, side, plant, requested_damage);
        char message[128];
        if (type == PLANT_STATUS_POISON) {
            snprintf(message, sizeof(message), "Veneno: %s sofreu %d de dano.",
                     plant->name_pt, damage);
            append_status_message(battle, message);
        } else if (type == PLANT_STATUS_BURN) {
            snprintf(message, sizeof(message), "Queimadura: %s sofreu %d de dano.",
                     plant->name_pt, damage);
            append_status_message(battle, message);
        } else {
            Plant *healed = Team_GetActivePlant(opponent);
            int recovered = 0;
            if (healed && Plant_IsAlive(healed)) {
                int old_hp = healed->current_hp;
                healed->current_hp += damage;
                if (healed->current_hp > healed->max_hp) healed->current_hp = healed->max_hp;
                recovered = healed->current_hp - old_hp;
            }
            snprintf(message, sizeof(message),
                     "Hospedeiro: %s sofreu %d; %s recuperou %d de vida.",
                     plant->name_pt, damage, healed ? healed->name_pt : "Adversário", recovered);
            append_status_message(battle, message);
        }
    }

    Plant_TickStatuses(plant);

    if (!Plant_IsAlive(plant) || team->active_index != plant_index) return;
    int ability_period = plant->id == SPECIES_SUNFLOWER ? 3 : 2;
    plant->ability_turn_count = (plant->ability_turn_count + 1) % ability_period;
    if (plant->ability_turn_count != 0) return;

    if (plant->id == SPECIES_SUNFLOWER) {
        int healed = heal_plant(plant, 15);
        if (healed > 0) {
            char message[96];
            snprintf(message, sizeof(message), "Girassol recuperou %d de vida.", healed);
            append_status_message(battle, message);
        }
    } else if (plant->id == SPECIES_PEPPER && plant->fruit_count < INT_MAX) {
        plant->fruit_count++;
        append_status_message(battle, "Pimenta gerou 1 fruto.");
    }
}

static int apply_stat_move(Plant *attacker, Plant *defender, const Move *move,
                           int *old_value, int *new_value)
{
    if (!attacker || !defender || !move || move->stat < 0 || move->stat >= MOVE_STAT_COUNT ||
        move->target < MOVE_TARGET_SELF || move->target > MOVE_TARGET_OPPONENT)
        return 0;
    Plant *target = move->target == MOVE_TARGET_SELF ? attacker : defender;
    if (move->stat_change_mode == MOVE_STAT_CHANGE_ADD_TO_CAP) {
        int base = get_plant_base_stat(target, move->stat);
        int current = get_plant_stat(target, move->stat);
        int cap = rounded_stat_amount(base, move->effect_cap);
        int step = rounded_stat_amount(base, move->effect_step);
        if (current >= cap) return 0;
        int updated = current + step;
        if (updated > cap) updated = cap;
        set_plant_stat(target, move->stat, updated);
        set_stat_status(target, stat_boost_status(move->stat),
                        updated / (float)base);
        *old_value = current;
        *new_value = updated;
        return 1;
    }

    int base = get_plant_base_stat(target, move->stat);
    int cap = rounded_stat_amount(base, move->effect_cap);
    int current_debuff = target->stat_debuff_amount[move->stat];
    int step = rounded_stat_amount(base, move->effect_step);
    if (current_debuff >= cap) return 0;
    if (step > cap - current_debuff) step = cap - current_debuff;
    int current_defense = target->defense;
    int updated_defense = (int)((double)current_defense * current_defense /
                                (current_defense + step));
    if (updated_defense < 1) updated_defense = 1;
    target->defense = updated_defense;
    target->stat_debuff_amount[move->stat] += step;
    set_stat_status(target, stat_debuff_status(move->stat),
                    updated_defense / (float)base);
    *old_value = current_defense;
    *new_value = updated_defense;
    return 1;
}

static void finish_action(BattleState *battle, int side, int acting_plant_index,
                          Team *acting_team, Team *target_team)
{
    battle->current_turn = 1 - side;
    apply_turn_statuses(battle, side, acting_plant_index);
    Battle_CheckConditions(battle);
    if (battle->state != BATTLE_STATE_ONGOING) return;

    Plant *acting_plant = acting_plant_index >= 0 && acting_plant_index < acting_team->count
        ? &acting_team->plants[acting_plant_index] : NULL;
    Plant *target_plant = Team_GetActivePlant(target_team);
    if ((!acting_plant || !Plant_IsAlive(acting_plant)) && has_reserve(acting_team)) {
        battle->current_turn = side;
        battle->state = BATTLE_STATE_REPLACEMENT;
    } else if ((!target_plant || !Plant_IsAlive(target_plant)) && has_reserve(target_team)) {
        battle->current_turn = 1 - side;
        battle->state = BATTLE_STATE_REPLACEMENT;
    }
}

static int calculate_damage(const Move *move, const Plant *attacker, const Plant *defender,
                            int *was_critical)
{
    const int chance = Battle_GetCriticalChance(attacker, move);
    const int critical = chance > 0 && rand() % 100 < chance;
    if (was_critical) *was_critical = critical;
    const double multipliers = critical ? 1.5 : 1.0;
    const double defense = defender->defense > 0 ? defender->defense : 1.0;
    const double base_damage = ((double)move->power * attacker->attack / defense / 2.0) + 2.0;
    int damage = (int)(base_damage * multipliers);
    return damage > 0 ? damage : 1;
}

int Battle_GetCriticalChance(const Plant *attacker, const Move *move)
{
    if (!move) return 0;
    int chance = move->critical_chance;
    float luck_multiplier = 1.0f;
    if (attacker && Plant_GetStatusValue(attacker, PLANT_STATUS_LUCK, &luck_multiplier)) {
        chance = (int)(chance * luck_multiplier);
    }
    if (chance < 0) chance = 0;
    if (chance > 100) chance = 100;
    return chance;
}

static int evaluate_condition(LogicCondition condition, const Team *own, const Team *enemy,
                              int own_plant_index)
{
    switch (condition) {
        case LOGIC_OWN_LOW_HP:
            if (own_plant_index < 0 || own_plant_index >= own->count) return 0;
            return Plant_IsAlive(&own->plants[own_plant_index]) &&
                   own->plants[own_plant_index].current_hp <= own->plants[own_plant_index].max_hp / 2;
        case LOGIC_ENEMY_STATUS: return has_damaging_status(active_plant(enemy));
        case LOGIC_ENEMY_LOW_HP: return has_low_hp(enemy);
        case LOGIC_ENEMY_FASTER: return enemy_is_faster(own, enemy, own_plant_index);
        case LOGIC_OWN_HIGHER_ATTACK:
            return own_stat_higher(own, enemy, own_plant_index, MOVE_STAT_ATTACK);
        case LOGIC_OWN_HIGHER_DEFENSE:
            return own_stat_higher(own, enemy, own_plant_index, MOVE_STAT_DEFENSE);
        case LOGIC_OWN_RESERVE: return has_reserve(own);
        default: return 0;
    }
}

LogicEvaluation Battle_EvaluatePlantFormula(const BattleState *battle, int side,
                                             int plant_index, LogicFormula formula)
{
    if (!battle || side < 0 || side > 1) return (LogicEvaluation){0};
    const Team *own = side == 0 ? &battle->player_team : &battle->enemy_team;
    const Team *enemy = side == 0 ? &battle->enemy_team : &battle->player_team;
    if (formula.left < 0 || formula.left >= LOGIC_CONDITION_COUNT ||
        formula.right < 0 || formula.right >= LOGIC_CONDITION_COUNT ||
        formula.left == formula.right || formula.operator < 0 ||
        formula.operator >= LOGIC_OPERATOR_COUNT || plant_index < 0 ||
        plant_index >= own->count || plant_index >= MAX_TEAM_SIZE)
        return (LogicEvaluation){0};

    LogicEvaluation value = {
        .left = evaluate_condition(formula.left, own, enemy, plant_index),
        .right = evaluate_condition(formula.right, own, enemy, plant_index)
    };
    switch (formula.operator) {
        case LOGIC_AND: value.result = value.left && value.right; break;
        case LOGIC_OR: value.result = value.left || value.right; break;
        case LOGIC_XOR: value.result = value.left != value.right; break;
        default: break;
    }
    return value;
}

LogicEvaluation Battle_EvaluateFormula(const BattleState *battle, int side, LogicFormula formula)
{
    if (!battle || side < 0 || side > 1) return (LogicEvaluation){0};
    const Team *own = side == 0 ? &battle->player_team : &battle->enemy_team;
    return Battle_EvaluatePlantFormula(battle, side, own->active_index, formula);
}

int Battle_GetFormulaPowerBonus(LogicOperator operator)
{
    static const int power_bonus[LOGIC_OPERATOR_COUNT] = {70, 45, 55};
    if (operator < 0 || operator >= LOGIC_OPERATOR_COUNT) return 0;
    return power_bonus[operator];
}

int Battle_GetPlantMovePower(const BattleState *battle, int side, int plant_index, int move_index)
{
    if (!battle || side < 0 || side > 1 || move_index < 0 || move_index >= MAX_MOVES) return 0;

    const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
    if (plant_index < 0 || plant_index >= team->count || plant_index >= MAX_TEAM_SIZE) return 0;
    const Move *move = &team->plants[plant_index].moves[move_index];
    if (move->effect != MOVE_EFFECT_DAMAGE) return 0;
    if (!logic_buff_enabled(side)) return move->power;
    LogicFormula formula = battle->formulas[side][plant_index];
    if (move_index != battle->boosted_move_indices[side][plant_index] ||
        !Battle_EvaluatePlantFormula(battle, side, plant_index, formula).result)
        return move->power;

    return move->power + Battle_GetFormulaPowerBonus(formula.operator);
}

static int can_use_move_strict(const BattleState *battle, int side, int plant_index, int move_index)
{
    if (!battle || battle->state != BATTLE_STATE_ONGOING || side < 0 || side > 1 ||
        move_index < 0 || move_index >= MAX_MOVES)
        return 0;

    const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
    if (plant_index < 0 || plant_index >= team->count || plant_index >= MAX_TEAM_SIZE ||
        plant_index != team->active_index)
        return 0;

    const Plant *plant = &team->plants[plant_index];
    if (plant->id == SPECIES_PEPPER && move_index == 2 && plant->fruit_count <= 0)
        return 0;

    const Move *move = &plant->moves[move_index];
    if (move->effect == MOVE_EFFECT_ATTRIBUTE) {
        if (move->stat < 0 || move->stat >= MOVE_STAT_COUNT ||
            move->target < MOVE_TARGET_SELF || move->target > MOVE_TARGET_OPPONENT)
            return 0;
        const Plant *target = plant;
        if (move->target == MOVE_TARGET_OPPONENT) {
            const Team *opponent = side == 0 ? &battle->enemy_team : &battle->player_team;
            if (opponent->active_index < 0 || opponent->active_index >= opponent->count) return 0;
            target = &opponent->plants[opponent->active_index];
        }
        int base = get_plant_base_stat(target, move->stat);
        int cap = rounded_stat_amount(base, move->effect_cap);
        if (move->stat_change_mode == MOVE_STAT_CHANGE_ADD_TO_CAP)
            return get_plant_stat(target, move->stat) < cap;
        if (move->stat_change_mode == MOVE_STAT_CHANGE_RATIO_DEBUFF)
            return target->stat_debuff_amount[move->stat] < cap;
        return 0;
    }
    if (move->effect != MOVE_EFFECT_DAMAGE) return 0;

    if (!logic_buff_enabled(side)) return 1;
    if (move_index != battle->boosted_move_indices[side][plant_index]) return 1;

    LogicFormula formula = battle->formulas[side][plant_index];
    return Battle_EvaluatePlantFormula(battle, side, plant_index, formula).result;
}

int Battle_CanUseMove(const BattleState *battle, int side, int plant_index, int move_index)
{
    if (can_use_move_strict(battle, side, plant_index, move_index)) return 1;
    if (!battle || battle->state != BATTLE_STATE_ONGOING || side != 0 ||
        plant_index < 0 || plant_index >= battle->player_team.count ||
        plant_index != battle->player_team.active_index ||
        move_index < 0 || move_index >= MAX_MOVES ||
        move_index != battle->boosted_move_indices[side][plant_index]) return 0;
    const Plant *plant = &battle->player_team.plants[plant_index];
    if (!Plant_IsAlive(plant) || plant->moves[move_index].effect != MOVE_EFFECT_DAMAGE ||
        (plant->id == SPECIES_PEPPER && move_index == 2 && plant->fruit_count <= 0) ||
        has_reserve(&battle->player_team)) return 0;
    /* Sem golpe nem reserva disponível, libera o golpe lógico pelo poder base. */
    for (int i = 0; i < MAX_MOVES; i++)
        if (can_use_move_strict(battle, side, plant_index, i)) return 0;
    return 1;
}

int Battle_GetMovePower(const BattleState *battle, int side, int move_index)
{
    if (!battle || side < 0 || side > 1) return 0;
    const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
    return Battle_GetPlantMovePower(battle, side, team->active_index, move_index);
}

int Battle_ApplyStatus(BattleState *battle, int side, int plant_index,
                       PlantStatusType type, int remaining_turns, float value)
{
    return Battle_ApplyStatusFrom(battle, -1, -1, side, plant_index,
                                  type, remaining_turns, value);
}

int Battle_ApplyStatusFrom(BattleState *battle, int source_side, int source_plant_index,
                           int target_side, int target_plant_index,
                           PlantStatusType type, int remaining_turns, float value)
{
    if (!battle || battle->state != BATTLE_STATE_ONGOING ||
        target_side < 0 || target_side > 1)
        return 0;
    Team *target_team = team_for_side(battle, target_side);
    if (target_plant_index < 0 || target_plant_index >= target_team->count ||
        !Plant_IsAlive(&target_team->plants[target_plant_index]))
        return 0;
    if (source_side >= 0) {
        if (source_side > 1 || source_plant_index < 0) return 0;
        Team *source_team = team_for_side(battle, source_side);
        if (source_plant_index >= source_team->count ||
            !Plant_IsAlive(&source_team->plants[source_plant_index]))
            return 0;
    } else if (source_plant_index >= 0) {
        return 0;
    }
    return Plant_AddStatusFrom(&target_team->plants[target_plant_index], type,
                               remaining_turns, value, source_side, source_plant_index);
}

int Battle_SkipParalyzedTurn(BattleState *battle)
{
    if (!battle || battle->state != BATTLE_STATE_ONGOING ||
        battle->current_turn < 0 || battle->current_turn > 1)
        return 0;

    int side = battle->current_turn;
    Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
    Plant *active = Team_GetActivePlant(team);
    if (!active || !Plant_IsAlive(active) ||
        !Plant_HasStatus(active, PLANT_STATUS_PARALYZED))
        return 0;

    int active_index = team->active_index;
    Plant_RemoveStatus(active, PLANT_STATUS_PARALYZED);
    snprintf(battle->last_message, sizeof(battle->last_message),
             "%s está paralisada e perdeu a ação!", active->name_pt);
    apply_turn_statuses(battle, side, active_index);
    Battle_CheckConditions(battle);

    if (battle->state == BATTLE_STATE_ONGOING && !Plant_IsAlive(active) &&
        has_reserve(team)) {
        battle->current_turn = side;
        battle->state = BATTLE_STATE_REPLACEMENT;
    } else if (battle->state == BATTLE_STATE_ONGOING) {
        battle->current_turn = 1 - side;
    }
    return 1;
}

void Battle_Init(BattleState *battle, Team player_team, Team enemy_team)
{
    *battle = (BattleState){
        .player_team = player_team,
        .enemy_team = enemy_team,
        .last_message = "A batalha começou!"
    };
    for (int i = 0; i < MAX_TEAM_SIZE; i++) {
        battle->formulas[0][i] =
            (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_STATUS, LOGIC_AND};
        battle->boosted_move_indices[0][i] = -1;
        battle->formulas[1][i] = (LogicFormula){0};
        battle->boosted_move_indices[1][i] = -1;
    }
    for (int side = 0; side < 2; side++) {
        Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
        for (int i = 0; i < team->count; i++) {
            if (team->plants[i].id == SPECIES_CLOVER &&
                !Plant_HasStatus(&team->plants[i], PLANT_STATUS_LUCK)) {
                Plant_AddStatus(&team->plants[i], PLANT_STATUS_LUCK, 0,
                                PLANT_STATUS_LUCK_CRITICAL_MULTIPLIER);
            }
            restore_temporary_stats(&team->plants[i]);
            team->plants[i].ability_turn_count = 0;
            team->plants[i].fruit_count = 0;
        }
    }
    const Plant *player = Team_GetActivePlant(&battle->player_team);
    const Plant *enemy = Team_GetActivePlant(&battle->enemy_team);
    if (player && enemy) {
        if (enemy->speed > player->speed) {
            battle->current_turn = 1;
            snprintf(battle->last_message, sizeof(battle->last_message),
                     "%s começou: maior velocidade!", enemy->name_pt);
        } else {
            snprintf(battle->last_message, sizeof(battle->last_message),
                     "%s começou: maior velocidade!", player->name_pt);
        }
    }
    Battle_CheckConditions(battle);
}

int Battle_ExecuteMove(BattleState *battle, int move_index)
{
    if (battle->state != BATTLE_STATE_ONGOING || move_index < 0 || move_index >= MAX_MOVES)
        return 0;

    Team *turn_team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    Plant *turn_plant = Team_GetActivePlant(turn_team);
    if (turn_plant && Plant_HasStatus(turn_plant, PLANT_STATUS_PARALYZED))
        return Battle_SkipParalyzedTurn(battle);

    if (!Battle_CanUseMove(battle, battle->current_turn,
                           battle->current_turn == 0 ? battle->player_team.active_index
                                                     : battle->enemy_team.active_index,
                           move_index))
        return 0;

    Team *attacker_team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    Team *defender_team = battle->current_turn == 0 ? &battle->enemy_team : &battle->player_team;
    Plant *attacker = Team_GetActivePlant(attacker_team);
    Plant *defender = Team_GetActivePlant(defender_team);

    if (!attacker || !defender || !Plant_IsAlive(attacker) || !Plant_IsAlive(defender))
        return 0;

    const Move *move = &attacker->moves[move_index];
    const int acting_side = battle->current_turn;
    const int acting_plant_index = attacker_team->active_index;
    BattleMoveFeedback feedback = {
        .sequence = battle->feedback.sequence + 1, .side = acting_side,
        .plant_index = acting_plant_index, .move_index = move_index,
        .target_side = move->effect == MOVE_EFFECT_ATTRIBUTE && move->target == MOVE_TARGET_SELF
            ? acting_side : 1-acting_side
    };
    Move effective_move = *move;
    effective_move.power = Battle_GetMovePower(battle, battle->current_turn, move_index);
    if (move->effect != MOVE_EFFECT_DAMAGE) {
        int old_value = 0;
        int new_value = 0;
        if (!apply_stat_move(attacker, defender, move, &old_value, &new_value)) return 0;
        feedback.hit = 1;
        battle->feedback = feedback;
        const char *stat_name = move->stat == MOVE_STAT_ATTACK ? "ataque"
                              : move->stat == MOVE_STAT_DEFENSE ? "defesa" : "velocidade";
        Plant *target = move->target == MOVE_TARGET_SELF ? attacker : defender;
        const char *target_text = move->target == MOVE_TARGET_SELF ? "próprio" : "rival";
        snprintf(battle->last_message, sizeof(battle->last_message),
                 "%s usou %s: %s %s de %s %d → %d.", attacker->name_pt,
                 move->name, stat_name, target_text, target->name_pt, old_value, new_value);
        finish_action(battle, acting_side, acting_plant_index, attacker_team, defender_team);
        return 1;
    }
    feedback.boosted = effective_move.power > move->power;
    if (attacker->id == SPECIES_PEPPER && move_index == 2) attacker->fruit_count--;
    if (move->accuracy < 100 && rand() % 100 >= move->accuracy) {
        battle->feedback = feedback;
        snprintf(battle->last_message, sizeof(battle->last_message),
                 "%s errou %s!", attacker->name_pt, move->name);
        finish_action(battle, acting_side, acting_plant_index, attacker_team, defender_team);
        return 1;
    }

    int critical = 0;
    int raw_damage = calculate_damage(&effective_move, attacker, defender, &critical);

    int damage = damage_plant(battle, 1 - acting_side, defender, raw_damage);
    feedback.hit = 1;
    feedback.critical = critical;
    feedback.damage = damage;
    battle->feedback = feedback;
    int knocked_out = !Plant_IsAlive(defender);
    if (effective_move.power > move->power) {
        snprintf(battle->last_message, sizeof(battle->last_message),
                 "%s usou %s fortalecido (poder %d)%s: %d de dano.%s",
                 attacker->name_pt, move->name, effective_move.power,
                 critical ? " CRÍTICO" : "", damage,
                 knocked_out ? " Planta derrotada!" : "");
    } else {
        snprintf(battle->last_message, sizeof(battle->last_message),
                 "%s usou %s (poder %d)%s: %d de dano.%s",
                 attacker->name_pt, move->name, effective_move.power,
                 critical ? " CRÍTICO" : "", damage,
                 knocked_out ? " Planta derrotada!" : "");
    }

    if (damage > 0 && defender->id == SPECIES_ALOE && Plant_IsAlive(defender)) {
        int healed = 0;
        int healing = damage / 8;
        if (healing > 0) {
            int previous_hp = defender->current_hp;
            defender->current_hp += healing;
            if (defender->current_hp > defender->max_hp)
                defender->current_hp = defender->max_hp;
            healed = defender->current_hp - previous_hp;
        }
        if (healed > 0) {
            char message[96];
            snprintf(message, sizeof(message), "Babosa recuperou %d de vida.", healed);
            append_status_message(battle, message);
        }
    }
    if (damage > 0 && defender->id == SPECIES_POISON_IVY &&
        move->category == MOVE_CATEGORY_PHYSICAL) {
        if (Plant_HasStatus(attacker, PLANT_STATUS_POISON)) {
            append_status_message(battle, "O atacante já está envenenado.");
        } else if (rand() % 100 < 30 &&
                   Battle_ApplyStatus(battle, acting_side, acting_plant_index,
                                      PLANT_STATUS_POISON, 0, 1.0f)) {
            append_status_message(battle, "Hera Venenosa envenenou o atacante.");
        } else {
            append_status_message(battle, "Hera Venenosa não envenenou o atacante desta vez.");
        }
    }
    if (damage > 0 && defender->id == SPECIES_CACTUS &&
        move->category == MOVE_CATEGORY_PHYSICAL) {
        int reflected = (damage * 3 + 9) / 10;
        int reflected_damage = damage_plant(battle, acting_side, attacker, reflected);
        char message[96];
        snprintf(message, sizeof(message), "Cacto devolveu %d de dano.", reflected_damage);
        append_status_message(battle, message);
    }
    if (damage > 0 && attacker->id == SPECIES_ORCHID && Plant_IsAlive(defender)) {
        if (Plant_HasStatus(defender, PLANT_STATUS_HOST)) {
            append_status_message(battle, "O adversário já está com Hospedeiro.");
        } else if (rand() % 100 < 40 &&
                   Battle_ApplyStatusFrom(battle, acting_side, acting_plant_index,
                                          1 - acting_side, defender_team->active_index,
                                          PLANT_STATUS_HOST, 0, 1.0f)) {
            append_status_message(battle, "Orquídea aplicou Hospedeiro.");
        } else {
            append_status_message(battle, "Orquídea não aplicou Hospedeiro desta vez.");
        }
    }
    if (damage > 0 && attacker->id == SPECIES_CARNIVOROUS_PLANT &&
        Plant_IsAlive(defender)) {
        if (Plant_HasStatus(defender, PLANT_STATUS_PARALYZED)) {
            append_status_message(battle, "O adversário já está paralisado.");
        } else if (rand() % 100 < CARNIVOROUS_PARALYSIS_CHANCE &&
                   Battle_ApplyStatus(battle, 1 - acting_side,
                                      defender_team->active_index,
                                      PLANT_STATUS_PARALYZED, 1, 1.0f)) {
            append_status_message(battle, "Planta Carnívora paralisou o adversário.");
        } else {
            append_status_message(battle, "Planta Carnívora não paralisou desta vez.");
        }
    }
    if (damage > 0 && attacker->id == SPECIES_PEPPER && move_index == 2 &&
        Plant_IsAlive(defender) && rand() % 100 < 30) {
        if (Battle_ApplyStatus(battle, 1 - acting_side,
                               defender_team->active_index, PLANT_STATUS_BURN, 0, 1.0f))
            append_status_message(battle, "O fruto incendiou o adversário.");
    }
    finish_action(battle, acting_side, acting_plant_index, attacker_team, defender_team);

    return 1;
}

int Battle_CanSwitch(const BattleState *battle, int plant_index)
{
    if (!battle) return 0;
    return can_switch_side(battle, battle->current_turn, plant_index);
}

int Battle_CanSideSwitch(const BattleState *battle, int side, int plant_index)
{
    return can_switch_side(battle, side, plant_index);
}

int Battle_SwitchPlant(BattleState *battle, int plant_index)
{
    if (!Battle_CanSwitch(battle, plant_index)) return 0;

    Team *team = battle->current_turn == 0 ? &battle->player_team : &battle->enemy_team;
    if (battle->state == BATTLE_STATE_ONGOING) {
        Plant *active = Team_GetActivePlant(team);
        if (active && Plant_HasStatus(active, PLANT_STATUS_PARALYZED))
            return Battle_SkipParalyzedTurn(battle);
    }
    int acting_side = battle->current_turn;
    int outgoing_index = team->active_index;
    if (outgoing_index >= 0 && outgoing_index < team->count)
        restore_temporary_stats(&team->plants[outgoing_index]);
    team->active_index = plant_index;
    snprintf(battle->last_message, sizeof(battle->last_message),
             "%s entrou em campo!", team->plants[plant_index].name_pt);

    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        battle->state = BATTLE_STATE_ONGOING;
        Battle_CheckConditions(battle);
        if (battle->state == BATTLE_STATE_ONGOING) {
            Team *opponent = acting_side == 0 ? &battle->enemy_team : &battle->player_team;
            Plant *opponent_active = Team_GetActivePlant(opponent);
            if ((!opponent_active || !Plant_IsAlive(opponent_active)) && has_reserve(opponent)) {
                battle->current_turn = 1 - acting_side;
                battle->state = BATTLE_STATE_REPLACEMENT;
            }
        }
    } else {
        apply_turn_statuses(battle, acting_side, outgoing_index);
        Battle_CheckConditions(battle);
        if (battle->state == BATTLE_STATE_ONGOING) battle->current_turn = 1 - acting_side;
    }
    return 1;
}

/* Hospedeiro só termina quando a planta que o aplicou é derrotada. */
static void clear_host_from_fallen_source(BattleState *battle)
{
    for (int target_side = 0; target_side < 2; target_side++) {
        Team *target_team = team_for_side(battle, target_side);
        for (int index = 0; index < target_team->count; index++) {
            Plant *target = &target_team->plants[index];
            int source_side = -1, source_index = -1;
            if (!Plant_GetStatusSource(target, PLANT_STATUS_HOST,
                                       &source_side, &source_index))
                continue;
            if (source_side < 0 || source_index < 0) continue;
            Team *source_team = team_for_side(battle, source_side);
            if (source_index >= source_team->count ||
                !Plant_IsAlive(&source_team->plants[source_index]))
                Plant_RemoveStatus(target, PLANT_STATUS_HOST);
        }
    }
}

void Battle_CheckConditions(BattleState *battle)
{
    clear_host_from_fallen_source(battle);
    if (!Team_HasLivingPlants(&battle->enemy_team)) {
        battle->state = BATTLE_STATE_PLAYER_WIN;
    } else if (!Team_HasLivingPlants(&battle->player_team)) {
        battle->state = BATTLE_STATE_ENEMY_WIN;
    }
}

/* As duas ações são escolhidas antes da resolução; a mais rápida resolve primeiro. */
static int validate_round_action(const BattleState *battle, int side, BattleAction action)
{
    if (action.type == BATTLE_ACTION_NONE) return 1;
    if (!battle || (side != 0 && side != 1)) return 0;
    const Team *team = const_team_for_side(battle, side);
    if (action.type == BATTLE_ACTION_MOVE)
        return Battle_CanUseMove(battle, side, team->active_index, action.index);
    if (action.type == BATTLE_ACTION_SWITCH)
        return can_switch_side(battle, side, action.index);
    return 0;
}

static void skip_round_side(BattleState *battle, int side)
{
    Team *team = team_for_side(battle, side);
    Plant *active = Team_GetActivePlant(team);
    if (!active || !Plant_IsAlive(active)) return;
    int index = team->active_index;
    if (Plant_HasStatus(active, PLANT_STATUS_PARALYZED)) {
        battle->current_turn = side;
        Battle_SkipParalyzedTurn(battle);
        return;
    }
    apply_turn_statuses(battle, side, index);
    Battle_CheckConditions(battle);
}

static int execute_round_action(BattleState *battle, int side, BattleAction action)
{
    if (action.type == BATTLE_ACTION_NONE) {
        skip_round_side(battle, side);
        return 1;
    }
    battle->current_turn = side;
    if (action.type == BATTLE_ACTION_MOVE) return Battle_ExecuteMove(battle, action.index);
    if (action.type == BATTLE_ACTION_SWITCH) return Battle_SwitchPlant(battle, action.index);
    return 0;
}

static void finish_round(BattleState *battle, int first_side,
                         int player_index, int enemy_index)
{
    Battle_CheckConditions(battle);
    if (battle->state != BATTLE_STATE_ONGOING && battle->state != BATTLE_STATE_REPLACEMENT) {
        battle->current_turn = 0;
        return;
    }
    battle->state = BATTLE_STATE_ONGOING;
    for (int order = 0; order < 2; order++) {
        int side = order == 0 ? first_side : 1 - first_side;
        int index = side == 0 ? player_index : enemy_index;
        Team *team = team_for_side(battle, side);
        if (index >= 0 && index < team->count &&
            !Plant_IsAlive(&team->plants[index]) && has_reserve(team)) {
            battle->current_turn = side;
            battle->state = BATTLE_STATE_REPLACEMENT;
            return;
        }
    }
    battle->current_turn = 0;
}

/* A ordem e os índices são registrados antes de qualquer dano ser aplicado. */
int Battle_PrepareRound(BattleState *battle, BattleAction player_action,
                        BattleAction enemy_action, BattleRound *round)
{
    if (!battle || !round || battle->state != BATTLE_STATE_ONGOING) return 0;
    if (!validate_round_action(battle, 0, player_action) ||
        !validate_round_action(battle, 1, enemy_action)) return 0;

    Team *player_team = &battle->player_team;
    Team *enemy_team = &battle->enemy_team;
    const Plant *player = Team_GetActivePlant(player_team);
    const Plant *enemy = Team_GetActivePlant(enemy_team);
    if (!player || !enemy || !Plant_IsAlive(player) || !Plant_IsAlive(enemy)) return 0;

    round->actions[0] = player_action;
    round->actions[1] = enemy_action;
    round->acted_indices[0] = player_team->active_index;
    round->acted_indices[1] = enemy_team->active_index;
    round->sides[0] = enemy->speed > player->speed ? 1 : 0;
    round->sides[1] = 1 - round->sides[0];
    round->step = 0;
    return 1;
}

/* Ações canceladas por nocaute não são executadas, mas encerram a rodada. */
int Battle_ExecuteRoundStep(BattleState *battle, BattleRound *round)
{
    if (!battle || !round || round->step < 0 || round->step >= 2) return 0;
    if (battle->state != BATTLE_STATE_ONGOING && battle->state != BATTLE_STATE_REPLACEMENT) {
        round->step = 2;
        return 2;
    }

    int side = round->sides[round->step];
    if (round->step > 0) {
        if (battle->state == BATTLE_STATE_REPLACEMENT) battle->state = BATTLE_STATE_ONGOING;
        if (battle->state != BATTLE_STATE_ONGOING) {
            round->step = 2;
            return 2;
        }
        Team *team = team_for_side(battle, side);
        Plant *active = Team_GetActivePlant(team);
        if (!active || !Plant_IsAlive(active) ||
            team->active_index != round->acted_indices[side]) {
            round->step = 2;
            return 2;
        }
    }

    if (!execute_round_action(battle, side, round->actions[side])) {
        round->step = 2;
        return 0;
    }
    round->step++;
    return 1;
}

int Battle_FinishRound(BattleState *battle, BattleRound *round)
{
    if (!battle || !round) return 0;
    finish_round(battle, round->sides[0], round->acted_indices[0], round->acted_indices[1]);
    return 1;
}

int Battle_ExecuteRound(BattleState *battle, BattleAction player_action, BattleAction enemy_action)
{
    BattleRound round;
    if (!Battle_PrepareRound(battle, player_action, enemy_action, &round)) return 0;
    for (int step = 0; step < 2; step++)
        if (Battle_ExecuteRoundStep(battle, &round) == 0) return 0;
    Battle_FinishRound(battle, &round);
    return 1;
}
