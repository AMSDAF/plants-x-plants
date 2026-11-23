/* Compõe o modal botânico e edita fórmulas sem executar ações de combate. */
#include "logic_setup_screen.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { LOGIC_QUIZ_QUESTION_COUNT = 13 };

typedef struct {
    bool p;
    bool has_q;
    bool q;
    const char *expression;
    bool answer;
} LogicQuizQuestion;

static const LogicQuizQuestion questions[LOGIC_QUIZ_QUESTION_COUNT] = {
    {true, true, false, "P OU Q", true},
    {true, true, false, "P E Q", false},
    {false, false, false, "NÃO P", true},
    {true, true, true, "P E Q", true},
    {false, true, false, "P OU Q", false},
    {false, true, true, "P XOR Q", true},
    {true, true, true, "P XOR Q", false},
    {true, true, false, "P E NÃO Q", true},
    {false, true, true, "P OU NÃO Q", false},
    {false, true, false, "NÃO P E NÃO Q", true},
    {true, true, false, "NÃO P OU Q", false},
    {false, false, false, "NÃO P", true},
    {true, false, false, "NÃO P", false}
};

static const Color ink = {24,52,30,255};
static const Color cream = {253,245,216,255};
static const Color sage = {214,229,170,255};
static const Rectangle attack_box = {414,282,730,44};
static const Rectangle condition_boxes[2] = {{414,350,350,60},{794,350,350,60}};
static const Rectangle confirm_box = {804,572,340,48};
static const Rectangle back_box = {98,638,230,38};
static const char *const conditions[LOGIC_CONDITION_COUNT] = {
    "Minha planta até 50% de HP", "Rival com status",
    "Rival até 50% de HP", "Rival mais rápido",
    "Tenho mais ATQ que o rival", "Tenho mais DEF que o rival",
    "Minha equipe tem reserva"
};
static const int option_height = 40;
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
static Rectangle operator_box(int index) { return (Rectangle){414+index*250,443,230,44}; }
static Rectangle field_box(int field) { return field == 0 ? attack_box : condition_boxes[field-1]; }

