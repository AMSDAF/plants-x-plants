/* Reutiliza fundo, molduras e sprites da abertura para os resultados da campanha. */
#include "result_screen.h"
#include "logic_quiz.h"
#include <math.h>

Rectangle result_screen_button_bounds(int index)
{
    return (Rectangle){index == 0 ? 160 : 680, 610, 440, 64};
}

void result_screen_begin(ResultScreen *screen)
{
    *screen = (ResultScreen){0};
}

void result_screen_begin_quiz(ResultScreen *screen, unsigned int *seen_mask)
{
    result_screen_begin(screen);
    screen->question_index = logic_quiz_pick(seen_mask);
    screen->quiz_open = true;
}

static Rectangle abandon_button(int index)
{
    return (Rectangle){index == 0 ? 245 : 655,410,380,64};
}

ResultAction result_screen_update(ResultScreen *screen, bool accept_input, float dt)
{
    screen->elapsed += fminf(0.1f, fmaxf(0, dt));
    ui_pointer_update(&screen->pointer,accept_input && !screen->confirm_blocked);
    if (!accept_input) return RESULT_NONE;
    if (screen->confirm_blocked) {
        if (!IsKeyDown(KEY_ENTER) && !IsKeyDown(KEY_SPACE) && !IsKeyDown(KEY_ESCAPE) &&
            !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) screen->confirm_blocked = false;
        return RESULT_NONE;
    }
    int answer = -1;
    if (screen->quiz_open) {
        if (ui_digit_pressed(1)) answer = 0;
        else if (ui_digit_pressed(2)) answer = 1;
    }
    if (ui_nav_left() || ui_nav_right() || ui_nav_up() || ui_nav_down() ||
        IsKeyPressed(KEY_TAB)) screen->cursor = 1-screen->cursor;
    Vector2 mouse = GetMousePosition(), motion = GetMouseDelta();
    for (int i = 0; i < 2; i++) {
        Rectangle box = screen->abandon_confirm ? abandon_button(i) : result_screen_button_bounds(i);
        if (CheckCollisionPointRec(mouse,box) && (motion.x != 0 || motion.y != 0)) screen->cursor = i;
        if (ui_pointer_clicked(&screen->pointer,box)) answer = i;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (screen->abandon_confirm) {
            screen->abandon_confirm = false;
            screen->cursor = 0;
            screen->pointer = (UiPointer){0};
            return RESULT_NONE;
        }
        if (!screen->quiz_open) answer = 1;
    }
    if (ui_confirm_pressed()) answer = screen->cursor;
    if (answer < 0) return RESULT_NONE;
    if (screen->abandon_confirm) {
        screen->abandon_confirm = false;
        screen->cursor = 0;
        return answer == 1 ? RESULT_SECONDARY : RESULT_NONE;
    }
    if (screen->quiz_open) {
        const LogicQuizQuestion *question = logic_quiz_question(screen->question_index);
        return question && ((answer == 0) == question->answer) ? RESULT_CORRECT : RESULT_WRONG;
    }
    if (answer == 1 && screen->defeat && !screen->campaign_lost) {
        screen->abandon_confirm = true;
        screen->confirm_blocked = true;
        screen->cursor = 0;
        screen->pointer = (UiPointer){0};
        return RESULT_NONE;
    }
    return answer == 0 ? RESULT_PRIMARY : RESULT_SECONDARY;
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
    if (defeated && screen->quiz_open) {
        const LogicQuizQuestion *question = logic_quiz_question(screen->question_index);
        if (!question) return;
        label(ui,fonts,"ÚLTIMA CHANCE · DESAFIO DE LÓGICA",(Rectangle){140,35,1000,30},24,cream);
        label(ui,fonts,"Conquiste sua revanche",(Rectangle){100,76,1080,75},72,cream);
        ui_botanical_panel(ui,(Rectangle){120,175,1040,398});
        label(ui,fonts,"Avalie a expressão usando os valores abaixo.",
            (Rectangle){160,200,960,42},28,ink);
        label(ui,fonts,TextFormat("P = %s",question->p ? "Verdadeiro" : "Falso"),
            (Rectangle){question->has_q ? 180 : 400,270,480,48},36,ink);
        if (question->has_q)
            label(ui,fonts,TextFormat("Q = %s",question->q ? "Verdadeiro" : "Falso"),
                (Rectangle){660,270,440,48},36,ink);
        int size = selection_ui_measure(ui,fonts,question->expression,72) <= 920 ? 72 : 36;
        label(ui,fonts,question->expression,(Rectangle){180,350,920,84},size,ink);
        label(ui,fonts,"Essa expressão é verdadeira?",(Rectangle){180,452,920,42},36,ink);
        label(ui,fonts,"Acertou: revanche. Errou: todo o progresso será perdido.",
            (Rectangle){155,521,970,28},24,ink);
        for (int i = 0; i < 2; i++)
            selection_ui_button(ui,fonts,result_screen_button_bounds(i),
                i == 0 ? "1 · Verdadeiro" : "2 · Falso",screen->cursor == i,false,1);
        label(ui,fonts,"1 / 2: responder · Setas: escolher · ENTER ou clique: confirmar",
            (Rectangle){100,687,1080,22},20,cream);
        return;
    }
    label(ui,fonts,defeated ? "BATALHA ENCERRADA" : "ESTUFA LÓGICA · MISSÃO CONCLUÍDA",
        (Rectangle){140,35,1000,30},24,cream);
    label(ui,fonts,defeated ? (screen->campaign_lost ? "Sua campanha terminou" : "Sua equipe foi derrotada") : "A vila floresce de novo!",
        (Rectangle){100,76,1080,75},72,cream);
    ui_botanical_panel(ui,(Rectangle){120,175,1040,398});
    label(ui,fonts,defeated ? (screen->campaign_lost ? "Todo o progresso desta campanha foi apagado."
                                                 : TextFormat("%s · %s",player_name,location))
                           : TextFormat("Obrigado, %s!",player_name),
        (Rectangle){160,198,960,42},36,ink);
    label(ui,fonts,defeated ? (screen->campaign_lost ? "Comece uma nova aventura para tentar novamente."
                                                 : "Acerte uma pergunta de lógica para ganhar uma revanche.")
                           : "Com lógica e uma boa equipe, você devolveu a vida à estufa.",
        (Rectangle){155,250,970,34},28,ink);
    label(ui,fonts,defeated ? (screen->campaign_lost ? "Sua equipe e os desafios da estufa foram reiniciados."
                                                 : "Se errar ou desistir, perderá todo o progresso.")
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
        const char *text = defeated ? (screen->campaign_lost ? (i == 0 ? "Nova campanha" : "Menu principal")
                                                          : (i == 0 ? "Responder para tentar" : "Encerrar campanha"))
                                   : (i == 0 ? "Jogar novamente" : "Menu principal");
        selection_ui_button(ui,fonts,result_screen_button_bounds(i),text,screen->cursor == i,false,1);
    }
    const char *help = defeated
        ? (screen->campaign_lost ? "Setas: escolher · ENTER ou clique: confirmar · ESC: menu"
                                 : "Setas: escolher · ENTER ou clique: confirmar · ESC: desistir (confirmação)")
        : "Setas / A D: escolher · ENTER ou clique: confirmar · ESC: voltar";
    label(ui,fonts,help,
        (Rectangle){100,687,1080,22},20,cream);
    if (screen->abandon_confirm) {
        DrawRectangle(0,0,1280,720,Fade((Color){12,25,17,255},0.85f));
        ui_botanical_panel(ui,(Rectangle){185,210,910,300});
        label(ui,fonts,"Encerrar a campanha?",(Rectangle){225,242,830,52},36,ink);
        label(ui,fonts,"Todo o progresso desta campanha será apagado.",
            (Rectangle){225,312,830,34},28,ink);
        label(ui,fonts,"Você ainda pode responder à pergunta e tentar uma revanche.",
            (Rectangle){225,354,830,30},24,ink);
        for (int i = 0; i < 2; i++)
            selection_ui_button(ui,fonts,abandon_button(i),i == 0 ? "Continuar" : "Apagar progresso",
                screen->cursor == i,false,1);
        label(ui,fonts,"Setas/WASD: escolher · ENTER/Espaço: confirmar · ESC: cancelar",
            (Rectangle){190,535,900,30},22,cream);
    }

}
