/* Verifica o editor por teclado/mouse e captura perguntas, estados e listas em várias resoluções. */
#include "logic_setup_screen.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    LogicSetupScreen screen;
    Team team;
    LogicFormula formulas[MAX_TEAM_SIZE];
    int moves[MAX_TEAM_SIZE];
    unsigned int solved;
    bool closed;
} Harness;
static void event(unsigned int type,int a,int b)
{ PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}}); }
static void tick(Harness *h)
{
    h->closed = logic_setup_update(&h->screen,&h->team,&h->solved,h->formulas,h->moves) || h->closed;
    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
}
static void key(Harness *h,int code)
{ event(2,code,0); tick(h); event(1,code,0); tick(h); }
static void click(Harness *h,int x,int y)
{ event(7,x,y); event(6,0,0); tick(h); event(5,0,0); tick(h); }
static void wheel(Harness *h,int direction)
{ event(7,600,440); event(8,0,direction); tick(h); }
static void settle(Harness *h)
{ for(int i=0;i<95;i++) tick(h); }
static void capture(Harness *h,UiTheme *ui,SelectionUiAssets *fonts,IntroAssets *art,
                    Texture2D background,int width,int height,const char *name)
{
    GameCanvas canvas={0}; assert(game_canvas_resize(&canvas,width,height,width,height));
    selection_ui_update_fonts(fonts,canvas.target.texture.width/1280.0f);
    Harness before=*h;
    srand(200); int expected=rand(); srand(200);
    BeginDrawing(); game_canvas_begin(&canvas); ClearBackground(DARKGREEN);
    if(background.id) DrawTexturePro(background,(Rectangle){0,0,background.width,background.height},
        (Rectangle){0,0,1280,720},(Vector2){0},0,WHITE);
    logic_setup_draw(ui,fonts,art,&h->screen,&h->team,h->solved);
    game_canvas_present(&canvas); EndDrawing();
    assert(rand()==expected && memcmp(&before,h,sizeof(before))==0);
    Image image=LoadImageFromTexture(canvas.target.texture); ImageFlipVertical(&image);
    assert(ExportImage(image,TextFormat("build/logic_ui_preview/%s.png",name)));
    UnloadImage(image); game_canvas_unload(&canvas);
}
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Golpes lógicos"); SetExitKey(KEY_NULL); SetTargetFPS(120);
    assert(IsWindowReady());
    assert(DirectoryExists("build/logic_ui_preview") || MakeDirectory("build/logic_ui_preview")==0);
    UiTheme ui; ui_theme_load(&ui); SelectionUiAssets fonts; selection_ui_assets_load(&fonts);
    IntroAssets art; intro_assets_load(&art);
    Texture2D background=LoadTexture("assets/maps/botanical_courtyard.png");
    Harness h={0};
    Team_SelectSpecies(&h.team,0,SPECIES_CLOVER); Team_SelectSpecies(&h.team,1,SPECIES_PEPPER);
    Team_SelectSpecies(&h.team,2,SPECIES_CACTUS);
    for(int i=0;i<MAX_TEAM_SIZE;i++) h.moves[i]=-1;
    logic_setup_begin(&h.screen,&h.team,h.formulas,h.moves);
    h.screen.question_indices[0]=0; h.screen.focus=1;
    capture(&h,&ui,&fonts,&art,background,1280,720,"question");
    key(&h,KEY_TWO);
    assert(!h.solved && h.screen.wrong_answer_count==1 && (h.screen.rejected_mask&1));
    assert(h.screen.question_indices[0]!=0);
    capture(&h,&ui,&fonts,&art,background,1280,720,"question_error");
    h.screen.question_indices[0]=2; /* NÃO P, com P falso. */
    capture(&h,&ui,&fonts,&art,background,1280,720,"question_unary");
    key(&h,KEY_ONE);
    assert(h.solved==1 && h.screen.success_time>0 && h.moves[0]==-1);
    capture(&h,&ui,&fonts,&art,background,1280,720,"unlock_transition"); settle(&h);
    capture(&h,&ui,&fonts,&art,background,1280,720,"editor");
    click(&h,700,304); assert(h.screen.dropdown==0);
    key(&h,KEY_DOWN); key(&h,KEY_ENTER);
    assert(h.screen.moves[0]==1 && h.moves[0]==-1);
    click(&h,600,380); key(&h,KEY_UP);
    assert(h.screen.option==LOGIC_OWN_RESERVE && h.screen.dropdown_scroll==2);
    capture(&h,&ui,&fonts,&art,background,1280,720,"dropdown_scrolled");
    key(&h,KEY_ENTER); assert(h.screen.formulas[0].left==LOGIC_OWN_RESERVE);
    click(&h,970,380); key(&h,KEY_UP); key(&h,KEY_ENTER);
    assert(h.screen.formulas[0].right==LOGIC_OWN_LOW_HP);
    click(&h,1050,465); assert(h.screen.formulas[0].operator==LOGIC_XOR);
    click(&h,1000,595); assert(h.moves[0]==1 && h.formulas[0].operator==LOGIC_XOR);
    assert((h.screen.confirmed_mask&1) && !(h.screen.dirty_mask&1)); settle(&h);
    capture(&h,&ui,&fonts,&art,background,1920,1080,"confirmed_1080p");
    click(&h,600,380); assert(h.screen.dropdown_scroll==2);
    wheel(&h,1); wheel(&h,1); assert(h.screen.dropdown_scroll==0);
    LogicFormula draft=h.screen.formulas[0];
    click(&h,600,440); /* Condição P duplicaria a condição Q: fica desabilitada. */
    assert(h.screen.dropdown==1 && memcmp(&draft,&h.screen.formulas[0],sizeof(draft))==0);
    capture(&h,&ui,&fonts,&art,background,1280,720,"dropdown");
    capture(&h,&ui,&fonts,&art,background,1920,1080,"dropdown_1080p");
    capture(&h,&ui,&fonts,&art,background,1024,768,"dropdown_letterbox");
    wheel(&h,-1); wheel(&h,-1); assert(h.screen.dropdown_scroll==2);
    key(&h,KEY_ESCAPE); assert(h.screen.dropdown==-1 && !h.closed);
    click(&h,530,465); assert(h.screen.formulas[0].operator==LOGIC_AND && (h.screen.dirty_mask&1));
    assert(h.formulas[0].operator==LOGIC_XOR);
    click(&h,700,304); click(&h,1000,595);
    assert(h.screen.dropdown==-1 && h.formulas[0].operator==LOGIC_XOR); /* Sem clique através da lista. */
    key(&h,KEY_ESCAPE); assert(h.closed);
    h.closed=false;
    logic_setup_begin(&h.screen,&h.team,h.formulas,h.moves);
    assert(h.screen.formulas[0].operator==LOGIC_AND && (h.screen.dirty_mask&1));
    click(&h,640,175); assert(h.screen.selected==1);
    h.screen.question_indices[1]=0; key(&h,KEY_ONE); settle(&h);
    click(&h,700,304); key(&h,KEY_DOWN);
    assert(h.screen.option==2); /* Crescer é atributo: a navegação pula essa opção. */
    key(&h,KEY_ENTER); assert(h.screen.moves[1]==2 && h.moves[1]==-1);
    capture(&h,&ui,&fonts,&art,background,1280,720,"pepper_draft");
    click(&h,980,175); h.screen.question_indices[2]=9;
    capture(&h,&ui,&fonts,&art,background,1920,1080,"long_expression_1080p");
    h.team.plants[2]=Plant_CreateFromSpecies(SPECIES_CARNIVOROUS_PLANT);
    capture(&h,&ui,&fonts,&art,background,1280,720,"long_plant_name");
    if(background.id) UnloadTexture(background);
    intro_assets_unload(&art); selection_ui_assets_unload(&fonts); ui_theme_unload(&ui); CloseWindow();
    puts("PASS: pergunta/erro/acerto, transição, rascunhos por planta, confirmação, lista única, teclado/roda, opção inválida, fechamento sem clique através, 720p/1080p/4:3 e desenho sem RNG/mutação.");
    return 0;
}
