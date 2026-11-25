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
    LOGIC_ENEMY_STATUS,
    LOGIC_ENEMY_LOW_HP,
    LOGIC_ENEMY_FASTER,
    LOGIC_OWN_HIGHER_ATTACK,
    LOGIC_OWN_HIGHER_DEFENSE,
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

typedef enum {
    BATTLE_ACTION_NONE,
    BATTLE_ACTION_MOVE,
    BATTLE_ACTION_SWITCH
} BattleActionType;

typedef struct {
    BattleActionType type;
    int index;
} BattleAction;

/* Plano da rodada: a ordem é fixada antes de qualquer ação ser executada. */
typedef struct {
    BattleAction actions[2];
    int sides[2];
    int acted_indices[2];
    int step;
} BattleRound;

/* Resultado descritivo do último golpe válido; interface e som apenas o observam. */
typedef struct {
    unsigned int sequence;
    int side, plant_index, move_index, target_side;
    int hit, critical, boosted, damage;
} BattleMoveFeedback;

typedef struct {
    Team player_team;
    Team enemy_team;
    int current_turn; /* 0 para o primeiro lado, 1 para o segundo. */
    BattleStateEnum state;
    LogicFormula formulas[2][MAX_TEAM_SIZE]; /* Índice 1 permanece sem buff lógico. */
    int boosted_move_indices[2][MAX_TEAM_SIZE]; /* -1 mantém todos os golpes normais. */
    char last_message[128];
    BattleMoveFeedback feedback;
    int damage_taken; /* Dano real recebido pelo lado 0 neste combate. */
} BattleState;

void Battle_Init(BattleState *battle, Team player_team, Team enemy_team);
LogicEvaluation Battle_EvaluateFormula(const BattleState *battle, int side, LogicFormula formula);
LogicEvaluation Battle_EvaluatePlantFormula(const BattleState *battle, int side,
                                             int plant_index, LogicFormula formula);
int Battle_GetFormulaPowerBonus(LogicOperator operator);
int Battle_GetMovePower(const BattleState *battle, int side, int move_index);
int Battle_GetPlantMovePower(const BattleState *battle, int side, int plant_index, int move_index);
int Battle_GetCriticalChance(const Plant *attacker, const Move *move);
int Battle_CanUseMove(const BattleState *battle, int side, int plant_index, int move_index);
int Battle_ApplyStatus(BattleState *battle, int side, int plant_index,
                       PlantStatusType type, int remaining_turns, float value);
/* Aplica um efeito registrando qual planta o originou, como exige Hospedeiro. */
int Battle_ApplyStatusFrom(BattleState *battle, int source_side, int source_plant_index,
                           int target_side, int target_plant_index,
                           PlantStatusType type, int remaining_turns, float value);
int Battle_SkipParalyzedTurn(BattleState *battle);
int Battle_ExecuteMove(BattleState *battle, int move_index);
int Battle_CanSwitch(const BattleState *battle, int plant_index);
int Battle_CanSideSwitch(const BattleState *battle, int side, int plant_index);
int Battle_SwitchPlant(BattleState *battle, int plant_index);
/* Prepara a rodada sem alterar o estado; retorna 0 se alguma ação for inválida. */
int Battle_PrepareRound(BattleState *battle, BattleAction player_action,
                        BattleAction enemy_action, BattleRound *round);
/* Executa a próxima etapa: 1 executou, 2 foi cancelada, 0 é inválida. */
int Battle_ExecuteRoundStep(BattleState *battle, BattleRound *round);
int Battle_FinishRound(BattleState *battle, BattleRound *round);
int Battle_ExecuteRound(BattleState *battle, BattleAction player_action, BattleAction enemy_action);
void Battle_CheckConditions(BattleState *battle);

#endif
