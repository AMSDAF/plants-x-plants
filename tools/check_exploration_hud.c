/* Verifica estados da interação, fontes e composição real do HUD em diferentes resoluções. */
#include "game.h"
#include "game_canvas.h"
#include "exploration_hud.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void capture(Game *game, GameCanvas *canvas, const char *name, int width, int height)
{
    assert(game_canvas_resize(canvas, width, height, width, height));
    Game before = *game;
    BeginDrawing();
    game_canvas_begin(canvas);
    game_draw(game);
    game_canvas_present(canvas);
    EndDrawing();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas->target.texture);
    ImageFlipVertical(&image);
    assert(ExportImage(image, TextFormat("build/hud_preview/%s.png", name)));
    UnloadImage(image);
}

static void check_prompt(const UiTheme *ui, HudInteraction interaction)
{
    Rectangle bounds = exploration_prompt_bounds(ui, &interaction);
    if (interaction.state == HUD_INTERACTION_NONE) {
        assert(bounds.width == 0);
        return;
    }
    assert(fabsf(bounds.x+bounds.width/2-640) <= 0.5f);
    assert(bounds.y+bounds.height == 696 && bounds.width <= 640);
    assert(bounds.height <= 104);
    Camera2D camera = {.offset={640, 360}, .target=interaction.target, .zoom=1};
    Rectangle normal = exploration_marker_bounds(&interaction, camera);
    camera.zoom = 2;
    Rectangle zoomed = exploration_marker_bounds(&interaction, camera);
    assert(normal.width == 20 && normal.height == 26);
    assert(normal.width == zoomed.width && normal.height == zoomed.height);
    camera.target = (Vector2){4000, 4000};
    assert(exploration_marker_bounds(&interaction, camera).width == 0);
}

static void check_states(Game *game)
{
    World *world = &game->world;
    world_init(world);
    world->location = WORLD_MAINTENANCE;
    world->bounds = (Rectangle){280, 191, 720, 420};
    game->player.feet = (Vector2){846, 416};
    world_snap_camera(world, game->player.feet);
    HudInteraction blocked = world_interaction_view(world, game->player.feet);
    assert(blocked.state == HUD_INTERACTION_BLOCKED);
    assert(strcmp(blocked.context, "Escotilha de manutenção") == 0);
    assert(strstr(blocked.message, "trancada") && !strstr(blocked.message, "[E]"));
    World before = *world;
    world_interact(world, &game->player.feet);
    assert(memcmp(&before, world, sizeof(before)) == 0);
    check_prompt(&game->ui, blocked);

    world->maintenance_key = true;
    HudInteraction enabled = world_interaction_view(world, game->player.feet);
    assert(enabled.state == HUD_INTERACTION_AVAILABLE && strstr(enabled.message, "Descer"));
    check_prompt(&game->ui, enabled);
    world->alarm_note = true;
    HudInteraction recorded = world_interaction_view(world, (Vector2){537, 281});
    assert(recorded.state == HUD_INTERACTION_INFO);
    check_prompt(&game->ui, recorded);
    HudInteraction none = world_interaction_view(world, (Vector2){780, 450});
    assert(none.state == HUD_INTERACTION_NONE);
    check_prompt(&game->ui, none);

    world->alarm_note = false;
    Vector2 shared = {490, 285};
    HudInteraction selected = world_interaction_view(world, shared);
    assert(selected.target.x == 537 && selected.target.y == 281);
    world_interact(world, &shared);
    assert(world->alarm_note && world->alarm); /* E atingiu a mesma pista indicada, não o alarme próximo. */

    world->dialogue_title = "Conversa";
    assert(world_interaction_view(world, game->player.feet).state == HUD_INTERACTION_NONE);
    world->dialogue_title = NULL;
    world->relay_panel_open = true;
    assert(world_interaction_view(world, game->player.feet).state == HUD_INTERACTION_NONE);
    world->relay_panel_open = false;
    world->completed = true;
    assert(world_interaction_view(world, game->player.feet).state == HUD_INTERACTION_NONE);
    world->completed = false;
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Verificacao do HUD creme");
    assert(IsWindowReady());
    assert(DirectoryExists("build/hud_preview") || MakeDirectory("build/hud_preview") == 0);
    Game game;
    game_init(&game);
    assert(game.ui.regular.texture.id && game.ui.semibold.texture.id);
    assert(game.ui.regular.texture.id != GetFontDefault().texture.id);
    const int glyphs[] = {'I', 'l', '1', 'O', '0', 0xe7, 0xe3, 0xf5, 0xe9, 0xc1};
    for (unsigned int i = 0; i < sizeof(glyphs)/sizeof(glyphs[0]); i++) {
        assert(GetGlyphIndex(game.ui.regular, glyphs[i]) != GetGlyphIndex(game.ui.regular, '?'));
        assert(GetGlyphIndex(game.ui.semibold, glyphs[i]) != GetGlyphIndex(game.ui.semibold, '?'));
    }
    check_states(&game);
    GameCanvas canvas = {0};
    game.screen = GAME_EXPLORATION;
    game.world.maintenance_key = false;
    capture(&game, &canvas, "blocked_720p", 1280, 720);
    capture(&game, &canvas, "blocked_1080p", 1920, 1080);
    game.world.maintenance_key = true;
    game.world.basement_visited = true;
    capture(&game, &canvas, "available_720p", 1280, 720);
    capture(&game, &canvas, "available_1080p", 1920, 1080);
    capture(&game, &canvas, "available_1440p", 2560, 1440);
    game.player.feet = (Vector2){537, 281};
    capture(&game, &canvas, "recorded", 1280, 720);
    game.player.feet = (Vector2){780, 450};
    capture(&game, &canvas, "no_target", 1280, 720);
    game.world.location = WORLD_EXTERIOR;
    game.world.bounds = (Rectangle){0, 0, 1672, 941};
    game.player.feet = (Vector2){1271, 401};
    world_snap_camera(&game.world, game.player.feet);
    capture(&game, &canvas, "exterior_1080p", 1920, 1080);
    UiTheme saved = game.ui;
    game.ui = (UiTheme){0};
    capture(&game, &canvas, "font_fallback", 1280, 720);
    game.ui = saved;

    game.screen = GAME_MENU;
    game.menu.menu_open = true;
    capture(&game, &canvas, "menu", 1280, 720);
    game.screen = GAME_TRAINER_SELECT;
    capture(&game, &canvas, "selection", 1280, 720);
    game.screen = GAME_NAME_ENTRY;
    game.name_entry.progress = 1;
    name_entry_append(&game.name_entry, 0xc1);
    name_entry_append(&game.name_entry, 'l');
    name_entry_append(&game.name_entry, 'v');
    name_entry_append(&game.name_entry, 'a');
    name_entry_append(&game.name_entry, 'r');
    name_entry_append(&game.name_entry, 'o');
    capture(&game, &canvas, "name", 1920, 1080);
    game.screen = GAME_VERSUS_SELECT;
    capture(&game, &canvas, "versus", 1280, 720);
    game_canvas_unload(&canvas);
    game_shutdown(&game);
    assert(!game.ui.regular.texture.id && !game.ui.semibold.texture.id);
    CloseWindow();
    puts("PASS: fontes, acentos, alvo compartilhado, bloqueio, estados, seta, resolucoes e desenho imutavel.");
    return 0;
}
