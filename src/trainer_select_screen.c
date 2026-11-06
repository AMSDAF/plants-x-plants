/* Apresenta retratos de seleção e devolve ações ao coordenador de telas. */
#include "trainer_select_screen.h"
#include "selection_portrait_v2_data.h"
#include "male_portrait_idle_data.h"
#include <math.h>

#define PORTRAIT_WIDTH 320
#define PORTRAIT_HEIGHT 480
#define CONFIRM_CONTROL 2
#define BACK_CONTROL 3

static const Color CREAM = {247, 237, 198, 255};
static const Color INK = {24, 52, 30, 255};
static const Color BORDER = {53, 76, 43, 255};
static const Color ACCENT_ORANGE = {190, 93, 28, 255};

static Rectangle control_bounds(int index)
{
    if (index < TRAINER_COUNT) return (Rectangle){280 + index * 384.0f, 116, 336, 480};
    return (Rectangle){index == CONFIRM_CONTROL ? 660 : 380, 622, 240, 52};
}

static Texture2D load_portrait(const char *path, int width, int height)
{
    if (!FileExists(path)) return (Texture2D){0};
    Texture2D texture = LoadTexture(path);
    if (texture.id && ((width && texture.width != width) || (height && texture.height != height))) {
        TraceLog(LOG_WARNING, "Retrato/atlas invalido: %s", path);
        UnloadTexture(texture);
        return (Texture2D){0};
    }
    if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

static Rectangle visible_portrait_bounds(Texture2D texture)
{
    if (!texture.id) return (Rectangle){0, 0, PORTRAIT_WIDTH, PORTRAIT_HEIGHT};
    Image image = LoadImageFromTexture(texture);
    Rectangle bounds = GetImageAlphaBorder(image, 0.1f);
    UnloadImage(image);
    if (bounds.width <= 0 || bounds.height <= 0)
        return (Rectangle){0, 0, PORTRAIT_WIDTH, PORTRAIT_HEIGHT};
    return bounds;
}

void trainer_select_assets_load(TrainerSelectAssets *assets)
{
    const char *names[] = {"male", "female"};
    *assets = (TrainerSelectAssets){0};
    forest_scene_assets_load(&assets->forest);
    trainer_background_load(&assets->background);
    assets->frame = load_portrait("assets/sprites/selection/prepared/portrait_frame.png", 1024, 1536);
    /* Recurso exclusivo da seleção; o novo menu não usa botões de madeira. */
    assets->buttons = load_portrait("assets/sprites/menu_buttons.png", 1254, 1254);
    if (assets->buttons.id) SetTextureFilter(assets->buttons, TEXTURE_FILTER_POINT);
    for (int i = 0; i < TRAINER_COUNT; i++) {
        assets->portraits[i] = load_portrait(TextFormat("assets/sprites/trainers/%s_portrait_v2.png", names[i]),
                                            SELECTION_PORTRAIT_V2_WIDTH, SELECTION_PORTRAIT_V2_HEIGHT);
        if (assets->portraits[i].id) {
            /* O enquadramento corporal exclui o halo, mas o desenho preserva a textura inteira. */
            assets->portrait_bounds[i] = SELECTION_PORTRAIT_V2_BOUNDS[i];
            continue;
        }
        /* Fontes anteriores permanecem como fallback estático; nenhum atlas antigo é aplicado. */
        assets->portraits[i] = load_portrait(TextFormat("assets/sprites/trainers/%s_portrait.png", names[i]),
                                            PORTRAIT_WIDTH, PORTRAIT_HEIGHT);
        if (!assets->portraits[i].id)
            assets->portraits[i] = load_portrait(TextFormat("assets/sprites/trainer_%s_portrait.png", names[i]), 0, 0);
        assets->portrait_bounds[i] = visible_portrait_bounds(assets->portraits[i]);
        TraceLog(LOG_WARNING, "Retrato v2 de %s indisponivel; usando fallback estatico.", names[i]);
    }
    assets->male_idle = load_portrait("assets/sprites/trainers/male_portrait_idle.png",
                                     MALE_PORTRAIT_IDLE_WIDTH*MALE_PORTRAIT_IDLE_FRAMES,
                                     MALE_PORTRAIT_IDLE_HEIGHT);
    assets->male_idle_halo = load_portrait("assets/sprites/trainers/male_portrait_idle_halo.png",
                                          SELECTION_PORTRAIT_V2_WIDTH, SELECTION_PORTRAIT_V2_HEIGHT);
    if (!assets->male_idle.id || !assets->male_idle_halo.id) {
        if (assets->male_idle.id) UnloadTexture(assets->male_idle);
        if (assets->male_idle_halo.id) UnloadTexture(assets->male_idle_halo);
        assets->male_idle = assets->male_idle_halo = (Texture2D){0};
        TraceLog(LOG_WARNING, "Repouso masculino indisponivel; usando retrato estatico.");
    }
}

void trainer_select_assets_unload(TrainerSelectAssets *assets)
{
    forest_scene_assets_unload(&assets->forest);
    trainer_background_unload(&assets->background);
    if (assets->male_idle.id) UnloadTexture(assets->male_idle);
    if (assets->male_idle_halo.id) UnloadTexture(assets->male_idle_halo);
    if (assets->frame.id) UnloadTexture(assets->frame);
    if (assets->buttons.id) UnloadTexture(assets->buttons);
    for (int i = 0; i < TRAINER_COUNT; i++) {
        if (assets->portraits[i].id) UnloadTexture(assets->portraits[i]);
    }
    *assets = (TrainerSelectAssets){0};
}

void trainer_select_screen_init(TrainerSelectScreen *screen)
{
    *screen = (TrainerSelectScreen){.selected = TRAINER_MALE, .pressed = -1, .hovered = -1,
                                   .emphasis = {1, 0}};
    forest_scene_init(&screen->forest);
}

int trainer_select_male_idle_frame(float elapsed)
{
    float phase = fmodf(fmaxf(0, elapsed), MALE_PORTRAIT_IDLE_SECONDS);
    float end = 0;
    for (int i = 0; i < MALE_PORTRAIT_IDLE_FRAMES; i++) {
        end += MALE_PORTRAIT_IDLE_DURATIONS[i];
        if (phase < end) return i;
    }
    return 0;
}

TrainerSelectAction trainer_select_screen_update(TrainerSelectScreen *screen, bool accept_input, float dt)
{
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    screen->background_time = fmodf(screen->background_time + dt, 128.0f);
    screen->male_idle_time = fmodf(screen->male_idle_time + dt, MALE_PORTRAIT_IDLE_SECONDS);
    if (accept_input && IsKeyPressed(KEY_F7)) {
        screen->background_preview = !screen->background_preview;
        screen->pressed = -1;
    }
    if (screen->background_preview) {
        screen->pressed = screen->hovered = -1;
        return accept_input && IsKeyPressed(KEY_ESCAPE) ? TRAINER_SELECT_BACK : TRAINER_SELECT_NONE;
    }
    forest_scene_update(&screen->forest, dt);
    screen->hovered = -1;
    TrainerSelectAction action = TRAINER_SELECT_NONE;
    if (accept_input) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) screen->selected = TRAINER_MALE;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) screen->selected = TRAINER_FEMALE;
        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < 4; i++) {
            if (CheckCollisionPointRec(mouse, control_bounds(i))) screen->hovered = i;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            screen->pressed = screen->hovered;
            if (screen->pressed >= 0 && screen->pressed < TRAINER_COUNT)
                screen->selected = (Trainer)screen->pressed;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (screen->pressed == screen->hovered) {
                if (screen->pressed == CONFIRM_CONTROL) action = TRAINER_SELECT_CONFIRM;
                if (screen->pressed == BACK_CONTROL) action = TRAINER_SELECT_BACK;
            }
            screen->pressed = -1;
        }
        if (IsKeyPressed(KEY_ENTER)) action = TRAINER_SELECT_CONFIRM;
        if (IsKeyPressed(KEY_ESCAPE)) action = TRAINER_SELECT_BACK;
    } else screen->pressed = -1;
    for (int i = 0; i < TRAINER_COUNT; i++) {
        float target = screen->selected == (Trainer)i ? 1.0f : 0.0f;
        screen->emphasis[i] += (target - screen->emphasis[i]) * (1 - expf(-10 * dt));
    }
    return action;
}

