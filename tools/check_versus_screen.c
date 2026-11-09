/* Verifica navegação, confirmação isolada e capturas OpenGL da tela Versus. */
#include "game.h"
#include "versus_grass_data.h"
#include "versus_foliage_data.h"
#include "versus_character_data.h"
#include "versus_cloud_data.h"
#include <math.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    BeginTextureMode(target); versus_background_draw(assets,time,0); EndTextureMode();
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

static Image scene_capture(Game *game, RenderTexture2D target)
{
    Game before=*game;
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before,game,sizeof(before))==0);
    Image image=LoadImageFromTexture(target.texture); ImageFlipVertical(&image);
    return image;
}

static void check_character_loader(void)
{
    char root[1024]; snprintf(root,sizeof(root),"%s",GetWorkingDirectory());
    const char *cases[]={"missing","bad_base","bad_atlas"};
    const char *resources[]={"versus_illustrations_source.png","versus/character_base.png","versus/character_atlas.png"};
    assert(DirectoryExists("build/versus_character_preview/fixtures") ||
           MakeDirectory("build/versus_character_preview/fixtures")==0);
    for (int c=0;c<3;c++) {
        char base[256]; snprintf(base,sizeof(base),"build/versus_character_preview/fixtures/%s",cases[c]);
        const char *suffixes[]={"","/assets","/assets/sprites","/assets/sprites/versus"};
        for (int i=0;i<4;i++) {
            const char *dir=TextFormat("%s%s",base,suffixes[i]);
            assert(DirectoryExists(dir) || MakeDirectory(dir)==0);
        }
        for (int i=0;i<3;i++) {
            if (i>0 && c==0) continue;
            int size=0;
            unsigned char *data=LoadFileData(TextFormat("assets/sprites/%s",resources[i]),&size);
            assert(data && size>0);
            assert(SaveFileData(TextFormat("%s/assets/sprites/%s",base,resources[i]),data,size));
            UnloadFileData(data);
        }
        if (c>0) {
            Image invalid=GenImageColor(16,16,WHITE);
            assert(ExportImage(invalid,TextFormat("%s/assets/sprites/versus/character_%s.png",base,c==1 ? "base" : "atlas")));
            UnloadImage(invalid);
        }
        assert(ChangeDirectory(base));
        VersusAssets partial; versus_assets_load(&partial);
        assert(partial.illustrations.id && !partial.character_base.id && !partial.character_atlas.id);
        versus_assets_unload(&partial);
        assert(!partial.illustrations.id);
        assert(ChangeDirectory(root));
    }
    puts("PASS: carga dos repousos ausente/base invalida/atlas invalido recupera a arte original.");
}

