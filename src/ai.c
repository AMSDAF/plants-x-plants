/* Escolhe um golpe ou uma substituição e usa as mesmas regras dos jogadores. */
#include "ai.h"

int AI_PlayTurn(BattleState *battle)
{
    if (battle->current_turn != 1) return 0;

    if (battle->state == BATTLE_STATE_REPLACEMENT) {
        for (int i = 0; i < battle->enemy_team.count; i++) {
            if (Battle_SwitchPlant(battle, i)) return 1;
        }
        return 0;
    }

    if (battle->state != BATTLE_STATE_ONGOING) return 0;

    const Plant *active = Team_GetActivePlant(&battle->enemy_team);
    if (!active) return 0;

    int best_move = 0;
    for (int i = 1; i < MAX_MOVES; i++) {
        const Move *candidate = &active->moves[i];
        const Move *best = &active->moves[best_move];
        if (candidate->power * candidate->accuracy > best->power * best->accuracy)
            best_move = i;
    }
    const Move *logic = Battle_GetAvailableLogicMove(battle);
    const Move *best = &active->moves[best_move];
    if (logic && logic->power * logic->accuracy > best->power * best->accuracy)
        best_move = BATTLE_LOGIC_MOVE_INDEX;
    return Battle_ExecuteMove(battle, best_move);
}
