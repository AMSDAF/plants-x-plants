/* Declara recursos, animação e ações da seleção de exploradores. */
#ifndef TRAINER_SELECT_SCREEN_H
#define TRAINER_SELECT_SCREEN_H

#include "player.h"
#include "trainer_background.h"

typedef enum {
    TRAINER_SELECT_NONE,
    TRAINER_SELECT_CONFIRM,
    TRAINER_SELECT_BACK
} TrainerSelectAction;

typedef struct {
    Texture2D portraits[TRAINER_COUNT];
    Rectangle portrait_bounds[TRAINER_COUNT]; /* Corpo sem halo, usado para escala e pés. */
    Texture2D male_idle;
    Texture2D male_idle_halo;
    Texture2D female_idle;
    Texture2D female_idle_halo;
    TrainerBackgroundAssets background;
} TrainerSelectAssets;

typedef struct {
    Trainer selected;
    int pressed;
    int hovered;
    bool background_preview;
    float background_time;
    float male_idle_time;
    float female_idle_time;
} TrainerSelectScreen;

void trainer_select_assets_load(TrainerSelectAssets *assets);
void trainer_select_assets_unload(TrainerSelectAssets *assets);
void trainer_select_screen_init(TrainerSelectScreen *screen);
int trainer_select_male_idle_frame(float elapsed);
int trainer_select_female_idle_frame(float elapsed);
TrainerSelectAction trainer_select_screen_update(TrainerSelectScreen *screen, bool accept_input, float dt);
void trainer_select_screen_draw(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets);
Rectangle trainer_select_card_bounds(Trainer trainer);
void trainer_select_draw_card(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets,
                              Trainer trainer, Rectangle bounds, float alpha, bool selected, float label_alpha);
void trainer_select_draw_chrome(const TrainerSelectScreen *screen, float alpha);

#endif
