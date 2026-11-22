/* Define a interface provisória e os comandos de navegação da batalha. */
#ifndef BATTLE_SCREEN_H
#define BATTLE_SCREEN_H
#include "ui_theme.h"

#include <stdbool.h>
#include "battle.h"
#include "ai.h"
#include "raylib.h"

typedef enum {
    BATTLE_SCREEN_NONE,
    BATTLE_SCREEN_TEAM_READY,
    BATTLE_SCREEN_RETURN
} BattleScreenAction;

typedef enum {
    BATTLE_PENDING_NONE,
    BATTLE_PENDING_CPU,
    BATTLE_PENDING_MOVE,
    BATTLE_PENDING_SWITCH,
    BATTLE_PENDING_PARALYSIS
} BattlePendingAction;

typedef struct {
    BattleState battle;
    BattlePendingAction pending_action;
    float action_timer;
    int pending_move_index;
    int pending_plant_index;
    AIActionType pending_cpu_action;
    int pending_cpu_index;
    int setup_side; /* -1 quando a montagem está concluída. */
    bool choosing_team;
    bool show_statuses;
    int roster_cursor;
    int team_slot;
    LogicFormula draft;
    int draft_move_index;
    int draft_plant_index;
} BattleScreen;

BattleScreenAction battle_screen_update(BattleScreen *screen, float dt);
void battle_screen_draw(const UiTheme *ui, const BattleScreen *screen,
                        Texture2D aloe_guard, Texture2D sunflower_guard);

#endif
