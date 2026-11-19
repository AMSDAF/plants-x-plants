/* Define os estados e as ações do combate, sem dependência da interface. */
#ifndef BATTLE_H
#define BATTLE_H

#include "plant.h"

typedef enum {
    BATTLE_STATE_ONGOING = 0,
    BATTLE_STATE_PLAYER_WIN,
    BATTLE_STATE_ENEMY_WIN,
    BATTLE_STATE_REPLACEMENT
} BattleStateEnum;

typedef enum {
    LOGIC_OWN_LOW_HP,
    LOGIC_ENEMY_RESERVE,
    LOGIC_ENEMY_LOW_HP,
    LOGIC_OWN_RESERVE,
    LOGIC_CONDITION_COUNT
} LogicCondition;

typedef enum { LOGIC_AND, LOGIC_OR, LOGIC_XOR, LOGIC_OPERATOR_COUNT } LogicOperator;

typedef struct {
    LogicCondition left;
    LogicCondition right;
    LogicOperator operator;
} LogicFormula;

typedef struct {
    int left;
    int right;
    int result;
} LogicEvaluation;

typedef struct {
    Team player_team;
    Team enemy_team;
    int current_turn; /* 0 para o primeiro lado, 1 para o segundo. */
    BattleStateEnum state;
    LogicFormula formulas[2];
    int boosted_move_indices[2];
    char last_message[128];
} BattleState;

void Battle_Init(BattleState *battle, Team player_team, Team enemy_team);
LogicEvaluation Battle_EvaluateFormula(const BattleState *battle, int side, LogicFormula formula);
int Battle_GetFormulaPowerBonus(LogicOperator operator);
int Battle_GetMovePower(const BattleState *battle, int side, int move_index);
int Battle_ExecuteMove(BattleState *battle, int move_index);
int Battle_CanSwitch(const BattleState *battle, int plant_index);
int Battle_SwitchPlant(BattleState *battle, int plant_index);
void Battle_CheckConditions(BattleState *battle);

#endif
