/* Declara recursos, animação e ações da seleção de exploradores. */
#ifndef TRAINER_SELECT_SCREEN_H
#define TRAINER_SELECT_SCREEN_H

#include "player.h"
#include "forest_scene.h"

typedef enum {
    TRAINER_SELECT_NONE,
    TRAINER_SELECT_CONFIRM,
    TRAINER_SELECT_BACK
} TrainerSelectAction;

typedef struct {
    Texture2D portraits[TRAINER_COUNT];
    Texture2D breathing[TRAINER_COUNT];
    Texture2D frame;
    ForestSceneAssets forest;
} TrainerSelectAssets;

typedef struct {
    Trainer selected;
    int pressed;
    int hovered;
    float elapsed_time;
    float emphasis[TRAINER_COUNT];
    ForestScene forest;
} TrainerSelectScreen;

void trainer_select_assets_load(TrainerSelectAssets *assets);
void trainer_select_assets_unload(TrainerSelectAssets *assets);
void trainer_select_screen_init(TrainerSelectScreen *screen);
TrainerSelectAction trainer_select_screen_update(TrainerSelectScreen *screen, bool accept_input, float dt);
void trainer_select_screen_draw(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets,
                               Texture2D buttons);

#endif
