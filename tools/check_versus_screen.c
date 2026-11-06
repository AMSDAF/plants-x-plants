/* Verifica navegação, confirmação isolada e capturas OpenGL da tela Versus. */
#include "game.h"
#include "versus_grass_data.h"
#include <math.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void input(unsigned int type, int a, int b)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}});
}
static void tick(Game *game)
{
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
}
static void key(Game *game, int code)
{
    input(2,code,0); tick(game);
    input(1,code,0); tick(game); tick(game);
}
static void click(Game *game, int x, int y)
{
    input(7,x,y); input(6,0,0); tick(game);
    input(5,0,0); tick(game); tick(game);
}
static void settle(Game *game)
{
    for (int i=0;i<15;i++) tick(game);
}
static void capture(Game *game, RenderTexture2D target, const char *path)
{
    Game before=*game;
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before,game,sizeof(before))==0);
    Image image=LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    assert(ExportImage(image,path));
    UnloadImage(image);
}
/* Compara pixels reais: toda alteração do cenário deve ficar junto às lâminas. */
static Image background_capture(const VersusAssets *assets, RenderTexture2D target, float time)
{
    BeginTextureMode(target); versus_background_draw(assets,time); EndTextureMode();
    Image image=LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    return image;
}

static void save_ppm(Image image, const char *path)
{
    Image rgb=ImageCopy(image);
    ImageFormat(&rgb,PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    FILE *file=fopen(path,"wb"); assert(file);
    fprintf(file,"P6\n%d %d\n255\n",rgb.width,rgb.height);
    size_t bytes=(size_t)rgb.width*rgb.height*3;
    assert(fwrite(rgb.data,1,bytes,file)==bytes);
    assert(fclose(file)==0);
    UnloadImage(rgb);
}

static void check_grass(Game *game, RenderTexture2D target)
{
    assert(DirectoryExists("build/versus_grass_preview") || MakeDirectory("build/versus_grass_preview")==0);
    game->screen=GAME_VERSUS_SELECT;
    const VersusAssets saved=game->versus_assets;
    assert(saved.grass_base.id && saved.grass_atlas.id);
    Image first=background_capture(&saved,target,0);
    Color *pixels=first.data;
    int total_changes=0;
    int region_changes[VERSUS_GRASS_COUNT]={0};
    float scale=1280.0f/1672;
    float origin_y=(720-941*scale)/2;
    for (int frame=0;frame<64;frame++) {
        Image current=background_capture(&saved,target,frame*.125f);
        const Color *next=current.data;
        for (int y=0;y<720;y++) {
            for (int x=0;x<1280;x++) {
                if (memcmp(&pixels[y*1280+x],&next[y*1280+x],sizeof(Color))==0) continue;
                bool inside=false;
                for (int i=0;i<VERSUS_GRASS_COUNT;i++) {
                    const VersusGrassRegion *r=&VERSUS_GRASS_REGIONS[i];
                    Rectangle bounds={r->origin.x*scale-1,origin_y+r->origin.y*scale-1,
                                      r->source.width*scale+2,r->source.height*scale+2};
                    if (CheckCollisionPointRec((Vector2){(float)x,(float)y},bounds)) {
                        inside=true; region_changes[i]++;
                    }
                }
                assert(inside);
                total_changes++;
            }
        }
        game->versus.elapsed=frame*.125f;
        Game before=*game;
        BeginTextureMode(target); game_draw(game); EndTextureMode();
        assert(memcmp(&before,game,sizeof(before))==0);
        Image full=LoadImageFromTexture(target.texture); ImageFlipVertical(&full);
        save_ppm(full,TextFormat("build/versus_grass_preview/runtime_%02d.ppm",frame));
        UnloadImage(full);
        UnloadImage(current);
    }
    assert(total_changes>0);
    for (int i=0;i<VERSUS_GRASS_COUNT;i++) assert(region_changes[i]>0);
    Image loop=background_capture(&saved,target,32);
    assert(memcmp(first.data,loop.data,(size_t)1280*720*4)==0);
    UnloadImage(loop); UnloadImage(first);
    /* As duas falhas parciais precisam recuperar exatamente o fundo estático. */
    VersusAssets fallback=saved;
    fallback.grass_base=fallback.grass_atlas=(Texture2D){0};
    Image original=background_capture(&fallback,target,0);
    fallback=saved; fallback.grass_base=(Texture2D){0};
    Image partial=background_capture(&fallback,target,1);
    assert(memcmp(original.data,partial.data,(size_t)1280*720*4)==0);
    UnloadImage(partial);
    fallback=saved; fallback.grass_atlas=(Texture2D){0};
    partial=background_capture(&fallback,target,2);
    assert(memcmp(original.data,partial.data,(size_t)1280*720*4)==0);
    UnloadImage(partial); UnloadImage(original);
    /* Fixa os pulsos já existentes com tempos inteiros: UI e arte dos cartões não mudam. */
    game->screen=GAME_VERSUS_SELECT;
    game->versus.elapsed=0;
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    Image ui=LoadImageFromTexture(target.texture); ImageFlipVertical(&ui);
    const Color *before=ui.data;
    for (int t=0;t<8;t++) {
        game->versus.elapsed=(float)t;
        BeginTextureMode(target); game_draw(game); EndTextureMode();
        Image current=LoadImageFromTexture(target.texture); ImageFlipVertical(&current);
        const Color *next=current.data;
        for (int y=258;y<552;y++) {
            for (int x=208;x<1072;x++) {
                if (x>=624 && x<656) continue;
                assert(memcmp(&before[y*1280+x],&next[y*1280+x],sizeof(Color))==0);
            }
        }
        if (t==0 || t==1 || t==3 || t==7)
            assert(ExportImage(current,TextFormat("build/versus_grass_preview/screen_%d.png",t)));
        UnloadImage(current);
    }
    UnloadImage(ui);
    VersusScreen clock=game->versus;
    clock.elapsed=127.99f;
    versus_screen_update(&clock,false,.1f);
    assert(clock.elapsed<.1f);
    puts("PASS: 20 touceiras, 64 instantes OpenGL, somente grama muda, ritmos distintos, ciclo de 32s, fallback parcial e cartoes imoveis.");
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280,720,"Verificacao Versus");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL); SetTargetFPS(60);
    Game game; game_init(&game);
    assert(game.versus_assets.background.id && game.versus_assets.illustrations.id);
    assert(DirectoryExists("build/versus_preview") || MakeDirectory("build/versus_preview")==0);
    RenderTexture2D target=LoadRenderTexture(1280,720);
    key(&game,KEY_ENTER);
    key(&game,KEY_DOWN); assert(game.menu.selected_option==1);
    input(2,KEY_ENTER,0); settle(&game);
    assert(game.screen==GAME_VERSUS_SELECT && !game.exit_requested);
    assert(!game.versus_mode_selected && game.input_blocked);
    float elapsed=game.versus.elapsed;
    settle(&game); assert(game.versus.elapsed>elapsed && !game.versus_mode_selected);
    input(1,KEY_ENTER,0); tick(&game); tick(&game);
    capture(&game,target,"build/versus_preview/cpu.png");
    key(&game,KEY_D); assert(game.versus.selected==VERSUS_PLAYER);
    assert(!game.versus_mode_selected);
    key(&game,KEY_LEFT); assert(game.versus.selected==VERSUS_CPU);
    key(&game,KEY_RIGHT); assert(game.versus.selected==VERSUS_PLAYER);
    CampaignSession campaign=game.campaign;
    key(&game,KEY_ENTER);
    assert(game.versus_mode_selected && game.versus_mode==VERSUS_PLAYER);
    assert(game.screen==GAME_VERSUS_SELECT && game.versus.confirmed);
    assert(memcmp(&campaign,&game.campaign,sizeof(campaign))==0);
    capture(&game,target,"build/versus_preview/player_confirmed.png");
    click(&game,410,400); assert(game.versus.selected==VERSUS_CPU && !game.versus.confirmed);
    assert(game.versus_mode==VERSUS_PLAYER);
    input(7,780,618); input(6,0,0); tick(&game);
    input(7,20,20); input(5,0,0); tick(&game);
    assert(game.versus_mode==VERSUS_PLAYER && game.versus.pressed==-1);
    click(&game,780,618); assert(game.versus_mode==VERSUS_CPU);
    capture(&game,target,"build/versus_preview/cpu_confirmed.png");
    click(&game,490,618); assert(game.screen==GAME_MENU && game.menu.menu_open);
    click(&game,1000,460);
    assert(game.screen==GAME_VERSUS_SELECT && !game.versus.confirmed);
    assert(game.versus.selected==VERSUS_CPU);
    click(&game,850,400); key(&game,KEY_ESCAPE);
    assert(game.screen==GAME_MENU && game.versus_mode==VERSUS_CPU);
    click(&game,1000,460); assert(game.versus.selected==VERSUS_CPU);
    VersusAssets saved=game.versus_assets;
    game.versus_assets=(VersusAssets){0};
    capture(&game,target,"build/versus_preview/fallback.png");
    game.versus_assets=saved;
    key(&game,KEY_ESCAPE);
    click(&game,1000,397); assert(game.screen==GAME_TRAINER_SELECT);
    key(&game,KEY_RIGHT); key(&game,KEY_ENTER);
    for (int i=0;i<45;i++) tick(&game);
    assert(game.screen==GAME_NAME_ENTRY && game.trainer_select.selected==TRAINER_FEMALE);
    key(&game,KEY_ESCAPE);
    for (int i=0;i<45;i++) tick(&game);
    assert(game.screen==GAME_TRAINER_SELECT);
    key(&game,KEY_ESCAPE); click(&game,1000,529); assert(game.exit_requested);
    check_grass(&game,target);
    UnloadRenderTexture(target); game_shutdown(&game);
    assert(!game.versus_assets.background.id && !game.versus_assets.illustrations.id);
    assert(!game.versus_assets.grass_base.id && !game.versus_assets.grass_atlas.id);
    CloseWindow();
    puts("PASS: Versus teclado/mouse, entrada mantida, confirmar/voltar, cancelamento de clique, modos em memoria, campanha preservada, fallback, desenho imutavel e recursos liberados.");
    return 0;
}
