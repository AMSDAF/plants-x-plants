/* Define a interface provisória e os comandos de navegação da batalha. */
#ifndef BATTLE_SCREEN_H
#define BATTLE_SCREEN_H

#include <stdbool.h>
#include "battle.h"

typedef enum {
    BATTLE_SCREEN_NONE,
    BATTLE_SCREEN_TEAM_READY,
    BATTLE_SCREEN_RESTART,
    BATTLE_SCREEN_MENU
} BattleScreenAction;

typedef struct {
    BattleState battle;
    bool versus_cpu;
    bool campaign_battle;
    float cpu_wait;
    int setup_side; /* -1 quando a montagem está concluída. */
    bool choosing_team;
    int roster_cursor;
    int team_slot;
    LogicFormula draft;
} BattleScreen;

BattleScreenAction battle_screen_update(BattleScreen *screen, float dt);
void battle_screen_draw(const BattleScreen *screen);

#endif
