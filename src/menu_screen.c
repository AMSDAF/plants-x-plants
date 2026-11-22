/* Compõe a tela inicial, anima vegetação/mascote e abre a seleção de personagem. */
#include "menu_screen.h"
#include "ui_theme.h"
#include "menu_mascot_data.h"
#include "menu_grass_data.h"
#include "menu_tree_data.h"
#include <math.h>
#include <stddef.h>

static const Color INK = {24, 52, 30, 255};

static bool menu_input_down(void)
{
    const int keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_UP, KEY_DOWN,
                        KEY_LEFT, KEY_RIGHT, KEY_W, KEY_A, KEY_S, KEY_D};
    for (unsigned int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if (IsKeyDown(keys[i])) return true;
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

/* Inspeciona alfa uma única vez: a escala usa a arte visível, não a tela transparente. */
static Texture2D load_art(const char *path, Rectangle *source)
{
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "Arte de menu ausente: %s", path);
        return (Texture2D){0};
    }
    Image image = LoadImage(path);
    if (!image.data) return (Texture2D){0};
    Rectangle visible = GetImageAlphaBorder(image, 32.0f / 255.0f);
    if (visible.width <= 0 || visible.height <= 0) {
        TraceLog(LOG_WARNING, "Arte de menu sem conteudo visivel: %s", path);
        UnloadImage(image);
        return (Texture2D){0};
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id) {
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        if (source) *source = visible;
    }
    return texture;
}

static void load_grass(MenuScreen *screen)
{
    const char *background_path = "assets/sprites/menu_grass_background.png";
    const char *atlas_path = "assets/sprites/menu_grass_atlas.png";
    if (!screen->background.id || screen->background.width != MENU_GRASS_SOURCE_WIDTH ||
        screen->background.height != MENU_GRASS_SOURCE_HEIGHT ||
        !FileExists(background_path) || !FileExists(atlas_path)) return;
    Texture2D background = LoadTexture(background_path);
    Texture2D atlas = LoadTexture(atlas_path);
    if (background.id && atlas.id && background.width == MENU_GRASS_SOURCE_WIDTH &&
        background.height == MENU_GRASS_SOURCE_HEIGHT &&
        atlas.width == MENU_GRASS_PAGE_WIDTH * MENU_GRASS_COLUMNS &&
        atlas.height == MENU_GRASS_PAGE_HEIGHT * (MENU_GRASS_FRAME_COUNT / MENU_GRASS_COLUMNS)) {
        SetTextureFilter(background, TEXTURE_FILTER_POINT);
        SetTextureFilter(atlas, TEXTURE_FILTER_POINT);
        screen->grass_background = background;
        screen->grass_atlas = atlas;
    } else {
        /* O par é indivisível: nunca exibe chão reconstruído sem suas touceiras. */
        if (background.id) UnloadTexture(background);
        if (atlas.id) UnloadTexture(atlas);
        TraceLog(LOG_WARNING, "Recursos de grama invalidos; usando fundo completo estatico.");
    }
}

static void load_trees(MenuScreen *screen)
{
    const char *background_path = "assets/sprites/menu_wind_background.png";
    const char *atlas_path = "assets/sprites/menu_tree_atlas.png";
    /* O fundo combinado depende também da grama: a carga tem fallback em camadas. */
    if (!screen->grass_background.id || !screen->grass_atlas.id ||
        !FileExists(background_path) || !FileExists(atlas_path)) return;
    Texture2D background = LoadTexture(background_path);
    Texture2D atlas = LoadTexture(atlas_path);
    if (background.id && atlas.id && background.width == MENU_GRASS_SOURCE_WIDTH &&
        background.height == MENU_GRASS_SOURCE_HEIGHT && atlas.width == MENU_TREE_ATLAS_WIDTH &&
        atlas.height == MENU_TREE_ATLAS_HEIGHT) {
        SetTextureFilter(background, TEXTURE_FILTER_POINT);
        SetTextureFilter(atlas, TEXTURE_FILTER_POINT);
        screen->tree_background = background;
        screen->tree_atlas = atlas;
    } else {
        if (background.id) UnloadTexture(background);
        if (atlas.id) UnloadTexture(atlas);
        TraceLog(LOG_WARNING, "Recursos de copas invalidos; preservando arvores estaticas e grama.");
    }
}

