/* Apresenta retratos de seleção e devolve ações ao coordenador de telas. */
#include "trainer_select_screen.h"
#include "selection_ui.h"
#include "selection_portrait_v2_data.h"
#include "male_portrait_idle_data.h"
#include "female_portrait_idle_data.h"
#include <math.h>

#define PORTRAIT_WIDTH 320
#define PORTRAIT_HEIGHT 480
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
    trainer_background_load(&assets->background);
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
    assets->female_idle = load_portrait("assets/sprites/trainers/female_portrait_idle.png",
                                       FEMALE_PORTRAIT_IDLE_WIDTH*FEMALE_PORTRAIT_IDLE_FRAMES,
                                       FEMALE_PORTRAIT_IDLE_HEIGHT);
    assets->female_idle_halo = load_portrait("assets/sprites/trainers/female_portrait_idle_halo.png",
                                            SELECTION_PORTRAIT_V2_WIDTH, SELECTION_PORTRAIT_V2_HEIGHT);
    if (!assets->female_idle.id || !assets->female_idle_halo.id) {
        if (assets->female_idle.id) UnloadTexture(assets->female_idle);
        if (assets->female_idle_halo.id) UnloadTexture(assets->female_idle_halo);
        assets->female_idle = assets->female_idle_halo = (Texture2D){0};
        TraceLog(LOG_WARNING, "Repouso feminino indisponivel; usando retrato estatico.");
    }
}

