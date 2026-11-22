/* Compõe o modal botânico e edita fórmulas sem executar ações de combate. */
#include "logic_setup_screen.h"
#include <math.h>

static const Color ink = {24,52,30,255};
static const Color cream = {253,245,216,255};
static const Color sage = {214,229,170,255};
static const Rectangle attack_box = {414,279,730,39};
static const Rectangle condition_boxes[2] = {{414,350,350,58},{794,350,350,58}};
static const Rectangle confirm_box = {865,574,290,44};
static const Rectangle back_box = {98,642,200,38};
static const char *const conditions[LOGIC_CONDITION_COUNT] = {
    "Minha planta tem até 50% de HP", "O rival tem reserva viva",
    "A planta rival tem até 50% de HP", "Minha equipe tem reserva viva"
};
static const char *const operators[LOGIC_OPERATOR_COUNT] = {"E","OU","XOR"};
static const char *const explanations[LOGIC_OPERATOR_COUNT] = {
    "As duas condições devem ser verdadeiras.",
    "Pelo menos uma condição deve ser verdadeira.",
    "Exatamente uma condição deve ser verdadeira."
};

static Rectangle tab_box(int index, int count)
{
    float width = (1000.0f-(count-1)*14)/count;
    return (Rectangle){140+index*(width+14),151,width,50};
}

static Rectangle answer_box(int index) { return (Rectangle){414+index*380,520,350,58}; }
static Rectangle operator_box(int index) { return (Rectangle){414+index*250,440,230,42}; }
static Rectangle field_box(int field) { return field == 0 ? attack_box : condition_boxes[field-1]; }

static Rectangle option_box(int field, int index)
{
    Rectangle box = field_box(field);
    return (Rectangle){box.x,box.y+box.height+index*54,box.width,54};
}

static bool clicked(Rectangle box)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(),box);
}

static bool confirm_key(void) { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }

static bool valid_formula(LogicFormula value)
{
    return value.left >= 0 && value.left < LOGIC_CONDITION_COUNT &&
        value.right >= 0 && value.right < LOGIC_CONDITION_COUNT && value.left != value.right &&
        value.operator >= 0 && value.operator < LOGIC_OPERATOR_COUNT;
}

void logic_setup_begin(LogicSetupScreen *screen, const Team *team,
                       const LogicFormula formulas[MAX_TEAM_SIZE], const int moves[MAX_TEAM_SIZE])
{
    for (int i = 0; i < team->count && i < MAX_TEAM_SIZE; i++) {
        unsigned int bit = 1u << i;
        if (!(screen->initialized_mask & bit)) {
            screen->formulas[i] = valid_formula(formulas[i]) ? formulas[i]
                : (LogicFormula){LOGIC_OWN_LOW_HP,LOGIC_ENEMY_RESERVE,LOGIC_OR};
            int move = moves[i];
            if (move < 0 || move >= MAX_MOVES || team->plants[i].moves[move].effect != MOVE_EFFECT_DAMAGE) {
                move = 0;
                while (move < MAX_MOVES && team->plants[i].moves[move].effect != MOVE_EFFECT_DAMAGE) move++;
            }
            screen->moves[i] = move;
            screen->initialized_mask |= bit;
        }
        if (moves[i] >= 0) screen->confirmed_mask |= bit;
    }
    if (screen->selected < 0 || screen->selected >= team->count) screen->selected = 0;
    screen->dropdown = -1;
    screen->focus = 0;
}

static bool option_enabled(const LogicSetupScreen *screen, const Plant *plant, int index)
{
    if (screen->dropdown == 0) return plant->moves[index].effect == MOVE_EFFECT_DAMAGE;
    LogicFormula value = screen->formulas[screen->selected];
    return index != (int)(screen->dropdown == 1 ? value.right : value.left);
}

