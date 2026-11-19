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
    assert(bounds.y+bounds.height == 696 && bounds.width <= 560);
    assert(bounds.height <= 72);
    Camera2D camera = {.offset={640, 360}, .target=interaction.target, .zoom=1};
    Rectangle normal = exploration_marker_bounds(&interaction, camera, 0);
    Rectangle raised = exploration_marker_bounds(&interaction, camera, 0.7f);
    Rectangle returned = exploration_marker_bounds(&interaction, camera, 1.4f);
    assert(fabsf(normal.y-raised.y-4) < 0.001f);
    assert(fabsf(normal.y-returned.y) < 0.001f && normal.x == raised.x);
    camera.zoom = 2;
    Rectangle zoomed = exploration_marker_bounds(&interaction, camera, 0);
    assert(normal.width == 18 && normal.height == 12);
    assert(normal.width == zoomed.width && normal.height == zoomed.height);
    camera.target = (Vector2){4000, 4000};
    assert(exploration_marker_bounds(&interaction, camera, 0).width == 0);
    /* A animação perto da borda não pode apagar a seta em parte do ciclo. */
    camera = (Camera2D){.zoom=1, .offset={640-interaction.target.x, 20-interaction.target.y}};
    for (int frame = 0; frame <= 84; frame++) {
        Rectangle edge = exploration_marker_bounds(&interaction, camera, frame/60.0f);
        assert(edge.width == 18 && edge.y >= 6 && edge.y+edge.height <= 714);
    }
}

