/* Verifica nomes, foco, transições e sessão com os módulos reais e OpenGL. */
#include "game.h"
#include "selection_ui.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void check_queue_codepoint(void *window, unsigned int codepoint);

static void input(unsigned int type,int a,int b)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}});
}

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
}

static void key(Game *game,int code)
{
    input(2,code,0); tick(game);
    input(1,code,0); tick(game); tick(game);
}

static void click(Game *game,int x,int y)
{
    input(7,x,y); input(6,0,0); tick(game);
    input(5,0,0); tick(game); tick(game);
}

static void settle(Game *game)
{
    for (int i=0;i<45;i++) tick(game);
}

static void text(Game *game,const char *value)
{
    while (*value) {
        int bytes=0;
        int codepoint=GetCodepointNext(value,&bytes);
        check_queue_codepoint(GetWindowHandle(),(unsigned int)codepoint);
        BeginDrawing(); EndDrawing();
        tick(game);
        value+=bytes;
    }
}

static void append(NameEntryScreen *screen,const char *value)
{
    while (*value) {
        int bytes=0,codepoint=GetCodepointNext(value,&bytes);
        assert(name_entry_append(screen,codepoint));
        value+=bytes;
    }
}

static void capture(Game *game,RenderTexture2D target,const char *path)
{
    Game before=*game;
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before,game,sizeof(before))==0);
    Image image=LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    if (strstr(path,".ppm")) {
        FILE *file=fopen(path,"wb"); assert(file);
        fprintf(file,"P6\n%d %d\n255\n",image.width,image.height);
        ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        size_t size=(size_t)image.width*image.height*3;
        assert(fwrite(image.data,1,size,file)==size);
        assert(fclose(file)==0);
    } else assert(ExportImage(image,path));
    UnloadImage(image);
}

static void check_names(void)
{
    NameEntryScreen screen;
    char result[PLAYER_NAME_CAPACITY];
    name_entry_init(&screen);
    assert(screen.count==0 && screen.draft[0]==0);
    assert(!name_entry_confirm(&screen,result) && screen.error);
    append(&screen,"   ");
    assert(!name_entry_confirm(&screen,result));
    name_entry_init(&screen);
    append(&screen,"  Antônio  ");
    assert(screen.count==11 && name_entry_confirm(&screen,result));
    assert(strcmp(result,"Antônio")==0);
    name_entry_init(&screen); append(&screen,"João");
    assert(screen.count==4 && strlen(screen.draft)==5);
    name_entry_backspace(&screen); name_entry_backspace(&screen);
    assert(screen.count==2 && strcmp(screen.draft,"Jo")==0);
    name_entry_init(&screen);
    for (int i=0;i<12;i++) assert(name_entry_append(&screen,0xe7));
    assert(screen.count==12 && strlen(screen.draft)==24 && !name_entry_append(&screen,'a'));
    name_entry_backspace(&screen);
    assert(screen.count==11 && strlen(screen.draft)==22);
    name_entry_init(&screen);
    assert(name_entry_append(&screen,0x10400));
    assert(strlen(screen.draft)==4 && screen.count==1);
    name_entry_backspace(&screen); assert(screen.draft[0]==0 && screen.count==0);
    for (int i=0;i<12;i++) assert(name_entry_append(&screen,0x10400));
    assert(strlen(screen.draft)==48 && screen.count==12 && !name_entry_append(&screen,0x10400));
    for (int i=0;i<12;i++) name_entry_backspace(&screen);
    assert(!name_entry_append(&screen,0x1f600));
    assert(!name_entry_append(&screen,'\n') && !name_entry_append(&screen,0x7f));
    append(&screen,"D'Ávila-2");
    assert(name_entry_confirm(&screen,result));
    Font font=GetFontDefault();
    int accents[]={0xe7,0xe3,0xf4,0xc1};
    for (unsigned int i=0;i<sizeof(accents)/sizeof(accents[0]);i++)
        assert(font.glyphs[GetGlyphIndex(font,accents[i])].value==accents[i]);
}