bool logic_setup_update(LogicSetupScreen *screen, const Team *team, unsigned int *solved,
                        LogicFormula formulas[MAX_TEAM_SIZE], int moves[MAX_TEAM_SIZE])
{
    if (team->count <= 0 || team->count > MAX_TEAM_SIZE) return true;
    int selected = screen->selected;
    for (int i = 0; i < team->count; i++) {
        if (clicked(tab_box(i,team->count))) {
            screen->selected = i;
            screen->focus = 0;
            screen->dropdown = -1;
            return false;
        }
    }
    const Plant *plant = &team->plants[selected];
    if (screen->dropdown >= 0) {
        if (IsKeyPressed(KEY_ESCAPE)) { screen->dropdown = -1; return false; }
        int count = screen->dropdown == 0 ? MAX_MOVES : LOGIC_CONDITION_COUNT;
        if (IsKeyPressed(KEY_UP)) screen->option = (screen->option+count-1)%count;
        if (IsKeyPressed(KEY_DOWN)) screen->option = (screen->option+1)%count;
        int choice = confirm_key() ? screen->option : -1;
        bool inside = false;
        for (int i = 0; i < count; i++) {
            Rectangle box = option_box(screen->dropdown,i);
            if (CheckCollisionPointRec(GetMousePosition(),box)) {
                inside = true;
                Vector2 delta = GetMouseDelta();
                if (delta.x || delta.y || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) screen->option = i;
            }
            if (clicked(box)) choice = i;
        }
        if (choice >= 0 && option_enabled(screen,plant,choice)) {
            if (screen->dropdown == 0) screen->moves[selected] = choice;
            else if (screen->dropdown == 1) screen->formulas[selected].left = (LogicCondition)choice;
            else screen->formulas[selected].right = (LogicCondition)choice;
            screen->dirty_mask |= 1u << selected;
            screen->dropdown = -1;
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !inside) screen->dropdown = -1;
        return false;
    }
    if (IsKeyPressed(KEY_ESCAPE) || clicked(back_box)) return true;
    bool unlocked = (*solved & (1u << selected)) != 0;
    int focus_count = unlocked ? 7 : 4;
    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN)) screen->focus = (screen->focus+1)%focus_count;
    if (IsKeyPressed(KEY_UP)) screen->focus = (screen->focus+focus_count-1)%focus_count;
    if (screen->focus == 0 && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))) {
        int direction = IsKeyPressed(KEY_RIGHT) ? 1 : -1;
        screen->selected = (selected+direction+team->count)%team->count;
        return false;
    }
    if (!unlocked) {
        if ((screen->focus == 1 || screen->focus == 2) &&
            (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))) screen->focus = 3-screen->focus;
        int answer = -1;
        if (IsKeyPressed(KEY_ONE)) answer = 0;
        else if (IsKeyPressed(KEY_TWO)) answer = 1;
        for (int i = 0; i < 2; i++)
            if (clicked(answer_box(i)) || (screen->focus == i+1 && confirm_key())) answer = i;
        if (screen->focus == 3 && confirm_key()) return true;
        if (answer >= 0) {
            if (answer == (selected%3 == 1 ? 1 : 0)) {
                *solved |= 1u << selected;
                screen->dirty_mask |= 1u << selected;
                screen->rejected_mask &= ~(1u << selected);
                screen->focus = 1;
            } else screen->rejected_mask |= 1u << selected;
        }
        return false;
    }
    for (int i = 0; i < 3; i++) {
        if (clicked(field_box(i)) || (screen->focus == i+1 && confirm_key())) {
            screen->focus = i+1;
            screen->dropdown = i;
            screen->option = i == 0 ? screen->moves[selected]
                : (int)(i == 1 ? screen->formulas[selected].left : screen->formulas[selected].right);
            return false;
        }
    }
    int op = -1;
    for (int i = 0; i < LOGIC_OPERATOR_COUNT; i++) if (clicked(operator_box(i))) op = i;
    if (screen->focus == 4 && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || confirm_key())) {
        int direction = IsKeyPressed(KEY_LEFT) ? -1 : 1;
        op = (screen->formulas[selected].operator+direction+LOGIC_OPERATOR_COUNT)%LOGIC_OPERATOR_COUNT;
    }
    if (op >= 0) {
        screen->formulas[selected].operator = (LogicOperator)op;
        screen->dirty_mask |= 1u << selected;
        screen->focus = 4;
    }
    if (clicked(confirm_box) || (screen->focus == 5 && confirm_key())) {
        int move = screen->moves[selected];
        if (valid_formula(screen->formulas[selected]) && move >= 0 && move < MAX_MOVES &&
            plant->moves[move].effect == MOVE_EFFECT_DAMAGE) {
            formulas[selected] = screen->formulas[selected];
            moves[selected] = move;
            screen->confirmed_mask |= 1u << selected;
            screen->dirty_mask &= ~(1u << selected);
        }
        screen->focus = 5;
    }
    return screen->focus == 6 && confirm_key();
}

static void text(const UiTheme *ui, const SelectionUiAssets *fonts, const char *value,
                  float x, float y, int size)
{
    selection_ui_pixel_text(ui,fonts,value,x,y,size,ink);
}

static void label(const UiTheme *ui, const SelectionUiAssets *fonts, const char *value,
                   Rectangle box, int size)
{
    selection_ui_pixel_box(ui,fonts,value,box,size,ink);
}