static void check_characters(Game *game, RenderTexture2D target)
{
    assert(DirectoryExists("build/versus_character_preview") || MakeDirectory("build/versus_character_preview")==0);
    game->screen=GAME_VERSUS_SELECT;
    versus_screen_init(&game->versus,VERSUS_CPU);
    assert(game->versus_assets.character_base.id && game->versus_assets.character_atlas.id);
    VersusScreen initial=game->versus;
    assert(initial.character_time[0]!=initial.character_time[1] && initial.character_time[2]!=initial.character_time[3]);
    assert(versus_character_frame(&initial,-1)==0 && versus_character_frame(&initial,4)==0);
    for (int i=0;i<4;i++) game->versus.character_time[i]=0;
    Image neutral=scene_capture(game,target);
    const Color *first=neutral.data;
    for (int actor=0;actor<4;actor++) {
        const VersusCharacterRegion *r=&VERSUS_CHARACTER_REGIONS[actor];
        float left=232+r->card*448.0f,top=290,sx=368.0f/482,sy=186.0f/244;
        Rectangle allowed={left+r->origin.x*sx-2,top+r->origin.y*sy-2,
                           r->poses[0].width*sx+4,r->poses[0].height*sy+4};
        int total=0;
        for (int f=0;f<64;f++) {
            game->versus.character_time[actor]=(f+.01f)/r->fps;
            assert(versus_character_frame(&game->versus,actor)==f);
            Image image=scene_capture(game,target);
            const Color *next=image.data;
            for (int y=0;y<720;y++) for (int x=0;x<1280;x++) {
                if (memcmp(&first[y*1280+x],&next[y*1280+x],sizeof(Color))==0) continue;
                assert(CheckCollisionPointRec((Vector2){(float)x,(float)y},allowed));
                assert(y<(int)ceilf(top+r->fixed_y*sy));
                total++;
            }
            if (f==0 || f==12 || f==42 || f==48 || f==52)
                save_ppm(image,TextFormat("build/versus_character_preview/actor_%d_%02d.ppm",actor,f));
            UnloadImage(image);
        }
        assert(total>0);
        game->versus.character_time[actor]=0;
        Image loop=scene_capture(game,target);
        assert(memcmp(loop.data,neutral.data,(size_t)1280*720*4)==0);
        UnloadImage(loop);
        game->versus.character_time[actor]=64.0f/r->fps;
        assert(versus_character_frame(&game->versus,actor)==0);
        game->versus.character_time[actor]=0;
    }
    UnloadImage(neutral);
    /* A ausência de qualquer derivado recupera exatamente a referência estática. */
    VersusAssets saved=game->versus_assets;
    game->versus_assets.character_base=game->versus_assets.character_atlas=(Texture2D){0};
    Image fallback=scene_capture(game,target);
    assert(ExportImage(fallback,"build/versus_character_preview/fallback.png"));
    game->versus_assets=saved; game->versus_assets.character_base=(Texture2D){0};
    Image partial=scene_capture(game,target);
    assert(memcmp(fallback.data,partial.data,(size_t)1280*720*4)==0); UnloadImage(partial);
    game->versus_assets=saved; game->versus_assets.character_atlas=(Texture2D){0};
    partial=scene_capture(game,target);
    assert(memcmp(fallback.data,partial.data,(size_t)1280*720*4)==0);
    UnloadImage(partial); UnloadImage(fallback); game->versus_assets=saved;
    game->versus=initial;
    Image start=scene_capture(game,target);
    for (int i=0;i<4;i++) {
        const VersusCharacterRegion *r=&VERSUS_CHARACTER_REGIONS[i];
        game->versus.character_time[i]=fmodf(initial.character_time[i]+32,64.0f/r->fps);
        assert(versus_character_frame(&game->versus,i)==versus_character_frame(&initial,i));
    }
    Image end=scene_capture(game,target);
    assert(memcmp(start.data,end.data,(size_t)1280*720*4)==0);
    UnloadImage(start); UnloadImage(end);
    game->versus=initial;
    for (int f=0;f<256;f++) {
        Image image=scene_capture(game,target);
        save_ppm(image,TextFormat("build/versus_character_preview/runtime_%02d.ppm",f));
        if (f==0 || f==8 || f==26 || f==44 || f==48)
            assert(ExportImage(image,TextFormat("build/versus_character_preview/screen_%02d.png",f)));
        UnloadImage(image);
        versus_screen_update(&game->versus,false,.0625f);
        versus_screen_update(&game->versus,false,.0625f);
    }
    VersusScreen before=game->versus;
    versus_screen_update(&game->versus,false,-1);
    for (int i=0;i<4;i++) assert(game->versus.character_time[i]==before.character_time[i]);
    versus_screen_update(&game->versus,false,1);
    for (int i=0;i<4;i++) {
        float seconds=64.0f/VERSUS_CHARACTER_REGIONS[i].fps;
        assert(fabsf(game->versus.character_time[i]-fmodf(before.character_time[i]+.1f,seconds))<.0001f);
    }
    puts("PASS: quatro repousos independentes, 256 poses OpenGL, mudancas restritas aos personagens, pes fixos, ciclos, dt limitado, fallback e desenho imutavel.");
}

static Image cloud_capture(const VersusAssets *assets, RenderTexture2D target, float time)
{
    BeginTextureMode(target); versus_background_draw(assets,2,time); EndTextureMode();
    Image image=LoadImageFromTexture(target.texture); ImageFlipVertical(&image);
    return image;
}

static bool corrected_location(int x, int y)
{
    float scale=1280.0f/1672, offset=(720-941*scale)/2;
    const VersusCloudPatch *patch=&VERSUS_CLOUD_CORRECTION;
    Rectangle bounds={patch->origin.x*scale-1,offset+patch->origin.y*scale-1,
                      VERSUS_CLOUD_CORRECTION_WIDTH*scale+2,VERSUS_CLOUD_CORRECTION_HEIGHT*scale+2};
    return CheckCollisionPointRec((Vector2){(float)x,(float)y},bounds);
}

