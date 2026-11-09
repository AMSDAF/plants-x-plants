/* Verifica composição, grama, mascote e navegação com o jogo e OpenGL reais. */
#include "game.h"
#include "menu_mascot_data.h"
#include "menu_grass_data.h"
#include "menu_tree_data.h"
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
    input(6, 0, 0);
    tick(game);
    input(5, 0, 0);
    tick(game);
    tick(game);
}

static Image capture(Game *game, RenderTexture2D target, const char *path)
{
    Game before = *game;
    BeginTextureMode(target);
    game_draw(game);
    EndTextureMode();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    if (path) assert(ExportImage(image, path));
    return image;
}

static void check_mascot_only_change(Image first, Image next, bool animated)
{
    assert(first.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 && next.format == first.format);
    const Color *a = first.data;
    const Color *b = next.data;
    int changes = 0;
    for (int y = 0; y < 720; y++) {
        for (int x = 0; x < 1280; x++) {
            bool changed = memcmp(&a[y * 1280 + x], &b[y * 1280 + x], sizeof(Color)) != 0;
            if (changed) {
                assert(x >= 510 && x < 775 && y >= 368 && y < 568);
                changes++;
            }
        }
    }
    if (animated) assert(changes > 0);
    else assert(changes <= 512); /* Tolerância de rasterização pontual entre texturas de dimensões diferentes. */
}

static void check_atlas_loader_fallback(void)
{
    /* Fixtures isoladas: nunca renomeia nem modifica os assets de execução. */
    char root[1024];
    snprintf(root, sizeof(root), "%s", GetWorkingDirectory());
    assert(DirectoryExists("build/menu_mascot_preview/missing/assets/sprites") || MakeDirectory("build/menu_mascot_preview/missing/assets/sprites") == 0);
    assert(DirectoryExists("build/menu_mascot_preview/invalid/assets/sprites") || MakeDirectory("build/menu_mascot_preview/invalid/assets/sprites") == 0);
    int size = 0;
    unsigned char *data = LoadFileData("assets/sprites/menu_mascot.png", &size);
    assert(data && size > 0);
    assert(SaveFileData("build/menu_mascot_preview/missing/assets/sprites/menu_mascot.png", data, size));
    assert(SaveFileData("build/menu_mascot_preview/invalid/assets/sprites/menu_mascot.png", data, size));
    UnloadFileData(data);
    const char *background_path = FileExists("assets/sprites/menu_background_v2.png")
        ? "assets/sprites/menu_background_v2.png" : "assets/sprites/menu_backround_v2.png";
    data = LoadFileData(background_path, &size);
    assert(data && size > 0);
    assert(SaveFileData("build/menu_mascot_preview/missing/assets/sprites/menu_backround_v2.png", data, size));
    assert(SaveFileData("build/menu_mascot_preview/invalid/assets/sprites/menu_backround_v2.png", data, size));
    UnloadFileData(data);
    Image invalid = LoadImage("assets/sprites/menu_mascot.png");
    ImageCrop(&invalid, (Rectangle){0, 0, 16, 16});
    assert(ExportImage(invalid, "build/menu_mascot_preview/invalid/assets/sprites/menu_mascot_idle.png"));
    assert(ExportImage(invalid, "build/menu_mascot_preview/invalid/assets/sprites/menu_grass_background.png"));
    assert(ExportImage(invalid, "build/menu_mascot_preview/invalid/assets/sprites/menu_grass_atlas.png"));
    UnloadImage(invalid);
    const char *fixtures[] = {"missing", "invalid"};
    for (int i = 0; i < 2; i++) {
        assert(ChangeDirectory(TextFormat("build/menu_mascot_preview/%s", fixtures[i])));
        MenuScreen partial;
        menu_screen_init(&partial);
        assert(partial.mascot.id && !partial.mascot_idle.id);
        assert(partial.background.id && !partial.grass_background.id && !partial.grass_atlas.id);
        assert(!partial.tree_background.id && !partial.tree_atlas.id);
        menu_screen_shutdown(&partial);
        assert(!partial.mascot.id && !partial.mascot_idle.id);
        assert(ChangeDirectory(root));
    }
    const char *tree_fixtures[] = {"tree_missing", "tree_invalid"};
    const char *sources[] = {background_path, "assets/sprites/menu_grass_background.png",
                             "assets/sprites/menu_grass_atlas.png", "assets/sprites/menu_wind_background.png"};
    const char *names[] = {"menu_backround_v2.png", "menu_grass_background.png",
                           "menu_grass_atlas.png", "menu_wind_background.png"};
    for (int i = 0; i < 2; i++) {
        char dir[1024];
        snprintf(dir, sizeof(dir), "build/menu_tree_preview/%s/assets/sprites", tree_fixtures[i]);
        assert(DirectoryExists(dir) || MakeDirectory(dir) == 0);
        for (int j = 0; j < 4; j++) {
            data = LoadFileData(sources[j], &size);
            assert(data && size > 0);
            assert(SaveFileData(TextFormat("%s/%s", dir, names[j]), data, size));
            UnloadFileData(data);
        }
        if (i == 1) {
            invalid = GenImageColor(16, 16, WHITE);
            assert(ExportImage(invalid, TextFormat("%s/menu_tree_atlas.png", dir)));
            UnloadImage(invalid);
        }
        assert(ChangeDirectory(TextFormat("build/menu_tree_preview/%s", tree_fixtures[i])));
        MenuScreen partial;
        menu_screen_init(&partial);
        assert(partial.background.id && partial.grass_background.id && partial.grass_atlas.id);
        assert(!partial.tree_background.id && !partial.tree_atlas.id);
        menu_screen_shutdown(&partial);
        assert(!partial.background.id && !partial.grass_background.id && !partial.tree_atlas.id);
        assert(ChangeDirectory(root));
    }
}

