/* Edita nomes UTF-8 e compõe a transição, sem alterar retratos ou regras do jogo. */
#include "name_entry_screen.h"
#include "selection_ui.h"
#include "name_unicode_data.h"
#include <math.h>
#include <string.h>

static float clamp01(float value) { return fminf(1,fmaxf(0,value)); }

static Rectangle control_bounds(int focus)
{
    if (focus==0) return (Rectangle){592,292,476,72};
    return (Rectangle){focus==1 ? 568 : 844,548,240,52};
}

static bool letter_or_digit(int codepoint)
{
    int low=0, high=(int)(sizeof(NAME_UNICODE_RANGES)/sizeof(NAME_UNICODE_RANGES[0]))-1;
    while (low<=high) {
        int middle=(low+high)/2;
        if (codepoint<NAME_UNICODE_RANGES[middle][0]) high=middle-1;
        else if (codepoint>NAME_UNICODE_RANGES[middle][1]) low=middle+1;
        else return true;
    }
    return false;
}

static bool space(int codepoint)
{
    return codepoint==32 || codepoint==0xa0 || codepoint==0x1680 ||
           (codepoint>=0x2000 && codepoint<=0x200a) || codepoint==0x202f ||
           codepoint==0x205f || codepoint==0x3000;
}

void name_entry_init(NameEntryScreen *screen)
{
    *screen=(NameEntryScreen){.pressed=-1};
}

void name_entry_begin(NameEntryScreen *screen, bool forward)
{
    if (forward) screen->progress=0;
    screen->direction=forward ? 1 : -1;
    screen->input_blocked=true;
    screen->pressed=-1;
    screen->backspace_time=0;
    screen->error=false;
    selection_ui_discard_text();
}

bool name_entry_append(NameEntryScreen *screen, int codepoint)
{
    if (screen->count>=PLAYER_NAME_MAX_CODEPOINTS) return false;
    if (space(codepoint)) codepoint=32;
    if (!letter_or_digit(codepoint) && codepoint!=32 && codepoint!='\'' && codepoint!='-') return false;
    int bytes=0;
    const char *encoded=CodepointToUTF8(codepoint,&bytes);
    size_t length=strlen(screen->draft);
    if (bytes<=0 || length+(size_t)bytes>=sizeof(screen->draft)) return false;
    memcpy(screen->draft+length,encoded,(size_t)bytes);
    screen->draft[length+bytes]='\0';
    screen->count++;
    screen->error=false;
    screen->caret_time=0;
    return true;
}

void name_entry_backspace(NameEntryScreen *screen)
{
    size_t length=strlen(screen->draft);
    if (!length) return;
    do { length--; } while (length && ((unsigned char)screen->draft[length]&0xc0)==0x80);
    screen->draft[length]='\0';
    screen->count--;
    screen->error=false;
    screen->caret_time=0;
}

bool name_entry_confirm(NameEntryScreen *screen, char output[PLAYER_NAME_CAPACITY])
{
    const char *start=screen->draft;
    while (*start==' ') start++;
    size_t length=strlen(start);
    while (length && start[length-1]==' ') length--;
    if (!length) { screen->error=true; return false; }
    memcpy(output,start,length);
    output[length]='\0';
    return true;
}

NameEntryAction name_entry_update(NameEntryScreen *screen, bool accept_input, float dt)
{
    dt=fminf(.1f,fmaxf(0,dt));
    screen->caret_time=fmodf(screen->caret_time+dt,1);
    if (screen->direction) {
        selection_ui_discard_text();
        screen->progress=clamp01(screen->progress+screen->direction*dt/NAME_ENTRY_TRANSITION_SECONDS);
        if (screen->progress>1-.00001f) screen->progress=1;
        if (screen->progress<.00001f) screen->progress=0;
        if (screen->direction>0 && screen->progress>=1) {
            screen->direction=0;
            screen->focus=0;
            screen->input_blocked=true;
            screen->caret_time=0;
        } else if (screen->direction<0 && screen->progress<=0) {
            screen->direction=0;
            return NAME_ENTRY_SELECTION;
        }
        return NAME_ENTRY_NONE;
    }
    if (screen->input_blocked || !accept_input) {
        selection_ui_discard_text();
        screen->pressed=-1;
        screen->backspace_time=0;
        if (!selection_ui_input_down()) screen->input_blocked=false;
        return NAME_ENTRY_NONE;
    }
    if (IsKeyPressed(KEY_ESCAPE)) return NAME_ENTRY_BACK;
    if (IsKeyPressed(KEY_TAB)) {
        bool reverse=IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT);
        screen->focus=(screen->focus+(reverse?2:1))%3;
        screen->caret_time=0;
    }
    Vector2 mouse=GetMousePosition();
    int hovered=-1;
    for (int i=0;i<3;i++) if (CheckCollisionPointRec(mouse,control_bounds(i))) hovered=i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        screen->pressed=hovered;
        if (hovered>=0) screen->focus=hovered;
        screen->caret_time=0;
    }
    bool activate=IsKeyPressed(KEY_ENTER);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        activate|=screen->pressed==hovered && hovered>0;
        screen->pressed=-1;
    }
    int codepoint;
    while ((codepoint=GetCharPressed())!=0) if (screen->focus==0) name_entry_append(screen,codepoint);
    if (screen->focus==0) {
        if (IsKeyPressed(KEY_BACKSPACE)) {
            name_entry_backspace(screen);
            screen->backspace_time=.4f;
        } else if (IsKeyDown(KEY_BACKSPACE)) {
            screen->backspace_time-=dt;
            while (screen->backspace_time<=0) {
                name_entry_backspace(screen);
                screen->backspace_time+=.055f;
            }
        } else screen->backspace_time=.4f;
    }
    if (activate) {
        if (screen->focus==1) return NAME_ENTRY_BACK;
        char trimmed[PLAYER_NAME_CAPACITY];
        if (name_entry_confirm(screen,trimmed)) return NAME_ENTRY_CONFIRM;
    }
    return NAME_ENTRY_NONE;
}

