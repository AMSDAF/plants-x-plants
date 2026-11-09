/* Verifica prévia, fallback, alinhamento e fechamento do ciclo pelo renderizador real. */
#include "game.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
}

static void key(Game *game, int code)
{
    PlayAutomationEvent((AutomationEvent){.type=2,.params={code}});
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
    PlayAutomationEvent((AutomationEvent){.type=1,.params={code}});
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
    game_update(game);
}

static void capture(Game *game, RenderTexture2D target, const char *path)
{
    Game before = *game;
    BeginTextureMode(target);
    game_draw(game);
    EndTextureMode();
    assert(memcmp(&before,game,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    /* PPM binário evita compressão PNG em centenas de capturas de diagnóstico. */
    if (strstr(path,".ppm")) {
        FILE *file = fopen(path,"wb");
        assert(file);
        fprintf(file,"P6\n%d %d\n255\n",image.width,image.height);
        ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        size_t bytes = (size_t)image.width*image.height*3;
        assert(fwrite(image.data,1,bytes,file)==bytes);
        assert(fclose(file)==0);
    } else assert(ExportImage(image,path));
    UnloadImage(image);
}

static void confirm_campaign_name(Game *game)
{
    assert(game->screen==GAME_NAME_ENTRY);
    for (int i=0;i<7;i++) {
        trainer_select_screen_update(&game->trainer_select,false,.1f);
        name_entry_update(&game->name_entry,false,.1f);
    }
    tick(game); tick(game);
    if (game->name_entry.count==0) {
        const char *name="Teste";
        while (*name) assert(name_entry_append(&game->name_entry,*name++));
    }
    key(game,KEY_ENTER);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Verificacao do fundo da selecao");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL);
    Game game;
    game_init(&game);
    game.screen = GAME_TRAINER_SELECT;
    key(&game,KEY_F7);
    assert(game.trainer_select.background_preview);
    key(&game,KEY_ENTER);
    assert(game.screen == GAME_TRAINER_SELECT);
    key(&game,KEY_RIGHT);
    assert(game.trainer_select.selected == TRAINER_MALE);
    RenderTexture2D target = LoadRenderTexture(1280,720);
    assert(target.id);
    TrainerBackgroundAssets *assets = &game.trainer_select_assets.background;
    assert(assets->original.id && assets->base.id && assets->atlas.id);
    for (int i=0; i<=512; i++) {
        game.trainer_select.background_time = i*.25f;
        capture(&game,target,TextFormat("build/trainer_selection_preview/runtime_%03d.ppm",i));
    }
    for (int i=0; i<3; i++) {
        game.trainer_select.background_time = 128.0f+(i-1)*.125f;
        capture(&game,target,TextFormat("build/trainer_selection_preview/boundary_%d.png",i));
    }
    TrainerBackgroundAssets saved = *assets;
    assets->atlas = (Texture2D){0};
    capture(&game,target,"build/trainer_selection_preview/fallback_atlas.png");
    *assets = saved;
    assets->base = (Texture2D){0};
    capture(&game,target,"build/trainer_selection_preview/fallback_base.png");
    *assets = saved;
    key(&game,KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    game.screen = GAME_TRAINER_SELECT;
    key(&game,KEY_F7);
    assert(!game.trainer_select.background_preview);
    key(&game,KEY_D);
    assert(game.trainer_select.selected == TRAINER_FEMALE);
    key(&game,KEY_ENTER);
    confirm_campaign_name(&game);
    assert(game.screen == GAME_EXPLORATION);
    game.trainer_select.background_time = 127.99f;
    trainer_select_screen_update(&game.trainer_select,false,.1f);
    assert(fabsf(game.trainer_select.background_time-.09f)<.001f);
    UnloadRenderTexture(target);
    game_shutdown(&game);
    assert(!assets->original.id && !assets->base.id && !assets->atlas.id);
    CloseWindow();
    puts("PASS: previa F7, ESC, bloqueio de confirmacao, selecao normal, fallback parcial, desenho imutavel, ciclo de 128 s e descarregamento.");
    return 0;
}