static void wrapped(const UiTheme *ui, const SelectionUiAssets *fonts, const char *value, Rectangle box)
{
    float size;
    Font font = selection_ui_font(ui,fonts,22,&size);
    ui_text_wrap_reveal_pixels(font,value,box,size,ink,10000,fonts->font_scale);
}

/* Moldura em degraus compartilha a paleta das artes, sem esticar folhas. */
static void frame(Rectangle box, bool selected, bool focused)
{
    static const Color colors[] = {{45,42,24,255},{141,68,27,255},{255,192,76,255},
        {183,92,35,255},{42,65,34,255}};
    for (int i = 0; i < 5; i++) {
        Rectangle inner = {box.x+i,box.y+i,box.width-i*2,box.height-i*2};
        selection_ui_panel(inner,5-i,colors[i]);
    }
    selection_ui_panel((Rectangle){box.x+5,box.y+5,box.width-10,box.height-10},2,selected ? sage : cream);
    if (focused) DrawRectangleLinesEx((Rectangle){box.x+1,box.y+1,box.width-2,box.height-2},2,ORANGE);
}

static void arrow(float x, float y, bool up)
{
    for (int i = 0; i < 4; i++) {
        int width = 14-i*4;
        if (width <= 0) width = 2;
        DrawRectangle((int)x+i*2,(int)y+(up ? 3-i : i)*2,width,2,ink);
        if (width > 4) DrawRectangle((int)x+i*2+2,(int)y+(up ? 3-i : i)*2,width-4,2,ORANGE);
    }
}

static void pointer(const SelectionUiAssets *fonts, Rectangle box)
{
    selection_ui_skin(fonts,SELECTION_POINTER,(Rectangle){box.x+10,box.y+box.height/2-9,14,18},1);
}

static void check(float x, float y)
{
    DrawLineEx((Vector2){x,y+6},(Vector2){x+5,y+11},3,(Color){71,123,36,255});
    DrawLineEx((Vector2){x+5,y+11},(Vector2){x+15,y},3,(Color){71,123,36,255});
}

