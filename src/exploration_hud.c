/* Desenha objetivo e interação centralizada, sem modificar o mundo. */
#include "exploration_hud.h"
#include "game_view.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const Color text_color = {24, 59, 48, 255};
static const Color paper_color = {246, 235, 204, 255};

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
    float message_width = ceilf(MeasureTextEx(ui_pixel_font(ui), message, 28, 0).x);
    float width = fminf(560, fmaxf(200, message_width+88));
    float height = strchr(message, '\n') || message_width > width-88 ? 104 : 76;
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
    ui_botanical_panel(ui, card);
    char message[512];
    prompt_message(interaction, message, sizeof(message));
    Font font = ui_pixel_font(ui);
    float message_width = ceilf(MeasureTextEx(font, message, 28, 0).x);
    bool wrapped = card.height > 76;
    float left = wrapped ? card.x+44 : roundf(card.x+(card.width-message_width)/2);
    ui_text_wrap_font(font, message,
                      (Rectangle){left,card.y+(wrapped ? 18 : 20),wrapped ? card.width-88 : message_width+1,card.height-36},
                      28, text_color);
}

void exploration_hud_draw(const UiTheme *ui, const char *location, const char *objective,
                          const HudInteraction *interaction, Camera2D camera)
{
    Font font = ui_pixel_font(ui);
    float objective_width = ceilf(MeasureTextEx(font, objective, 28, 0).x);
    float location_width = ceilf(MeasureTextEx(font, location, 20, 0).x);
    float width = fminf(700, fmaxf(320, fmaxf(objective_width, location_width)+72));
    float height = objective_width > width-72 ? 130 : 94;
    ui_botanical_panel(ui, (Rectangle){20,18,width,height});
    DrawTextEx(font, location, (Vector2){56,31}, 20, 0, text_color);
    ui_text_wrap_font(font, objective, (Rectangle){56,60,width-72,height-56}, 28, text_color);

    draw_prompt(ui, interaction, camera);
}

/* Ícones pequenos e sem textura: pulso suave e tamanho fixo em pixels de interface. */
void exploration_discovery_draw(const HudDiscoveryMarker *markers, int count, float elapsed)
{
    if (!markers || count <= 0) return;
    for (int i = 0; i < count; i++) {
        const HudDiscoveryMarker *marker = &markers[i];
        float wave = (1-cosf(elapsed*2*PI/1.8f+i*0.7f))/2;
        int x = (int)roundf(marker->position.x);
        int y = (int)roundf(marker->position.y-12-2*wave);
        if (marker->conversation) {
            Color border = {27,49,32,220}, fill = {249,238,198,245};
            DrawRectangle(x-10,y-10,20,17,border);
            DrawRectangle(x-12,y-8,24,13,border);
            DrawRectangle(x-9,y-8,18,13,fill);
            DrawRectangle(x-10,y-7,20,11,fill);
            DrawRectangle(x-6,y+5,6,5,(Color){27,49,32,220});
            DrawRectangle(x-4,y+4,3,4,(Color){249,238,198,245});
            for (int dot = 0; dot < 3; dot++) DrawRectangle(x-7+dot*5,y-3,3,3,(Color){61,83,41,255});
        } else {
            Color glow = marker->blocked ? (Color){205,182,125,220}
                                        : Fade((Color){255,214,83,255},0.7f+0.3f*wave);
            DrawRectangle(x-2,y-9,6,20,(Color){49,57,25,180});
            DrawRectangle(x-9,y-2,20,6,(Color){49,57,25,180});
            DrawRectangle(x-1,y-8,3,17,glow);
            DrawRectangle(x-8,y-1,17,3,glow);
            DrawRectangle(x-3,y-3,7,7,glow);
            DrawRectangle(x-1,y-1,3,3,(Color){255,247,207,255});
        }
    }
}
