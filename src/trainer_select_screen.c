/* Apresenta retratos de seleção e devolve ações ao coordenador de telas. */
#include "trainer_select_screen.h"
#include <math.h>

#define PORTRAIT_WIDTH 320
#define PORTRAIT_HEIGHT 480
#define BREATH_FRAMES 32
#define BREATH_COLUMNS 8
#define BREATH_SECONDS 5.12f
#define CONFIRM_CONTROL 2
#define BACK_CONTROL 3

static const Color CREAM = {230, 216, 181, 255};
static const Color COPPER = {174, 119, 72, 255};
static const Color OLIVE = {137, 148, 96, 255};

static Rectangle control_bounds(int index)
{
    if (index < TRAINER_COUNT) return (Rectangle){280 + index * 384.0f, 120, 336, 510};
    return (Rectangle){index == CONFIRM_CONTROL ? 660 : 380, 644, 240, 52};
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
    if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

void trainer_select_assets_load(TrainerSelectAssets *assets)
{
    const char *names[] = {"male", "female"};
    *assets = (TrainerSelectAssets){0};
    forest_scene_assets_load(&assets->forest);
    assets->frame = load_portrait("assets/sprites/selection/prepared/portrait_frame.png", 1024, 1536);
    for (int i = 0; i < TRAINER_COUNT; i++) {
        assets->portraits[i] = load_portrait(TextFormat("assets/sprites/trainers/%s_portrait.png", names[i]),
                                            PORTRAIT_WIDTH, PORTRAIT_HEIGHT);
        /* A fonte original também funciona se o derivado estático estiver ausente. */
        if (!assets->portraits[i].id)
            assets->portraits[i] = load_portrait(TextFormat("assets/sprites/trainer_%s_portrait.png", names[i]), 0, 0);
        assets->breathing[i] = load_portrait(TextFormat("assets/sprites/trainers/%s_portrait_breath.png", names[i]),
                                            PORTRAIT_WIDTH * BREATH_COLUMNS,
                                            PORTRAIT_HEIGHT * (BREATH_FRAMES / BREATH_COLUMNS));
        if (!assets->breathing[i].id)
            TraceLog(LOG_INFO, "Respiracao de %s indisponivel; usando retrato estatico.", names[i]);
    }
}

void trainer_select_assets_unload(TrainerSelectAssets *assets)
{
    forest_scene_assets_unload(&assets->forest);
    if (assets->frame.id) UnloadTexture(assets->frame);
    for (int i = 0; i < TRAINER_COUNT; i++) {
        if (assets->portraits[i].id) UnloadTexture(assets->portraits[i]);
        if (assets->breathing[i].id) UnloadTexture(assets->breathing[i]);
    }
    *assets = (TrainerSelectAssets){0};
}

void trainer_select_screen_init(TrainerSelectScreen *screen)
{
    *screen = (TrainerSelectScreen){.selected = TRAINER_MALE, .pressed = -1, .hovered = -1,
                                   .emphasis = {1, 0}};
    forest_scene_init(&screen->forest);
}

TrainerSelectAction trainer_select_screen_update(TrainerSelectScreen *screen, bool accept_input, float dt)
{
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    forest_scene_update(&screen->forest, dt);
    screen->elapsed_time = fmodf(screen->elapsed_time + dt, BREATH_SECONDS);
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

static void draw_portrait(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets, int index)
{
    Texture2D texture = assets->breathing[index];
    Rectangle source = {0, 0, PORTRAIT_WIDTH, PORTRAIT_HEIGHT};
    /* A abertura da moldura limita a arte; os pés seguem o mesmo pivô. */
    float scale = 0.83f;
    Rectangle dest = {448 + index * 384.0f - PORTRAIT_WIDTH * scale / 2, 166,
                      PORTRAIT_WIDTH * scale, PORTRAIT_HEIGHT * scale};
    if (texture.id) {
        float phase = screen->elapsed_time / BREATH_SECONDS + index * 0.375f;
        int frame = (int)(phase * BREATH_FRAMES) % BREATH_FRAMES;
        source.x = (frame % BREATH_COLUMNS) * PORTRAIT_WIDTH;
        source.y = (frame / BREATH_COLUMNS) * PORTRAIT_HEIGHT;
    } else {
        texture = assets->portraits[index];
        source = (Rectangle){0, 0, (float)texture.width, (float)texture.height};
        if (texture.id && (texture.width != PORTRAIT_WIDTH || texture.height != PORTRAIT_HEIGHT)) {
            /* Fallback da fonte: escala uniforme, sem cortar chapéu ou botas. */
            float fallback_scale = fminf(245.0f / texture.width, 377.0f / texture.height);
            dest.width = texture.width * fallback_scale;
            dest.height = texture.height * fallback_scale;
            dest.x = 448 + index * 384.0f - dest.width / 2;
            dest.y = 553 - dest.height;
        }
    }
    if (texture.id) {
        unsigned char light = (unsigned char)(229 + 26 * screen->emphasis[index]);
        DrawTexturePro(texture, source, dest, (Vector2){0}, 0, (Color){light, light, light, 255});
    } else centered_text("Retrato indisponivel", 448 + index * 384, 350, 20, CREAM);
}

void trainer_select_screen_draw(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets,
                               Texture2D buttons)
{
    forest_scene_draw(&screen->forest, &assets->forest);
    DrawRectangleGradientV(240, 22, 800, 94, Fade((Color){23, 37, 27, 255}, 0.84f), Fade((Color){23, 37, 27, 255}, 0.67f));
    DrawLine(160, 76, 275, 76, COPPER);
    DrawLine(1005, 76, 1120, 76, COPPER);
    centered_text("Escolha seu explorador", 640, 43, 36, CREAM);
    centered_text("Uma nova trilha começa com você.", 640, 89, 18, (Color){165, 177, 145, 255});
    const char *names[] = {"Explorador", "Exploradora"};
    for (int i = 0; i < TRAINER_COUNT; i++) {
        Rectangle bounds = control_bounds(i);
        float emphasis = screen->emphasis[i];
        /* Sombra local somente na abertura; mantém o cenário visível através dela. */
        DrawRectangleGradientV((int)bounds.x + 42, 164, (int)bounds.width - 84, 390,
                              Fade((Color){31, 44, 31, 255}, 0.52f), Fade((Color){31, 44, 31, 255}, 0.70f));
        int center = 448 + i * 384;
        DrawEllipse(center, 552, 60, 6, Fade(BLACK, 0.22f));
        draw_portrait(screen, assets, i);
        if (assets->frame.id) {
            float width = bounds.height * assets->frame.width / assets->frame.height;
            Rectangle dest = {center - width / 2, bounds.y, width, bounds.height};
            Color tint = {(unsigned char)(223 + 32 * emphasis), (unsigned char)(224 + 23 * emphasis),
                          (unsigned char)(207 + 12 * emphasis), 255};
            DrawTexturePro(assets->frame, (Rectangle){0, 0, (float)assets->frame.width, (float)assets->frame.height},
                           dest, (Vector2){0}, 0, tint);
        } else {
            DrawRectangleLinesEx(bounds, 2, Fade(OLIVE, 0.4f + emphasis * 0.6f));
            DrawRectangle(center - 120, 574, 240, 46, (Color){73, 48, 30, 255});
        }
        centered_text(names[i], center + 1, 588, 22, Fade(BLACK, 0.7f));
        centered_text(names[i], center, 587, 22, CREAM);
        DrawCircle(center + 104, 600, 3, Fade(OLIVE, emphasis));
    }
    for (int i = CONFIRM_CONTROL; i <= BACK_CONTROL; i++) {
        Rectangle bounds = control_bounds(i);
        bool hover = screen->hovered == i;
        bool pressed = hover && screen->pressed == i;
        int row = pressed ? 2 : (hover || i == CONFIRM_CONTROL ? 1 : 0);
        if (buttons.id) {
            DrawTexturePro(buttons, (Rectangle){70, row == 0 ? 60 : row == 1 ? 367 : 677, 1116, 254},
                           bounds, (Vector2){0}, 0, WHITE);
        } else {
            DrawRectangleRec(bounds, (Color){77, 55, 38, 255});
            DrawRectangleLinesEx(bounds, 1, hover ? OLIVE : COPPER);
        }
        centered_text(i == CONFIRM_CONTROL ? "Confirmar" : "Voltar", (int)(bounds.x + bounds.width / 2),
                      (int)bounds.y + 16 + (pressed ? 1 : 0), 22, CREAM);
    }
    DrawRectangleGradientV(260, 698, 760, 22, Fade((Color){23, 37, 27, 255}, 0.76f), Fade((Color){23, 37, 27, 255}, 0.9f));
    centered_text("Setas / A-D: escolher     |     ENTER: confirmar     |     ESC: voltar", 640, 704, 14,
                  (Color){165, 177, 145, 255});
}
