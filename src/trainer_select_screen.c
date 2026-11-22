/* Apresenta retratos de seleção e devolve ações ao coordenador de telas. */
#include "trainer_select_screen.h"
#include "ui_theme.h"
#include "selection_ui.h"
#include "male_portrait_idle_data.h"
#include "female_portrait_idle_data.h"
#include <math.h>

#define CONFIRM_CONTROL 2
#define BACK_CONTROL 3

#define CREAM SELECTION_CREAM
#define INK SELECTION_INK
#define BORDER SELECTION_BORDER
#define ACCENT_ORANGE SELECTION_ORANGE
#define pixel_panel selection_ui_panel
#define centered_text selection_ui_text

static Rectangle control_bounds(int index)
{
    if (index < TRAINER_COUNT) return (Rectangle){300 + index * 360.0f, 145, 320, 456};
    return selection_ui_footer_bounds(index == CONFIRM_CONTROL);
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

void trainer_select_assets_load(TrainerSelectAssets *assets)
{
    *assets = (TrainerSelectAssets){0};
    trainer_background_load(&assets->background);
    selection_ui_assets_load(&assets->ui);
    assets->male_idle = load_portrait("assets/sprites/trainers/male_portrait_idle.png",
                                     MALE_PORTRAIT_IDLE_WIDTH * MALE_PORTRAIT_IDLE_FRAMES,
                                     MALE_PORTRAIT_IDLE_HEIGHT);
    assets->female_idle = load_portrait("assets/sprites/trainers/female_portrait_idle.png",
                                       FEMALE_PORTRAIT_IDLE_WIDTH * FEMALE_PORTRAIT_IDLE_FRAMES,
                                       FEMALE_PORTRAIT_IDLE_HEIGHT);
    if (!assets->male_idle.id || !assets->female_idle.id)
        TraceLog(LOG_WARNING, "Retrato de repouso indisponivel; exibindo aviso no cartao.");
}

void trainer_select_assets_unload(TrainerSelectAssets *assets)
{
    trainer_background_unload(&assets->background);
    selection_ui_assets_unload(&assets->ui);
    if (assets->male_idle.id) UnloadTexture(assets->male_idle);
    if (assets->female_idle.id) UnloadTexture(assets->female_idle);
    *assets = (TrainerSelectAssets){0};
}

void trainer_select_screen_init(TrainerSelectScreen *screen)
{
    *screen = (TrainerSelectScreen){.selected = TRAINER_MALE, .pressed = -1, .hovered = -1,
                                   .female_idle_time = FEMALE_PORTRAIT_IDLE_PHASE_OFFSET};
}

static int portrait_idle_frame(float elapsed, const float durations[4], float seconds)
{
    float phase = fmodf(fmaxf(0, elapsed), seconds);
    float end = 0;
    for (int i = 0; i < 4; i++) {
        end += durations[i];
        if (phase < end) return i;
    }
    return 0;
}

int trainer_select_male_idle_frame(float elapsed)
{
    return portrait_idle_frame(elapsed, MALE_PORTRAIT_IDLE_DURATIONS, MALE_PORTRAIT_IDLE_SECONDS);
}

int trainer_select_female_idle_frame(float elapsed)
{
    return portrait_idle_frame(elapsed, FEMALE_PORTRAIT_IDLE_DURATIONS, FEMALE_PORTRAIT_IDLE_SECONDS);
}

TrainerSelectAction trainer_select_screen_update(TrainerSelectScreen *screen, bool accept_input, float dt)
{
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    screen->background_time = fmodf(screen->background_time + dt, 128.0f);
    screen->male_idle_time = fmodf(screen->male_idle_time + dt, MALE_PORTRAIT_IDLE_SECONDS);
    screen->female_idle_time = fmodf(screen->female_idle_time + dt, FEMALE_PORTRAIT_IDLE_SECONDS);
    if (accept_input && IsKeyPressed(KEY_F7)) {
        screen->background_preview = !screen->background_preview;
        screen->pressed = -1;
    }
    if (screen->background_preview) {
        screen->pressed = screen->hovered = -1;
        return accept_input && IsKeyPressed(KEY_ESCAPE) ? TRAINER_SELECT_BACK : TRAINER_SELECT_NONE;
    }
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
    return action;
}

static void draw_portrait_texture(const UiTheme *ui, const SelectionUiAssets *assets,
                                  Texture2D texture, Rectangle source, Rectangle body,
                                  Rectangle card, float alpha)
{
    if (texture.id && body.height > 0 && body.width > 0) {
        /* Escala e pés dependem do corpo; margens e halo não entram na medida. */
        float scale = fminf(card.height * 0.70f / body.height, card.width * 0.66f / body.width);
        Rectangle dest = {roundf(card.x + card.width/2 - (body.x + body.width/2)*scale),
                          roundf(card.y + card.height * 0.80f - (body.y + body.height)*scale),
                          source.width*scale, source.height*scale};
        DrawTexturePro(texture, source, dest, (Vector2){0}, 0, Fade(WHITE,alpha));
    } else selection_ui_pixel_box(ui, assets, "Retrato indisponível",
                                  (Rectangle){card.x+24, card.y+card.height/2, card.width-48, 40},
                                  20, Fade(INK, alpha));
}

static void draw_portrait(const UiTheme *ui, const TrainerSelectScreen *screen, const TrainerSelectAssets *assets, int index, Rectangle card, float alpha)
{
    if (index == TRAINER_MALE) {
        int frame = trainer_select_male_idle_frame(screen->male_idle_time);
        Rectangle source = {frame * MALE_PORTRAIT_IDLE_WIDTH, 0,
                            MALE_PORTRAIT_IDLE_WIDTH, MALE_PORTRAIT_IDLE_HEIGHT};
        draw_portrait_texture(ui, &assets->ui, assets->male_idle, source, MALE_PORTRAIT_IDLE_BODY, card, alpha);
    } else {
        int frame = trainer_select_female_idle_frame(screen->female_idle_time);
        Rectangle source = {frame * FEMALE_PORTRAIT_IDLE_WIDTH, 0,
                            FEMALE_PORTRAIT_IDLE_WIDTH, FEMALE_PORTRAIT_IDLE_HEIGHT};
        draw_portrait_texture(ui, &assets->ui, assets->female_idle, source, FEMALE_PORTRAIT_IDLE_BODY, card, alpha);
    }
}

Rectangle trainer_select_card_bounds(Trainer trainer)
{
    return control_bounds((int)trainer);
}

void trainer_select_draw_card(const UiTheme *ui, const TrainerSelectScreen *screen, const TrainerSelectAssets *assets,
                              Trainer trainer, Rectangle bounds, float alpha, bool selected, float label_alpha)
{
    const char *label=trainer==TRAINER_MALE ? "Homem" : "Mulher";
    selection_ui_skin(&assets->ui, selected ? SELECTION_PORTRAIT_SELECTED : SELECTION_PORTRAIT_NORMAL,
                       bounds, alpha);
    draw_portrait(ui, screen,assets,(int)trainer,bounds,alpha);
    float factor = bounds.height / 456;
    Rectangle plate = {bounds.x+bounds.width/2-100*factor, bounds.y+378*factor, 200*factor, 42*factor};
    if (label_alpha > 0) {
        pixel_panel(plate, 3, Fade(SELECTION_ORANGE, alpha*label_alpha));
        pixel_panel((Rectangle){plate.x+2, plate.y+2, plate.width-4, plate.height-4},
                     3, Fade((Color){255,245,224,255}, alpha*label_alpha));
        selection_ui_pixel_box(ui, &assets->ui, label, plate, 30, Fade(INK, alpha*label_alpha));
    }
    if (selected && label_alpha > 0)
        selection_ui_skin(&assets->ui, SELECTION_POINTER,
            (Rectangle){bounds.x-23, bounds.y+bounds.height/2-13, 19, 26}, alpha*label_alpha);
}

void trainer_select_draw_chrome(const UiTheme *ui, const TrainerSelectScreen *screen,
                                const TrainerSelectAssets *assets, float alpha)
{
    selection_ui_header(ui, &assets->ui, "Escolha seu personagem", "Quem vai começar esta jornada?",
                         OPENING_CHARACTER, alpha);
    for (int i=CONFIRM_CONTROL;i<=BACK_CONTROL;i++) {
        Rectangle bounds=control_bounds(i);
        bool hover=screen->hovered==i;
        bool pressed=hover && screen->pressed==i;
        selection_ui_button(ui, &assets->ui, bounds,
            i==CONFIRM_CONTROL ? "Confirmar · ENTER" : "Voltar · ESC", hover, pressed, alpha);
    }
    selection_ui_pixel_box(ui, &assets->ui, "Setas / A-D: escolher",
                           (Rectangle){350,643,470,28}, 20, Fade((Color){253,245,216,255},alpha));
}

void trainer_select_screen_draw(const UiTheme *ui, const TrainerSelectScreen *screen, const TrainerSelectAssets *assets)
{
    if (screen->background_preview) {
        trainer_background_draw(&assets->background,screen->background_time);
        return;
    }
    trainer_background_draw_dimmed(&assets->background,screen->background_time);
    trainer_select_draw_chrome(ui, screen,assets,1);
    for (int i=0;i<TRAINER_COUNT;i++)
        trainer_select_draw_card(ui, screen,assets,(Trainer)i,trainer_select_card_bounds((Trainer)i),1,screen->selected==(Trainer)i,1);
}