static void centered_text(const char *text, int center, int y, int size, Color color)
{
    DrawText(text, center - MeasureText(text, size) / 2, y, size, color);
}

/* Retângulos em três faixas formam cantos em degraus, sem antialiasing. */
static void pixel_panel(Rectangle bounds, int corner, Color color)
{
    int x = (int)bounds.x, y = (int)bounds.y;
    int w = (int)bounds.width, h = (int)bounds.height;
    DrawRectangle(x + 2*corner, y, w - 4*corner, h, color);
    DrawRectangle(x + corner, y + corner, w - 2*corner, h - 2*corner, color);
    DrawRectangle(x, y + 2*corner, w, h - 4*corner, color);
}

static void draw_corner_accents(Rectangle bounds)
{
    int x = (int)bounds.x, y = (int)bounds.y;
    int w = (int)bounds.width, h = (int)bounds.height;
    for (int side = 0; side < 2; side++) {
        for (int bottom = 0; bottom < 2; bottom++) {
            int left = side ? x + w - 40 : x + 16;
            int top = bottom ? y + h - 40 : y + 16;
            DrawRectangle(left, bottom ? top + 18 : top, 24, 6, ACCENT_ORANGE);
            DrawRectangle(side ? left + 18 : left, top, 6, 24, ACCENT_ORANGE);
        }
    }
}