void trainer_select_assets_unload(TrainerSelectAssets *assets)
{
    trainer_background_unload(&assets->background);
    if (assets->male_idle.id) UnloadTexture(assets->male_idle);
    if (assets->male_idle_halo.id) UnloadTexture(assets->male_idle_halo);
    if (assets->female_idle.id) UnloadTexture(assets->female_idle);
    if (assets->female_idle_halo.id) UnloadTexture(assets->female_idle_halo);
    for (int i = 0; i < TRAINER_COUNT; i++) {
        if (assets->portraits[i].id) UnloadTexture(assets->portraits[i]);
    }
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

static void draw_portrait_texture(Texture2D texture, Rectangle source, Rectangle body, int index, Rectangle card, float alpha)
{
    if (texture.id && body.height > 0 && body.width > 0) {
        /* Escala e pés dependem do corpo; margens e halo não entram na medida. */
        float scale = fminf(360.0f / body.height, 272.0f / body.width);
        Rectangle dest = {roundf(448 + index*384.0f - (body.x + body.width/2)*scale),
                          roundf(526 - (body.y + body.height)*scale),
                          source.width*scale, source.height*scale};
        Rectangle original=trainer_select_card_bounds((Trainer)index);
        float factor=card.height/original.height;
        dest=(Rectangle){roundf(card.x+(dest.x-original.x)*factor),roundf(card.y+(dest.y-original.y)*factor),
                         dest.width*factor,dest.height*factor};
        DrawTexturePro(texture, source, dest, (Vector2){0}, 0, Fade(WHITE,alpha));
    } else centered_text("Retrato indisponivel", (int)roundf(card.x+card.width/2),
                         (int)roundf(card.y+224*card.height/480),20,Fade(INK,alpha));
}

static void draw_portrait(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets, int index, Rectangle card, float alpha)
{
    if (index == TRAINER_MALE && assets->male_idle.id && assets->male_idle_halo.id) {
        Texture2D halo = assets->male_idle_halo;
        draw_portrait_texture(halo, (Rectangle){0,0,(float)halo.width,(float)halo.height},
                              SELECTION_PORTRAIT_V2_BOUNDS[TRAINER_MALE], index, card, alpha);
        int frame = trainer_select_male_idle_frame(screen->male_idle_time);
        Rectangle source = {frame*MALE_PORTRAIT_IDLE_WIDTH,0,
                            MALE_PORTRAIT_IDLE_WIDTH,MALE_PORTRAIT_IDLE_HEIGHT};
        draw_portrait_texture(assets->male_idle, source, MALE_PORTRAIT_IDLE_BODY, index, card, alpha);
    } else if (index == TRAINER_FEMALE && assets->female_idle.id && assets->female_idle_halo.id) {
        Texture2D halo = assets->female_idle_halo;
        draw_portrait_texture(halo, (Rectangle){0,0,(float)halo.width,(float)halo.height},
                              SELECTION_PORTRAIT_V2_BOUNDS[TRAINER_FEMALE], index, card, alpha);
        int frame = trainer_select_female_idle_frame(screen->female_idle_time);
        Rectangle source = {frame*FEMALE_PORTRAIT_IDLE_WIDTH,0,
                            FEMALE_PORTRAIT_IDLE_WIDTH,FEMALE_PORTRAIT_IDLE_HEIGHT};
        draw_portrait_texture(assets->female_idle, source, FEMALE_PORTRAIT_IDLE_BODY, index, card, alpha);
    } else {
        Texture2D texture = assets->portraits[index];
        draw_portrait_texture(texture, (Rectangle){0,0,(float)texture.width,(float)texture.height},
                              assets->portrait_bounds[index], index, card, alpha);
    }
}

Rectangle trainer_select_card_bounds(Trainer trainer)
{
    return control_bounds((int)trainer);
}

void trainer_select_draw_card(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets,
                              Trainer trainer, Rectangle bounds, float alpha, bool selected, float label_alpha)
{
    const char *label=trainer==TRAINER_MALE ? "Homem" : "Mulher";
    pixel_panel(bounds,8,Fade(selected ? INK : BORDER,alpha));
    int border=selected ? 8 : 4;
    Rectangle inner={bounds.x+border,bounds.y+border,bounds.width-2*border,bounds.height-2*border};
    pixel_panel(inner,8,Fade(CREAM,alpha));
    if (selected) selection_ui_accents(bounds,alpha);
    draw_portrait(screen,assets,(int)trainer,bounds,alpha);
    int center=(int)roundf(bounds.x+bounds.width/2);
    float factor=bounds.height/480;
    int y=(int)roundf(bounds.y+432*factor);
    centered_text(label,center,y,30,Fade(INK,alpha*label_alpha));
    if (selected) selection_ui_cursor(center-MeasureText(label,30)/2-12,y+15,screen->background_time,alpha*label_alpha);
}

void trainer_select_draw_chrome(const TrainerSelectScreen *screen, float alpha)
{
    pixel_panel((Rectangle){260,28,760,64},4,Fade(INK,alpha));
    pixel_panel((Rectangle){264,32,752,56},4,Fade(CREAM,alpha));
    centered_text("Escolha seu personagem",640,40,40,Fade(INK,alpha));
    for (int i=CONFIRM_CONTROL;i<=BACK_CONTROL;i++) {
        Rectangle bounds=control_bounds(i);
        bool hover=screen->hovered==i;
        bool pressed=hover && screen->pressed==i;
        pixel_panel(bounds,4,Fade(INK,alpha));
        Rectangle inner={bounds.x+4,bounds.y+4,bounds.width-8,bounds.height-8};
        Color fill=pressed ? (Color){212,211,155,255} : hover ? (Color){232,227,176,255} : CREAM;
        pixel_panel(inner,4,Fade(fill,alpha));
        if (hover) DrawRectangle((int)bounds.x+16,(int)bounds.y+12,4,28,Fade(ACCENT_ORANGE,alpha));
        centered_text(i==CONFIRM_CONTROL ? "Confirmar" : "Voltar",(int)(bounds.x+bounds.width/2),(int)bounds.y+12,30,Fade(INK,alpha));
    }
    pixel_panel((Rectangle){224,688,832,28},4,Fade(INK,alpha));
    centered_text("Setas / A-D: escolher  |  ENTER: confirmar  |  ESC: voltar",640,692,20,Fade(CREAM,alpha));
}

void trainer_select_screen_draw(const TrainerSelectScreen *screen, const TrainerSelectAssets *assets)
{
    trainer_background_draw(&assets->background,screen->background_time);
    if (screen->background_preview) return;
    trainer_select_draw_chrome(screen,1);
    for (int i=0;i<TRAINER_COUNT;i++)
        trainer_select_draw_card(screen,assets,(Trainer)i,trainer_select_card_bounds((Trainer)i),1,screen->selected==(Trainer)i,1);
}