Rectangle name_entry_portrait_bounds(void) { return (Rectangle){172,140,308,440}; }

Rectangle name_entry_transition_bounds(const NameEntryScreen *screen, Trainer trainer)
{
    Rectangle from=trainer_select_card_bounds(trainer), to=name_entry_portrait_bounds();
    float t=clamp01(screen->progress);
    t=t*t*(3-2*t); /* Aceleração e desaceleração suaves, sem ultrapassar os extremos. */
    return (Rectangle){roundf(from.x+(to.x-from.x)*t),roundf(from.y+(to.y-from.y)*t),
                       roundf(from.width+(to.width-from.width)*t),roundf(from.height+(to.height-from.height)*t)};
}

static void draw_name_ui(const NameEntryScreen *screen)
{
    float alpha=clamp01((screen->progress-.5f)/.5f);
    if (alpha<=0) return;
    int rise=(int)roundf(12*(1-alpha));
    Color ink=Fade(SELECTION_INK,alpha), cream=Fade(SELECTION_CREAM,alpha);
    Color orange=Fade(SELECTION_ORANGE,alpha);
    selection_ui_panel((Rectangle){260,28+rise,760,64},4,ink);
    selection_ui_panel((Rectangle){264,32+rise,752,56},4,cream);
    selection_ui_text("Como você se chama?",640,40+rise,40,ink);
    selection_ui_panel((Rectangle){556,190+rise,548,314},8,ink);
    selection_ui_panel((Rectangle){564,198+rise,532,298},8,cream);
    DrawText("Seu nome",592,232+rise,30,ink);
    Rectangle field=control_bounds(0);
    field.y+=rise;
    selection_ui_panel(field,4,screen->focus==0 ? orange : ink);
    selection_ui_panel((Rectangle){field.x+4,field.y+4,field.width-8,field.height-8},4,
                       Fade((Color){255,249,222,255},alpha));
    BeginScissorMode((int)field.x+12,(int)field.y+8,(int)field.width-24,(int)field.height-16);
    DrawText(screen->draft,(int)field.x+16,(int)field.y+20,30,ink);
    if (screen->focus==0 && screen->caret_time<.5f) {
        int x=(int)fminf(field.x+field.width-20,field.x+18+MeasureText(screen->draft,30));
        DrawRectangle(x,(int)field.y+18,3,34,ink);
    }
    EndScissorMode();
    const char *counter=TextFormat("%d/12",screen->count);
    DrawText(counter,1068-MeasureText(counter,20),380+rise,20,ink);
    DrawText("Esse será o seu nome na aventura.",592,420+rise,20,ink);
    if (screen->error) DrawText("Digite um nome para continuar.",592,458+rise,20,orange);
    for (int i=1;i<3;i++) {
        Rectangle bounds=control_bounds(i);
        bounds.y+=rise;
        selection_ui_panel(bounds,4,screen->focus==i ? orange : ink);
        selection_ui_panel((Rectangle){bounds.x+4,bounds.y+4,bounds.width-8,bounds.height-8},4,cream);
        selection_ui_text(i==1 ? "Voltar" : "Confirmar",(int)(bounds.x+bounds.width/2),(int)bounds.y+12,30,ink);
    }
    selection_ui_panel((Rectangle){208,688+rise,864,28},4,ink);
    selection_ui_text("Digite seu nome | ENTER: confirmar | ESC: voltar",640,692+rise,20,cream);
}

void name_entry_draw(const NameEntryScreen *screen, const TrainerSelectScreen *selection,
                     const TrainerSelectAssets *assets)
{
    trainer_background_draw(&assets->background,selection->background_time);
    float old=1-clamp01(screen->progress/.25f);
    if (old>0) {
        trainer_select_draw_chrome(selection,old);
        Trainer other=selection->selected==TRAINER_MALE ? TRAINER_FEMALE : TRAINER_MALE;
        trainer_select_draw_card(selection,assets,other,trainer_select_card_bounds(other),old,false,1);
    }
    draw_name_ui(screen);
    /* O cartão atravessa o painel pela frente; retrato e halo nunca são encobertos. */
    trainer_select_draw_card(selection,assets,selection->selected,
                             name_entry_transition_bounds(screen,selection->selected),1,true,old);
}