void menu_screen_init(MenuScreen *screen)
{
    *screen = (MenuScreen){0};
    /* Aceita a grafia fornecida sem alterar ou duplicar a fonte original. */
    const char *background_path = FileExists("assets/sprites/menu_background_v2.png")
        ? "assets/sprites/menu_background_v2.png" : "assets/sprites/menu_backround_v2.png";
    screen->background = load_art(background_path, NULL);
    load_grass(screen);
    load_trees(screen);
    screen->logo = load_art("assets/sprites/logo_v2.png", &screen->logo_source);
    screen->mascot = load_art("assets/sprites/menu_mascot.png", &screen->mascot_source);
    const char *idle_path = "assets/sprites/menu_mascot_idle.png";
    if (FileExists(idle_path)) {
        Texture2D idle = LoadTexture(idle_path);
        bool geometry_matches = !screen->mascot.id ||
            (screen->mascot_source.width == MENU_MASCOT_CROP_WIDTH && screen->mascot_source.height == MENU_MASCOT_CROP_HEIGHT);
        if (idle.id && idle.width == MENU_MASCOT_FRAME_SIZE * MENU_MASCOT_COLUMNS &&
            idle.height == MENU_MASCOT_FRAME_SIZE * (MENU_MASCOT_FRAME_COUNT / MENU_MASCOT_COLUMNS) && geometry_matches) {
            SetTextureFilter(idle, TEXTURE_FILTER_POINT);
            screen->mascot_idle = idle;
        } else {
            if (idle.id) UnloadTexture(idle);
            TraceLog(LOG_WARNING, "Atlas de repouso do mascote invalido; usando imagem estatica.");
        }
    } else TraceLog(LOG_INFO, "Atlas de repouso do mascote ausente; usando imagem estatica.");
}

MenuAction menu_screen_update(MenuScreen *screen, bool accept_input)
{
    float dt = GetFrameTime();
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    /* O relógio avança inclusive enquanto ENTER/ESC aguardam soltura. */
    screen->mascot_time = fmodf(screen->mascot_time + dt, MENU_MASCOT_FRAME_COUNT / MENU_MASCOT_FPS);
    screen->grass_time = fmodf(screen->grass_time + dt, MENU_GRASS_FRAME_COUNT / MENU_GRASS_FPS);
    /* Seis segundos fecham os três ciclos visuais, sem reiniciar nas transições da tela inicial. */
    screen->ui_time = fmodf(screen->ui_time + dt, 6.0f);
    if (!accept_input) {
        /* Sincroniza a soltura com o bloqueio de game.c ao voltar de outra tela. */
        if (!menu_input_down()) screen->input_blocked = false;
        return MENU_ACTION_NONE;
    }
    if (screen->input_blocked) {
        if (!menu_input_down()) screen->input_blocked = false;
        return MENU_ACTION_NONE;
    }
    if (IsKeyPressed(KEY_ENTER)) return MENU_ACTION_CAMPAIGN;
    if (IsKeyPressed(KEY_ESCAPE)) return MENU_ACTION_EXIT;
    return MENU_ACTION_NONE;
}

static void draw_background(const MenuScreen *screen)
{
    Texture2D original = screen->background;
    if (!original.id) return;
    bool animated = screen->grass_background.id && screen->grass_atlas.id;
    bool trees_animated = animated && screen->tree_background.id && screen->tree_atlas.id;
    Texture2D texture = trees_animated ? screen->tree_background : animated ? screen->grass_background : original;
    float scale = fmaxf(1280.0f / original.width, 720.0f / original.height);
    float width = original.width * scale, height = original.height * scale;
    Rectangle transform = {roundf((1280 - width) / 2), roundf((720 - height) / 2), width, height};
    DrawTexturePro(texture, (Rectangle){0, 0, (float)texture.width, (float)texture.height},
                   transform, (Vector2){0}, 0, WHITE);
    if (!animated) return;
    int frame = (int)(screen->grass_time * MENU_GRASS_FPS) % MENU_GRASS_FRAME_COUNT;
    if (trees_animated) {
        for (int i = 0; i < MENU_TREE_REGION_COUNT; i++) {
            const MenuTreeRegion *region = &MENU_TREE_REGIONS[i];
            Rectangle source = region->poses[region->frames[frame]];
            Rectangle dest = {transform.x + region->origin.x * scale, transform.y + region->origin.y * scale,
                              source.width * scale, source.height * scale};
            DrawTexturePro(screen->tree_atlas, source, dest, (Vector2){0}, 0, WHITE);
        }
    }
    for (int i = 0; i < MENU_GRASS_REGION_COUNT; i++) {
        const MenuGrassRegion *region = &MENU_GRASS_REGIONS[i];
        Rectangle source = region->source;
        source.x += (frame % MENU_GRASS_COLUMNS) * MENU_GRASS_PAGE_WIDTH;
        source.y += (frame / MENU_GRASS_COLUMNS) * MENU_GRASS_PAGE_HEIGHT;
        /* Origem e tamanho usam a mesma transformação do fundo, sem arredondar patches. */
        Rectangle dest = {transform.x + region->origin.x * scale, transform.y + region->origin.y * scale,
                          region->source.width * scale, region->source.height * scale};
        DrawTexturePro(screen->grass_atlas, source, dest, (Vector2){0}, 0, WHITE);
    }
}