static int visible_options(int field) { return field == 0 ? MAX_MOVES : 5; }
static Rectangle dropdown_box(int field)
{
    Rectangle box = field_box(field);
    return (Rectangle){box.x,box.y+box.height+4,box.width,12+visible_options(field)*option_height};
}
static Rectangle option_box(const LogicSetupScreen *screen, int field, int index)
{
    int row = index-screen->dropdown_scroll;
    if (row < 0 || row >= visible_options(field)) return (Rectangle){0};
    Rectangle box = dropdown_box(field);
    return (Rectangle){box.x+7,box.y+6+row*option_height,box.width-22,option_height};
}
static void reveal_option(LogicSetupScreen *screen, int count)
{
    int visible = visible_options(screen->dropdown);
    if (screen->option < screen->dropdown_scroll) screen->dropdown_scroll = screen->option;
    if (screen->option >= screen->dropdown_scroll+visible) screen->dropdown_scroll = screen->option-visible+1;
    int max_scroll = count-visible;
    if (max_scroll < 0) max_scroll = 0;
    if (screen->dropdown_scroll > max_scroll) screen->dropdown_scroll = max_scroll;
    if (screen->dropdown_scroll < 0) screen->dropdown_scroll = 0;
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

static void assign_question(LogicSetupScreen *screen, int plant_index)
{
    unsigned int plant_bit = 1u << plant_index;
    if (screen->question_assigned_mask & plant_bit) return;

    unsigned int all_questions = (1u << LOGIC_QUIZ_QUESTION_COUNT) - 1u;
    if ((screen->questions_seen_mask & all_questions) == all_questions)
        screen->questions_seen_mask = 0;

    int available[LOGIC_QUIZ_QUESTION_COUNT];
    int available_count = 0;
    for (int i = 0; i < LOGIC_QUIZ_QUESTION_COUNT; i++)
        if (!(screen->questions_seen_mask & (1u << i))) available[available_count++] = i;

    int choice = available[rand() % available_count];
    screen->question_indices[plant_index] = choice;
    screen->questions_seen_mask |= 1u << choice;
    screen->question_assigned_mask |= plant_bit;
}

static void advance_question(LogicSetupScreen *screen, int plant_index)
{
    unsigned int plant_bit = 1u << plant_index;
    int previous = screen->question_indices[plant_index];
    screen->question_assigned_mask &= ~plant_bit;
    assign_question(screen, plant_index);

    if (screen->question_indices[plant_index] == previous) {
        screen->questions_seen_mask &= ~(1u << previous);
        screen->question_assigned_mask &= ~plant_bit;
        assign_question(screen, plant_index);
    }
}

void logic_setup_begin(LogicSetupScreen *screen, const Team *team,
                       const LogicFormula formulas[MAX_TEAM_SIZE], const int moves[MAX_TEAM_SIZE])
{
    for (int i = 0; i < team->count && i < MAX_TEAM_SIZE; i++) {
        unsigned int bit = 1u << i;
        if (!(screen->initialized_mask & bit)) {
            screen->formulas[i] = valid_formula(formulas[i]) ? formulas[i]
                : (LogicFormula){LOGIC_OWN_LOW_HP,LOGIC_ENEMY_STATUS,LOGIC_OR};
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
    assign_question(screen, screen->selected);
    screen->dropdown = -1;
    screen->dropdown_scroll = 0;
    screen->elapsed = 0;
    screen->success_time = 0;
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
    float dt = fminf(.1f,fmaxf(0,GetFrameTime()));
    screen->elapsed += dt;
    screen->success_time = fmaxf(0,screen->success_time-dt);
    if (team->count <= 0 || team->count > MAX_TEAM_SIZE) return true;
    int selected = screen->selected;
    for (int i = 0; i < team->count; i++) {
        if (clicked(tab_box(i,team->count))) {
            screen->selected = i;
            assign_question(screen, i);
            screen->focus = 0;
            screen->dropdown = -1;
            return false;
        }
    }
    const Plant *plant = &team->plants[selected];
    if (screen->dropdown >= 0) {
        if (IsKeyPressed(KEY_ESCAPE)) { screen->dropdown = -1; return false; }
        int count = screen->dropdown == 0 ? MAX_MOVES : LOGIC_CONDITION_COUNT;
        int direction = IsKeyPressed(KEY_UP) ? -1 : IsKeyPressed(KEY_DOWN) ? 1 : 0;
        if (direction) {
            for (int attempts = 0; attempts < count; attempts++) {
                screen->option = (screen->option+count+direction)%count;
                if (option_enabled(screen,plant,screen->option)) break;
            }
            reveal_option(screen,count);
        }
        if (CheckCollisionPointRec(GetMousePosition(),dropdown_box(screen->dropdown))) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                int max_scroll = count-visible_options(screen->dropdown);
                if (max_scroll < 0) max_scroll = 0;
                screen->dropdown_scroll += wheel > 0 ? -1 : 1;
                if (screen->dropdown_scroll < 0) screen->dropdown_scroll = 0;
                if (screen->dropdown_scroll > max_scroll) screen->dropdown_scroll = max_scroll;
                if (screen->option < screen->dropdown_scroll) screen->option = screen->dropdown_scroll;
                if (screen->option >= screen->dropdown_scroll+visible_options(screen->dropdown))
                    screen->option = screen->dropdown_scroll+visible_options(screen->dropdown)-1;
            }
        }
        int choice = confirm_key() ? screen->option : -1;
        bool inside = CheckCollisionPointRec(GetMousePosition(),dropdown_box(screen->dropdown));
        for (int i = 0; i < count; i++) {
            Rectangle box = option_box(screen,screen->dropdown,i);
            if (box.width <= 0) continue;
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
        assign_question(screen, screen->selected);
        return false;
    }
    if (!unlocked) {
        if ((screen->focus == 1 || screen->focus == 2) &&
            (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))) screen->focus = 3-screen->focus;
        int answer = -1;
        if (IsKeyPressed(KEY_ONE)) answer = 0;
        else if (IsKeyPressed(KEY_TWO)) answer = 1;
        for (int i = 0; i < 2; i++) {
            if (clicked(answer_box(i))) { screen->focus = i+1; answer = i; }
            else if (screen->focus == i+1 && confirm_key()) answer = i;
        }
        if (screen->focus == 3 && confirm_key()) return true;
        if (answer >= 0) {
            screen->focus = answer+1;
            const LogicQuizQuestion *question = &questions[screen->question_indices[selected]];
            if ((answer == 0) == question->answer) {
                *solved |= 1u << selected;
                screen->dirty_mask |= 1u << selected;
                screen->rejected_mask &= ~(1u << selected);
                screen->success_time = .75f;
                screen->focus = 1;
            } else {
                screen->rejected_mask |= 1u << selected;
                screen->wrong_answer_count++;
                advance_question(screen, selected);
            }
        }
        return false;
    }
    for (int i = 0; i < 3; i++) {
        if (clicked(field_box(i)) || (screen->focus == i+1 && confirm_key())) {
            screen->focus = i+1;
            screen->dropdown = i;
            screen->option = i == 0 ? screen->moves[selected]
                : (int)(i == 1 ? screen->formulas[selected].left : screen->formulas[selected].right);
            screen->dropdown_scroll = 0;
            reveal_option(screen,i == 0 ? MAX_MOVES : LOGIC_CONDITION_COUNT);
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
            screen->success_time = .55f;
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

/* Cantos em degraus com recortes concêntricos: o preenchimento nunca invade a borda. */
static void chamfer(Rectangle box, int cut, Color color)
{
    int x = (int)roundf(box.x), y = (int)roundf(box.y);
    int w = (int)roundf(box.width), h = (int)roundf(box.height);
    if (w <= 0 || h <= 0) return;
    if (cut > h/2) cut = h/2;
    DrawRectangle(x,y+cut,w,h-2*cut,color);
    for (int row = 0; row < cut; row++) {
        int inset = cut-row;
        DrawRectangle(x+inset,y+row,w-2*inset,1,color);
        DrawRectangle(x+inset,y+h-1-row,w-2*inset,1,color);
    }
}

static void frame(Rectangle box, bool selected, bool focused, float elapsed)
{
    Color gold = {255,(unsigned char)(196+18*(.5f+.5f*sinf(elapsed*3))),52,255};
    chamfer((Rectangle){box.x+2,box.y+3,box.width,box.height},8,(Color){66,43,24,70});
    if (focused) chamfer((Rectangle){box.x-2,box.y-2,box.width+4,box.height+4},10,gold);
    const Color layers[] = {{46,42,25,255},{133,62,27,255},{246,170,54,255},
        {255,226,139,255},{180,92,35,255},{56,75,38,255}};
    for (int i = 0; i < 6; i++)
        chamfer((Rectangle){box.x+i,box.y+i,box.width-i*2,box.height-i*2},8-i,layers[i]);
    chamfer((Rectangle){box.x+6,box.y+6,box.width-12,box.height-12},2,selected ? sage : cream);
    DrawRectangle((int)box.x+10,(int)box.y+7,(int)box.width-20,2,
        selected ? (Color){234,244,182,255} : (Color){255,251,229,255});
}

static void arrow(float x, float y, bool up)
{
    for (int row = 0; row < 5; row++) {
        int width = 18-row*4;
        if (width < 2) width = 2;
        int yy = (int)y+(up ? 4-row : row)*2;
        DrawRectangle((int)x+row*2,yy,width,2,ink);
        if (width > 4) DrawRectangle((int)x+row*2+2,yy,width-4,2,(Color){240,158,35,255});
    }
}

static void pointer(const SelectionUiAssets *fonts, Rectangle box)
{
    selection_ui_skin(fonts,SELECTION_POINTER,(Rectangle){box.x+12,box.y+box.height/2-10,15,20},1);
}

/* A seta indica foco sem desenhar um retângulo sobre os cantos da arte botânica. */
static void action_button(const UiTheme *ui, const SelectionUiAssets *fonts, Rectangle box,
                          const char *value, bool focused)
{
    selection_ui_button(ui,fonts,box,value,false,false,1);
    if (focused) pointer(fonts,(Rectangle){box.x-36,box.y,26,box.height});
}

static void check(float x, float y)
{
    const Color green = {68,115,32,255};
    for (int i = 0; i < 3; i++) DrawRectangle((int)x+i*3,(int)y+7+i*3,4,4,green);
    for (int i = 0; i < 5; i++) DrawRectangle((int)x+6+i*3,(int)y+13-i*3,4,4,green);
}

/* Texto em tamanhos já rasterizados, com margem vertical baseada na tinta dos glifos. */
static void field_label(const UiTheme *ui, const SelectionUiAssets *fonts, const char *value,
                         Rectangle box, int size, Color color)
{
    if (selection_ui_measure(ui,fonts,value,size) > box.width && size > 22) size = 22;
    box.width = fminf(box.width,selection_ui_measure(ui,fonts,value,size));
    selection_ui_pixel_box(ui,fonts,value,box,size,color);
}

void logic_setup_draw(const UiTheme *ui, const SelectionUiAssets *fonts, const IntroAssets *art,
                      const LogicSetupScreen *screen, const Team *team, unsigned int solved)
{
    if (team->count <= 0 || team->count > MAX_TEAM_SIZE || screen->selected < 0 || screen->selected >= team->count) return;
    int index = screen->selected;
    unsigned int bit = 1u << index;
    bool unlocked = (solved & bit) != 0;
    const Plant *plant = &team->plants[index];
    float elapsed = screen->elapsed;
    DrawRectangle(0,0,1280,720,Fade(ink,.76f));
    ui_botanical_panel(ui,(Rectangle){56,38,1168,654});
    label(ui,fonts,"Desafio do golpe lógico",(Rectangle){110,52,1060,64},72);
    label(ui,fonts,"Uma resposta certa, um golpe desbloqueado",(Rectangle){110,117,1060,28},28);
    for (int i = 0; i < team->count; i++) {
        Rectangle box = tab_box(i,team->count);
        frame(box,i == index,screen->focus == 0 && i == index,elapsed);
        int size = selection_ui_measure(ui,fonts,team->plants[i].name_pt,30) <= box.width-64 ? 30 : 24;
        label(ui,fonts,team->plants[i].name_pt,(Rectangle){box.x+20,box.y+9,box.width-64,30},size);
        if ((screen->confirmed_mask & (1u << i)) && !(screen->dirty_mask & (1u << i)))
            check(box.x+box.width-34,box.y+17);
        else if (solved & (1u << i)) {
            DrawRectangle((int)(box.x+box.width-28),(int)box.y+21,8,8,(Color){187,123,26,255});
        }
    }
    DrawLine(82,216,1198,216,(Color){102,126,67,255});
    frame((Rectangle){100,232,255,365},false,false,elapsed);
    chamfer((Rectangle){107,239,241,43},3,(Color){231,229,180,255});
    int name_size = selection_ui_measure(ui,fonts,plant->name_pt,30) > 227 ? 24 : 30;
    label(ui,fonts,plant->name_pt,(Rectangle){114,243,227,32},name_size);
    Rectangle source = art->sources[plant->id];
    Texture2D sprite = art->plants[plant->id];
    if (sprite.id && source.width > 0 && source.height > 0) {
        float scale = fminf(180/source.width,208/source.height);
        DrawTexturePro(sprite,source,(Rectangle){roundf(227-source.width*scale/2),roundf(501-source.height*scale),
            source.width*scale,source.height*scale},(Vector2){0},0,WHITE);
    } else label(ui,fonts,plant->name_pt,(Rectangle){115,340,225,60},24);
    DrawLine(116,516,339,516,(Color){102,126,67,255});
    if (unlocked) check(116,538);
    label(ui,fonts,unlocked ? "Pergunta resolvida" : "Recompensa",(Rectangle){138,535,200,28},20);
    const char *state = !(screen->confirmed_mask & bit) ? "Configure seu golpe"
        : screen->dirty_mask & bit ? "Alterações pendentes" : "Configuração salva";
    label(ui,fonts,unlocked ? state : "Um golpe lógico",(Rectangle){112,566,231,22},20);
    if (unlocked && screen->success_time > 0)
        chamfer((Rectangle){110,529,235,61},4,Fade((Color){127,185,65,255},screen->success_time*.12f));
    DrawLine(377,232,377,617,(Color){102,126,67,255});
    if (!unlocked) {
        int question_index = screen->question_indices[index];
        if (question_index < 0 || question_index >= LOGIC_QUIZ_QUESTION_COUNT) question_index = 0;
        const LogicQuizQuestion *question = &questions[question_index];
        label(ui,fonts,"Avalie a expressão",(Rectangle){414,230,730,42},36);
        for (int i = 0; i < (question->has_q ? 2 : 1); i++) {
            Rectangle value = {question->has_q ? 414+i*380 : 604,285,350,64};
            frame(value,false,false,elapsed);
            label(ui,fonts,i ? "Q" : "P",(Rectangle){value.x+14,value.y+12,48,36},36);
            DrawLine((int)value.x+74,(int)value.y+13,(int)value.x+74,(int)value.y+51,(Color){163,165,105,255});
            label(ui,fonts,(i ? question->q : question->p) ? "Verdadeiro" : "Falso",
                (Rectangle){value.x+88,value.y+13,246,34},28);
        }
        label(ui,fonts,"Expressão",(Rectangle){464,352,630,26},22);
        frame((Rectangle){464,382,630,82},false,false,elapsed);
        int expression_size = selection_ui_measure(ui,fonts,question->expression,72) <= 594 ? 72 : 36;
        label(ui,fonts,question->expression,(Rectangle){482,389,594,64},expression_size);
        label(ui,fonts,"Essa expressão é verdadeira?",(Rectangle){414,478,730,31},28);
        for (int i = 0; i < 2; i++) {
            Rectangle box = answer_box(i);
            bool focused = screen->focus == i+1;
            frame(box,focused,focused,elapsed);
            if (focused) pointer(fonts,box);
            label(ui,fonts,i ? "2 · Falso" : "1 · Verdadeiro",(Rectangle){box.x+34,box.y+12,box.width-50,34},28);
        }
        if (screen->rejected_mask & bit) {
            chamfer((Rectangle){414,590,730,30},4,(Color){241,213,174,255});
            selection_ui_pixel_text(ui,fonts,"!",426,594,22,(Color){150,63,33,255});
            selection_ui_pixel_box(ui,fonts,"Resposta incorreta. Tente esta nova pergunta.",
                (Rectangle){448,594,680,22},22,(Color){130,54,30,255});
        } else label(ui,fonts,"Escolha uma resposta para desbloquear o golpe.",(Rectangle){414,591,730,27},20);
    } else {
        LogicFormula formula = screen->formulas[index];
        label(ui,fonts,"Monte seu golpe lógico",(Rectangle){414,224,730,37},36);
        text(ui,fonts,"Ataque",420,257,22);
        frame(attack_box,false,screen->focus == 1,elapsed);
        field_label(ui,fonts,plant->moves[screen->moves[index]].name,(Rectangle){430,290,680,28},28,ink);
        arrow(1113,299,screen->dropdown == 0);
        for (int i = 0; i < 2; i++) {
            Rectangle box = condition_boxes[i];
            text(ui,fonts,i ? "Condição Q" : "Condição P",box.x+6,328,22);
            frame(box,false,screen->focus == i+2,elapsed);
            field_label(ui,fonts,conditions[i ? formula.right : formula.left],
                (Rectangle){box.x+14,box.y+10,box.width-56,40},24,ink);
            arrow(box.x+box.width-30,box.y+26,screen->dropdown == i+1);
        }
        text(ui,fonts,"Operador",420,417,22);
        for (int i = 0; i < LOGIC_OPERATOR_COUNT; i++) {
            Rectangle box = operator_box(i);
            frame(box,(int)formula.operator == i,screen->focus == 4 && (int)formula.operator == i,elapsed);
            if ((int)formula.operator == i) pointer(fonts,box);
            label(ui,fonts,operators[i],(Rectangle){box.x+28,box.y+8,box.width-56,28},28);
        }
        frame((Rectangle){414,501,730,62},false,false,elapsed);
        label(ui,fonts,TextFormat("P %s Q",operators[formula.operator]),(Rectangle){425,507,708,28},30);
        label(ui,fonts,explanations[formula.operator],(Rectangle){425,537,708,20},20);
        text(ui,fonts,TextFormat("Verdadeira: +%d de poder",Battle_GetFormulaPowerBonus(formula.operator)),414,576,20);
        text(ui,fonts,"Falsa: este golpe fica bloqueado",414,600,20);
        action_button(ui,fonts,confirm_box,"Confirmar configuração",screen->focus == 5);
        if (screen->success_time > .55f)
            DrawRectangleRec((Rectangle){402,222,760,402},Fade(cream,.35f*(screen->success_time-.55f)/.2f));
    }
    DrawLine(82,632,1198,632,(Color){102,126,67,255});
    action_button(ui,fonts,back_box,"Voltar · ESC",screen->focus == (unlocked ? 6 : 3));
    label(ui,fonts,"TAB / setas: navegar · ENTER ou clique: selecionar",(Rectangle){350,644,800,30},20);
    if (screen->dropdown >= 0) {
        int field = screen->dropdown;
        int count = field == 0 ? MAX_MOVES : LOGIC_CONDITION_COUNT;
        int current = field == 0 ? screen->moves[index] : (int)(field == 1 ? screen->formulas[index].left : screen->formulas[index].right);
        Rectangle panel = dropdown_box(field);
        frame(panel,false,false,elapsed);
        for (int i = 0; i < count; i++) {
            Rectangle box = option_box(screen,field,i);
            if (box.width <= 0) continue;
            bool enabled = option_enabled(screen,plant,i), focused = screen->option == i;
            bool selected = i == current;
            if (focused && enabled) {
                chamfer(box,4,(Color){239,167,40,255});
                chamfer((Rectangle){box.x+2,box.y+2,box.width-4,box.height-4},3,sage);
            } else if (selected) chamfer(box,4,(Color){224,235,184,255});
            else if (!enabled) chamfer(box,3,(Color){222,219,200,255});
            if (selected) check(box.x+9,box.y+10);
            else if (focused && enabled) pointer(fonts,box);
            const char *value = field == 0 ? plant->moves[i].name : conditions[i];
            field_label(ui,fonts,value,(Rectangle){box.x+34,box.y+5,box.width-43,30},24,
                enabled ? ink : (Color){111,108,86,255});
            if (i < count-1) DrawLine((int)box.x+9,(int)(box.y+box.height-1),(int)(box.x+box.width-9),
                (int)(box.y+box.height-1),(Color){207,195,149,180});
        }
        if (count > visible_options(field)) {
            Rectangle track = {panel.x+panel.width-12,panel.y+10,4,panel.height-20};
            DrawRectangleRec(track,(Color){216,203,155,255});
            float height = track.height*visible_options(field)/count;
            float y = track.y+(track.height-height)*screen->dropdown_scroll/(count-visible_options(field));
            DrawRectangleRec((Rectangle){track.x,y,track.width,height},(Color){132,107,54,255});
        }
    }
}