static void check_marker_filter(UiTheme *ui)
{
    /* Mede a luz total da seta isolada ao longo do movimento subpixel real na GPU. */
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    assert(target.id && ui->interaction_chevron.width == 18 && ui->interaction_chevron.height == 12);
    Image sprite = LoadImageFromTexture(ui->interaction_chevron);
    for (int y = 0; y < sprite.height; y++)
        for (int x = 0; x < sprite.width; x++) {
            Color color = GetImageColor(sprite, x, y);
            assert(color.a == 0 || color.a == 255);
        }
    UnloadImage(sprite);
    HudInteraction interaction = {.state=HUD_INTERACTION_AVAILABLE, .context="Alvo",
        .message="Interagir", .target={640, 360}, .marker_anchor={640, 340}};
    float saved_time = ui->marker_time;
    double ratios[1] = {0};
    for (int filter = 0; filter < 1; filter++) {
        SetTextureFilter(ui->interaction_chevron, TEXTURE_FILTER_POINT);
        unsigned int minimum = ~0u, maximum = 0;
        for (int frame = 0; frame < 42; frame++) {
            ui->marker_time = frame * 1.4f / 42;
            BeginTextureMode(target);
            ClearBackground(BLACK);
            exploration_hud_draw(ui, "Local", "Objetivo", 0, &interaction, (Camera2D){.zoom=1});
            EndTextureMode();
            Image image = LoadImageFromTexture(target.texture);
            ImageFlipVertical(&image);
            unsigned int light = 0;
            for (int y = 310; y < 345; y++)
                for (int x = 620; x < 660; x++) {
                    Color color = GetImageColor(image, x, y);
                    light += color.r + color.g + color.b;
                }
            if (light < minimum) minimum = light;
            if (light > maximum) maximum = light;
            UnloadImage(image);
        }
        assert(minimum > 0);
        ratios[filter] = (double)maximum/minimum;
    }
    printf("Pixel marker luminance max/min: %.3f\n", ratios[0]);
    assert(ratios[0] < 1.001);
    ui->marker_time = saved_time;
    UnloadRenderTexture(target);
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
    assert(blocked.marker_anchor.x == 846);
    assert(blocked.marker_anchor.y <= game->player.feet.y-58/world_camera(world).zoom);
    assert(strstr(blocked.message, "trancada") && !strstr(blocked.message, "[E]"));
    World before = *world;
    world_interact(world, &game->player.feet);
    assert(memcmp(&before, world, sizeof(before)) == 0);
    check_prompt(&game->ui, blocked);

    world->maintenance_key = true;
    HudInteraction alarm = world_interaction_view(world, (Vector2){445, 310});
    assert(alarm.marker_anchor.x == 433); /* Centro da arte, diferente do ponto de alcance. */
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
    Vector2 shared = {495, 310};
    HudInteraction selected = world_interaction_view(world, shared);
    assert(selected.target.x == 537 && selected.target.y == 300);
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
    assert(game.ui.interaction_chevron.id);
    check_marker_filter(&game.ui);
    UiTheme animation = {0};
    ui_theme_update(&animation, 0.7f);
    assert(fabsf(animation.marker_time-0.7f) < 0.001f);
    ui_theme_update(&animation, 0.7f);
    assert(fabsf(animation.marker_time) < 0.001f);
    ui_theme_update(&animation, -1);
    assert(animation.marker_time == 0);
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
    ui_theme_update(&game.ui, 0.7f);
    capture(&game, &canvas, "chevron_raised_1080p", 1920, 1080);
    ui_theme_update(&game.ui, 0.7f);
    game.player.feet = (Vector2){537, 281};
    capture(&game, &canvas, "recorded", 1280, 720);
    game.player.feet = (Vector2){780, 450};
    capture(&game, &canvas, "no_target", 1280, 720);
    game.world.location = WORLD_EXTERIOR;
    game.world.bounds = (Rectangle){0, 0, 1672, 941};
    game.player.feet = (Vector2){1271, 401};
    world_snap_camera(&game.world, game.player.feet);
    capture(&game, &canvas, "exterior_1080p", 1920, 1080);
    World saved_world = game.world;
    Vector2 saved_feet = game.player.feet;
    static const struct {
        const char *name;
        WorldLocation location;
        Vector2 feet;
    } marker_samples[] = {
        {"marker_alarm", WORLD_MAINTENANCE, {445, 310}},
        {"marker_note", WORLD_MAINTENANCE, {537, 281}},
        {"marker_exit", WORLD_MAINTENANCE, {640, 565}},
        {"marker_valve", WORLD_EXTERIOR, {1168, 644}},
        {"marker_drain", WORLD_EXTERIOR, {733, 555}},
        {"marker_workshop", WORLD_EXTERIOR, {243, 645}},
        {"marker_depot", WORLD_EXTERIOR, {278, 379}},
        {"marker_nursery", WORLD_EXTERIOR, {1396, 330}},
        {"marker_bridge", WORLD_EXTERIOR, {810, 561}},
        {"marker_npc", WORLD_EXTERIOR, {630, 507}},
        {"marker_guard", WORLD_DEPOT, {640, 420}},
        {"marker_power", WORLD_BASEMENT, {640, 320}}
    };
    for (unsigned int i = 0; i < sizeof(marker_samples)/sizeof(marker_samples[0]); i++) {
        world_init(&game.world);
        game.world.location = marker_samples[i].location;
        game.player.feet = marker_samples[i].feet;
        world_snap_camera(&game.world, game.player.feet);
        HudInteraction selected = world_interaction_view(&game.world, game.player.feet);
        assert(selected.state != HUD_INTERACTION_NONE);
        capture(&game, &canvas, marker_samples[i].name, 1280, 720);
    }
    game.world = saved_world;
    game.player.feet = saved_feet;
    UiTheme saved = game.ui;
    game.ui = (UiTheme){0};
    capture(&game, &canvas, "font_fallback", 1280, 720);
    game.ui = saved;

    game.screen = GAME_MENU;
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
    game_canvas_unload(&canvas);
    game_shutdown(&game);
    assert(!game.ui.regular.texture.id && !game.ui.semibold.texture.id);
    assert(!game.ui.interaction_chevron.id);
    CloseWindow();
    puts("PASS: fontes, acentos, alvo compartilhado, bloqueio, estados, seta, resolucoes e desenho imutavel.");
    return 0;
}
