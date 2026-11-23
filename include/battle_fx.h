/* Anima efeitos de apresentação sem alterar HP, turnos ou sorteios do combate. */
#ifndef BATTLE_FX_H
#define BATTLE_FX_H
#include "battle.h"
#include "raylib.h"
typedef enum {
    BATTLE_FX_LEAF, BATTLE_FX_THORN, BATTLE_FX_BITE, BATTLE_FX_WHIP,
    BATTLE_FX_FIRE, BATTLE_FX_SOLAR, BATTLE_FX_PETAL, BATTLE_FX_SPARK,
    BATTLE_FX_BODY, BATTLE_FX_BUFF, BATTLE_FX_DEBUFF
} BattleFxKind;
#define BATTLE_FX_IMPACT_TIME 0.38f
#define BATTLE_FX_DURATION 0.95f
typedef struct {
    bool active;
    float elapsed;
    BattleMoveFeedback move;
    BattleFxKind kind;
    MoveCategory category;
    int hp_before[2][MAX_TEAM_SIZE], hp_after[2][MAX_TEAM_SIZE];
} BattleFx;
BattleFxKind battle_fx_kind(int species, int move_index, const Move *move);
void battle_fx_begin(BattleFx *fx, const BattleState *before, const BattleState *after);
void battle_fx_update(BattleFx *fx, float dt);
float battle_fx_hp(const BattleFx *fx, int side, int index, int actual);
Rectangle battle_fx_plant_bounds(const BattleFx *fx, int side, Rectangle bounds);
Color battle_fx_tint(const BattleFx *fx, int side, Color base);
void battle_fx_draw(const BattleFx *fx, const BattleState *battle);
#endif
