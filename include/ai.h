/* Declara a execução de uma ação do adversário pela API de combate. */
#ifndef AI_H
#define AI_H

#include "battle.h"

typedef enum {
    AI_ACTION_NONE,
    AI_ACTION_MOVE,
    AI_ACTION_SWITCH
} AIActionType;

AIActionType AI_GetPlannedAction(const BattleState *battle, int *action_index);
int AI_PlayTurn(BattleState *battle);

#endif
