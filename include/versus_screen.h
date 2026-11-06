/* Define a escolha de oponente e os recursos da tela Versus. */
#ifndef VERSUS_SCREEN_H
#define VERSUS_SCREEN_H
#include "raylib.h"

typedef enum { VERSUS_CPU, VERSUS_PLAYER } VersusMode;
typedef enum { VERSUS_NONE, VERSUS_CONFIRM, VERSUS_BACK } VersusAction;
typedef struct {
    Texture2D background;
    Texture2D grass_base;
    Texture2D grass_atlas;
    Texture2D foliage_base;
    Texture2D foliage_atlas;
    Texture2D illustrations;
} VersusAssets;
typedef struct {
    VersusMode selected;
    int hovered;
    int pressed;
    float elapsed;
    bool confirmed;
} VersusScreen;

void versus_assets_load(VersusAssets *assets);
void versus_assets_unload(VersusAssets *assets);
void versus_screen_init(VersusScreen *screen, VersusMode selected);
VersusAction versus_screen_update(VersusScreen *screen, bool accept_input, float dt);
void versus_background_draw(const VersusAssets *assets, float elapsed);
void versus_screen_draw(const VersusScreen *screen, const VersusAssets *assets, Texture2D logo, Rectangle logo_source);
#endif
