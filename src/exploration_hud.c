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
    int padding = available ? 62 : 24;
    int message_width = ui_measure_bold(ui, interaction->message, 20);
    float width = fminf(560, fmaxf(160, message_width + padding));
    float height = message_width > width - padding ? 72 : 44;
    return (Rectangle){roundf((GAME_SCREEN_WIDTH-width)/2), GAME_SCREEN_HEIGHT-24-height, width, height};
}

Rectangle exploration_marker_bounds(const HudInteraction *interaction, Camera2D camera, float animation_time)
{
    if (interaction->state == HUD_INTERACTION_NONE) return (Rectangle){0};
    Vector2 target = GetWorldToScreen2D(interaction->target, camera);
    if (target.x < 0 || target.x > GAME_SCREEN_WIDTH || target.y < 0 || target.y > GAME_SCREEN_HEIGHT)
        return (Rectangle){0};
    Vector2 point = GetWorldToScreen2D(interaction->marker_anchor, camera);
    float bob = 2 * (1 - cosf(animation_time * 2 * PI / 1.4f));
    /* Reserva todo o percurso antes da animação; a borda não alterna a visibilidade. */
    float x = fminf(fmaxf(point.x-9, 6), GAME_SCREEN_WIDTH-24);
    float y = fminf(fmaxf(point.y-18, 10), GAME_SCREEN_HEIGHT-18);
    return (Rectangle){roundf(x), roundf(y-bob), 18, 12};
}

static void arrow_shape(Rectangle bounds, Color color)
{
    /* Degraus de pixels, também na versão de reserva sem textura. */
    float sx = bounds.width/18, sy = bounds.height/12;
    for (int step = 0; step < 8; step++) {
        DrawRectangleRec((Rectangle){bounds.x+(1+step)*sx, bounds.y+(1+step)*sy, 3*sx, 3*sy}, color);
        DrawRectangleRec((Rectangle){bounds.x+(14-step)*sx, bounds.y+(1+step)*sy, 3*sx, 3*sy}, color);
    }
}

static void draw_prompt(const UiTheme *ui, const HudInteraction *interaction, Camera2D camera)
{
    Rectangle card = exploration_prompt_bounds(ui, interaction);
    if (!card.width) return;
    Rectangle arrow = exploration_marker_bounds(interaction, camera, ui ? ui->marker_time : 0);
    if (arrow.width) {
        if (ui && ui->interaction_chevron.id) {
            Texture2D texture = ui->interaction_chevron;
            DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                           arrow, (Vector2){0}, 0, WHITE);
        } else {
            arrow_shape(arrow, text_color);
            arrow_shape((Rectangle){arrow.x+2, arrow.y+1, arrow.width-4, arrow.height-4}, paper_color);
        }
    }
    /* Aviso compacto: uma borda simples, tecla e ação, sem título repetido. */
    DrawRectangleRec(card, text_color);
    DrawRectangleRec((Rectangle){card.x+2, card.y+2, card.width-4, card.height-4}, paper_color);
    bool available = interaction->state == HUD_INTERACTION_AVAILABLE;
    int left = (int)card.x + 12;
    if (available) {
        Rectangle key = {card.x+12, card.y+(card.height-28)/2, 28, 28};
        DrawRectangleRec(key, text_color);
        ui_text_bold(ui, "E", (int)(key.x+(key.width-ui_measure_bold(ui, "E", 20))/2),
                     (int)key.y+4, 20, paper_color);
        left += 38;
    }
    ui_text_wrap(ui, interaction->message,
                 (Rectangle){left, card.y+12, card.x+card.width-left-12, card.height-24},
                 20, text_color, true);
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