int main(void)
{
    assert(DirectoryExists("build/name_entry_preview") || MakeDirectory("build/name_entry_preview")==0);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Verificacao de nome de campanha");
    assert(IsWindowReady()); SetExitKey(KEY_NULL); SetTargetFPS(60);
    check_names();
    Game game; game_init(&game);
    game.screen=GAME_TRAINER_SELECT;
    for (int gender=0;gender<2;gender++) {
        trainer_select_screen_init(&game.trainer_select);
        game.trainer_select.selected=(Trainer)gender;
        name_entry_init(&game.name_entry);
        game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
        float male=game.trainer_select.male_idle_time, female=game.trainer_select.female_idle_time;
        input(2,KEY_ENTER,0); tick(&game);
        assert(game.screen==GAME_NAME_ENTRY && game.name_entry.direction==1);
        text(&game,"AD"); /* Texto durante a transição deve ser descartado. */
        settle(&game);
        assert(game.name_entry.progress==1 && game.name_entry.input_blocked);
        assert(game.name_entry.draft[0]==0 && game.name_entry.focus==0);
        assert(game.trainer_select.male_idle_time>male && game.trainer_select.female_idle_time>female);
        assert(game.trainer_select.selected==(Trainer)gender);
        input(1,KEY_ENTER,0); tick(&game); tick(&game);
        text(&game,gender ? "  João  " : "Antônio");
        assert(strcmp(game.name_entry.draft,gender ? "  João  " : "Antônio")==0);
        key(&game,KEY_A); key(&game,KEY_D); key(&game,KEY_LEFT); key(&game,KEY_RIGHT);
        assert(game.trainer_select.selected==(Trainer)gender);
        key(&game,KEY_TAB); assert(game.name_entry.focus==1);
        input(2,KEY_LEFT_SHIFT,0); key(&game,KEY_TAB); input(1,KEY_LEFT_SHIFT,0); tick(&game);
        assert(game.name_entry.focus==0);
        click(&game,950,570); /* Confirmar por mouse transfere nome e personagem. */
        assert(game.screen==GAME_EXPLORATION && game.campaign.ready);
        assert(game.campaign.trainer==(Trainer)gender && game.player.trainer==(Trainer)gender);
        assert(strcmp(game.campaign.player_name,gender ? "João" : "Antônio")==0);
        key(&game,KEY_ESCAPE);
        game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
        game.name_entry.progress=0;
        key(&game,KEY_ENTER); settle(&game);
        assert(game.screen==GAME_NAME_ENTRY);
        char draft[PLAYER_NAME_CAPACITY]; strcpy(draft,game.name_entry.draft);
        key(&game,KEY_ESCAPE); settle(&game);
        assert(game.screen==GAME_TRAINER_SELECT && game.trainer_select.selected==(Trainer)gender);
        assert(strcmp(draft,game.name_entry.draft)==0);
    }
    /* Foco, validação e repetição de Backspace pelo módulo real. */
    game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
    name_entry_init(&game.name_entry);
    key(&game,KEY_ENTER); settle(&game);
    key(&game,KEY_ENTER); assert(game.screen==GAME_NAME_ENTRY && game.name_entry.error);
    text(&game,"çãôéABCDEFGHI");
    assert(game.name_entry.count==12);
    input(2,KEY_BACKSPACE,0); tick(&game);
    for (int i=0;i<12;i++) name_entry_update(&game.name_entry,true,.1f);
    input(1,KEY_BACKSPACE,0); tick(&game);
    assert(game.name_entry.count==0 && game.name_entry.draft[0]==0);
    text(&game,"João");
    click(&game,680,570); settle(&game);
    assert(game.screen==GAME_TRAINER_SELECT && strcmp(game.name_entry.draft,"João")==0);
    key(&game,KEY_ENTER); settle(&game);
    assert(strcmp(game.name_entry.draft,"João")==0 && game.name_entry.focus==0);
    key(&game,KEY_TAB); key(&game,KEY_TAB); assert(game.name_entry.focus==2);
    key(&game,KEY_ENTER); assert(game.screen==GAME_EXPLORATION);
    /* Botão Voltar focado por teclado e soltura de mouse mantido na entrada. */
    game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
    name_entry_init(&game.name_entry);
    input(7,820,360); input(6,0,0); tick(&game);
    key(&game,KEY_ENTER); settle(&game);
    assert(game.screen==GAME_NAME_ENTRY && game.name_entry.input_blocked);
    text(&game,"Não"); assert(game.name_entry.count==0);
    input(5,0,0); tick(&game); tick(&game);
    text(&game,"João");
    key(&game,KEY_TAB); assert(game.name_entry.focus==1);
    input(2,KEY_ENTER,0); tick(&game); settle(&game);
    assert(game.screen==GAME_TRAINER_SELECT && game.input_blocked);
    input(1,KEY_ENTER,0); tick(&game); tick(&game);
    assert(!game.input_blocked);
    /* Capturas temporais controladas: relógios avançam com o mesmo dt da transição. */
    RenderTexture2D target=LoadRenderTexture(1280,720);
    for (int gender=0;gender<2;gender++) {
        trainer_select_screen_init(&game.trainer_select);
        game.trainer_select.selected=(Trainer)gender;
        name_entry_init(&game.name_entry);
        game.screen=GAME_TRAINER_SELECT;
        capture(&game,target,TextFormat("build/name_entry_preview/selection_%d.ppm",gender));
        game.screen=GAME_NAME_ENTRY;
        name_entry_begin(&game.name_entry,true);
        for (int i=0;i<=12;i++) {
            Rectangle card=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
            assert(card.x>=172 && card.width>=308 && card.width<=336);
            capture(&game,target,TextFormat("build/name_entry_preview/forward_%d_%02d.ppm",gender,i));
            if (i<12) {
                trainer_select_screen_update(&game.trainer_select,false,.05f);
                name_entry_update(&game.name_entry,false,.05f);
            }
        }
        Rectangle last=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
        Rectangle desired=name_entry_portrait_bounds();
        assert(memcmp(&last,&desired,sizeof(last))==0 && game.name_entry.progress==1);
        capture(&game,target,TextFormat("build/name_entry_preview/empty_%d.png",gender));
        append(&game.name_entry,gender ? "João" : "Antônio");
        capture(&game,target,TextFormat("build/name_entry_preview/final_%d.png",gender));
        name_entry_begin(&game.name_entry,false);
        for (int i=0;i<=12;i++) {
            capture(&game,target,TextFormat("build/name_entry_preview/reverse_%d_%02d.ppm",gender,i));
            if (i<12) {
                trainer_select_screen_update(&game.trainer_select,false,.05f);
                name_entry_update(&game.name_entry,false,.05f);
            }
        }
        last=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
        desired=trainer_select_card_bounds((Trainer)gender);
        assert(memcmp(&last,&desired,sizeof(last))==0 && game.name_entry.progress==0);
        game.screen=GAME_TRAINER_SELECT;
        capture(&game,target,TextFormat("build/name_entry_preview/selection_end_%d.ppm",gender));
    }
    UnloadRenderTexture(target); game_shutdown(&game); CloseWindow();
    puts("PASS: UTF-8/acentos, 12 codepoints, Backspace multibyte/repetido, nomes vazios, foco, fila nativa, descarte, entrada mantida, sessão, rascunho, dois sentidos/personagens e desenho imutável.");
    return 0;
}
