/* Valida entradas, transições, fallback e composição com os módulos reais e OpenGL. */
#include "game.h"
#include "male_portrait_idle_data.h"
#include "female_portrait_idle_data.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void input(unsigned int type, int a, int b)
{
    PlayAutomationEvent((AutomationEvent){.type = type, .params = {a, b, 0, 0}});
}

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing();
    game_draw(game);
    EndDrawing();
}

static void key(Game *game, int code)
{
    input(2, code, 0);
    tick(game);
    input(1, code, 0);
    tick(game);
    tick(game);
}

static void click(Game *game, int x, int y)
{
    input(7, x, y);
    input(6, MOUSE_BUTTON_LEFT, 0);
    tick(game);
    input(5, MOUSE_BUTTON_LEFT, 0);
    tick(game);
    tick(game);
}

static void campaign(Game *game)
{
    assert(game->screen == GAME_MENU);
    key(game, KEY_ENTER);
    for (int i = 0; i < 20; i++) tick(game);
    assert(game->screen == GAME_TRAINER_SELECT && !game->input_blocked);
}

static void capture(Game *game, RenderTexture2D target, const char *path, bool thumbnail)
{
    Game before = *game;
    BeginTextureMode(target);
    game_draw(game);
    EndTextureMode();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image shot = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&shot);
    if (thumbnail) ImageResize(&shot, 640, 360);
    assert(ExportImage(shot, path));
    UnloadImage(shot);
}

