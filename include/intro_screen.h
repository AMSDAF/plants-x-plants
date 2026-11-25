/* Apresenta o pedido de ajuda, a equipe inicial e a viagem até a vila. */
#ifndef INTRO_SCREEN_H
#define INTRO_SCREEN_H
#include "ui_input.h"

#include "trainer_select_screen.h"
#include "plant_data.h"

#define INTRO_STARTER_COUNT STARTER_SPECIES_COUNT
#define INTRO_TEAM_SIZE 3
#define INTRO_FONT_COUNT 6

typedef enum { INTRO_LETTER, INTRO_ACCEPT, INTRO_STARTERS, INTRO_TRAVEL } IntroStage;
typedef enum { INTRO_NONE, INTRO_BACK, INTRO_FINISH } IntroAction;

typedef enum {
    INTRO_CARD_NORMAL, INTRO_CARD_SELECTED, INTRO_FOCUS_OUTLINE,
    INTRO_FOOTER_BUTTON, INTRO_LEAF_BADGE, INTRO_CURSOR, INTRO_UI_COUNT
} IntroUiAsset;

/* Sprites e molduras da seleção pertencem ao jogo e são carregados uma única vez. */
typedef struct {
    Texture2D plants[SPECIES_COUNT];
    Rectangle sources[SPECIES_COUNT];
    Texture2D ui[INTRO_UI_COUNT];
    Font fonts[INTRO_FONT_COUNT];
    float font_scale;
} IntroAssets;

typedef struct {
    IntroStage stage;
    UiPointer pointer;
    int cursor;
    int selected[INTRO_TEAM_SIZE]; /* Índices na lista de iniciais, não IDs de espécie. */
    int count;
    float elapsed;
    float reveal;
    bool input_blocked;
} IntroScreen;

void intro_screen_init(IntroScreen *screen);
void intro_assets_load(IntroAssets *assets);
void intro_assets_unload(IntroAssets *assets);
void intro_assets_update_fonts(IntroAssets *assets, float scale);
IntroAction intro_screen_update(IntroScreen *screen, bool accept_input, float dt);
bool intro_screen_team(const IntroScreen *screen, Team *team);
void intro_screen_draw(const UiTheme *ui, const IntroScreen *screen,
                       const TrainerSelectAssets *assets, const IntroAssets *plant_assets,
                       const char *player_name);

#endif
