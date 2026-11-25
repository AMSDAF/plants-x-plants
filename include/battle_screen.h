/* Define a arena botânica, seus recursos e comandos de navegação. */
#ifndef BATTLE_SCREEN_H
#define BATTLE_SCREEN_H
#include "ui_input.h"
#include "ui_theme.h"

#include <stdbool.h>
#include "battle.h"
#include "battle_fx.h"
#include "ai.h"
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

typedef enum {
    BATTLE_PENDING_NONE,
    BATTLE_PENDING_CPU,
    BATTLE_PENDING_MOVE,
    BATTLE_PENDING_SWITCH,
    BATTLE_PENDING_PARALYSIS,
    BATTLE_PENDING_ROUND,
    BATTLE_PENDING_ROUND_RESULT
} BattlePendingAction;

/* Mensagens avançam após 1,5 s de leitura ou confirmação, depois da animação. */
#define BATTLE_ROUND_MESSAGES 12

typedef struct {
    BattleState battle;
    UiPointer pointer;
    BattleFx fx;
    BattlePendingAction pending_action;
    float action_timer;
    int pending_move_index;
    int pending_plant_index;
    AIActionType pending_cpu_action;
    int pending_cpu_index;
    BattleAction pending_player_action;
    BattleAction pending_enemy_action;
    BattleRound round;
    bool round_active;
    char round_result_message[2048];
    bool message_armed;
    float message_elapsed;
    int round_message_offsets[BATTLE_ROUND_MESSAGES];
    int round_message_count;
    int round_message_index;
    int animation_side; /* -1 quando nenhuma planta está animando. */
    int animation_plant_index;
    bool animation_status; /* Golpes de status pulam; físicos avançam e voltam. */
    int setup_side; /* -1 quando a montagem está concluída. */
    bool choosing_team;
    bool show_statuses;
    int roster_cursor;
    int team_slot;
    LogicFormula draft;
    int draft_move_index;
    BattleScene scene;
    BattleMenu menu;
    int menu_cursor;
    int draft_plant_index;
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
