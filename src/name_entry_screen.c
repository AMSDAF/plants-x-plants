/* Edita nomes UTF-8 e compõe a transição, sem alterar retratos ou regras do jogo. */
#include "name_entry_screen.h"
#include "ui_theme.h"
#include "selection_ui.h"
#include "name_unicode_data.h"
#include "game_canvas.h"
#include "input_map.h"
#include <math.h>
#include <string.h>

static float clamp01(float value) { return fminf(1,fmaxf(0,value)); }

static Rectangle control_bounds(int focus)
{
    if (focus==0) return (Rectangle){548,280,524,80};
    return selection_ui_footer_bounds(focus == 2);
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

static const char *virtual_keys[NAME_VIRTUAL_KEY_COUNT] = {
    "A","B","C","D","E","F","G","H","I","J",
    "K","L","M","N","O","P","Q","R","S","T",
    "U","V","W","X","Y","Z","ESP","DEL"
};

static int virtual_key_columns(int row)
{
    static const int columns[3] = {10, 10, 8};
    return row >= 0 && row < 3 ? columns[row] : 0;
}

static int virtual_key_row(int index)
{
    if (index < 0 || index >= NAME_VIRTUAL_KEY_COUNT) return -1;
    if (index < 10) return 0;
    if (index < 20) return 1;
    return 2;
}

static int virtual_key_column(int index)
{
    int row = virtual_key_row(index);
    return row < 0 ? -1 : index - (row == 0 ? 0 : row == 1 ? 10 : 20);
}

Rectangle name_entry_virtual_key_bounds(int index)
{
    int row = virtual_key_row(index);
    if (row < 0) return (Rectangle){0};
    int columns = virtual_key_columns(row);
    float width = 50, gap = 6;
    float row_width = columns * width + (columns - 1) * gap;
    float x = 840 - row_width / 2 + virtual_key_column(index) * (width + gap);
    float y = 492 + row * 42;
    return (Rectangle){x, y, width, 34};
}

const char *name_entry_virtual_key_label(int index)
{
    return index >= 0 && index < NAME_VIRTUAL_KEY_COUNT ? virtual_keys[index] : "";
}

static void press_virtual_key(NameEntryScreen *screen, int index)
{
    if (index < 0 || index >= NAME_VIRTUAL_KEY_COUNT) return;
    if (index == NAME_VIRTUAL_KEY_COUNT - 1) name_entry_backspace(screen);
    else if (index == NAME_VIRTUAL_KEY_COUNT - 2) name_entry_append(screen, 32);
    else name_entry_append(screen, virtual_keys[index][0]);
}

void name_entry_init(NameEntryScreen *screen)
{
    *screen=(NameEntryScreen){.pressed=-1, .virtual_key=-1};
}

void name_entry_begin(NameEntryScreen *screen, bool forward)
{
    if (forward) screen->progress=0;
    screen->direction=forward ? 1 : -1;
    screen->input_blocked=true;
    screen->pressed=-1;
    screen->virtual_key=-1;
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
            screen->virtual_key=-1;
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
    bool pad_activated=input_gamepad_active() && !screen->gamepad_mode;
    if (input_gamepad_active()) screen->gamepad_mode=true;
    if (pad_activated && screen->focus==0 && screen->virtual_key<0) screen->virtual_key=0;
    if (IsKeyPressed(KEY_ESCAPE)) return NAME_ENTRY_BACK;
    bool pad_back=input_gamepad_back_pressed();
    bool erase=pad_back && (screen->virtual_key>=0 || screen->focus==0);
    if (pad_back && !erase) return NAME_ENTRY_BACK;
    if (IsKeyPressed(KEY_TAB) || input_left_bumper_pressed() || input_right_bumper_pressed()) {
        bool reverse=IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT)||
                     input_left_bumper_pressed();
        screen->focus=(screen->focus+(reverse?2:1))%3;
        screen->virtual_key=-1;
        screen->caret_time=0;
    }
    if (input_direction_pressed(INPUT_DIR_UP)) {
        if (screen->virtual_key>=0) {
            int row=virtual_key_row(screen->virtual_key), column=virtual_key_column(screen->virtual_key);
            if (row>0) screen->virtual_key=virtual_key_columns(row-1)>column
                ? (row==1?0:10)+column : NAME_VIRTUAL_KEY_COUNT-1;
            else { screen->virtual_key=-1; screen->focus=0; }
        } else if (screen->focus>0) screen->focus=0;
    }
    if (input_direction_pressed(INPUT_DIR_DOWN)) {
        if (screen->virtual_key<0 && screen->focus==0) screen->virtual_key=0;
        else if (screen->virtual_key>=0) {
            int row=virtual_key_row(screen->virtual_key), column=virtual_key_column(screen->virtual_key);
            if (row<2) screen->virtual_key=(row==0?10:20)+
                (column<virtual_key_columns(row+1)?column:virtual_key_columns(row+1)-1);
            else { screen->virtual_key=-1; screen->focus=1; }
        }
    }
    if (input_horizontal_pressed() && screen->virtual_key>=0) {
        int row=virtual_key_row(screen->virtual_key), column=virtual_key_column(screen->virtual_key);
        int columns=virtual_key_columns(row);
        if (input_direction_pressed(INPUT_DIR_LEFT)) column=(column+columns-1)%columns;
        else column=(column+1)%columns;
        screen->virtual_key=(row==0?0:row==1?10:20)+column;
    }
    Vector2 mouse=GetMousePosition();
    int hovered=-1, hovered_key=-1;
    for (int i=0;i<3;i++) if (CheckCollisionPointRec(mouse,control_bounds(i))) hovered=i;
    for (int i=0;i<NAME_VIRTUAL_KEY_COUNT;i++)
        if (CheckCollisionPointRec(mouse,name_entry_virtual_key_bounds(i))) hovered_key=i;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        screen->pressed=hovered>=0 ? hovered : hovered_key>=0 ? 3+hovered_key : -1;
        if (hovered>=0) { screen->focus=hovered; screen->virtual_key=-1; }
        else if (hovered_key>=0) { screen->virtual_key=hovered_key; screen->focus=0; }
        screen->caret_time=0;
    }
    bool activate=input_primary_pressed();
    bool pad_confirm=input_gamepad_secondary_pressed();
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (screen->pressed==hovered && hovered>0) activate=true;
        if (screen->pressed>=3 && screen->pressed-3==hovered_key && hovered_key>=0)
            press_virtual_key(screen,hovered_key);
        screen->pressed=-1;
    }
    int codepoint;
    while ((codepoint=GetCharPressed())!=0) if (screen->focus==0) name_entry_append(screen,codepoint);
    if (erase || IsKeyPressed(KEY_BACKSPACE)) {
        name_entry_backspace(screen);
        screen->backspace_time=.4f;
    } else if (IsKeyDown(KEY_BACKSPACE) || input_gamepad_back_down()) {
        screen->backspace_time-=dt;
        while (screen->backspace_time<=0) {
            name_entry_backspace(screen);
            screen->backspace_time+=.055f;
        }
    } else screen->backspace_time=.4f;
    if (activate) {
        if (screen->virtual_key>=0) press_virtual_key(screen,screen->virtual_key);
        else if (screen->focus==1) return NAME_ENTRY_BACK;
        else {
            char trimmed[PLAYER_NAME_CAPACITY];
            if (name_entry_confirm(screen,trimmed)) return NAME_ENTRY_CONFIRM;
        }
    }
    /* X confirma o nome mesmo com uma tecla do teclado virtual destacada. */
    if (pad_confirm) {
        char trimmed[PLAYER_NAME_CAPACITY];
        if (name_entry_confirm(screen,trimmed)) return NAME_ENTRY_CONFIRM;
    }
    return NAME_ENTRY_NONE;
}

