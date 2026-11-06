/* Apresenta modos Versus sem iniciar seleção de plantas ou combate. */
#include "versus_screen.h"
#include "selection_ui.h"
#include "versus_grass_data.h"
#include "versus_foliage_data.h"
#include <math.h>

#define CONFIRM_CONTROL 2
#define BACK_CONTROL 3

static Rectangle control_bounds(int index)
{
    if (index < 2) return (Rectangle){208 + index * 448.0f, 258, 416, 294};
    return (Rectangle){index == CONFIRM_CONTROL ? 660 : 380, 592, 240, 52};
}

static Texture2D load_texture(const char *path)
{
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "Recurso Versus ausente: %s", path);
        return (Texture2D){0};
    }
    Texture2D texture = LoadTexture(path);
    if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

void versus_assets_load(VersusAssets *assets)
{
    *assets = (VersusAssets){0};
    assets->background = load_texture("assets/sprites/versus_background_generated_v1.png");
    if (assets->background.id && assets->background.width == 1672 && assets->background.height == 941) {
        assets->grass_base = load_texture("assets/sprites/versus/grass_base.png");
        assets->grass_atlas = load_texture("assets/sprites/versus/grass_atlas.png");
        if (!assets->grass_base.id || assets->grass_base.width != 1672 || assets->grass_base.height != 941 ||
            !assets->grass_atlas.id || assets->grass_atlas.width != VERSUS_GRASS_PAGE_WIDTH*VERSUS_GRASS_COLUMNS ||
            assets->grass_atlas.height != VERSUS_GRASS_PAGE_HEIGHT*(VERSUS_GRASS_FRAMES/VERSUS_GRASS_COLUMNS)) {
            if (assets->grass_base.id) UnloadTexture(assets->grass_base);
            if (assets->grass_atlas.id) UnloadTexture(assets->grass_atlas);
            assets->grass_base = assets->grass_atlas = (Texture2D){0};
            TraceLog(LOG_WARNING, "Grama do Versus indisponivel; preservando fundo original completo.");
        }
    }
    if (assets->grass_base.id && assets->grass_atlas.id) {
        assets->foliage_base = load_texture("assets/sprites/versus/foliage_base.png");
        assets->foliage_atlas = load_texture("assets/sprites/versus/foliage_atlas.png");
        if (!assets->foliage_base.id || assets->foliage_base.width != 1672 || assets->foliage_base.height != 941 ||
            !assets->foliage_atlas.id || assets->foliage_atlas.width != VERSUS_FOLIAGE_ATLAS_WIDTH ||
            assets->foliage_atlas.height != VERSUS_FOLIAGE_ATLAS_HEIGHT) {
            if (assets->foliage_base.id) UnloadTexture(assets->foliage_base);
            if (assets->foliage_atlas.id) UnloadTexture(assets->foliage_atlas);
            assets->foliage_base = assets->foliage_atlas = (Texture2D){0};
            TraceLog(LOG_WARNING, "Folhagem do Versus indisponivel; preservando arvores estaticas e grama animada.");
        }
    }
    if (!assets->background.id)
        assets->background = load_texture("assets/sprites/versus_background_v1.png");
    assets->illustrations = load_texture("assets/sprites/versus_illustrations_source.png");
}

void versus_assets_unload(VersusAssets *assets)
{
    if (assets->background.id) UnloadTexture(assets->background);
    if (assets->grass_base.id) UnloadTexture(assets->grass_base);
    if (assets->grass_atlas.id) UnloadTexture(assets->grass_atlas);
    if (assets->foliage_base.id) UnloadTexture(assets->foliage_base);
    if (assets->foliage_atlas.id) UnloadTexture(assets->foliage_atlas);
    if (assets->illustrations.id) UnloadTexture(assets->illustrations);
    *assets = (VersusAssets){0};
}

void versus_screen_init(VersusScreen *screen, VersusMode selected)
{
    *screen = (VersusScreen){.selected = selected, .hovered = -1, .pressed = -1};
}

