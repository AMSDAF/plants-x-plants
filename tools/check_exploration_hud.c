/* Verifica estados da interação, fontes e composição real do HUD em diferentes resoluções. */
#include "game.h"
#include "game_canvas.h"
#include "exploration_hud.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <limits.h>
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
    assert(bounds.height == 76 || bounds.height == 104);
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
    world_interact(world, &game->player.feet, game->campaign.player_name);
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
    assert(recorded.state == HUD_INTERACTION_AVAILABLE && strstr(recorded.message, "Reler"));
    check_prompt(&game->ui, recorded);
    HudInteraction none = world_interaction_view(world, (Vector2){780, 450});
    assert(none.state == HUD_INTERACTION_NONE);
    check_prompt(&game->ui, none);

    world->alarm_note = false;
    Vector2 shared = {495, 310};
    HudInteraction selected = world_interaction_view(world, shared);
    assert(selected.target.x == 537 && selected.target.y == 300);
    world_interact(world, &shared, game->campaign.player_name);
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

/* Exercita pedidos, pistas e agradecimentos com o nome de maior largura. */
static void check_character_copy(const UiTheme *ui)
{
    const Vector2 positions[WORLD_NPC_COUNT] = {{600,507}, {868,578}, {790,485}};
    int overflow = 0;
    BeginDrawing();
    for (int character = 0; character < WORLD_NPC_COUNT; character++) {
        for (int flags = 0; flags < 64; flags++) {
            for (int contact = 0; contact < 4; contact++) {
                World world;
                world_init(&world);
                world.location = character == 2 ? WORLD_NURSERY : WORLD_EXTERIOR;
                world.maintenance_key = (flags & 1) != 0;
                world.north_channel = (flags & 1) != 0;
                world.south_channel = (flags & 2) != 0;
                world.drain_open = (flags & 4) != 0;
                world.water = (flags & 8) != 0;
                world.light = (flags & 16) != 0;
                world.alarm = (flags & 32) != 0;
                world.guardian_defeated = (flags & 2) != 0;
                world.canal_note = (flags & 4) != 0;
                world.alarm_note = (flags & 8) != 0;
                if (character == 2) world.npc_introduced[0] = (flags & 32) != 0;
                world.npc_introduced[character] = (contact & 1) != 0;
                world.npc_help_thanked[character] = (contact & 2) != 0;
                Vector2 feet = positions[character];
                world_interact(&world, &feet, "WWWWWWWWWWWW");
                assert(world.dialogue_title);
                for (int page = 0; page < WORLD_DIALOGUE_PAGE_COUNT && world.dialogue_pages[page]; page++) {
                    int lines = ui_text_wrap_font(ui->dialogue_font, world.dialogue_pages[page],
                                                  (Rectangle){0,0,858,1024}, 28, WHITE);
                    if (lines > 3) {
                        if (overflow < 12) fprintf(stderr, "%s: %d linhas: %s\n", world.dialogue_title, lines, world.dialogue_pages[page]);
                        overflow++;
                    }
                }
                if (character == 1 && world_bridge_open(&world)) {
                    assert(world.npc_help_thanked[1]);
                    if (!(contact & 2)) {
                        int request_page = (contact & 1) ? 0 : 1;
                        assert(strstr(world.dialogue_pages[request_page], "você conseguiu"));
                        assert(strstr(world.dialogue_pages[request_page], "Obrigado"));
                    }
                }
            }
        }
    }
    EndDrawing();
    assert(overflow == 0);
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
    assert(game.ui.interaction_frame.id && game.ui.interaction_frame_source.width > 0);
    assert(game.ui.dialogue_frame.id && game.ui.gardener_portrait.id);
    assert(game.ui.dialogue_skin.id);
    Image skin = LoadImageFromTexture(game.ui.dialogue_skin);
    assert(skin.data);
    Rectangle skin_source = game.ui.dialogue_skin_source;
    assert(skin_source.width > 0 && skin_source.height > 0);
    /* Cantos recortados transparentes, sem apagar o papel no interior. */
    for (int corner = 0; corner < 4; corner++) {
        int x = (int)(skin_source.x + ((corner & 1) ? skin_source.width-1 : 0));
        int y = (int)(skin_source.y + ((corner & 2) ? skin_source.height-1 : 0));
        assert(GetImageColor(skin, x, y).a < 26);
    }
    assert(GetImageColor(skin, (int)(skin_source.x+skin_source.width*0.3f),
                        (int)(skin_source.y+skin_source.height*0.5f)).a >= 250);
    UnloadImage(skin);
    assert(game.ui.technician_portrait.id && game.ui.researcher_portrait.id);
    assert(game.ui.technician_portrait_source.width > 0 && game.ui.researcher_portrait_source.width > 0);
    assert(game.ui.dialogue_font.texture.id && game.ui.dialogue_font.texture.id != GetFontDefault().texture.id);
    check_character_copy(&game.ui);
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
        assert(GetGlyphIndex(game.ui.dialogue_font, glyphs[i]) != GetGlyphIndex(game.ui.dialogue_font, '?'));
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
    game.player.feet = (Vector2){600,507};
    world_snap_camera(&game.world, game.player.feet);
    capture(&game, &canvas, "interaction_lia_720p", 1280, 720);
    capture(&game, &canvas, "interaction_lia_1080p", 1920, 1080);
    capture(&game, &canvas, "interaction_lia_1440p", 2560, 1440);
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
    world_init(&game.world);
    game.player.feet = (Vector2){868,578};
    world_snap_camera(&game.world, game.player.feet);
    capture(&game, &canvas, "botanical_bridge_closed_720p", 1280, 720);
    capture(&game, &canvas, "botanical_bridge_closed_1080p", 1920, 1080);
    capture(&game, &canvas, "botanical_bridge_closed_1440p", 2560, 1440);
    game.world.north_channel = true;
    game.world.drain_open = false;
    capture(&game, &canvas, "botanical_bridge_open", 1280, 720);
    game.player.feet = (Vector2){1271,401};
    world_snap_camera(&game.world, game.player.feet);
    capture(&game, &canvas, "botanical_nursery_closed", 1280, 720);
    game.world.water = true;
    game.world.light = true;
    game.world.alarm = false;
    capture(&game, &canvas, "botanical_nursery_open", 1280, 720);
    game.world = saved_world;
    game.player.feet = saved_feet;
    UiTheme saved = game.ui;
    game.ui.interaction_frame = (Texture2D){0};
    capture(&game, &canvas, "interaction_missing_frame", 1280, 720);
    game.ui = saved;
    game.ui.dialogue_font = (Font){0};
    capture(&game, &canvas, "interaction_missing_font", 1280, 720);
    game.ui = saved;
    game.ui = (UiTheme){0};
    capture(&game, &canvas, "font_fallback", 1280, 720);
    game.ui = saved;

    game.screen = GAME_EXPLORATION;
    game.world.location = WORLD_EXTERIOR;
    game.player.feet = (Vector2){600, 507};
    world_snap_camera(&game.world, game.player.feet);
    World original_world = game.world;
    Vector2 talk_feet = {600, 507};
    world_init(&game.world);
    const char *visitor = "ÁÉÍÓÚáéíóúÇç";
    world_interact(&game.world, &talk_feet, visitor);
    assert(game.world.dialogue_title && strcmp(game.world.dialogue_title, "Lia") == 0);
    assert(strstr(game.world.dialogue_pages[0], visitor));
    game.world.dialogue_visible_characters = INT_MAX;
    capture(&game, &canvas, "dialogue_lia_introduction", 1280, 720);
    game.world.dialogue_title = NULL;
    talk_feet = (Vector2){868, 578};
    world_interact(&game.world, &talk_feet, visitor);
    assert(game.world.dialogue_title && strcmp(game.world.dialogue_title, "Bento") == 0);
    assert(strstr(game.world.dialogue_pages[0], visitor));
    game.world.dialogue_visible_characters = INT_MAX;
    capture(&game, &canvas, "dialogue_bento_introduction", 1280, 720);
    game.world.dialogue_title = NULL;
    game.world.location = WORLD_NURSERY;
    talk_feet = (Vector2){790, 485};
    world_interact(&game.world, &talk_feet, visitor);
    assert(game.world.dialogue_title && strcmp(game.world.dialogue_title, "Íris") == 0);
    assert(strstr(game.world.dialogue_pages[0], "A Lia comentou"));
    game.world.dialogue_visible_characters = INT_MAX;
    capture(&game, &canvas, "dialogue_iris_introduction", 1280, 720);
    game.world = original_world;
    game.world.dialogue_title = NULL;
    game.world.npc_introduced[0] = true;
    game.world.maintenance_key = false;
    game.world.gate_unlocked = false;
    game.world.water = false;
    game.world.light = false;
    game.world.alarm = true;
    talk_feet = (Vector2){600,507};
    world_interact(&game.world, &talk_feet, "Álvaro");
    game.world.dialogue_visible_characters = 0;
    capture(&game, &canvas, "dialogue_typing_empty", 1280, 720);
    game.world.dialogue_visible_characters = 7;
    capture(&game, &canvas, "dialogue_typing_start", 1280, 720);
    game.world.dialogue_visible_characters = 60;
    capture(&game, &canvas, "dialogue_typing_accent", 1280, 720);
    game.world.dialogue_visible_characters = INT_MAX;
    capture(&game, &canvas, "dialogue_gardener_720p", 1280, 720);
    capture(&game, &canvas, "dialogue_gardener_1080p", 1920, 1080);
    capture(&game, &canvas, "dialogue_gardener_1440p", 2560, 1440);
    game.world.dialogue_page = 1;
    capture(&game, &canvas, "dialogue_last_page", 1280, 720);
    game.world.dialogue_title = NULL;
    game.world.npc_introduced[1] = true;
    game.world.north_channel = false;
    game.world.south_channel = false;
    talk_feet = (Vector2){868,578};
    world_interact(&game.world, &talk_feet, "Álvaro");
    game.world.dialogue_visible_characters = INT_MAX;
    capture(&game, &canvas, "dialogue_technician", 1280, 720);
    game.world.dialogue_title = NULL;
    game.world.north_channel = true;
    game.world.drain_open = false;
    game.world.npc_help_thanked[1] = false;
    world_interact(&game.world, &talk_feet, "Álvaro");
    assert(strstr(game.world.dialogue_pages[1], "válvula"));
    game.world.dialogue_visible_characters = INT_MAX;
    capture(&game, &canvas, "dialogue_bento_bridge_thanks", 1280, 720);
    game.world.dialogue_title = NULL;
    game.world.location = WORLD_NURSERY;
    game.world.npc_introduced[2] = true;
    game.world.guardian_defeated = false;
    game.world.alarm_note = false;
    game.world.canal_note = false;
    talk_feet = (Vector2){790,485};
    world_interact(&game.world, &talk_feet, "Álvaro");
    game.world.dialogue_visible_characters = INT_MAX;
    game.player.feet = (Vector2){760, 505};
    world_snap_camera(&game.world, game.player.feet);
    capture(&game, &canvas, "dialogue_researcher", 1280, 720);
    UiTheme portraits = game.ui;
    game.ui.researcher_portrait = (Texture2D){0};
    capture(&game, &canvas, "dialogue_researcher_missing_portrait", 1280, 720);
    game.ui = portraits;
    game.world.location = WORLD_EXTERIOR;
    game.player.feet = (Vector2){600, 507};
    world_snap_camera(&game.world, game.player.feet);
    game.world.dialogue_title = "Bento";
    game.ui.technician_portrait = (Texture2D){0};
    capture(&game, &canvas, "dialogue_technician_missing_portrait", 1280, 720);
    game.ui = portraits;
    game.world.dialogue_title = "Placa dos canais";
    capture(&game, &canvas, "dialogue_sign", 1280, 720);
    game.world.dialogue_title = "Lia";
    game.world.dialogue_page = 0;
    game.world.dialogue_pages[0] = "A ponte precisa de um único canal aberto e pressão correta. A água, a luz e o alarme têm estados próprios. Confira as pistas e os controles antes de abrir a central de relés e testar sua solução.";
    capture(&game, &canvas, "dialogue_long_text", 1280, 720);
    game.world.dialogue_pages[0] = game.world.dialogue_pages[1];
    saved = game.ui;
    game.ui.gardener_portrait = (Texture2D){0};
    capture(&game, &canvas, "dialogue_missing_portrait", 1280, 720);
    game.ui.dialogue_skin = (Texture2D){0};
    capture(&game, &canvas, "dialogue_missing_skin", 1280, 720);
    game.ui.dialogue_frame = (Texture2D){0};
    capture(&game, &canvas, "dialogue_missing_assets", 1280, 720);
    game.ui = (UiTheme){0};
    capture(&game, &canvas, "dialogue_missing_font", 1280, 720);
    game.ui = saved;
    game.world.dialogue_title = NULL;

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
    assert(!game.ui.interaction_frame.id);
    assert(!game.ui.dialogue_frame.id && !game.ui.gardener_portrait.id);
    assert(!game.ui.dialogue_skin.id);
    assert(!game.ui.technician_portrait.id && !game.ui.researcher_portrait.id);
    assert(!game.ui.dialogue_font.texture.id);
    CloseWindow();
    puts("PASS: fontes, acentos, alvo compartilhado, bloqueio, estados, seta, resolucoes, dialogos, paginas, retratos/reserva e desenho imutavel.");
    return 0;
}
