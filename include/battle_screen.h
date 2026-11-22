/* Define a arena botânica, seus recursos e comandos de navegação. */
#ifndef BATTLE_SCREEN_H
#define BATTLE_SCREEN_H
#include "ui_theme.h"

#include <stdbool.h>
#include "battle.h"
#include "raylib.h"
#include "plant_data.h"
#include "selection_ui.h"

typedef enum { BATTLE_MENU_ROOT, BATTLE_MENU_MOVES, BATTLE_MENU_TEAM } BattleMenu;
typedef enum { BATTLE_SCENE_DEPOT, BATTLE_SCENE_NURSERY } BattleScene;
typedef enum {
    BATTLE_ART_DEPOT, BATTLE_ART_NURSERY, BATTLE_ART_GROUND,
    BATTLE_ART_STATUS, BATTLE_ART_COMMANDS, BATTLE_ART_LOCATION, BATTLE_ART_COUNT
} BattleArt;

/* Possui somente as artes exclusivas; as seis iniciais e fontes são emprestadas. */
typedef struct {
    Texture2D textures[BATTLE_ART_COUNT];
    Texture2D bosses[2];
    Rectangle boss_sources[2];
} BattleScreenAssets;

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
    int draft_move_index;
    BattleScene scene;
    BattleMenu menu;
    int menu_cursor;
} BattleScreen;

void battle_screen_assets_load(BattleScreenAssets *assets);
void battle_screen_assets_unload(BattleScreenAssets *assets);
void battle_screen_draw_art(const UiTheme *ui, const BattleScreenAssets *assets,
                             BattleArt art, Rectangle bounds);
BattleScreenAction battle_screen_update(BattleScreen *screen, float dt);
void battle_screen_draw(const UiTheme *ui, const BattleScreen *screen,
                        const BattleScreenAssets *assets, const SelectionUiAssets *fonts,
                        const Texture2D plants[SPECIES_COUNT], const Rectangle sources[SPECIES_COUNT]);

#endif