static void draw_art(Texture2D texture, Rectangle source, int center, int y, float height)
{
    if (!texture.id) return;
    float scale = height / source.height;
    float width = source.width * scale;
    DrawTexturePro(texture, source, (Rectangle){roundf(center - width / 2), (float)y, width, height},
                   (Vector2){0}, 0, WHITE);
}

static void draw_mascot(const MenuScreen *screen)
{
    if (!screen->mascot_idle.id) {
        draw_art(screen->mascot, screen->mascot_source, 640, 374, 240);
        return;
    }
    int frame = (int)(screen->mascot_time * MENU_MASCOT_FPS) % MENU_MASCOT_FRAME_COUNT;
    Rectangle source = {(frame % MENU_MASCOT_COLUMNS) * MENU_MASCOT_FRAME_SIZE,
                        (frame / MENU_MASCOT_COLUMNS) * MENU_MASCOT_FRAME_SIZE,
                        MENU_MASCOT_FRAME_SIZE, MENU_MASCOT_FRAME_SIZE};
    float scale = 240.0f / MENU_MASCOT_CROP_HEIGHT;
    /* Margens transparentes não alteram a escala nem o pivô da arte aprovada. */
    Rectangle dest = {roundf(640 - MENU_MASCOT_CROP_WIDTH * scale / 2) - MENU_MASCOT_OFFSET_X * scale,
                      374 - MENU_MASCOT_OFFSET_Y * scale,
                      MENU_MASCOT_FRAME_SIZE * scale, MENU_MASCOT_FRAME_SIZE * scale};
    DrawTexturePro(screen->mascot_idle, source, dest, (Vector2){0}, 0, WHITE);
}

void menu_screen_draw(const UiTheme *ui, const MenuScreen *screen, const SelectionUiAssets *fonts)
{
    ClearBackground((Color){118, 143, 67, 255});
    draw_background(screen);
    int logo_offset = (int)roundf(4.0f * sinf(2.0f * PI * screen->ui_time / 3.0f));
    if (screen->logo.id) draw_art(screen->logo, screen->logo_source, 640, 28 + logo_offset, 324);
    else selection_ui_pixel_box(ui, fonts, "Plants x Plants", (Rectangle){200,90+logo_offset,880,80}, 72, INK);
    if (screen->mascot.id || screen->mascot_idle.id) {
        /* Sombra pequena em degraus, sem elipse suavizada nem halo. */
        DrawRectangle(583, 607, 114, 6, (Color){124, 110, 53, 255});
        DrawRectangle(595, 613, 90, 3, (Color){124, 110, 53, 255});
        draw_mascot(screen);
    }
    if (fmodf(screen->ui_time, 1.2f) < 0.8f) {
        selection_ui_pixel_box(ui, fonts, "Pressione ENTER", (Rectangle){402,646,480,40},
                               30, (Color){242,232,188,255});
        selection_ui_pixel_box(ui, fonts, "Pressione ENTER", (Rectangle){400,644,480,40}, 30, INK);
    }
}

void menu_screen_shutdown(MenuScreen *screen)
{
    if (screen->background.id) UnloadTexture(screen->background);
    if (screen->grass_background.id) UnloadTexture(screen->grass_background);
    if (screen->grass_atlas.id) UnloadTexture(screen->grass_atlas);
    if (screen->tree_background.id) UnloadTexture(screen->tree_background);
    if (screen->tree_atlas.id) UnloadTexture(screen->tree_atlas);
    if (screen->logo.id) UnloadTexture(screen->logo);
    if (screen->mascot.id) UnloadTexture(screen->mascot);
    if (screen->mascot_idle.id) UnloadTexture(screen->mascot_idle);
    *screen = (MenuScreen){0};
}

void menu_screen_reset_input(MenuScreen *screen)
{
    screen->input_blocked = true;
}
