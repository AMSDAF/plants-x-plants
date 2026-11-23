/* Define os recursos industriais e a composição do painel de relés. */
#ifndef RELAY_UI_H
#define RELAY_UI_H

#include "world.h"
#include "selection_ui.h"

typedef enum { RELAY_PANEL_SKIN, RELAY_BUTTON_SKIN, RELAY_SWITCH_ON, RELAY_SWITCH_OFF, RELAY_ART_COUNT } RelayArt;
typedef struct { Texture2D textures[RELAY_ART_COUNT]; } RelayUiAssets;

void relay_ui_load(RelayUiAssets *assets);
void relay_ui_unload(RelayUiAssets *assets);
void relay_ui_draw(const UiTheme *ui, const SelectionUiAssets *fonts,
                   const RelayUiAssets *assets, const World *world);

#endif
