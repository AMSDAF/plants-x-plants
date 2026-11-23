/* Reutiliza fundo, molduras e sprites da abertura para os resultados da campanha. */
#include "result_screen.h"
#include <math.h>

Rectangle result_screen_button_bounds(int index)
{
    return (Rectangle){index == 0 ? 160 : 680, 610, 440, 64};
}

void result_screen_begin(ResultScreen *screen)
{
    *screen = (ResultScreen){0};
}

ResultAction result_screen_update(ResultScreen *screen, bool accept_input, float dt)
{
    screen->elapsed += fminf(0.1f, fmaxf(0, dt));
    if (!accept_input) return RESULT_NONE;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_A) ||
        IsKeyPressed(KEY_D) || IsKeyPressed(KEY_TAB)) screen->cursor = 1-screen->cursor;
    Vector2 mouse = GetMousePosition(), motion = GetMouseDelta();
    for (int i = 0; i < 2; i++) {
        bool hover = CheckCollisionPointRec(mouse,result_screen_button_bounds(i));
        if (hover && (motion.x != 0 || motion.y != 0)) screen->cursor = i;
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return i == 0 ? RESULT_PRIMARY : RESULT_SECONDARY;
    }
    if (IsKeyPressed(KEY_ESCAPE)) return RESULT_SECONDARY;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
        return screen->cursor == 0 ? RESULT_PRIMARY : RESULT_SECONDARY;
    return RESULT_NONE;
}

static void label(const UiTheme *ui, const SelectionUiAssets *fonts, const char *text,
                  Rectangle box, int size, Color color)
{
    selection_ui_pixel_box(ui,fonts,text,box,size,color);
}

/* A fonte e a arte são emprestadas; nenhuma textura é criada durante o desenho. */
void result_screen_draw(const UiTheme *ui, const SelectionUiAssets *fonts,
                        const TrainerBackgroundAssets *background, const IntroAssets *plants,
                        const ResultScreen *screen, const Team *team, const char *player_name,
                        bool defeated, const char *location)
{
    trainer_background_draw_dimmed(background,screen->elapsed);
    const Color ink = SELECTION_INK, cream = SELECTION_CREAM;
    label(ui,fonts,defeated ? "BATALHA ENCERRADA" : "ESTUFA LÓGICA · MISSÃO CONCLUÍDA",
        (Rectangle){140,35,1000,30},24,cream);
    label(ui,fonts,defeated ? "Sua equipe foi derrotada" : "A vila floresce de novo!",
        (Rectangle){100,76,1080,75},72,cream);
    ui_botanical_panel(ui,(Rectangle){120,175,1040,398});
    label(ui,fonts,defeated ? TextFormat("%s · %s",player_name,location)
                           : TextFormat("Obrigado, %s!",player_name),
        (Rectangle){160,198,960,42},36,ink);
    label(ui,fonts,defeated ? "Respire fundo. Sua equipe pode tentar outra vez."
                           : "Com lógica e uma boa equipe, você devolveu a vida à estufa.",
        (Rectangle){155,250,970,34},28,ink);
    label(ui,fonts,defeated ? "As plantas se recuperam e seu progresso na vila continua."
                           : "A vila está em boas mãos. Essa vitória também é delas!",
        (Rectangle){155,292,970,30},24,ink);
    int count = team->count > MAX_TEAM_SIZE ? MAX_TEAM_SIZE : team->count;
    if (count < 0) count = 0;
    float spacing = count > 0 ? fminf(250,900.0f/count) : 250;
    for (int i = 0; i < count; i++) {
        const Plant *plant = &team->plants[i];
        float center = 640+(i-(count-1)/2.0f)*spacing;
        DrawEllipse((int)center,510,66,12,(Color){204,199,143,255});
        if (plant->id >= 0 && plant->id < SPECIES_COUNT) {
            Texture2D texture = plants->plants[plant->id];
            Rectangle source = plants->sources[plant->id];
            if (texture.id && source.width > 0 && source.height > 0) {
                float scale = fminf(144/source.width,164/source.height);
                Rectangle box = {roundf(center-source.width*scale/2),roundf(512-source.height*scale),
                                 roundf(source.width*scale),roundf(source.height*scale)};
                DrawTexturePro(texture,source,box,(Vector2){0},0,WHITE);
            } else {
                DrawRectangle((int)center-4,407,8,98,(Color){84,116,48,255});
                DrawEllipse((int)center-18,425,25,12,(Color){116,151,64,255});
                DrawEllipse((int)center+18,454,25,12,(Color){116,151,64,255});
            }
        }
        label(ui,fonts,plant->name_pt,(Rectangle){center-spacing/2,531,spacing,26},24,ink);
    }
    for (int i = 0; i < 2; i++) {
        const char *text = defeated ? (i == 0 ? "Tentar novamente" : "Voltar à vila")
                                   : (i == 0 ? "Jogar novamente" : "Menu principal");
        selection_ui_button(ui,fonts,result_screen_button_bounds(i),text,screen->cursor == i,false,1);
    }
    label(ui,fonts,"Setas / A D: escolher · ENTER ou clique: confirmar · ESC: voltar",
        (Rectangle){100,687,1080,22},20,cream);
}
