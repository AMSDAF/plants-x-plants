/* Escolhe um golpe ou uma substituição e usa as mesmas regras dos jogadores. */
#include "ai.h"

#include <stddef.h>

AIActionType AI_GetPlannedAction(const BattleState *battle, int *action_index)
{
    if (action_index) *action_index = -1;
    if (battle->current_turn != 1) return AI_ACTION_NONE;

    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        for (int i = 0; i < battle->enemy_team.count; i++) {
            if (Battle_CanSwitch(battle, i)) {
                if (action_index) *action_index = i;
                return AI_ACTION_SWITCH;
            }
        }
        return AI_ACTION_NONE;
    }

    if (battle->state != BATTLE_STATE_ONGOING) return AI_ACTION_NONE;

    const Team *enemy_team = &battle->enemy_team;
    const Plant *active = enemy_team->active_index >= 0 &&
                          enemy_team->active_index < enemy_team->count
        ? &enemy_team->plants[enemy_team->active_index] : NULL;
    if (!active) return AI_ACTION_NONE;

    const Team *player_team = &battle->player_team;
    const Plant *opponent = player_team->active_index >= 0 &&
                            player_team->active_index < player_team->count
        ? &player_team->plants[player_team->active_index] : NULL;
    if (opponent && active->current_hp * 10 > active->max_hp * 7 &&
        opponent->current_hp * 10 > opponent->max_hp * 3) {
        for (int i = 0; i < MAX_MOVES; i++) {
            if (active->moves[i].effect != MOVE_EFFECT_DAMAGE &&
                Battle_CanUseMove(battle, 1, battle->enemy_team.active_index, i)) {
                if (action_index) *action_index = i;
                return AI_ACTION_MOVE;
            }
        }
    }

    int best_move = -1;
    for (int i = 0; i < MAX_MOVES; i++) {
        if (!Battle_CanUseMove(battle, 1, battle->enemy_team.active_index, i)) continue;
        if (best_move < 0) {
            best_move = i;
            continue;
        }
        const Move *candidate = &active->moves[i];
        const Move *best = &active->moves[best_move];
        int candidate_score = Battle_GetMovePower(battle, 1, i) * candidate->accuracy +
            candidate->critical_chance * 4 + candidate->priority * 500;
        int best_score = Battle_GetMovePower(battle, 1, best_move) * best->accuracy +
            best->critical_chance * 4 + best->priority * 500;
        if (candidate_score > best_score) best_move = i;
    }
    if (best_move >= 0) {
        if (action_index) *action_index = best_move;
        return AI_ACTION_MOVE;
    }

    for (int i = 0; i < battle->enemy_team.count; i++) {
        if (Battle_CanSwitch(battle, i) &&
            Battle_EvaluatePlantFormula(battle, 1, i,
                battle->formulas[1][i]).result)
        {
            if (action_index) *action_index = i;
            return AI_ACTION_SWITCH;
        }
    }
    for (int i = 0; i < battle->enemy_team.count; i++) {
        if (Battle_CanSwitch(battle, i)) {
            if (action_index) *action_index = i;
            return AI_ACTION_SWITCH;
        }
    }
    return AI_ACTION_NONE;
}

int AI_PlayTurn(BattleState *battle)
{
    int action_index = -1;
    AIActionType action = AI_GetPlannedAction(battle, &action_index);
    if (action == AI_ACTION_MOVE) return Battle_ExecuteMove(battle, action_index);
    if (action == AI_ACTION_SWITCH) return Battle_SwitchPlant(battle, action_index);
    return 0;
}
