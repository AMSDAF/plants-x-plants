/* Apresenta o HUD creme e a seta a partir de dados somente de leitura. */
#ifndef EXPLORATION_HUD_H
#define EXPLORATION_HUD_H

#include "ui_theme.h"

typedef enum { HUD_INTERACTION_NONE, HUD_INTERACTION_AVAILABLE,
               HUD_INTERACTION_BLOCKED, HUD_INTERACTION_INFO } HudInteractionState;

typedef struct {
    HudInteractionState state;
    const char *context;
    const char *message;
    Vector2 target;
    Vector2 marker_anchor;
} HudInteraction;

/* Posição em pixels da tela; os ícones não dependem do zoom da câmera. */
typedef struct {
    Vector2 position;
    bool conversation;
    bool blocked;
} HudDiscoveryMarker;
void exploration_discovery_draw(const HudDiscoveryMarker *markers, int count, float elapsed);

Rectangle exploration_prompt_bounds(const UiTheme *ui, const HudInteraction *interaction);
Rectangle exploration_marker_bounds(const HudInteraction *interaction, Camera2D camera, float animation_time);
void exploration_hud_draw(const UiTheme *ui, const char *location, const char *objective,
                          const HudInteraction *interaction, Camera2D camera);

#endif
