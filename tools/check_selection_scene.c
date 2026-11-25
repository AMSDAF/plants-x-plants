/* Valida entradas, transições, fallback e composição com os módulos reais e OpenGL. */
#include "game.h"
#include "check_intro_flow.h"
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
    assert(game->screen == GAME_LEADERBOARD && !game->input_blocked);
    assert(game->leaderboard.time_count == game->leaderboard.damage_count);
    key(game, KEY_ENTER);
    for (int i = 0; i < 20; i++) tick(game);
    assert(game->screen == GAME_TRAINER_SELECT && !game->input_blocked);
}

static void leave_session(Game *game)
{
    key(game, KEY_ESCAPE);
    assert(game->exit_confirm_open && game->screen == GAME_EXPLORATION);
    key(game, KEY_RIGHT);
    key(game, KEY_ENTER);
    assert(!game->exit_confirm_open && game->screen == GAME_MENU);
}

static void check_botanical_walk(Game *game)
{
    const int keys[] = {KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_UP};
    Player player = game->player;
    for (int i = FACING_SOUTH; i <= FACING_NORTH; i++) {
        input(2, keys[i], 0);
        player_update(&player, &game->world, 0.05f);
        assert(player.moving && player.facing == (Facing)i && player.walk_time > 0.0f);
        input(1, keys[i], 0);
        player_update(&player, &game->world, 0.05f);
        assert(!player.moving && player.walk_time == 0.0f);
    }
}

static void check_botanical_size(const PlayerAssets *assets)
{
    RenderTexture2D target = LoadRenderTexture(128, 128);
    for (int trainer = 0; trainer < TRAINER_COUNT; trainer++) {
        Player player;
        player_init(&player, (Trainer)trainer, (Vector2){500, 300});
        Rectangle sizes[2];
        for (int view = 0; view < 2; view++) {
            Camera2D camera = {.offset = {64, 100}, .target = player.feet,
                               .zoom = view == 0 ? 1.0f : 1280.0f / 720.0f};
            BeginTextureMode(target);
            ClearBackground(BLANK);
            BeginMode2D(camera);
            player_draw(NULL, &player, assets, camera);
            EndMode2D();
            EndTextureMode();
            Image sprite = LoadImageFromTexture(target.texture);
            sizes[view] = GetImageAlphaBorder(sprite, 0.5f);
            UnloadImage(sprite);
            assert(sizes[view].height >= (view ? 75 : 48) && sizes[view].height <= (view ? 87 : 55));
        }
        assert(fabsf(sizes[0].width * (0.36f / 0.23f) - sizes[1].width) <= 2.0f);
        assert(fabsf(sizes[0].height * (0.36f / 0.23f) - sizes[1].height) <= 2.0f);
    }
    UnloadRenderTexture(target);
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

static void check_portrait_loaders(void)
{
    /* Falha parcial em diretório isolado, sem alterar os quatro assets reais. */
    char root[1024];
    snprintf(root, sizeof(root), "%s", GetWorkingDirectory());
    const char *fixture = "build/trainer_cleanup/invalid_portrait";
    const char *directory = "build/trainer_cleanup/invalid_portrait/assets/sprites/trainers";
    assert(DirectoryExists(directory) || MakeDirectory(directory) == 0);
    Image invalid = GenImageColor(1, 1, WHITE);
    assert(ExportImage(invalid, TextFormat("%s/male_portrait_idle.png", directory)));
    UnloadImage(invalid);
    int size = 0;
    unsigned char *data = LoadFileData("assets/sprites/trainers/female_portrait_idle.png", &size);
    assert(data && size > 0);
    assert(SaveFileData(TextFormat("%s/female_portrait_idle.png", directory), data, size));
    UnloadFileData(data);
    assert(ChangeDirectory(fixture));
    TrainerSelectAssets assets;
    trainer_select_assets_load(&assets);
    assert(!assets.male_idle.id && assets.female_idle.id);
    trainer_select_assets_unload(&assets);
    assert(!assets.female_idle.id);
    assert(ChangeDirectory(root));
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
    check_complete_intro(game, key);
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
    check_portrait_loaders();
    Game game;
    game_init(&game);
    for (int i = 0; i < TRAINER_COUNT; i++) {
        Texture2D sheet = game.player_assets.botanical_sheets[i];
        assert(sheet.id && sheet.width == 768 && sheet.height == 1024);
    }
    check_botanical_size(&game.player_assets);
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    assert(game.trainer_select_assets.male_idle.id);
    assert(game.trainer_select_assets.female_idle.id);
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
    input(7, 980, 660);
    input(6, 0, 0);
    tick(&game);
    input(7, 100, 100);
    input(5, 0, 0);
    tick(&game);
    assert(game.screen == GAME_TRAINER_SELECT);
    click(&game, 980, 660);
    confirm_campaign_name(&game);
    assert(game.screen == GAME_EXPLORATION && game.player.trainer == TRAINER_FEMALE);
    check_botanical_walk(&game);
    capture(&game, target, "build/selection_preview/botanical_female_game.png", false);
    leave_session(&game);
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
    leave_session(&game);
    campaign(&game);
    click(&game, 180, 660);
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
    check_botanical_walk(&game);
    capture(&game, target, "build/selection_preview/botanical_male_game.png", false);
    leave_session(&game);
    campaign(&game);

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
    /* Captura os atlas de repouso usados diretamente na seleção. */
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
    for (int i = 0; i < TRAINER_COUNT; i++) {
        assert(!game.player_assets.botanical_sheets[i].id);
    }
    assert(!game.trainer_select_assets.female_idle.id && !game.trainer_select_assets.male_idle.id);
    assert(!game.trainer_select_assets.background.base.id && !game.trainer_select_assets.background.atlas.id);
    CloseWindow();
    puts("PASS: teclado/mouse, confirmar/voltar, ESC, reentrada, ambos os treinadores, entrada mantida, relogio continuo, F7, desenho imutavel, ambos os estados OpenGL, fallback e descarregamento.");
    return 0;
}
