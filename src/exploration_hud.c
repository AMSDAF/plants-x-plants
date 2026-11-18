/* Desenha objetivo, pistas e interação centralizada, sem modificar o mundo. */
#include "exploration_hud.h"
#include "game_view.h"
#include <math.h>

static const Color text_color = {24, 59, 48, 255};
static const Color paper_color = {246, 235, 204, 255};

Rectangle exploration_prompt_bounds(const UiTheme *ui, const HudInteraction *interaction)
{
    if (interaction->state == HUD_INTERACTION_NONE) return (Rectangle){0};
    bool available = interaction->state == HUD_INTERACTION_AVAILABLE;
    int padding = available ? 120 : 48;
    int message_width = ui_measure_bold(ui, interaction->message, available ? 24 : 22);
    int label_width = ui_measure(ui, interaction->context, 18);
    int content_width = message_width > label_width ? message_width : label_width;
    float width = fminf(640, fmaxf(400, content_width + padding));
    float height = message_width > width - padding ? 104 : 76;
    return (Rectangle){roundf((GAME_SCREEN_WIDTH-width)/2), GAME_SCREEN_HEIGHT-24-height, width, height};
}

Rectangle exploration_marker_bounds(const HudInteraction *interaction, Camera2D camera)
{
    if (interaction->state == HUD_INTERACTION_NONE) return (Rectangle){0};
    Vector2 target = GetWorldToScreen2D(interaction->target, camera);
    if (target.x < 0 || target.x > GAME_SCREEN_WIDTH || target.y < 0 || target.y > GAME_SCREEN_HEIGHT)
        return (Rectangle){0};
    Vector2 point = GetWorldToScreen2D(interaction->marker_anchor, camera);
    Rectangle bounds = {roundf(point.x)-10, roundf(point.y)-32, 20, 26};
    if (bounds.x < 6 || bounds.x+bounds.width > GAME_SCREEN_WIDTH-6 ||
        bounds.y < 6 || bounds.y+bounds.height > GAME_SCREEN_HEIGHT-6) return (Rectangle){0};
    return bounds;
}

static void arrow_shape(Rectangle bounds, Color color)
{
    float center = bounds.x+bounds.width/2;
    float head = bounds.y+bounds.height*0.43f;
    DrawRectangleRec((Rectangle){center-bounds.width*0.18f, bounds.y,
                                 bounds.width*0.36f, bounds.height*0.55f}, color);
    DrawTriangle((Vector2){bounds.x, head}, (Vector2){center, bounds.y+bounds.height},
                 (Vector2){bounds.x+bounds.width, head}, color);
}

static void draw_prompt(const UiTheme *ui, const HudInteraction *interaction, Camera2D camera)
{
    Rectangle card = exploration_prompt_bounds(ui, interaction);
    if (!card.width) return;
    Rectangle arrow = exploration_marker_bounds(interaction, camera);
    if (arrow.width) {
        arrow_shape((Rectangle){arrow.x+1, arrow.y+2, arrow.width, arrow.height}, (Color){44, 32, 19, 80});
        arrow_shape(arrow, (Color){73, 48, 30, 255});
        arrow_shape((Rectangle){arrow.x+3, arrow.y+2, arrow.width-6, arrow.height-6}, paper_color);
    }
    ui_panel(card);
    bool available = interaction->state == HUD_INTERACTION_AVAILABLE;
    int left = (int)card.x + (available ? 112 : 24);
    const char *suffix = interaction->state == HUD_INTERACTION_BLOCKED ? " · Indisponível"
                        : interaction->state == HUD_INTERACTION_INFO ? " · Já registrada" : "";
    ui_text(ui, TextFormat("%s%s", interaction->context, suffix), left, (int)card.y+10, 18, text_color);
    if (available) {
        Rectangle key = {card.x+22, card.y+(card.height-44)/2, 46, 44};
        DrawRectangleRounded(key, 0.22f, 8, text_color);
        ui_text_bold(ui, "E", (int)(key.x+(key.width-ui_measure_bold(ui, "E", 28))/2),
                     (int)key.y+6, 28, paper_color);
        DrawLine((int)card.x+89, (int)card.y+16, (int)card.x+89, (int)(card.y+card.height-16),
                 (Color){172, 153, 114, 255});
    }
    ui_text_wrap(ui, interaction->message,
                 (Rectangle){left, card.y+34, card.x+card.width-left-20, card.height-40},
                 available ? 24 : 22, text_color, true);
}

void exploration_hud_draw(const UiTheme *ui, const char *location, const char *objective,
                          int clues, const HudInteraction *interaction, Camera2D camera)
{
    int objective_width = ui_measure_bold(ui, objective, 22);
    int location_width = ui_measure(ui, location, 18);
    float width = fminf(700, fmaxf(280, (objective_width > location_width ? objective_width : location_width)+36));
    float height = objective_width > width-36 ? 94 : 72;
    ui_panel((Rectangle){20, 18, width, height});
    ui_text(ui, location, 38, 27, 18, text_color);
    ui_text_wrap(ui, objective, (Rectangle){38, 49, width-36, height-34}, 22, text_color, true);

    Rectangle counter = {GAME_SCREEN_WIDTH-156, 18, 136, 42};
    ui_panel(counter);
    ui_text(ui, "Pistas", (int)counter.x+13, 28, 18, text_color);
    ui_text_bold(ui, TextFormat("%d/2", clues), (int)counter.x+84, 24, 24, text_color);
    draw_prompt(ui, interaction, camera);
}
