/* Desenha objetivo, pistas e interação centralizada, sem modificar o mundo. */
#include "exploration_hud.h"
#include "game_view.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const Color text_color = {24, 59, 48, 255};
static const Color paper_color = {246, 235, 204, 255};

static Font prompt_font(const UiTheme *ui)
{
    return ui && ui->dialogue_font.texture.id ? ui->dialogue_font : ui_font(ui, false);
}

static void prompt_message(const HudInteraction *interaction, char *buffer, int capacity)
{
    const char *message = interaction->message ? interaction->message : "";
    if (interaction->state == HUD_INTERACTION_AVAILABLE) {
        static const char prefix[] = "Pressione \"E\" para ";
        snprintf(buffer, capacity, "%s%s", prefix, message);
        /* As ações começam com verbos ASCII; nomes e acentos seguintes permanecem intactos. */
        int first = sizeof(prefix)-1;
        if (first < capacity && buffer[first] >= 'A' && buffer[first] <= 'Z') buffer[first] += 'a'-'A';
    } else {
        snprintf(buffer, capacity, "%s", message);
        char *sentence = strstr(buffer, ". ");
        if (sentence) sentence[1] = '\n';
    }
}

Rectangle exploration_prompt_bounds(const UiTheme *ui, const HudInteraction *interaction)
{
    if (interaction->state == HUD_INTERACTION_NONE) return (Rectangle){0};
    char message[512];
    prompt_message(interaction, message, sizeof(message));
    float message_width = ceilf(MeasureTextEx(prompt_font(ui), message, 28, 0).x);
    float width = fminf(560, fmaxf(200, message_width+88));
    float height = strchr(message, '\n') || message_width > width-88 ? 104 : 76;
    return (Rectangle){roundf((GAME_SCREEN_WIDTH-width)/2), GAME_SCREEN_HEIGHT-24-height, width, height};
}

static void draw_prompt_frame(const UiTheme *ui, Rectangle bounds)
{
    Texture2D texture = ui ? ui->interaction_frame : (Texture2D){0};
    Rectangle source = ui ? ui->interaction_frame_source : (Rectangle){0};
    if (!texture.id || source.width <= 0 || source.height <= 0) {
        DrawRectangleRec(bounds, paper_color);
        DrawRectangleLinesEx(bounds, 3, (Color){145,76,36,255});
        DrawRectangleLinesEx((Rectangle){bounds.x+3,bounds.y+3,bounds.width-6,bounds.height-6}, 1, text_color);
        return;
    }
    /* Cantos fixos mantêm folhas e rebites na escala do conceito aprovado. */
    float sx = source.height*0.75f, sy = source.height*0.48f;
    float x[] = {source.x,source.x+sx,source.x+source.width-sx,source.x+source.width};
    float y[] = {source.y,source.y+sy,source.y+source.height-sy,source.y+source.height};
    float dx[] = {bounds.x,bounds.x+38,bounds.x+bounds.width-38,bounds.x+bounds.width};
    float dy[] = {bounds.y,bounds.y+34,bounds.y+bounds.height-34,bounds.y+bounds.height};
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            DrawTexturePro(texture, (Rectangle){x[col],y[row],x[col+1]-x[col],y[row+1]-y[row]},
                           (Rectangle){dx[col],dy[row],dx[col+1]-dx[col],dy[row+1]-dy[row]}, (Vector2){0}, 0, WHITE);
        }
    }
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
    draw_prompt_frame(ui, card);
    char message[512];
    prompt_message(interaction, message, sizeof(message));
    Font font = prompt_font(ui);
    float message_width = ceilf(MeasureTextEx(font, message, 28, 0).x);
    bool wrapped = card.height > 76;
    float left = wrapped ? card.x+44 : roundf(card.x+(card.width-message_width)/2);
    ui_text_wrap_font(font, message,
                      (Rectangle){left,card.y+(wrapped ? 18 : 20),wrapped ? card.width-88 : message_width+1,card.height-36},
                      28, text_color);
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