static void check_static_fallback_loaders(void)
{
    /* Diretórios isolados verificam as duas camadas legadas sem alterar os assets reais. */
    char root[1024];
    snprintf(root, sizeof(root), "%s", GetWorkingDirectory());
    const char *names[] = {"male", "female"};
    for (int original = 0; original < 2; original++) {
        const char *fixture = original ? "build/selection_cleanup/original_fallback" :
                                         "build/selection_cleanup/prepared_fallback";
        const char *directory = TextFormat("%s/assets/sprites/trainers", fixture);
        assert(DirectoryExists(directory) || MakeDirectory(directory) == 0);
        for (int i = 0; i < TRAINER_COUNT; i++) {
            const char *source = original ? TextFormat("assets/sprites/trainer_%s_portrait.png", names[i]) :
                                            TextFormat("assets/sprites/trainers/%s_portrait.png", names[i]);
            int size = 0;
            unsigned char *data = LoadFileData(source, &size);
            assert(data && size > 0);
            const char *target = original ? TextFormat("%s/assets/sprites/trainer_%s_portrait.png", fixture, names[i]) :
                                            TextFormat("%s/assets/sprites/trainers/%s_portrait.png", fixture, names[i]);
            assert(SaveFileData(target, data, size));
            UnloadFileData(data);
        }
        assert(ChangeDirectory(fixture));
        TrainerSelectAssets assets;
        trainer_select_assets_load(&assets);
        for (int i = 0; i < TRAINER_COUNT; i++) {
            assert(assets.portraits[i].id);
            assert(original ? assets.portraits[i].width > 320 : assets.portraits[i].width == 320);
            assert(assets.portrait_bounds[i].height > 0);
        }
        assert(!assets.male_idle.id && !assets.female_idle.id);
        trainer_select_assets_unload(&assets);
        assert(ChangeDirectory(root));
    }
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
    assert(DirectoryExists("build/selection_preview") || MakeDirectory("build/selection_preview") == 0);
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "Validacao da selecao de personagens");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    check_static_fallback_loaders();
    Game game;
    game_init(&game);
    assert(game.trainer_select_assets.portraits[0].width == 816);
    assert(game.trainer_select_assets.portraits[1].height == 1072);
    assert(game.trainer_select_assets.male_idle.id && game.trainer_select_assets.male_idle_halo.id);
    assert(game.trainer_select_assets.female_idle.id && game.trainer_select_assets.female_idle_halo.id);
    assert(game.trainer_select.female_idle_time == FEMALE_PORTRAIT_IDLE_PHASE_OFFSET);
    float start = 0;
    for (int i = 0; i < MALE_PORTRAIT_IDLE_FRAMES; i++) {
        assert(trainer_select_male_idle_frame(start + .001f) == i);
        assert(trainer_select_male_idle_frame(start + MALE_PORTRAIT_IDLE_DURATIONS[i] - .001f) == i);
        start += MALE_PORTRAIT_IDLE_DURATIONS[i];
    }
    assert(trainer_select_male_idle_frame(MALE_PORTRAIT_IDLE_SECONDS) == 0);
    start = 0;
    for (int i = 0; i < FEMALE_PORTRAIT_IDLE_FRAMES; i++) {
        assert(trainer_select_female_idle_frame(start + .001f) == i);
        assert(trainer_select_female_idle_frame(start + FEMALE_PORTRAIT_IDLE_DURATIONS[i] - .001f) == i);
        start += FEMALE_PORTRAIT_IDLE_DURATIONS[i];
    }
    assert(trainer_select_female_idle_frame(FEMALE_PORTRAIT_IDLE_SECONDS) == 0);
    assert(game.trainer_select_assets.background.base.id && game.trainer_select_assets.background.atlas.id);
    game.menu.menu_open = true;
    campaign(&game);
    float background_before = game.trainer_select.background_time;
    float idle_before = game.trainer_select.male_idle_time;
    float female_before = game.trainer_select.female_idle_time;
    key(&game, KEY_RIGHT);
    assert(game.trainer_select.selected == TRAINER_FEMALE);
    assert(game.trainer_select.background_time > background_before);
    assert(game.trainer_select.male_idle_time > idle_before);
    assert(game.trainer_select.female_idle_time > female_before);
    key(&game, KEY_LEFT);
    assert(game.trainer_select.selected == TRAINER_MALE);
    key(&game, KEY_D);
    assert(game.trainer_select.selected == TRAINER_FEMALE);
    key(&game, KEY_A);
    assert(game.trainer_select.selected == TRAINER_MALE);
    click(&game, 820, 360);
    assert(game.screen == GAME_TRAINER_SELECT && game.trainer_select.selected == TRAINER_FEMALE);
    key(&game, KEY_F7);
    assert(game.trainer_select.background_preview);
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_TRAINER_SELECT);
    key(&game, KEY_F7);
    assert(!game.trainer_select.background_preview && game.trainer_select.selected == TRAINER_FEMALE);
    input(7, 780, 670);
    input(6, 0, 0);
    tick(&game);
    input(7, 100, 100);
    input(5, 0, 0);
    tick(&game);
    assert(game.screen == GAME_TRAINER_SELECT);
    click(&game, 780, 670);
    confirm_campaign_name(&game);
    assert(game.screen == GAME_EXPLORATION && game.player.trainer == TRAINER_FEMALE);
    key(&game, KEY_ESCAPE);
    campaign(&game);
    input(2, KEY_D, 0);
    tick(&game);
    input(2, KEY_ENTER, 0);
    tick(&game);
    assert(game.screen == GAME_NAME_ENTRY && game.input_blocked);
    Vector2 feet = game.player.feet;
    for (int i = 0; i < 5; i++) tick(&game);
    assert(game.input_blocked && game.player.feet.x == feet.x && game.player.feet.y == feet.y);
    input(1, KEY_ENTER, 0);
    tick(&game);
    assert(game.input_blocked);
    input(1, KEY_D, 0);
    tick(&game);
    assert(!game.input_blocked);
    confirm_campaign_name(&game);
    key(&game, KEY_ESCAPE);
    campaign(&game);
    click(&game, 490, 670);
    assert(game.screen == GAME_MENU);
    campaign(&game);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    campaign(&game);
    key(&game, KEY_F7);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    campaign(&game);
    assert(!game.trainer_select.background_preview);
    key(&game, KEY_ENTER);
    confirm_campaign_name(&game);
    assert(game.screen == GAME_EXPLORATION && game.player.trainer == TRAINER_MALE);
    key(&game, KEY_ESCAPE);
    campaign(&game);

    RenderTexture2D target = LoadRenderTexture(1280, 720);
    trainer_select_screen_init(&game.trainer_select);
    game.trainer_select.selected = TRAINER_FEMALE;
    game.trainer_select.male_idle_time = MALE_PORTRAIT_IDLE_SECONDS - .025f;
    trainer_select_screen_update(&game.trainer_select, false, .1f);
    assert(fabsf(game.trainer_select.male_idle_time - .075f) < .001f);
    game.trainer_select.selected = TRAINER_MALE;
    game.trainer_select.female_idle_time = FEMALE_PORTRAIT_IDLE_SECONDS - .025f;
    trainer_select_screen_update(&game.trainer_select, false, .1f);
    assert(fabsf(game.trainer_select.female_idle_time - .075f) < .001f);
    trainer_select_screen_init(&game.trainer_select);
    assert(DirectoryExists("build/male_portrait_idle_preview") || MakeDirectory("build/male_portrait_idle_preview") == 0);
    float pose_time = 0;
    for (int i = 0; i < MALE_PORTRAIT_IDLE_FRAMES; i++) {
        game.trainer_select.male_idle_time = pose_time + .001f;
        capture(&game, target, TextFormat("build/male_portrait_idle_preview/runtime_pose_%d.png",i), false);
        pose_time += MALE_PORTRAIT_IDLE_DURATIONS[i];
    }
    game.trainer_select.male_idle_time = MALE_PORTRAIT_IDLE_SECONDS;
    capture(&game, target, "build/male_portrait_idle_preview/runtime_loop_end.png", false);
    Texture2D saved_idle = game.trainer_select_assets.male_idle;
    game.trainer_select_assets.male_idle = (Texture2D){0};
    capture(&game, target, "build/male_portrait_idle_preview/runtime_fallback.png", false);
    game.trainer_select_assets.male_idle = saved_idle;
    Texture2D saved_halo = game.trainer_select_assets.male_idle_halo;
    game.trainer_select_assets.male_idle_halo = (Texture2D){0};
    capture(&game, target, "build/male_portrait_idle_preview/runtime_fallback_halo.png", false);
    game.trainer_select_assets.male_idle_halo = saved_halo;
    game.trainer_select.male_idle_time = 0;
    assert(DirectoryExists("build/female_portrait_idle_preview") || MakeDirectory("build/female_portrait_idle_preview") == 0);
    game.trainer_select.selected = TRAINER_FEMALE;
    pose_time = 0;
    for (int i = 0; i < FEMALE_PORTRAIT_IDLE_FRAMES; i++) {
        game.trainer_select.female_idle_time = pose_time + .001f;
        capture(&game, target, TextFormat("build/female_portrait_idle_preview/runtime_pose_%d.png",i), false);
        pose_time += FEMALE_PORTRAIT_IDLE_DURATIONS[i];
    }
    game.trainer_select.female_idle_time = FEMALE_PORTRAIT_IDLE_SECONDS;
    capture(&game, target, "build/female_portrait_idle_preview/runtime_loop_end.png", false);
    saved_idle = game.trainer_select_assets.female_idle;
    game.trainer_select_assets.female_idle = (Texture2D){0};
    capture(&game, target, "build/female_portrait_idle_preview/runtime_fallback.png", false);
    game.trainer_select_assets.female_idle = saved_idle;
    saved_halo = game.trainer_select_assets.female_idle_halo;
    game.trainer_select_assets.female_idle_halo = (Texture2D){0};
    capture(&game, target, "build/female_portrait_idle_preview/runtime_fallback_halo.png", false);
    game.trainer_select_assets.female_idle_halo = saved_halo;
    float male_time = 0;
    for (int m = 0; m < 4; m++) {
        float female_time = 0;
        for (int f = 0; f < 4; f++) {
            game.trainer_select.male_idle_time = male_time + .001f;
            game.trainer_select.female_idle_time = female_time + .001f;
            capture(&game,target,TextFormat("build/female_portrait_idle_preview/pair_%d_%d.png",m,f),false);
            female_time += FEMALE_PORTRAIT_IDLE_DURATIONS[f];
        }
        male_time += MALE_PORTRAIT_IDLE_DURATIONS[m];
    }
    trainer_select_screen_init(&game.trainer_select);
    capture(&game, target, "build/selection_preview/selected_male.png", false);
    game.trainer_select.selected = TRAINER_FEMALE;
    for (int i = 0; i < 60; i++) trainer_select_screen_update(&game.trainer_select, false, 1.0f / 60);
    capture(&game, target, "build/selection_preview/selected_female.png", false);
    for (int frame = 0; frame < 48; frame++) {
        for (int step = 0; step < 5; step++) trainer_select_screen_update(&game.trainer_select, false, 0.025f);
        capture(&game, target, TextFormat("build/selection_preview/loop_%02d.png", frame), true);
    }
    /* Copia emprestada para simular falha parcial, sem renomear fontes no disco. */
    TrainerSelectAssets saved = game.trainer_select_assets;
    game.trainer_select_assets.background.atlas = (Texture2D){0};
    capture(&game, target, "build/selection_preview/fallback.png", false);
    game.trainer_select_assets = saved;
    UnloadRenderTexture(target);
    game_shutdown(&game);
    assert(!game.trainer_select_assets.female_idle.id && !game.trainer_select_assets.female_idle_halo.id);
    assert(!game.trainer_select_assets.background.base.id && !game.trainer_select_assets.background.atlas.id);
    CloseWindow();
    puts("PASS: teclado/mouse, confirmar/voltar, ESC, reentrada, ambos os treinadores, entrada mantida, relogio continuo, F7, desenho imutavel, ambos os estados OpenGL, fallback e descarregamento.");
    return 0;
}