static void check_removed_cloud(Image image)
{
    const Color *pixels=image.data;
    float scale=1280.0f/1672, offset=(720-941*scale)/2;
    for (int y=(int)ceilf(offset+228*scale);y<(int)floorf(offset+277*scale);y++)
        for (int x=(int)ceilf(1357*scale);x<(int)floorf(1515*scale);x++) {
            Color c=pixels[y*1280+x];
            assert(!(c.r>220 && c.g>210 && c.b>140));
        }
}

static void check_clouds(Game *game, RenderTexture2D target)
{
    assert(DirectoryExists("build/versus_cloud_preview") || MakeDirectory("build/versus_cloud_preview")==0);
    const VersusAssets saved=game->versus_assets;
    assert(saved.cloud_atlas.id && saved.cloud_correction.id);
    VersusAssets old=saved; old.cloud_atlas=old.cloud_correction=(Texture2D){0};
    Image original=cloud_capture(&old,target,0);
    Image start=cloud_capture(&saved,target,0);
    const Color *before=original.data,*first=start.data;
    const float times[]={0,8,32,128,256,512,768,1024,1536,2048,3071.875f,3072};
    int changes=0;
    for (unsigned int i=0;i<sizeof(times)/sizeof(times[0]);i++) {
        Image image=cloud_capture(&saved,target,times[i]);
        const Color *pixels=image.data;
        check_removed_cloud(image);
        for (int y=0;y<720;y++) for (int x=0;x<1280;x++) {
            if (corrected_location(x,y)) assert(memcmp(&pixels[y*1280+x],&first[y*1280+x],sizeof(Color))==0);
            if (memcmp(&pixels[y*1280+x],&before[y*1280+x],sizeof(Color))==0) continue;
            assert(y<=(int)ceilf(VERSUS_CLOUD_SKY_HEIGHT*1280.0f/1672)+1 || corrected_location(x,y));
            if (i==2) changes++;
        }
        if (i==11) assert(memcmp(image.data,start.data,(size_t)1280*720*4)==0);
        assert(ExportImage(image,TextFormat("build/versus_cloud_preview/background_%02d.png",i)));
        game->versus.elapsed=2; game->versus.cloud_time=times[i];
        capture(game,target,TextFormat("build/versus_cloud_preview/screen_%02d.png",i));
        UnloadImage(image);
    }
    assert(changes>1000);
    for (int i=0;i<VERSUS_CLOUD_COUNT;i++) {
        const VersusCloud *c=&VERSUS_CLOUDS[i];
        float seam=(1672-c->origin.x)/(1672+c->source.width)*c->seconds;
        Image left=cloud_capture(&saved,target,seam-.125f);
        Image right=cloud_capture(&saved,target,seam+.125f);
        const Color *a=left.data,*b=right.data;
        int changed=0;
        for (int p=0;p<1280*720;p++) {
            int delta=abs((int)a[p].r-b[p].r)+abs((int)a[p].g-b[p].g)+abs((int)a[p].b-b[p].b);
            if (delta>64) changed++;
        }
        assert(changed<2000); /* Não existe uma nuvem inteira saltando dentro da janela. */
        assert(ExportImage(left,TextFormat("build/versus_cloud_preview/wrap_%d_before.png",i)));
        assert(ExportImage(right,TextFormat("build/versus_cloud_preview/wrap_%d_after.png",i)));
        UnloadImage(left); UnloadImage(right);
    }
    VersusAssets fallback=saved; fallback.cloud_atlas=(Texture2D){0};
    Image still=cloud_capture(&fallback,target,0);
    Image later=cloud_capture(&fallback,target,1000);
    assert(memcmp(still.data,later.data,(size_t)1280*720*4)==0);
    check_removed_cloud(still);
    const Color *pixels=still.data;
    for (int y=0;y<720;y++) for (int x=0;x<1280;x++)
        if (!corrected_location(x,y)) assert(memcmp(&pixels[y*1280+x],&before[y*1280+x],sizeof(Color))==0);
    assert(ExportImage(still,"build/versus_cloud_preview/fallback_static.png"));
    UnloadImage(still); UnloadImage(later);
    fallback=saved; fallback.cloud_correction=(Texture2D){0};
    later=cloud_capture(&fallback,target,0);
    assert(memcmp(later.data,start.data,(size_t)1280*720*4)==0);
    check_removed_cloud(later); UnloadImage(later);
    UnloadImage(original); UnloadImage(start);
    VersusScreen clock=game->versus;
    clock.elapsed=127.99f;clock.cloud_time=500;
    versus_screen_update(&clock,false,.1f);
    assert(clock.elapsed<.1f && fabsf(clock.cloud_time-500.1f)<.001f);
    clock.cloud_time=3071.99f;versus_screen_update(&clock,false,.1f);
    assert(clock.cloud_time<.11f);
    puts("PASS: tres nuvens preservadas, nuvem baixa removida, somente ceu/local corrigido muda, ciclos independentes, reinicio fora da janela, fallback corrigido e animacoes anteriores identicas.");
}