static void draw_cursor(int right, int center_y, float elapsed)
{
    /* Pulso discreto na grade inteira, com o mesmo ritmo do menu. */
    float pulse = 1.0f + 0.1f * sinf(2.0f * PI * elapsed);
    int width = 2 * (int)roundf(7 * pulse) + 1;
    int left = right - width;
    for (int step = 0; step < 5; step++) {
        int x0 = left + (int)roundf(step * width / 5.0f);
        int x1 = left + (int)roundf((step + 1) * width / 5.0f);
        int half_height = (int)roundf((10 - step*2) * pulse);
        DrawRectangle(x0, center_y - half_height, x1 - x0, half_height*2 + 1, INK);
    }
}

static void draw_portrait_texture(Texture2D texture, Rectangle source, Rectangle body, int index)
{
    if (texture.id && body.height > 0 && body.width > 0) {
        /* Escala e pés dependem do corpo; margens e halo não entram na medida. */
        float scale = fminf(360.0f / body.height, 272.0f / body.width);
        Rectangle dest = {roundf(448 + index*384.0f - (body.x + body.width/2)*scale),
                          roundf(526 - (body.y + body.height)*scale),
                          source.width*scale, source.height*scale};
        DrawTexturePro(texture, source, dest, (Vector2){0}, 0, WHITE);
    } else centered_text("Retrato indisponivel", 448 + index*384, 340, 20, INK);
}

static void draw_portrait(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets, int index)
{
    if (index == TRAINER_MALE && assets->male_idle.id && assets->male_idle_halo.id) {
        Texture2D halo = assets->male_idle_halo;
        draw_portrait_texture(halo, (Rectangle){0,0,(float)halo.width,(float)halo.height},
                              SELECTION_PORTRAIT_V2_BOUNDS[TRAINER_MALE], index);
        int frame = trainer_select_male_idle_frame(screen->male_idle_time);
        Rectangle source = {frame*MALE_PORTRAIT_IDLE_WIDTH,0,
                            MALE_PORTRAIT_IDLE_WIDTH,MALE_PORTRAIT_IDLE_HEIGHT};
        draw_portrait_texture(assets->male_idle, source, MALE_PORTRAIT_IDLE_BODY, index);
    } else {
        Texture2D texture = assets->portraits[index];
        draw_portrait_texture(texture, (Rectangle){0,0,(float)texture.width,(float)texture.height},
                              assets->portrait_bounds[index], index);
    }
}

void trainer_select_screen_draw(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets,
                               Texture2D buttons)
{
    /* A assinatura é mantida para os chamadores existentes; os controles são geométricos. */
    (void)buttons;
    trainer_background_draw(&assets->background, screen->background_time);
    if (screen->background_preview) return;

    pixel_panel((Rectangle){260, 28, 760, 64}, 4, INK);
    pixel_panel((Rectangle){264, 32, 752, 56}, 4, CREAM);
    centered_text("Escolha seu personagem", 640, 40, 40, INK);
    const char *names[] = {"Homem", "Mulher"};
    for (int i = 0; i < TRAINER_COUNT; i++) {
        Rectangle bounds = control_bounds(i);
        bool selected = screen->selected == (Trainer)i;
        pixel_panel(bounds, 8, selected ? INK : BORDER);
        int border = selected ? 8 : 4;
        Rectangle inner = {bounds.x + border, bounds.y + border,
                           bounds.width - 2*border, bounds.height - 2*border};
        pixel_panel(inner, 8, CREAM);
        if (selected) draw_corner_accents(bounds);
        draw_portrait(screen, assets, i);
        int center = 448 + i*384;
        centered_text(names[i], center, 548, 30, INK);
        if (selected) draw_cursor(center - MeasureText(names[i], 30)/2 - 12, 563, screen->background_time);
    }
    for (int i = CONFIRM_CONTROL; i <= BACK_CONTROL; i++) {
        Rectangle bounds = control_bounds(i);
        bool hover = screen->hovered == i;
        bool pressed = hover && screen->pressed == i;
        pixel_panel(bounds, 4, INK);
        Rectangle inner = {bounds.x + 4, bounds.y + 4, bounds.width - 8, bounds.height - 8};
        Color fill = pressed ? (Color){212, 211, 155, 255} :
                     hover ? (Color){232, 227, 176, 255} : CREAM;
        pixel_panel(inner, 4, fill);
        if (hover) DrawRectangle((int)bounds.x + 16, (int)bounds.y + 12, 4, 28, ACCENT_ORANGE);
        centered_text(i == CONFIRM_CONTROL ? "Confirmar" : "Voltar", (int)(bounds.x + bounds.width/2),
                      (int)bounds.y + 12, 30, INK);
    }
    pixel_panel((Rectangle){224, 688, 832, 28}, 4, INK);
    centered_text("Setas / A-D: escolher  |  ENTER: confirmar  |  ESC: voltar", 640, 692, 20, CREAM);
}