void logic_setup_draw(const UiTheme *ui, const SelectionUiAssets *fonts, const IntroAssets *art,
                      const LogicSetupScreen *screen, const Team *team, unsigned int solved)
{
    if (team->count <= 0 || screen->selected < 0 || screen->selected >= team->count) return;
    int index = screen->selected;
    unsigned int bit = 1u << index;
    bool unlocked = (solved & bit) != 0;
    const Plant *plant = &team->plants[index];
    DrawRectangle(0,0,1280,720,Fade(ink,.76f));
    ui_botanical_panel(ui,(Rectangle){56,38,1168,654});
    label(ui,fonts,"Desafio do golpe lógico",(Rectangle){110,52,1060,64},72);
    label(ui,fonts,"Uma resposta certa, um golpe desbloqueado",(Rectangle){110,117,1060,28},28);
    for (int i = 0; i < team->count; i++) {
        Rectangle box = tab_box(i,team->count);
        frame(box,i == index,screen->focus == 0 && i == index);
        label(ui,fonts,team->plants[i].name_pt,(Rectangle){box.x+20,box.y+8,box.width-40,32},36);
    }
    DrawLine(82,216,1198,216,(Color){102,126,67,255});
    frame((Rectangle){100,232,255,365},false,false);
    label(ui,fonts,plant->name_pt,(Rectangle){112,240,231,34},36);
    DrawLine(112,279,343,279,(Color){102,126,67,255});
    Rectangle source = art->sources[plant->id];
    Texture2D sprite = art->plants[plant->id];
    if (sprite.id && source.width > 0 && source.height > 0) {
        float scale = fminf(183/source.width,214/source.height);
        DrawTexturePro(sprite,source,(Rectangle){roundf(227-source.width*scale/2),roundf(502-source.height*scale),
            source.width*scale,source.height*scale},(Vector2){0},0,WHITE);
    } else label(ui,fonts,plant->name_pt,(Rectangle){115,340,225,60},24);
    DrawLine(116,516,339,516,(Color){102,126,67,255});
    if (unlocked) check(117,540);
    label(ui,fonts,unlocked ? "Pergunta resolvida" : "Recompensa",(Rectangle){138,535,200,28},20);
    const char *state = !(screen->confirmed_mask & bit) ? "Configure seu golpe"
        : screen->dirty_mask & bit ? "Alterações pendentes" : "Configuração salva";
    label(ui,fonts,unlocked ? state : "Um golpe lógico",(Rectangle){112,566,231,22},20);
    DrawLine(377,232,377,617,(Color){102,126,67,255});
    if (!unlocked) {
        static const char *const expressions[] = {"P OU Q","P E Q","NÃO P"};
        int question = index%3;
        frame((Rectangle){414,265,350,65},false,false);
        label(ui,fonts,question == 2 ? "P: Falso" : "P: Verdadeiro",(Rectangle){424,277,330,40},28);
        if (question != 2) {
            frame((Rectangle){794,265,350,65},false,false);
            label(ui,fonts,"Q: Falso",(Rectangle){804,277,330,40},28);
        }
        frame((Rectangle){560,363,430,85},false,false);
        label(ui,fonts,expressions[question],(Rectangle){574,375,402,59},36);
        label(ui,fonts,"Essa expressão é verdadeira?",(Rectangle){414,467,730,35},28);
        for (int i = 0; i < 2; i++) {
            Rectangle box = answer_box(i);
            bool focused = screen->focus == i+1;
            frame(box,focused,focused);
            if (focused) pointer(fonts,box);
            label(ui,fonts,i ? "2 · Falso" : "1 · Verdadeiro",(Rectangle){box.x+30,box.y+10,box.width-42,38},28);
        }
        if (screen->rejected_mask & bit)
            label(ui,fonts,"Ainda não! Pense no operador e tente outra vez.",(Rectangle){414,590,730,24},20);
    } else {
        LogicFormula formula = screen->formulas[index];
        label(ui,fonts,"Monte seu golpe lógico",(Rectangle){414,224,730,37},36);
        text(ui,fonts,"Ataque",420,255,22);
        frame(attack_box,false,screen->focus == 1);
        text(ui,fonts,plant->moves[screen->moves[index]].name,430,284,28);
        arrow(1116,294,screen->dropdown == 0);
        for (int i = 0; i < 2; i++) {
            Rectangle box = condition_boxes[i];
            text(ui,fonts,i ? "Condição Q" : "Condição P",box.x+6,325,22);
            frame(box,false,screen->focus == i+2);
            wrapped(ui,fonts,conditions[i ? formula.right : formula.left],
                (Rectangle){box.x+16,box.y+2,box.width-49,54});
            arrow(box.x+box.width-28,box.y+24,screen->dropdown == i+1);
        }
        text(ui,fonts,"Operador",420,413,22);
        for (int i = 0; i < LOGIC_OPERATOR_COUNT; i++) {
            Rectangle box = operator_box(i);
            frame(box,(int)formula.operator == i,screen->focus == 4 && (int)formula.operator == i);
            if ((int)formula.operator == i) pointer(fonts,box);
            label(ui,fonts,operators[i],(Rectangle){box.x+24,box.y+6,box.width-48,30},28);
        }
        frame((Rectangle){414,493,730,69},false,false);
        label(ui,fonts,TextFormat("P %s Q",operators[formula.operator]),(Rectangle){425,498,708,32},30);
        label(ui,fonts,explanations[formula.operator],(Rectangle){425,533,708,23},20);
        text(ui,fonts,TextFormat("Verdadeira: +%d de poder",Battle_GetFormulaPowerBonus(formula.operator)),414,576,20);
        text(ui,fonts,"Falsa: este golpe fica bloqueado",414,600,20);
        selection_ui_button(ui,fonts,confirm_box,"Confirmar configuração",screen->focus == 5,false,1);
    }
    DrawLine(82,632,1198,632,(Color){102,126,67,255});
    selection_ui_button(ui,fonts,back_box,"Voltar · ESC",screen->focus == (unlocked ? 6 : 3),false,1);
    label(ui,fonts,"TAB / setas: navegar · ENTER ou clique: selecionar",(Rectangle){330,645,820,30},20);
    if (screen->dropdown >= 0) {
        int field = screen->dropdown;
        int count = field == 0 ? MAX_MOVES : LOGIC_CONDITION_COUNT;
        int current = field == 0 ? screen->moves[index] : (int)(field == 1
            ? screen->formulas[index].left : screen->formulas[index].right);
        Rectangle first = option_box(field,0);
        DrawRectangle((int)first.x+4,(int)first.y+4,(int)first.width,count*54,Fade(ink,.7f));
        for (int i = 0; i < count; i++) {
            Rectangle box = option_box(field,i);
            bool enabled = option_enabled(screen,plant,i);
            bool focused = screen->option == i;
            frame(box,i == current || (enabled && focused),enabled && focused);
            if (!enabled) DrawRectangleRec((Rectangle){box.x+5,box.y+5,box.width-10,box.height-10},(Color){213,210,192,255});
            if (i == current) check(box.x+12,box.y+18);
            else if (focused && enabled) pointer(fonts,box);
            wrapped(ui,fonts,field == 0 ? plant->moves[i].name : conditions[i],
                (Rectangle){box.x+33,box.y+1,box.width-48,52});
        }
    }
}
