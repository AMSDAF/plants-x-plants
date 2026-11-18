/* Define a interface provisória e os comandos de navegação da batalha. */
#ifndef BATTLE_SCREEN_H
#define BATTLE_SCREEN_H
#include "ui_theme.h"

#include <stdbool.h>
#include "battle.h"
#include "raylib.h"

typedef enum {
    BATTLE_SCREEN_NONE,
    BATTLE_SCREEN_TEAM_READY,
    BATTLE_SCREEN_RETURN
} BattleScreenAction;

typedef struct {
    BattleState battle;
    float cpu_wait;
    int setup_side; /* -1 quando a montagem está concluída. */
    bool choosing_team;
    int roster_cursor;
    int team_slot;
    LogicFormula draft;
} BattleScreen;

BattleScreenAction battle_screen_update(BattleScreen *screen, float dt);
void battle_screen_draw(const UiTheme *ui, const BattleScreen *screen,
                        Texture2D aloe_guard, Texture2D sunflower_guard);

#endif
