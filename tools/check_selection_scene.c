/* Valida entradas, transições, fallback e composição com os módulos reais e OpenGL. */
#include "game.h"
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

int main(void)
{
    assert(DirectoryExists("build/selection_preview") || MakeDirectory("build/selection_preview") == 0);
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "Validacao da floresta de selecao");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    Game game;
    game_init(&game);
    assert(game.trainer_select_assets.frame.id && game.trainer_select_assets.forest.base.id);
    assert(game.trainer_select_assets.forest.clouds.id && game.trainer_select_assets.forest.trunks.id);
    assert(game.trainer_select_assets.forest.canopies.id && game.trainer_select_assets.forest.ground.id);
    assert(game.trainer_select_assets.forest.leaves.id);
    game.menu.menu_open = true;
    campaign(&game);
    key(&game, KEY_RIGHT);
    assert(game.trainer_select.selected == TRAINER_FEMALE);
    key(&game, KEY_LEFT);
    assert(game.trainer_select.selected == TRAINER_MALE);
    key(&game, KEY_D);
    assert(game.trainer_select.selected == TRAINER_FEMALE);
    key(&game, KEY_A);
    assert(game.trainer_select.selected == TRAINER_MALE);
    click(&game, 820, 360);
    assert(game.screen == GAME_TRAINER_SELECT && game.trainer_select.selected == TRAINER_FEMALE);
    input(7, 780, 670);
    input(6, 0, 0);
    tick(&game);
    input(7, 100, 100);
    input(5, 0, 0);
    tick(&game);
    assert(game.screen == GAME_TRAINER_SELECT);
    click(&game, 780, 670);
    assert(game.screen == GAME_EXPLORATION && game.player.trainer == TRAINER_FEMALE);
    key(&game, KEY_ESCAPE);
    campaign(&game);
    input(2, KEY_D, 0);
    tick(&game);
    input(2, KEY_ENTER, 0);
    tick(&game);
    assert(game.screen == GAME_EXPLORATION && game.input_blocked);
    Vector2 feet = game.player.feet;
    for (int i = 0; i < 5; i++) tick(&game);
    assert(game.input_blocked && game.player.feet.x == feet.x && game.player.feet.y == feet.y);
    input(1, KEY_ENTER, 0);
    tick(&game);
    assert(game.input_blocked);
    input(1, KEY_D, 0);
    tick(&game);
    assert(!game.input_blocked);
    key(&game, KEY_ESCAPE);
    campaign(&game);
    click(&game, 490, 670);
    assert(game.screen == GAME_MENU);
    campaign(&game);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    campaign(&game);
    key(&game, KEY_ENTER);
    assert(game.screen == GAME_EXPLORATION && game.player.trainer == TRAINER_MALE);
    key(&game, KEY_ESCAPE);
    campaign(&game);

    /* Garante avanço e continuidade no limite de wrap, inclusive em longa execução. */
    ForestScene forest;
    forest_scene_init(&forest);
    forest.cloud_x[0] = 1420.1f;
    forest_scene_update(&forest, 0.1f);
    assert(forest.cloud_x[0] > -140 && forest.cloud_x[0] < -139);
    for (int i = 0; i < 7200; i++) forest_scene_update(&forest, 0.1f);
    assert(isfinite(forest.wind_phase) && forest.wind_phase >= 0 && forest.wind_phase < 2 * PI);
    for (int i = 0; i < FOREST_CLOUD_COUNT; i++)
        assert(isfinite(forest.cloud_x[i]) && forest.cloud_x[i] >= -170 && forest.cloud_x[i] <= 1450);
    for (int i = 0; i < FOREST_LEAF_COUNT; i++)
        assert(forest.leaves[i].position.y >= -60 && forest.leaves[i].position.y <= 760);
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    trainer_select_screen_init(&game.trainer_select);
    capture(&game, target, "build/selection_preview/selected_male.png", false);
    game.trainer_select.selected = TRAINER_FEMALE;
    for (int i = 0; i < 60; i++) trainer_select_screen_update(&game.trainer_select, false, 1.0f / 60);
    capture(&game, target, "build/selection_preview/selected_female.png", false);
    ForestScene saved_forest = game.trainer_select.forest;
    game.trainer_select.forest.cloud_x[0] = 1420.1f;
    capture(&game, target, "build/selection_preview/cloud_wrap_before.png", false);
    forest_scene_update(&game.trainer_select.forest, 0.1f);
    capture(&game, target, "build/selection_preview/cloud_wrap_after.png", false);
    game.trainer_select.forest = saved_forest;
    for (int frame = 0; frame < 48; frame++) {
        for (int step = 0; step < 5; step++) trainer_select_screen_update(&game.trainer_select, false, 0.025f);
        capture(&game, target, TextFormat("build/selection_preview/loop_%02d.png", frame), true);
    }
    /* Copia emprestada para simular falha parcial, sem renomear fontes no disco. */
    TrainerSelectAssets saved = game.trainer_select_assets;
    game.trainer_select_assets.forest = (ForestSceneAssets){0};
    game.trainer_select_assets.frame = (Texture2D){0};
    game.trainer_select_assets.breathing[0] = (Texture2D){0};
    game.trainer_select_assets.breathing[1] = (Texture2D){0};
    capture(&game, target, "build/selection_preview/fallback.png", false);
    game.trainer_select_assets = saved;
    game.trainer_select_assets.forest.trunks = (Texture2D){0};
    capture(&game, target, "build/selection_preview/no_trunks.png", false);
    game.trainer_select_assets = saved;
    UnloadRenderTexture(target);
    game_shutdown(&game);
    assert(!game.trainer_select_assets.frame.id && !game.trainer_select_assets.forest.base.id);
    CloseWindow();
    puts("PASS: teclado/mouse, confirmar/voltar, ESC, reentrada, ambos os treinadores, entrada mantida, wrap e 12 minutos simulados, desenho imutavel, 48 fases OpenGL, fallback e descarregamento.");
    return 0;
}