static void check_cloud_loader(RenderTexture2D target)
{
    char root[1024]; snprintf(root,sizeof(root),"%s",GetWorkingDirectory());
    const char *cases[]={"missing_atlas","bad_atlas","bad_correction"};
    const char *resources[]={"versus_background_generated_v1.png","versus/cloud_correction.png","versus/cloud_atlas.png"};
    assert(DirectoryExists("build/versus_cloud_preview/fixtures") || MakeDirectory("build/versus_cloud_preview/fixtures")==0);
    for (int c=0;c<3;c++) {
        char base[256]; snprintf(base,sizeof(base),"build/versus_cloud_preview/fixtures/%s",cases[c]);
        const char *suffixes[]={"","/assets","/assets/sprites","/assets/sprites/versus"};
        for (int i=0;i<4;i++) {
            const char *dir=TextFormat("%s%s",base,suffixes[i]);
            assert(DirectoryExists(dir) || MakeDirectory(dir)==0);
        }
        for (int i=0;i<3;i++) {
            if (i==2 && c==0) continue;
            int size=0;
            unsigned char *data=LoadFileData(TextFormat("assets/sprites/%s",resources[i]),&size);
            assert(data && size>0);
            assert(SaveFileData(TextFormat("%s/assets/sprites/%s",base,resources[i]),data,size));
            UnloadFileData(data);
        }
        if (c>0) {
            Image invalid=GenImageColor(16,16,WHITE);
            assert(ExportImage(invalid,TextFormat("%s/assets/sprites/versus/cloud_%s.png",base,c==1 ? "atlas" : "correction")));
            UnloadImage(invalid);
        }
        assert(ChangeDirectory(base));
        VersusAssets partial; versus_assets_load(&partial);
        assert(partial.background.id);
        if (c<2) assert(!partial.cloud_atlas.id && partial.cloud_correction.id);
        else assert(partial.cloud_atlas.id && !partial.cloud_correction.id);
        Image image=cloud_capture(&partial,target,600);
        check_removed_cloud(image); UnloadImage(image);
        versus_assets_unload(&partial);
        assert(!partial.cloud_atlas.id && !partial.cloud_correction.id);
        assert(ChangeDirectory(root));
    }
    puts("PASS: atlas ausente/invalido e correcao invalida mantem a nuvem baixa removida usando o recurso valido.");
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
    check_characters(&game,target);
    check_character_loader();
    check_clouds(&game,target);
    check_cloud_loader(target);
    UnloadRenderTexture(target); game_shutdown(&game);
    assert(!game.versus_assets.background.id && !game.versus_assets.illustrations.id);
    assert(!game.versus_assets.grass_base.id && !game.versus_assets.grass_atlas.id);
    assert(!game.versus_assets.foliage_base.id && !game.versus_assets.foliage_atlas.id);
    assert(!game.versus_assets.character_base.id && !game.versus_assets.character_atlas.id);
    assert(!game.versus_assets.cloud_atlas.id && !game.versus_assets.cloud_correction.id);
    CloseWindow();
    puts("PASS: Versus teclado/mouse, entrada mantida, confirmar/voltar, cancelamento de clique, modos em memoria, campanha preservada, fallback, desenho imutavel e recursos liberados.");
    return 0;
}