Rectangle name_entry_portrait_bounds(void) { return (Rectangle){160,160,300,420}; }

Rectangle name_entry_transition_bounds(const NameEntryScreen *screen, Trainer trainer)
{
    Rectangle from=trainer_select_card_bounds(trainer), to=name_entry_portrait_bounds();
    float t=clamp01(screen->progress);
    t=t*t*(3-2*t); /* Aceleração e desaceleração suaves, sem ultrapassar os extremos. */
    return (Rectangle){roundf(from.x+(to.x-from.x)*t),roundf(from.y+(to.y-from.y)*t),
                       roundf(from.width+(to.width-from.width)*t),roundf(from.height+(to.height-from.height)*t)};
}

static void draw_name_ui(const UiTheme *ui, const NameEntryScreen *screen, const SelectionUiAssets *assets)
{
    float alpha=clamp01((screen->progress-.5f)/.5f);
    if (alpha<=0) return;
    Color ink=Fade(SELECTION_INK,alpha);
    Color orange=Fade(SELECTION_ORANGE,alpha);
    selection_ui_header(ui, assets, "Como você se chama?", "Escolha o nome da sua aventura.",
                         OPENING_NAME, alpha);
    selection_ui_skin(assets, SELECTION_NAME_PANEL, (Rectangle){500,175,620,380}, alpha);
    selection_ui_pixel_text(ui, assets, "Seu nome",548,246,30,ink);
    Rectangle field=control_bounds(0);
    selection_ui_panel(field,4,screen->focus==0 ? orange : ink);
    selection_ui_panel((Rectangle){field.x+4,field.y+4,field.width-8,field.height-8},4,
                       Fade((Color){255,249,222,255},alpha));
    game_canvas_begin_scissor((Rectangle){field.x+12, field.y+8, field.width-24, field.height-16});
    float text_width = selection_ui_measure(ui, assets, screen->draft, 36);
    float text_x = field.x + 16 - fmaxf(0, text_width - field.width + 40);
    selection_ui_pixel_text(ui, assets, screen->draft,text_x,field.y+20,36,ink);
    if (screen->focus==0 && screen->caret_time<.5f) {
        int x=(int)roundf(text_x+text_width+3);
        DrawRectangle(x,(int)field.y+18,2,40,ink);
    }
    EndScissorMode();
    const char *counter=TextFormat("%d/12",screen->count);
    selection_ui_pixel_text(ui, assets, counter,1072-selection_ui_measure(ui,assets,counter,22),378,22,ink);
    if (!screen->gamepad_mode)
        selection_ui_pixel_text(ui, assets, "Esse será o seu nome na aventura.",548,440,24,ink);
    if (screen->error) selection_ui_pixel_text(ui, assets, "Digite um nome para continuar.",
                                              548,screen->gamepad_mode ? 466 : 488,22,orange);
    if (screen->gamepad_mode) {
        Rectangle keyboard_panel={540,470,600,150};
        selection_ui_panel(keyboard_panel,4,Fade(SELECTION_BORDER,alpha));
        selection_ui_panel((Rectangle){keyboard_panel.x+4,keyboard_panel.y+4,
                                       keyboard_panel.width-8,keyboard_panel.height-8},
                           4,Fade((Color){255,249,222,255},alpha));
        for (int i=0;i<NAME_VIRTUAL_KEY_COUNT;i++) {
            Rectangle bounds=name_entry_virtual_key_bounds(i);
            bool hovered=CheckCollisionPointRec(GetMousePosition(),bounds);
            bool selected=screen->virtual_key==i || hovered;
            selection_ui_panel(bounds,3,Fade(selected ? SELECTION_ORANGE : SELECTION_BORDER,alpha));
            selection_ui_panel((Rectangle){bounds.x+3,bounds.y+3,bounds.width-6,bounds.height-6},
                               3,Fade((Color){255,249,222,255},alpha));
            selection_ui_pixel_box(ui,assets,name_entry_virtual_key_label(i),bounds,20,Fade(SELECTION_INK,alpha));
        }
    }
    for (int i=1;i<3;i++) {
        Rectangle bounds=control_bounds(i);
        bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        selection_ui_button(ui, assets, bounds, screen->gamepad_mode ? (i==1 ? "Voltar · B" : "Confirmar · X") : (i==1 ? "Voltar · ESC" : "Confirmar · ENTER"),
                            screen->focus==i || hovered, hovered && screen->pressed==i, alpha);
    }
    selection_ui_pixel_box(ui, assets, screen->gamepad_mode ? "D-pad: escolher · A: digitar · X: confirmar · B: apagar" : "Digite seu nome para continuar",
                           (Rectangle){350,643,470,28},20,Fade((Color){253,245,216,255},alpha));
}

void name_entry_draw(const UiTheme *ui, const NameEntryScreen *screen, const TrainerSelectScreen *selection,
                     const TrainerSelectAssets *assets)
{
    trainer_background_draw_dimmed(&assets->background,selection->background_time);
    float old=1-clamp01(screen->progress/.25f);
    if (old>0) {
        trainer_select_draw_chrome(ui, selection,assets,old);
        Trainer other=selection->selected==TRAINER_MALE ? TRAINER_FEMALE : TRAINER_MALE;
        trainer_select_draw_card(ui, selection,assets,other,trainer_select_card_bounds(other),old,false,1);
    }
    draw_name_ui(ui, screen, &assets->ui);
    /* O cartão atravessa o painel pela frente; retrato e halo nunca são encobertos. */
    trainer_select_draw_card(ui, selection,assets,selection->selected,
                             name_entry_transition_bounds(screen,selection->selected),1,old>0,old);
}