static void check_wind(Game *game, RenderTexture2D target)
{
    assert(DirectoryExists("build/menu_grass_preview") || MakeDirectory("build/menu_grass_preview") == 0);
    assert(DirectoryExists("build/menu_tree_preview") || MakeDirectory("build/menu_tree_preview") == 0);
    assert(game->menu.grass_background.id && game->menu.grass_atlas.id);
    assert(game->menu.tree_background.id && game->menu.tree_atlas.id);
    game->screen = GAME_MENU;
    game->menu.menu_open = false;
    game->menu.input_blocked = false;
    game->menu.grass_time = 1.0f;
    game->menu.mascot_time = 0.5f;
    key(game, KEY_ENTER);
    assert(game->menu.grass_time > 1.0f && game->menu.grass_time < 1.31f);
    assert(game->menu.mascot_time > 0.5f && game->menu.mascot_time < 0.81f);
    key(game, KEY_ESCAPE);
    assert(game->menu.grass_time > 1.0f && game->menu.grass_time < 1.61f);
    MenuScreen saved = game->menu;
    game->menu.grass_background = (Texture2D){0};
    game->menu.grass_atlas = (Texture2D){0};
    Image fallback = capture(game, target, "build/menu_grass_preview/static_fallback.png");
    game->menu.grass_background = saved.grass_background;
    Image half = capture(game, target, NULL);
    assert(memcmp(fallback.data, half.data, GetPixelDataSize(1280, 720, half.format)) == 0);
    UnloadImage(half);
    game->menu.grass_background = (Texture2D){0};
    game->menu.grass_atlas = saved.grass_atlas;
    half = capture(game, target, NULL);
    assert(memcmp(fallback.data, half.data, GetPixelDataSize(1280, 720, half.format)) == 0);
    UnloadImage(half);
    UnloadImage(fallback);
    game->menu = saved;
    game->menu.tree_background = (Texture2D){0};
    game->menu.tree_atlas = (Texture2D){0};
    fallback = capture(game, target, "build/menu_tree_preview/grass_fallback.png");
    game->menu.tree_background = saved.tree_background;
    half = capture(game, target, NULL);
    assert(memcmp(fallback.data, half.data, GetPixelDataSize(1280, 720, half.format)) == 0);
    UnloadImage(half);
    game->menu.tree_background = (Texture2D){0};
    game->menu.tree_atlas = saved.tree_atlas;
    half = capture(game, target, NULL);
    assert(memcmp(fallback.data, half.data, GetPixelDataSize(1280, 720, half.format)) == 0);
    UnloadImage(half);
    UnloadImage(fallback);
    game->menu = saved;
    game->menu.ui_time = 0;
    game->menu.mascot_time = 0;
    game->menu.grass_time = 0;
    Image first = capture(game, target, NULL);
    float scale = fmaxf(1280.0f / MENU_GRASS_SOURCE_WIDTH, 720.0f / MENU_GRASS_SOURCE_HEIGHT);
    float origin_y = roundf((720 - MENU_GRASS_SOURCE_HEIGHT * scale) / 2);
    int changes = 0;
    for (int frame = 0; frame < MENU_GRASS_FRAME_COUNT; frame++) {
        game->menu.grass_time = (frame + 0.01f) / MENU_GRASS_FPS;
        game->menu.mascot_time = fmodf(frame / MENU_GRASS_FPS, MENU_MASCOT_FRAME_COUNT / MENU_MASCOT_FPS);
        Image phase = capture(game, target, TextFormat("build/menu_grass_preview/title_%02d.png", frame));
        assert(ExportImage(phase, TextFormat("build/menu_tree_preview/title_%02d.png", frame)));
        const Color *a = first.data, *b = phase.data;
        for (int y = 0; y < 720; y++) {
            for (int x = 0; x < 1280; x++) {
                if (memcmp(&a[y*1280+x], &b[y*1280+x], sizeof(Color)) == 0) continue;
                bool in_grass = x >= 510 && x < 775 && y >= 368 && y < 568;
                for (int i = 0; i < MENU_GRASS_REGION_COUNT; i++) {
                    const MenuGrassRegion *r = &MENU_GRASS_REGIONS[i];
                    Rectangle bounds = {r->origin.x*scale - 1, origin_y+r->origin.y*scale - 1,
                                        r->source.width*scale + 2, r->source.height*scale + 2};
                    if (CheckCollisionPointRec((Vector2){(float)x, (float)y}, bounds)) in_grass = true;
                }
                for (int i = 0; i < MENU_TREE_REGION_COUNT; i++) {
                    const MenuTreeRegion *r = &MENU_TREE_REGIONS[i];
                    Rectangle bounds = {r->origin.x*scale - 1, origin_y+r->origin.y*scale - 1,
                                        r->poses[0].width*scale + 2, r->poses[0].height*scale + 2};
                    if (CheckCollisionPointRec((Vector2){(float)x, (float)y}, bounds)) in_grass = true;
                }
                assert(in_grass); /* Só copas, grama e mascote mudam na reprodução conjunta. */
                changes++;
            }
        }
        UnloadImage(phase);
    }
    assert(changes > 0);
    UnloadImage(first);
    game->menu.menu_open = true;
    game->menu.grass_time = 2.5f;
    Image menu = capture(game, target, "build/menu_grass_preview/menu.png");
    assert(ExportImage(menu, "build/menu_tree_preview/menu.png"));
    UnloadImage(menu);
    game->menu = saved;
    puts("PASS: 32 fases de copas/grama/mascote, somente regioes previstas mudam, relogios continuos e fallback em camadas.");
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
    assert(DirectoryExists("build/menu_v2_preview") || MakeDirectory("build/menu_v2_preview") == 0);
    assert(DirectoryExists("build/menu_mascot_preview") || MakeDirectory("build/menu_mascot_preview") == 0);
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "Verificacao do menu pixel art");
    assert(IsWindowReady());
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    Game game;
    game_init(&game);
    assert(game.menu.background.id && game.menu.logo.id && game.menu.mascot.id);
    assert(game.menu.mascot_idle.id && game.menu.mascot_idle.width == 4096 && game.menu.mascot_idle.height == 2048);
    check_atlas_loader_fallback();
    assert(game.trainer_select_assets.background.base.id);
    assert(game.menu.logo_source.width == 1481 && game.menu.mascot_source.width == 968);
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    game.menu.ui_time = 0;
    game.menu.grass_time = 0; /* Isola o mascote nas comparações já existentes. */
    Image title = capture(&game, target, "build/menu_v2_preview/title.png");
    Texture2D idle = game.menu.mascot_idle;
    game.menu.mascot_idle = (Texture2D){0};
    Image static_mascot = capture(&game, target, "build/menu_mascot_preview/static_fallback.png");
    check_mascot_only_change(title, static_mascot, false);
    UnloadImage(static_mascot);
    game.menu.mascot_idle = idle;
    for (int i = 0; i < 30; i++) tick(&game);
    game.menu.ui_time = 0;
    game.menu.grass_time = 0;
    Image still = capture(&game, target, NULL);
    check_mascot_only_change(title, still, true);
    UnloadImage(title);
    UnloadImage(still);
    input(2, KEY_ENTER, 0);
    game.menu.mascot_time = 0.5f;
    tick(&game);
    assert(game.menu.menu_open && game.menu.input_blocked);
    assert(game.menu.mascot_time > 0.5f && game.menu.mascot_time <= 0.61f);
    for (int i = 0; i < 15; i++) tick(&game);
    assert(game.screen == GAME_MENU && game.menu.pending_option == -1);
    input(1, KEY_ENTER, 0);
    tick(&game);
    tick(&game);
    assert(!game.menu.input_blocked);
    key(&game, KEY_DOWN);
    assert(game.menu.selected_option == 1);
    key(&game, KEY_DOWN);
    assert(game.menu.selected_option == 2);
    key(&game, KEY_DOWN);
    assert(game.menu.selected_option == 0);
    key(&game, KEY_UP);
    assert(game.menu.selected_option == 2);
    key(&game, KEY_UP);
    assert(game.menu.selected_option == 1);
    key(&game, KEY_UP);
    assert(game.menu.selected_option == 0);
    click(&game, 1000, 460);
    assert(game.screen == GAME_VERSUS_SELECT && !game.exit_requested);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    input(7, 1000, 397);
    input(6, 0, 0);
    tick(&game);
    input(7, 100, 100);
    input(5, 0, 0);
    tick(&game);
    assert(game.screen == GAME_MENU && game.menu.pressed_option == -1);
    game.menu.ui_time = 0;
    game.menu.grass_time = 0;
    Image menu = capture(&game, target, "build/menu_v2_preview/menu.png");
    for (int i = 0; i < 30; i++) tick(&game);
    game.menu.ui_time = 0;
    game.menu.grass_time = 0;
    still = capture(&game, target, NULL);
    check_mascot_only_change(menu, still, true);
    UnloadImage(menu);
    UnloadImage(still);
    game.menu.mascot_time = 0.5f;
    key(&game, KEY_ESCAPE);
    assert(!game.menu.menu_open && game.screen == GAME_MENU);
    assert(game.menu.mascot_time > 0.5f && game.menu.mascot_time < 0.81f);
    key(&game, KEY_ENTER);
    assert(game.menu.menu_open && game.screen == GAME_MENU);
    click(&game, 1000, 397);
    assert(game.screen == GAME_TRAINER_SELECT);
    key(&game, KEY_RIGHT);
    key(&game, KEY_ENTER);
    confirm_campaign_name(&game);
    assert(game.screen == GAME_EXPLORATION && game.player.trainer == TRAINER_FEMALE);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU && game.menu.menu_open);
    key(&game, KEY_ENTER);
    for (int i = 0; i < 20; i++) tick(&game);
    assert(game.screen == GAME_TRAINER_SELECT);
    trainer_select_screen_init(&game.trainer_select);
    Image selection = capture(&game, target, "build/menu_v2_preview/selection_unchanged.png");
    UnloadImage(selection);
    key(&game, KEY_ESCAPE);
    assert(game.screen == GAME_MENU);
    key(&game, KEY_DOWN);
    key(&game, KEY_DOWN);
    assert(game.menu.selected_option == 2);
    key(&game, KEY_ENTER);
    for (int i = 0; i < 15; i++) tick(&game);
    assert(game.exit_requested);
    game.exit_requested = false;
    click(&game, 1000, 529);
    assert(game.exit_requested);
    game.exit_requested = false;
    key(&game, KEY_ESCAPE);
    assert(!game.menu.menu_open);
    MenuScreen saved = game.menu;
    game.menu.background = (Texture2D){0};
    game.menu.logo = (Texture2D){0};
    game.menu.mascot = (Texture2D){0};
    game.menu.mascot_idle = (Texture2D){0};
    Image fallback = capture(&game, target, "build/menu_v2_preview/fallback.png");
    UnloadImage(fallback);
    game.menu = saved;
    /* Amostra todas as fases em ambas as telas na mesma escala do jogo. */
    for (int screen = 0; screen < 2; screen++) {
        game.menu.menu_open = screen != 0;
        for (int frame = 0; frame < MENU_MASCOT_FRAME_COUNT; frame++) {
            game.menu.mascot_time = (frame + 0.01f) / MENU_MASCOT_FPS;
            Image phase = capture(&game, target, NULL);
            if (frame == 2) assert(ExportImage(phase, TextFormat("build/menu_mascot_preview/%s.png", screen ? "menu" : "title")));
            ImageCrop(&phase, (Rectangle){500, 360, 280, 280});
            assert(ExportImage(phase, TextFormat("build/menu_mascot_preview/%s_%02d.png", screen ? "menu" : "title", frame)));
            UnloadImage(phase);
        }
    }
    check_wind(&game, target);
    /* Capturas de inspeção visual: mesmas fases nas duas telas, outros relógios fixos. */
    assert(DirectoryExists("build/menu_ui_preview") || MakeDirectory("build/menu_ui_preview") == 0);
    const float ui_phases[] = {0, 0.25f, 0.75f, 0.85f, 1.0f, 1.25f, 2.25f, 3.0f};
    game.menu.mascot_time = 0;
    game.menu.grass_time = 0;
    for (int screen = 0; screen < 2; screen++) {
        game.menu.menu_open = screen != 0;
        for (int phase = 0; phase < 8; phase++) {
            game.menu.ui_time = ui_phases[phase];
            Image sample = capture(&game, target, TextFormat("build/menu_ui_preview/%s_%02d.png", screen ? "menu" : "title", phase));
            UnloadImage(sample);
        }
    }
    UnloadRenderTexture(target);
    game_shutdown(&game);
    assert(!game.menu.logo.id && !game.menu.mascot.id && !game.menu.mascot_idle.id && !game.menu.background.id && !game.trainer_select_assets.background.base.id);
    assert(!game.menu.grass_background.id && !game.menu.grass_atlas.id);
    assert(!game.menu.tree_background.id && !game.menu.tree_atlas.id);
    CloseWindow();
    puts("PASS: atlas/fallback alinhados, apenas mascote anima, pes/sombra/cenario fixos, relogio continuo no ENTER/ESC, 8 fases em ambas as telas, navegacao teclado/mouse, Campanha/exploracao/retorno, Sair, desenho imutavel e descarregamento.");
    return 0;
}
