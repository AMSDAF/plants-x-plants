/* Verifica navegação, confirmação isolada e capturas OpenGL da tela Versus. */
#include "game.h"
#include "versus_grass_data.h"
#include "versus_foliage_data.h"
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
    VersusAssets saved=game->versus_assets;
    saved.foliage_base=saved.foliage_atlas=(Texture2D){0};
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
                if (x<224 || (x>=608 && x<672) || x>=1056) continue;
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

/* Fixtures isoladas exercitam a carga real sem renomear recursos do jogo. */
static void check_foliage_loader(void)
{
    char root[1024]; snprintf(root,sizeof(root),"%s",GetWorkingDirectory());
    const char *cases[]={"missing","bad_base","bad_atlas"};
    const char *resources[]={"versus_background_generated_v1.png","versus/grass_base.png",
                             "versus/grass_atlas.png","versus/foliage_base.png","versus/foliage_atlas.png"};
    assert(DirectoryExists("build/versus_foliage_preview/fixtures") ||
           MakeDirectory("build/versus_foliage_preview/fixtures")==0);
    for (int c=0;c<3;c++) {
        char base[256]; snprintf(base,sizeof(base),"build/versus_foliage_preview/fixtures/%s",cases[c]);
        const char *suffixes[]={"","/assets","/assets/sprites","/assets/sprites/versus"};
        for (int i=0;i<4;i++) {
            const char *dir=TextFormat("%s%s",base,suffixes[i]);
            assert(DirectoryExists(dir) || MakeDirectory(dir)==0);
        }
        for (int i=0;i<5;i++) {
            if (i>=3 && c==0) continue;
            int size=0;
            unsigned char *data=LoadFileData(TextFormat("assets/sprites/%s",resources[i]),&size);
            assert(data && size>0);
            assert(SaveFileData(TextFormat("%s/assets/sprites/%s",base,resources[i]),data,size));
            UnloadFileData(data);
        }
        if (c>0) {
            Image invalid=GenImageColor(16,16,WHITE);
            assert(ExportImage(invalid,TextFormat("%s/assets/sprites/versus/foliage_%s.png",base,c==1 ? "base" : "atlas")));
            UnloadImage(invalid);
        }
        assert(ChangeDirectory(base));
        VersusAssets partial; versus_assets_load(&partial);
        assert(partial.background.id && partial.grass_base.id && partial.grass_atlas.id);
        assert(!partial.foliage_base.id && !partial.foliage_atlas.id);
        versus_assets_unload(&partial);
        assert(!partial.background.id && !partial.grass_base.id && !partial.grass_atlas.id);
        assert(ChangeDirectory(root));
    }
    puts("PASS: carga real com folhagem ausente, base invalida e atlas invalido; grama preservada e recursos liberados.");
}

