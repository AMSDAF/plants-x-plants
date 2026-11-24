/* Escolhe um golpe ou uma substituição e usa as mesmas regras dos jogadores. */
#include "ai.h"

#include <stddef.h>

AIActionType AI_GetActionForSide(const BattleState *battle, int side, int *action_index)
{
    if (action_index) *action_index = -1;
    if (!battle || (side != 0 && side != 1)) return AI_ACTION_NONE;

    const Team *own_team = side == 0 ? &battle->player_team : &battle->enemy_team;
    const Team *opponent_team = side == 0 ? &battle->enemy_team : &battle->player_team;

    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        if (battle->current_turn != side) return AI_ACTION_NONE;
        for (int i = 0; i < own_team->count; i++) {
            if (Battle_CanSideSwitch(battle, side, i)) {
                if (action_index) *action_index = i;
                return AI_ACTION_SWITCH;
            }
        }
        return AI_ACTION_NONE;
    }

    if (battle->state != BATTLE_STATE_ONGOING) return AI_ACTION_NONE;

    const Plant *active = own_team->active_index >= 0 &&
                          own_team->active_index < own_team->count
        ? &own_team->plants[own_team->active_index] : NULL;
    if (!active) return AI_ACTION_NONE;

    const Plant *opponent = opponent_team->active_index >= 0 &&
                            opponent_team->active_index < opponent_team->count
        ? &opponent_team->plants[opponent_team->active_index] : NULL;
    if (opponent && active->current_hp * 10 > active->max_hp * 7 &&
        opponent->current_hp * 10 > opponent->max_hp * 3) {
        for (int i = 0; i < MAX_MOVES; i++) {
            const Move *move = &active->moves[i];
            /* Um preparo basta; acumular vários buffs cede ataques demais ao rival. */
            if (move->effect == MOVE_EFFECT_ATTRIBUTE) {
                if (move->target == MOVE_TARGET_SELF &&
                    ((move->stat == MOVE_STAT_ATTACK && active->attack > active->base_attack) ||
                     (move->stat == MOVE_STAT_DEFENSE && active->defense > active->base_defense)))
                    continue;
                if (move->target == MOVE_TARGET_OPPONENT && move->stat == MOVE_STAT_DEFENSE &&
                    opponent->defense < opponent->base_defense) continue;
            }
            /* Velocidade só ajuda se a planta ainda não age antes do rival. */
            if (active->moves[i].effect == MOVE_EFFECT_ATTRIBUTE &&
                active->moves[i].stat == MOVE_STAT_SPEED &&
                active->speed > opponent->speed) continue;
            if (active->moves[i].effect != MOVE_EFFECT_DAMAGE &&
                Battle_CanUseMove(battle, side, own_team->active_index, i)) {
                if (action_index) *action_index = i;
                return AI_ACTION_MOVE;
            }
        }
    }

    int best_move = -1;
    for (int i = 0; i < MAX_MOVES; i++) {
        if (!Battle_CanUseMove(battle, side, own_team->active_index, i)) continue;
        if (best_move < 0) {
            best_move = i;
            continue;
        }
        const Move *candidate = &active->moves[i];
        const Move *best = &active->moves[best_move];
        int candidate_score = Battle_GetMovePower(battle, side, i) * candidate->accuracy +
            candidate->critical_chance * 4 + candidate->priority * 500;
        int best_score = Battle_GetMovePower(battle, side, best_move) * best->accuracy +
            best->critical_chance * 4 + best->priority * 500;
        if (candidate_score > best_score) best_move = i;
    }
    if (best_move >= 0) {
        if (action_index) *action_index = best_move;
        return AI_ACTION_MOVE;
    }

    for (int i = 0; i < own_team->count; i++) {
        if (Battle_CanSideSwitch(battle, side, i)) {
            if (action_index) *action_index = i;
            return AI_ACTION_SWITCH;
        }
    }
    return AI_ACTION_NONE;
}

AIActionType AI_GetPlannedAction(const BattleState *battle, int *action_index)
{
    return AI_GetActionForSide(battle, 1, action_index);
}

int AI_PlayTurn(BattleState *battle)
{
    int action_index = -1;
    AIActionType action = AI_GetPlannedAction(battle, &action_index);
    if (action == AI_ACTION_MOVE) return Battle_ExecuteMove(battle, action_index);
    if (action == AI_ACTION_SWITCH) return Battle_SwitchPlant(battle, action_index);
    return 0;
}