VersusAction versus_screen_update(VersusScreen *screen, bool accept_input, float dt)
{
    screen->elapsed = fmodf(screen->elapsed + fminf(0.1f, fmaxf(0, dt)), 128);
    screen->hovered = -1;
    if (!accept_input) {
        screen->pressed = -1;
        return VERSUS_NONE;
    }
    VersusMode previous = screen->selected;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) screen->selected = VERSUS_CPU;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) screen->selected = VERSUS_PLAYER;
    for (int i = 0; i < 4; i++)
        if (CheckCollisionPointRec(GetMousePosition(), control_bounds(i))) screen->hovered = i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        screen->pressed = screen->hovered;
        if (screen->pressed >= 0 && screen->pressed < 2)
            screen->selected = (VersusMode)screen->pressed;
    }
    if (previous != screen->selected) screen->confirmed = false;
    VersusAction action = VERSUS_NONE;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (screen->pressed == screen->hovered) {
            if (screen->pressed == CONFIRM_CONTROL) action = VERSUS_CONFIRM;
            if (screen->pressed == BACK_CONTROL) action = VERSUS_BACK;
        }
        screen->pressed = -1;
    }
    if (IsKeyPressed(KEY_ENTER)) action = VERSUS_CONFIRM;
    if (IsKeyPressed(KEY_ESCAPE)) action = VERSUS_BACK;
    return action;
}

static void panel(Rectangle bounds, bool selected)
{
    selection_ui_panel(bounds, 8, selected ? SELECTION_INK : SELECTION_BORDER);
    int border = selected ? 8 : 4;
    selection_ui_panel((Rectangle){bounds.x + border, bounds.y + border,
                       bounds.width - 2*border, bounds.height - 2*border}, 8, SELECTION_CREAM);
}

void versus_background_draw(const VersusAssets *assets, float elapsed)
{
    ClearBackground((Color){142,155,80,255});
    /* O par é indivisível: sem atlas ou base válidos, não remove a grama original. */
    bool animated = assets->grass_base.id && assets->grass_atlas.id;
    bool foliage = animated && assets->foliage_base.id && assets->foliage_atlas.id;
    Texture2D texture = foliage ? assets->foliage_base : animated ? assets->grass_base : assets->background;
    if (!texture.id) return;
    float scale = fmaxf(1280.0f/texture.width, 720.0f/texture.height);
    float x = (1280-texture.width*scale)/2;
    float y = (720-texture.height*scale)/2;
    DrawTexturePro(texture, (Rectangle){0,0,(float)texture.width,(float)texture.height},
                   (Rectangle){x,y,texture.width*scale,texture.height*scale}, (Vector2){0}, 0, WHITE);
    if (!animated) return;
    if (foliage) {
        /* Somente folhas em alfa; o relógio e a composição da grama permanecem iguais. */
        for (int i = 0; i < VERSUS_FOLIAGE_COUNT; i++) {
            const VersusFoliageRegion *region = &VERSUS_FOLIAGE_REGIONS[i];
            int frame = (int)(fmodf(fmaxf(0,elapsed),32.0f)*region->fps) % 32;
            Rectangle source = region->poses[region->frames[frame]];
            DrawTexturePro(assets->foliage_atlas,source,
                           (Rectangle){x+region->origin.x*scale,y+region->origin.y*scale,
                                       source.width*scale,source.height*scale}, (Vector2){0},0,WHITE);
        }
    }
    for (int i = 0; i < VERSUS_GRASS_COUNT; i++) {
        const VersusGrassRegion *region = &VERSUS_GRASS_REGIONS[i];
        int frame = (int)(fmodf(fmaxf(0,elapsed),32.0f)*region->fps) % VERSUS_GRASS_FRAMES;
        Rectangle source = region->source;
        source.x += (frame % VERSUS_GRASS_COLUMNS)*VERSUS_GRASS_PAGE_WIDTH;
        source.y += (frame / VERSUS_GRASS_COLUMNS)*VERSUS_GRASS_PAGE_HEIGHT;
        DrawTexturePro(assets->grass_atlas,source,
                       (Rectangle){x+region->origin.x*scale,y+region->origin.y*scale,
                                   source.width*scale,source.height*scale}, (Vector2){0},0,WHITE);
    }
}