static void check_foliage(Game *game, RenderTexture2D target)
{
    assert(DirectoryExists("build/versus_foliage_preview") || MakeDirectory("build/versus_foliage_preview")==0);
    VersusAssets saved=game->versus_assets;
    assert(saved.foliage_base.id && saved.foliage_atlas.id);
    VersusAssets grass=saved;
    grass.foliage_base=grass.foliage_atlas=(Texture2D){0};
    Image first=background_capture(&saved,target,0);
    int changes[VERSUS_FOLIAGE_COUNT]={0};
    float scale=1280.0f/1672, origin_y=(720-941*scale)/2;
    for (int frame=0;frame<64;frame++) {
        float time=frame*.125f;
        Image current=background_capture(&saved,target,time);
        Image reference=background_capture(&grass,target,time);
        const Color *pixels=current.data,*before=first.data,*old=reference.data;
        for (int y=0;y<720;y++) {
            for (int x=0;x<1280;x++) {
                if (memcmp(&pixels[y*1280+x],&before[y*1280+x],sizeof(Color))==0) continue;
                bool allowed=false;
                for (int i=0;i<VERSUS_FOLIAGE_COUNT;i++) {
                    const VersusFoliageRegion *r=&VERSUS_FOLIAGE_REGIONS[i];
                    Rectangle bounds={r->origin.x*scale-1,origin_y+r->origin.y*scale-1,
                                      r->poses[0].width*scale+2,r->poses[0].height*scale+2};
                    if (CheckCollisionPointRec((Vector2){(float)x,(float)y},bounds)) {
                        allowed=true; changes[i]++;
                    }
                }
                for (int i=0;i<VERSUS_GRASS_COUNT;i++) {
                    const VersusGrassRegion *r=&VERSUS_GRASS_REGIONS[i];
                    Rectangle bounds={r->origin.x*scale-1,origin_y+r->origin.y*scale-1,
                                      r->source.width*scale+2,r->source.height*scale+2};
                    if (CheckCollisionPointRec((Vector2){(float)x,(float)y},bounds)) allowed=true;
                }
                assert(allowed);
            }
        }
        /* A mesma grama precisa produzir exatamente os mesmos pixels em todos os tempos. */
        for (int i=0;i<VERSUS_GRASS_COUNT;i++) {
            const VersusGrassRegion *r=&VERSUS_GRASS_REGIONS[i];
            int x0=(int)ceilf(r->origin.x*scale),y0=(int)ceilf(origin_y+r->origin.y*scale);
            int x1=(int)floorf((r->origin.x+r->source.width)*scale);
            int y1=(int)floorf(origin_y+(r->origin.y+r->source.height)*scale);
            for (int y=y0;y<y1;y++) for (int x=x0;x<x1;x++)
                assert(memcmp(&pixels[y*1280+x],&old[y*1280+x],sizeof(Color))==0);
        }
        if (frame==0) {
            VersusAssets missing=saved; missing.foliage_base=(Texture2D){0};
            Image fallback=background_capture(&missing,target,time);
            assert(memcmp(reference.data,fallback.data,(size_t)1280*720*4)==0);
            assert(ExportImage(fallback,"build/versus_foliage_preview/fallback_base.png"));
            UnloadImage(fallback);
            missing=saved; missing.foliage_atlas=(Texture2D){0};
            fallback=background_capture(&missing,target,time);
            assert(memcmp(reference.data,fallback.data,(size_t)1280*720*4)==0);
            assert(ExportImage(fallback,"build/versus_foliage_preview/fallback_atlas.png"));
            UnloadImage(fallback);
        }
        game->versus.elapsed=time;
        Game state=*game;
        BeginTextureMode(target); game_draw(game); EndTextureMode();
        assert(memcmp(&state,game,sizeof(state))==0);
        Image full=LoadImageFromTexture(target.texture); ImageFlipVertical(&full);
        save_ppm(full,TextFormat("build/versus_foliage_preview/runtime_%02d.ppm",frame));
        if (frame==0 || frame==1 || frame==15 || frame==31 || frame==32 || frame==63)
            assert(ExportImage(full,TextFormat("build/versus_foliage_preview/screen_%02d.png",frame)));
        UnloadImage(full); UnloadImage(reference); UnloadImage(current);
    }
    for (int i=0;i<VERSUS_FOLIAGE_COUNT;i++) assert(changes[i]>0);
    Image loop=background_capture(&saved,target,32);
    assert(memcmp(first.data,loop.data,(size_t)1280*720*4)==0);
    UnloadImage(loop); UnloadImage(first);
    for (int i=0;i<3;i++) {
        game->versus.elapsed=32+(i-1)*.125f;
        capture(game,target,TextFormat("build/versus_foliage_preview/boundary_%d.png",i));
    }
    puts("PASS: 13 copas/arbustos, 64 instantes, somente vegetacao muda, grama identica pixel a pixel, ciclo e transicoes, fallback independente e desenho imutavel.");
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
    check_foliage(&game,target);
    check_foliage_loader();
    UnloadRenderTexture(target); game_shutdown(&game);
    assert(!game.versus_assets.background.id && !game.versus_assets.illustrations.id);
    assert(!game.versus_assets.grass_base.id && !game.versus_assets.grass_atlas.id);
    assert(!game.versus_assets.foliage_base.id && !game.versus_assets.foliage_atlas.id);
    CloseWindow();
    puts("PASS: Versus teclado/mouse, entrada mantida, confirmar/voltar, cancelamento de clique, modos em memoria, campanha preservada, fallback, desenho imutavel e recursos liberados.");
    return 0;
}