void versus_screen_draw(const VersusScreen *screen, const VersusAssets *assets, Texture2D logo, Rectangle logo_source)
{
    versus_background_draw(assets,screen->elapsed);
    if (logo.id && logo_source.height > 0) {
        float scale = fminf(580.0f/logo_source.width, 136.0f/logo_source.height);
        DrawTexturePro(logo,logo_source,
                       (Rectangle){(1280-logo_source.width*scale)/2,
                                   8+roundf(2*sinf(screen->elapsed*PI)),
                                   logo_source.width*scale,logo_source.height*scale},
                       (Vector2){0},0,WHITE);
    }
    selection_ui_panel((Rectangle){260,160,760,64},4,SELECTION_INK);
    selection_ui_panel((Rectangle){264,164,752,56},4,SELECTION_CREAM);
    selection_ui_text("Escolha o modo versus",640,176,36,SELECTION_INK);
    for (int i = 0; i < 2; i++) {
        Rectangle card = control_bounds(i);
        bool selected = screen->selected == (VersusMode)i;
        panel(card,selected);
        if (assets->illustrations.id) {
            /* Recortes da referência preservam a arte; os controles são desenhados em código. */
            Rectangle source = {i == 0 ? 294 : 886, 384, 482, 244};
            Texture2D texture = assets->illustrations;
            source.x *= texture.width/1672.0f;
            source.width *= texture.width/1672.0f;
            source.y *= texture.height/941.0f;
            source.height *= texture.height/941.0f;
            DrawTexturePro(texture, source, (Rectangle){card.x+24,card.y+32,368,186},
                           (Vector2){0},0,WHITE);
        } else selection_ui_text(i == 0 ? "Computador" : "Dois jogadores",
                                  (int)(card.x+card.width/2),(int)card.y+120,26,SELECTION_INK);
        if (selected) selection_ui_accents(card,1);
        const char *label = i == 0 ? "VS CPU" : "VS PLAYER";
        int center = (int)(card.x+card.width/2);
        selection_ui_text(label,center,502,30,SELECTION_INK);
        if (selected) selection_ui_cursor(center-MeasureText(label,30)/2-12,517,screen->elapsed,1);
    }
    for (int i = CONFIRM_CONTROL; i <= BACK_CONTROL; i++) {
        Rectangle bounds = control_bounds(i);
        bool hover = screen->hovered == i;
        bool pressed = hover && screen->pressed == i;
        selection_ui_panel(bounds,4,SELECTION_INK);
        Color fill = pressed ? (Color){212,211,155,255} : hover ? (Color){232,227,176,255} : SELECTION_CREAM;
        selection_ui_panel((Rectangle){bounds.x+4,bounds.y+4,bounds.width-8,bounds.height-8},4,fill);
        if (hover) DrawRectangle((int)bounds.x+16,(int)bounds.y+12,4,28,SELECTION_ORANGE);
        selection_ui_text(i == CONFIRM_CONTROL ? "Confirmar" : "Voltar",
                          (int)(bounds.x+bounds.width/2),(int)bounds.y+12,30,SELECTION_INK);
    }
    if (screen->confirmed) {
        selection_ui_panel((Rectangle){208,654,864,26},4,SELECTION_INK);
        selection_ui_text(screen->selected == VERSUS_CPU
            ? "VS CPU selecionado. Combate em desenvolvimento."
            : "VS PLAYER selecionado. Combate em desenvolvimento.",640,658,20,SELECTION_CREAM);
    }
    selection_ui_panel((Rectangle){224,688,832,28},4,SELECTION_INK);
    selection_ui_text("Setas / A-D: escolher  |  ENTER: confirmar  |  ESC: voltar",640,692,20,SELECTION_CREAM);
}
