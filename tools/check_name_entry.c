/* Verifica nomes, foco, transições e sessão com os módulos reais e OpenGL. */
#include "game.h"
#include "check_intro_flow.h"
#include "selection_ui.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void check_queue_codepoint(void *window, unsigned int codepoint);

static void input(unsigned int type,int a,int b)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}});
}

static void tick(Game *game)
{
    game_update(game);
    BeginDrawing(); game_draw(game); EndDrawing();
}

static void key(Game *game,int code)
{
    input(2,code,0); tick(game);
    input(1,code,0); tick(game); tick(game);
}

static void advance_dialogue(Game *game, int code)
{
    World *world = &game->world;
    assert(world->dialogue_title);
    const char *page = world->dialogue_pages[world->dialogue_page];
    if (world->dialogue_visible_characters < GetCodepointCount(page)) {
        int current_page = world->dialogue_page;
        key(game, code);
        assert(world->dialogue_title && world->dialogue_page == current_page);
        assert(world->dialogue_visible_characters == GetCodepointCount(page));
    }
    key(game, code);
}

static void click(Game *game,int x,int y)
{
    input(7,x,y); input(6,0,0); tick(game);
    input(5,0,0); tick(game); tick(game);
}

static void settle(Game *game)
{
    for (int i=0;i<45;i++) tick(game);
}

static void text(Game *game,const char *value)
{
    while (*value) {
        int bytes=0;
        int codepoint=GetCodepointNext(value,&bytes);
        check_queue_codepoint(GetWindowHandle(),(unsigned int)codepoint);
        BeginDrawing(); EndDrawing();
        tick(game);
        value+=bytes;
    }
}

static void append(NameEntryScreen *screen,const char *value)
{
    while (*value) {
        int bytes=0,codepoint=GetCodepointNext(value,&bytes);
        assert(name_entry_append(screen,codepoint));
        value+=bytes;
    }
}

static void capture(Game *game,RenderTexture2D target,const char *path)
{
    Game before=*game;
    BeginTextureMode(target); game_draw(game); EndTextureMode();
    assert(memcmp(&before,game,sizeof(before))==0);
    Image image=LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    if (strstr(path,".ppm")) {
        FILE *file=fopen(path,"wb"); assert(file);
        fprintf(file,"P6\n%d %d\n255\n",image.width,image.height);
        ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        size_t size=(size_t)image.width*image.height*3;
        assert(fwrite(image.data,1,size,file)==size);
        assert(fclose(file)==0);
    } else assert(ExportImage(image,path));
    UnloadImage(image);
}

static void check_names(void)
{
    NameEntryScreen screen;
    char result[PLAYER_NAME_CAPACITY];
    name_entry_init(&screen);
    assert(screen.count==0 && screen.draft[0]==0);
    assert(!name_entry_confirm(&screen,result) && screen.error);
    append(&screen,"   ");
    assert(!name_entry_confirm(&screen,result));
    name_entry_init(&screen);
    append(&screen,"  Antônio  ");
    assert(screen.count==11 && name_entry_confirm(&screen,result));
    assert(strcmp(result,"Antônio")==0);
    name_entry_init(&screen); append(&screen,"João");
    assert(screen.count==4 && strlen(screen.draft)==5);
    name_entry_backspace(&screen); name_entry_backspace(&screen);
    assert(screen.count==2 && strcmp(screen.draft,"Jo")==0);
    name_entry_init(&screen);
    for (int i=0;i<12;i++) assert(name_entry_append(&screen,0xe7));
    assert(screen.count==12 && strlen(screen.draft)==24 && !name_entry_append(&screen,'a'));
    name_entry_backspace(&screen);
    assert(screen.count==11 && strlen(screen.draft)==22);
    name_entry_init(&screen);
    assert(name_entry_append(&screen,0x10400));
    assert(strlen(screen.draft)==4 && screen.count==1);
    name_entry_backspace(&screen); assert(screen.draft[0]==0 && screen.count==0);
    for (int i=0;i<12;i++) assert(name_entry_append(&screen,0x10400));
    assert(strlen(screen.draft)==48 && screen.count==12 && !name_entry_append(&screen,0x10400));
    for (int i=0;i<12;i++) name_entry_backspace(&screen);
    assert(!name_entry_append(&screen,0x1f600));
    assert(!name_entry_append(&screen,'\n') && !name_entry_append(&screen,0x7f));
    append(&screen,"D'Ávila-2");
    assert(name_entry_confirm(&screen,result));
    Font font=GetFontDefault();
    int accents[]={0xe7,0xe3,0xf4,0xc1};
    for (unsigned int i=0;i<sizeof(accents)/sizeof(accents[0]);i++)
        assert(font.glyphs[GetGlyphIndex(font,accents[i])].value==accents[i]);
}

static void capture_native(Game *game, const char *path)
{
    float previous_scale = game->trainer_select_assets.ui.font_scale;
    selection_ui_update_fonts(&game->trainer_select_assets.ui, 1.5f);
    Game before = *game;
    GameCanvas canvas = {0};
    assert(game_canvas_resize(&canvas, 1920, 1080, 1920, 1080));
    BeginDrawing();
    game_canvas_begin(&canvas);
    game_draw(game);
    game_canvas_present(&canvas);
    EndDrawing();
    assert(memcmp(&before, game, sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture);
    ImageFlipVertical(&image);
    assert(ExportImage(image, path));
    UnloadImage(image);
    game_canvas_unload(&canvas);
    selection_ui_update_fonts(&game->trainer_select_assets.ui, previous_scale);
}

static void walk_axis(Game *game, bool horizontal, float target)
{
    float *position = horizontal ? &game->player.feet.x : &game->player.feet.y;
    bool forward = target > *position;
    int code = horizontal ? (forward ? KEY_D : KEY_A) : (forward ? KEY_S : KEY_W);
    input(2, code, 0);
    for (int i = 0; i < 400 && (forward ? *position < target : *position > target); i++) {
        float before = *position;
        player_update(&game->player, &game->world, 1.0f / 60.0f);
        world_follow_camera(&game->world, game->player.feet, 1.0f / 60.0f);
        if (*position == before) break;
    }
    input(1, code, 0);
    tick(game);
}

/* Isola o fluxo da campanha dos sorteios e do bloqueio do golpe fortalecido. */
static void configure_guardian_check(Game *game)
{
    BattleState *battle = &game->battle_screen.battle;
    for (int side = 0; side < 2; side++) {
        Team *team = side ? &battle->enemy_team : &battle->player_team;
        for (int i = 0; i < team->count; i++) {
            battle->boosted_move_indices[side][i] = 2;
            team->plants[i].moves[0].accuracy = 100;
            team->plants[i].moves[0].critical_chance = 0;
        }
    }
}

static void resolve_guardian_action(Game *game)
{
    assert(game->battle_screen.pending_action != BATTLE_PENDING_NONE);
    for (int i = 0; i < 24 && game->battle_screen.pending_action != BATTLE_PENDING_NONE; i++)
        battle_screen_update(&game->battle_screen,.1f);
    assert(game->battle_screen.pending_action == BATTLE_PENDING_NONE);
}

static void win_guardian(Game *game)
{
    configure_guardian_check(game);
    BattleState *battle = &game->battle_screen.battle;
    for (int i = 0; i < battle->enemy_team.count; i++) {
        battle->enemy_team.plants[i].current_hp = 1;
        key(game, KEY_ONE);
        resolve_guardian_action(game);
        if (battle->state == BATTLE_STATE_PLAYER_WIN) break;
        assert(battle->state == BATTLE_STATE_REPLACEMENT && battle->current_turn == 1);
        assert(Battle_SwitchPlant(battle, i + 1));
        battle->enemy_team.plants[i + 1].moves[0].accuracy = 0;
        assert(Battle_ExecuteMove(battle, 0));
    }
    assert(battle->state == BATTLE_STATE_PLAYER_WIN);
    key(game, KEY_ENTER);
    assert(game->screen == GAME_EXPLORATION);
}

static void lose_guardian(Game *game)
{
    configure_guardian_check(game);
    BattleState *battle = &game->battle_screen.battle;
    for (int i = 0; i < battle->player_team.count; i++) {
        Plant *plant = &battle->player_team.plants[i];
        plant->current_hp = 1;
        int accuracy = plant->moves[0].accuracy;
        plant->moves[0].accuracy = 0;
        assert(Battle_ExecuteMove(battle, 0));
        plant->moves[0].accuracy = accuracy;
        assert(Battle_ExecuteMove(battle, 0));
        if (battle->state == BATTLE_STATE_ENEMY_WIN) break;
        assert(Battle_SwitchPlant(battle, i + 1));
    }
    assert(battle->state == BATTLE_STATE_ENEMY_WIN);
    key(game, KEY_ENTER);
    assert(game->screen == GAME_EXPLORATION);
}

static void interact_at(Game *game, float x, float y)
{
    walk_axis(game, true, x);
    walk_axis(game, false, y);
    key(game, KEY_E);
}

static void enter_bridge_control(Game *game)
{
    interact_at(game, 810, 570);
    assert(game->world.location == WORLD_BRIDGE_CONTROL);
}

static void enter_maintenance(Game *game)
{
    walk_axis(game, false, 588);
    walk_axis(game, true, 385);
    walk_axis(game, false, 629);
    walk_axis(game, true, 352);
    walk_axis(game, false, 670);
    interact_at(game, 243, 645);
    assert(game->world.location == WORLD_MAINTENANCE);
}

static void leave_maintenance_path(Game *game)
{
    walk_axis(game, false, 670);
    walk_axis(game, true, 352);
    walk_axis(game, false, 629);
    walk_axis(game, true, 385);
    walk_axis(game, false, 588);
    walk_axis(game, true, 434);
    walk_axis(game, false, 478);
}

static void enter_depot(Game *game)
{
    walk_axis(game, false, 470);
    walk_axis(game, true, 396);
    walk_axis(game, false, 437);
    interact_at(game, 278, 379);
    assert(game->world.location == WORLD_DEPOT);
}

static void approach_nursery(Game *game)
{
    if (game->player.feet.x < 1160) {
        walk_axis(game, false, 472);
        walk_axis(game, true, 1210);
        walk_axis(game, false, 444);
    }
    interact_at(game, 1396, 330);
}

static void check_greenhouse(Game *game)
{
    World *world = &game->world;
    assert(world->detail_enabled && game->world_assets.courtyard_detail.id);
    key(game, KEY_F8);
    assert(!world->detail_enabled);
    key(game, KEY_F8);
    assert(world->detail_enabled);
    capture_native(game, "build/name_entry_preview/exploration_1080p.png");
    World camera_test;
    world_init(&camera_test);
    Vector2 far_corner = {world->bounds.width, world->bounds.height};
    world_follow_camera(&camera_test, far_corner, 0.05f);
    Vector2 partial = world_camera(&camera_test).target;
    world_snap_camera(&camera_test, far_corner);
    Vector2 limit = world_camera(&camera_test).target;
    assert(partial.x > world->bounds.x && partial.x < limit.x);
    assert(partial.y > world->bounds.y && partial.y < limit.y);
    assert(fabsf(limit.x - (world->bounds.x + world->bounds.width - GAME_SCREEN_WIDTH)) <= 0.5f);
    assert(fabsf(limit.y - (world->bounds.y + world->bounds.height - GAME_SCREEN_HEIGHT)) <= 0.5f);

    Player walking, running;
    player_init(&walking, TRAINER_MALE, (Vector2){858, 556});
    player_init(&running, TRAINER_MALE, walking.feet);
    input(2, KEY_D, 0);
    player_update(&walking, world, 0.1f);
    input(2, KEY_LEFT_SHIFT, 0);
    player_update(&running, world, 0.1f);
    assert(running.feet.x > walking.feet.x && running.walk_time > walking.walk_time);
    for (int i = 0; i < 10; i++) player_update(&running, world, 0.1f);
    assert(running.feet.x < 907); /* A corrida respeita a ponte fechada. */
    input(1, KEY_LEFT_SHIFT, 0);
    input(1, KEY_D, 0);
    tick(game);

    Vector2 initial = {560, 740};
    Player diagonal, canal_edge;
    player_init(&diagonal, TRAINER_MALE, initial);
    player_init(&canal_edge, TRAINER_MALE, (Vector2){899, 550});
    input(2, KEY_D, 0);
    input(2, KEY_W, 0);
    player_update(&diagonal, world, 0.1f);
    assert(diagonal.feet.x > initial.x && diagonal.feet.y < initial.y);
    assert(fabsf(hypotf(diagonal.feet.x - initial.x, diagonal.feet.y - initial.y) - 19) < 0.01f);
    player_update(&canal_edge, world, 0.1f);
    assert(canal_edge.feet.x == 899 && canal_edge.feet.y < 550);
    input(1, KEY_W, 0);
    input(1, KEY_D, 0);
    tick(game);
    Vector2 corner = world_constrain(world, initial, (Vector2){524, 817});
    assert(corner.x == initial.x && corner.y == initial.y);
    for (int y = 311; y <= 431; y++) {
        Vector2 loop = world_constrain(world, initial, (Vector2){759, y});
        assert(loop.x == 759 && loop.y == y);
    }
    Vector2 garden_bed = world_constrain(world, initial, (Vector2){655, 354});
    assert(garden_bed.x == initial.x && garden_bed.y == initial.y);
    for (int row = 0; row < 8; row++) {
        World sample = {.alarm = (row & 4) != 0, .water = (row & 2) != 0,
                        .light = (row & 1) != 0, .maintenance_key = true};
        assert(world_gate_rule(&sample) == (row == 3));
    }
    const int pressures[] = {0, 0, 5, 3, 3, 1, 8, 6};
    for (int row = 0; row < 8; row++) {
        World sample = {.north_channel = (row & 4) != 0, .south_channel = (row & 2) != 0,
                        .drain_open = (row & 1) != 0};
        assert(world_bridge_pressure(&sample) == pressures[row]);
        assert(world_bridge_open(&sample) == (row == 3 || row == 4));
    }
    const unsigned int solution = (1u << 1) | (1u << 4) | (1u << 6) | (1u << 7);
    int solutions = 0;
    for (unsigned int mask = 0; mask < (1u << WORLD_RELAY_COUNT); mask++) {
        if (world_relays_match(mask)) {
            assert(mask == solution);
            solutions++;
        }
    }
    assert(solutions == 1 && !world_relays_match(solution | (1u << WORLD_RELAY_COUNT)));

    World animated = {.dialogue_title="Teste", .dialogue_pages={"Água, pressão e luz.", NULL}};
    world_update_dialogue(&animated, 0.0125f);
    assert(animated.dialogue_visible_characters == 0);
    world_update_dialogue(&animated, 0.0125f);
    assert(animated.dialogue_visible_characters == 1);
    world_update_dialogue(&animated, -1);
    world_update_dialogue(&animated, NAN);
    assert(animated.dialogue_visible_characters == 1);
    world_update_dialogue(&animated, 0.1f);
    assert(animated.dialogue_visible_characters == 5);
    world_update_dialogue(&animated, 60);
    assert(animated.dialogue_visible_characters == GetCodepointCount(animated.dialogue_pages[0]));
    assert(animated.dialogue_title); /* Terminar a animação não fecha a conversa. */

    RenderTexture2D target = LoadRenderTexture(1280, 720);
    assert(game->world_assets.courtyard.id && game->world_assets.bridge_open.id);
    assert(game->world_assets.tiles.id);
    capture(game, target, "build/name_entry_preview/greenhouse_start.png");
    interact_at(game, 535, 782);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Placa do jardim") == 0);
    advance_dialogue(game, KEY_ENTER);
    assert(!world->dialogue_title);
    walk_axis(game, true, 576);
    walk_axis(game, false, 510);
    interact_at(game, 600, 507);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Lia") == 0);
    assert(world->dialogue_page == 0 && strstr(world->dialogue_pages[0], "Eu sou a Lia"));
    assert(strstr(world->dialogue_pages[0], game->campaign.player_name));
    assert(strstr(world->dialogue_pages[1], "Babosa") && world->dialogue_pages[2]);
    Vector2 dialogue_position = game->player.feet;
    key(game, KEY_D);
    assert(game->player.feet.x == dialogue_position.x && game->player.feet.y == dialogue_position.y);
    capture(game, target, "build/name_entry_preview/greenhouse_dialogue.png");
    key(game, KEY_E);
    assert(world->dialogue_title && world->dialogue_page == 0);
    assert(world->dialogue_visible_characters == GetCodepointCount(world->dialogue_pages[0]));
    input(2, KEY_E, 0); tick(game);
    assert(world->dialogue_page == 1 && world->dialogue_visible_characters < GetCodepointCount(world->dialogue_pages[1]));
    for (int held = 0; held < 3; held++) tick(game);
    assert(world->dialogue_title && world->dialogue_page == 1);
    assert(world->dialogue_visible_characters < GetCodepointCount(world->dialogue_pages[1]));
    input(1, KEY_E, 0); tick(game);
    key(game, KEY_ESCAPE);
    assert(!world->dialogue_title && game->screen == GAME_EXPLORATION);
    interact_at(game, 600, 507);
    assert(strstr(world->dialogue_pages[0], "Olha quem voltou") && !strstr(world->dialogue_pages[0], "Eu sou a Lia"));
    assert(strstr(world->dialogue_pages[0], game->campaign.player_name));
    assert(world->dialogue_pages[1] && !world->dialogue_pages[2]);
    key(game, KEY_ESCAPE);
    walk_axis(game, true, 630);
    assert(game->player.feet.x < 622); /* Os pés da jardineira bloqueiam a passagem. */
    walk_axis(game, false, 479);
    assert(world_interaction_view(world, (Vector2){429,479}).state == HUD_INTERACTION_NONE);
    enter_maintenance(game);
    Camera2D room_camera = world_camera(world);
    assert(fabsf(room_camera.zoom - (1280.0f / 720.0f) * 0.68f) < 0.001f);
    assert(room_camera.offset.x == GAME_SCREEN_WIDTH / 2.0f);
    assert(room_camera.offset.y == GAME_SCREEN_HEIGHT / 2.0f);
    /* O enquadramento remoto mantém os interiores completos, com margens. */
    for (int location = WORLD_MAINTENANCE; location < WORLD_LOCATION_COUNT; location++) {
        World sample = *world;
        sample.location = (WorldLocation)location;
        Camera2D camera = world_camera(&sample);
        Vector2 top_left = GetWorldToScreen2D((Vector2){280, 191}, camera);
        Vector2 bottom_right = GetWorldToScreen2D((Vector2){1000, 611}, camera);
        assert(fabsf(camera.zoom - room_camera.zoom) < 0.001f);
        assert(top_left.x > 0 && top_left.y > 0);
        assert(bottom_right.x < GAME_SCREEN_WIDTH && bottom_right.y < GAME_SCREEN_HEIGHT);
        assert(fabsf(bottom_right.x - top_left.x - 720 * camera.zoom) < 0.001f);
        assert(fabsf(bottom_right.y - top_left.y - 420 * camera.zoom) < 0.001f);
    }
    assert(game->world_assets.maintenance_interior.id);
    Vector2 previous = {780, 380};
    Vector2 blocked = world_constrain(world, previous, (Vector2){650, 335});
    assert(blocked.x == previous.x && blocked.y == previous.y);
    Vector2 clear = world_constrain(world, previous, (Vector2){780, 340});
    assert(clear.x == 780 && clear.y == 340);
    walk_axis(game, true, 780); /* Corredor livre ao lado da prateleira central. */
    walk_axis(game, false, 205);
    assert(game->player.feet.y >= world->bounds.y + 4);
    interact_at(game, 846, 416);
    assert(world->location == WORLD_MAINTENANCE && !world->basement_visited);
    capture(game, target, "build/name_entry_preview/greenhouse_maintenance.png");
    capture_native(game, "build/name_entry_preview/maintenance_1080p.png");
    WorldAssets saved_assets = game->world_assets;
    game->world_assets.maintenance_interior.id = 0;
    capture(game, target, "build/name_entry_preview/maintenance_fallback.png");
    game->world_assets = saved_assets;
    walk_axis(game, true, 520);
    walk_axis(game, false, 370);
    interact_at(game, 445, 360); /* Aproxima pela frente da bancada ampliada. */
    interact_at(game, 537, 281);
    assert(!world->alarm && world->alarm_note);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Bilhete da manutenção") == 0);
    assert(strstr(world->dialogue_pages[0], "SE D, ENTÃO C"));
    advance_dialogue(game, KEY_E);
    key(game, KEY_E); /* O bilhete continua legível depois de registrado. */
    assert(world->dialogue_title && world->alarm_note && !world->alarm);
    advance_dialogue(game, KEY_E);
    walk_axis(game, false, 370); /* Contorna a prateleira antes de voltar à porta. */
    interact_at(game, 640, 572);
    assert(world->location == WORLD_EXTERIOR);
    leave_maintenance_path(game);
    enter_depot(game);
    interact_at(game, 640, 420);
    assert(game->screen == GAME_BATTLE && game->encounter == WORLD_ENCOUNTER_ALOE);
    assert(game->battle_screen.setup_side < 0);
    lose_guardian(game);
    assert(world->location == WORLD_EXTERIOR && game->player.feet.x == world_spawn().x);
    assert(game->campaign.has_team && !world->maintenance_key && world->alarm_note && !world->alarm);
    enter_depot(game);
    interact_at(game, 640, 420);
    assert(game->screen == GAME_BATTLE && game->battle_screen.setup_side < 0);
    capture(game, target, "build/name_entry_preview/greenhouse_guard.png");
    win_guardian(game);
    assert(world->location == WORLD_DEPOT && world->maintenance_key && !world->light);
    interact_at(game, 640, 572);
    walk_axis(game, false, 437);
    walk_axis(game, true, 396);
    walk_axis(game, false, 470);
    walk_axis(game, true, 429);
    enter_maintenance(game);
    interact_at(game, 846, 416);
    assert(world->location == WORLD_BASEMENT && world->basement_visited);
    capture(game, target, "build/name_entry_preview/greenhouse_basement.png");
    Vector2 clear_floor = world_constrain(world, (Vector2){350,390}, (Vector2){350,360});
    assert(clear_floor.x == 350 && clear_floor.y == 360);
    assert(world_interaction_view(world, (Vector2){890,445}).state == HUD_INTERACTION_NONE);
    interact_at(game, 640, 572);
    assert(world->location == WORLD_MAINTENANCE);
    interact_at(game, 846, 416);
    assert(world->location == WORLD_BASEMENT);
    interact_at(game, 640, 320);
    assert(world->light);
    assert(world_interaction_view(world, (Vector2){890,445}).state == HUD_INTERACTION_NONE);
    interact_at(game, 640, 572);
    assert(world->location == WORLD_MAINTENANCE);
    interact_at(game, 640, 572);
    assert(world->location == WORLD_EXTERIOR);
    leave_maintenance_path(game);
    walk_axis(game, false, 570);
    enter_bridge_control(game);
    interact_at(game, 539, 390);
    assert(world->north_channel && world->drain_open);
    assert(world_bridge_pressure(world) == 1 && !world_bridge_open(world));
    capture(game, target, "build/name_entry_preview/greenhouse_bridge_control.png");
    interact_at(game, 640, 572);
    HudInteraction removed_sign = world_interaction_view(world, (Vector2){770, 625});
    assert(removed_sign.state == HUD_INTERACTION_NONE);
    interact_at(game, 733, 555);
    assert(!world->drain_open && world_bridge_open(world));
    key(game, KEY_E);
    assert(world->drain_open && !world_bridge_open(world));
    enter_bridge_control(game);
    interact_at(game, 742, 390);
    assert(world->north_channel && world->south_channel && world_bridge_pressure(world) == 6);
    assert(!world_bridge_open(world));
    interact_at(game, 539, 390);
    assert(!world->north_channel && world->south_channel && world_bridge_open(world));
    interact_at(game, 640, 572);
    interact_at(game, 868, 578);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Bento") == 0);
    assert(strstr(world->dialogue_pages[0], "Sou o Bento"));
    assert(strstr(world->dialogue_pages[0], game->campaign.player_name));
    assert(strstr(world->dialogue_pages[1], "Ponte liberada"));
    capture(game, target, "build/name_entry_preview/campaign_technician.png");
    advance_dialogue(game, KEY_ENTER);
    assert(world->dialogue_title && world->dialogue_page == 1);
    advance_dialogue(game, KEY_ENTER);
    assert(world->dialogue_title && world->dialogue_page == 2);
    advance_dialogue(game, KEY_ENTER);
    walk_axis(game, false, 573);
    walk_axis(game, true, 888);
    walk_axis(game, false, 556);
    capture(game, target, "build/name_entry_preview/botanical_bridge_open.png");
    walk_axis(game, true, 1080);
    assert(game->player.feet.x > 1045);
    approach_nursery(game);
    assert(world->location == WORLD_EXTERIOR && !world_gate_open(world));
    capture(game, target, "build/name_entry_preview/botanical_nursery_entrance.png");
    walk_axis(game, false, 280);
    assert(game->player.feet.y >= 318); /* A fachada fica fora do caminho. */
    walk_axis(game, false, 444);
    walk_axis(game, true, 1210);
    walk_axis(game, false, 472);
    walk_axis(game, true, 1080);
    walk_axis(game, false, 626);
    interact_at(game, 1168, 644);
    assert(world->water && world_gate_rule(world));
    walk_axis(game, true, 1087);
    approach_nursery(game);
    assert(world->location == WORLD_NURSERY && world->gate_unlocked);
    capture(game, target, "build/name_entry_preview/greenhouse_open.png");
    interact_at(game, 790, 485);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Íris") == 0);
    assert(strstr(world->dialogue_pages[0], "Sou a Íris"));
    assert(strstr(world->dialogue_pages[0], game->campaign.player_name));
    assert(strstr(world->dialogue_pages[1], "Girassol"));
    capture(game, target, "build/name_entry_preview/campaign_researcher.png");
    advance_dialogue(game, KEY_E);
    assert(world->dialogue_title && world->dialogue_page == 1);
    advance_dialogue(game, KEY_E);
    assert(strstr(world->dialogue_pages[world->dialogue_page], "placa nos canais"));
    advance_dialogue(game, KEY_E);
    walk_axis(game, true, 740);
    walk_axis(game, false, 375);
    interact_at(game, 782, 315);
    assert(!world->relay_panel_open);
    interact_at(game, 640, 420);
    assert(game->screen == GAME_BATTLE && game->encounter == WORLD_ENCOUNTER_SUNFLOWER);
    win_guardian(game);
    assert(world->guardian_defeated);
    walk_axis(game, true, 740);
    walk_axis(game, false, 375);
    interact_at(game, 782, 315);
    key(game, KEY_ENTER);
    assert(world->relay_panel_open && world->relay_rejected && world->relay_step == 0);
    click(game, 289, 237); /* B satisfaz o primeiro XOR. */
    key(game, KEY_ENTER);
    assert(world->relay_step == 1 && !world->relay_rejected);
    key(game, KEY_ENTER);
    assert(world->relay_step == 2 && world->relay_attempts == 3);
    key(game, KEY_ENTER);
    assert(world->relay_step == 2 && world->relay_attempts == 3); /* Ainda falta a pista. */
    key(game, KEY_ESCAPE);
    interact_at(game, 640, 572);
    assert(world->location == WORLD_EXTERIOR);
    walk_axis(game, false, 419);
    interact_at(game, 1271, 401);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Placa dos canais") == 0);
    assert(strstr(world->dialogue_pages[0], "E = (B XOR F)"));
    advance_dialogue(game, KEY_E);
    assert(world->canal_note);
    walk_axis(game, false, 419);
    approach_nursery(game);
    assert(world->location == WORLD_NURSERY);
    walk_axis(game, true, 740);
    walk_axis(game, false, 375);
    interact_at(game, 782, 315);
    assert(world->relay_panel_open && !world->completed && world->relay_step == 2);
    Vector2 before = game->player.feet;
    key(game, KEY_D);
    assert(game->player.feet.x == before.x && game->player.feet.y == before.y);
    key(game, KEY_TWO); /* Quebra a primeira regra enquanto a terceira está visível. */
    key(game, KEY_ENTER);
    assert(world->relay_attempts == 4 && world->relay_rejected && world->relay_step == 0);
    key(game, KEY_TWO);
    key(game, KEY_SIX); /* B e F satisfazem as oito primeiras regras. */
    for (int i = 0; i < WORLD_RELAY_RULE_COUNT - 1; i++) {
        assert(world->relay_step == i && !world->completed);
        key(game, KEY_ENTER);
    }
    key(game, KEY_ENTER);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 1 && world->relay_rejected && !world->completed);
    capture(game, target, "build/name_entry_preview/greenhouse_relays.png");
    key(game, KEY_SIX);
    key(game, KEY_FIVE);
    key(game, KEY_SEVEN);
    key(game, KEY_EIGHT);
    key(game, KEY_LEFT);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 2);
    unsigned int attempts = world->relay_attempts;
    key(game, KEY_ESCAPE);
    assert(game->screen == GAME_EXPLORATION && !world->relay_panel_open);
    key(game, KEY_E);
    assert(world->relay_panel_open && world->relay_mask == solution && world->relay_attempts == attempts);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 2);
    key(game, KEY_ENTER);
    assert(world->relay_step == WORLD_RELAY_RULE_COUNT - 1 && !world->completed);
    click(game, 232, 615);
    assert(world->completed);
    assert(!world->relay_panel_open && world->relay_attempts == attempts + 2);
    assert(world->dialogue_title && strcmp(world->dialogue_title, "Íris") == 0);
    assert(strstr(world->dialogue_pages[0], game->campaign.player_name));
    assert(strstr(world->dialogue_pages[0], "Obrigada"));
    world_update_dialogue(world, 60);
    capture(game, target, "build/name_entry_preview/greenhouse_complete_thanks.png");
    advance_dialogue(game, KEY_ENTER);
    advance_dialogue(game, KEY_ENTER);
    assert(world->completed && !world->dialogue_title);
    capture(game, target, "build/name_entry_preview/greenhouse_complete.png");
    key(game, KEY_ENTER);
    assert(game->screen == GAME_MENU);
    UnloadRenderTexture(target);
    puts("PASS: diálogos, placas, prédios, guardiões, chave, subsolo, ponte, pistas e saída.");
}

int main(int argc, char **argv)
{
    bool setup_only = argc > 1 && strcmp(argv[1], "--setup-only") == 0;
    assert(DirectoryExists("build/name_entry_preview") || MakeDirectory("build/name_entry_preview")==0);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Verificacao de nome de campanha");
    assert(IsWindowReady()); SetExitKey(KEY_NULL); SetTargetFPS(60);
    check_names();
    Game game; game_init(&game);
    game.screen=GAME_TRAINER_SELECT;
    for (int gender=0;gender<2;gender++) {
        trainer_select_screen_init(&game.trainer_select);
        game.trainer_select.selected=(Trainer)gender;
        name_entry_init(&game.name_entry);
        game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
        float male=game.trainer_select.male_idle_time, female=game.trainer_select.female_idle_time;
        input(2,KEY_ENTER,0); tick(&game);
        assert(game.screen==GAME_NAME_ENTRY && game.name_entry.direction==1);
        text(&game,"AD"); /* Texto durante a transição deve ser descartado. */
        settle(&game);
        assert(game.name_entry.progress==1 && game.name_entry.input_blocked);
        assert(game.name_entry.draft[0]==0 && game.name_entry.focus==0);
        assert(game.trainer_select.male_idle_time>male && game.trainer_select.female_idle_time>female);
        assert(game.trainer_select.selected==(Trainer)gender);
        input(1,KEY_ENTER,0); tick(&game); tick(&game);
        text(&game,gender ? "  João  " : "Antônio");
        assert(strcmp(game.name_entry.draft,gender ? "  João  " : "Antônio")==0);
        key(&game,KEY_A); key(&game,KEY_D); key(&game,KEY_LEFT); key(&game,KEY_RIGHT);
        assert(game.trainer_select.selected==(Trainer)gender);
        key(&game,KEY_TAB); assert(game.name_entry.focus==1);
        input(2,KEY_LEFT_SHIFT,0); key(&game,KEY_TAB); input(1,KEY_LEFT_SHIFT,0); tick(&game);
        assert(game.name_entry.focus==0);
        capture_native(&game, TextFormat("build/name_entry_preview/name_1080p_%d.png", gender));
        click(&game,950,660); /* Confirmar por mouse transfere nome e personagem. */
        check_complete_intro(&game, key);
        assert(game.screen==GAME_EXPLORATION);
        assert(game.campaign.trainer==(Trainer)gender && game.player.trainer==(Trainer)gender);
        assert(strcmp(game.campaign.player_name,gender ? "João" : "Antônio")==0);
        key(&game,KEY_ESCAPE);
        game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
        game.name_entry.progress=0;
        key(&game,KEY_ENTER); settle(&game);
        assert(game.screen==GAME_NAME_ENTRY);
        char draft[PLAYER_NAME_CAPACITY]; strcpy(draft,game.name_entry.draft);
        key(&game,KEY_ESCAPE); settle(&game);
        assert(game.screen==GAME_TRAINER_SELECT && game.trainer_select.selected==(Trainer)gender);
        assert(strcmp(draft,game.name_entry.draft)==0);
    }
    /* Foco, validação e repetição de Backspace pelo módulo real. */
    game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
    name_entry_init(&game.name_entry);
    key(&game,KEY_ENTER); settle(&game);
    key(&game,KEY_ENTER); assert(game.screen==GAME_NAME_ENTRY && game.name_entry.error);
    text(&game,"çãôéABCDEFGHI");
    assert(game.name_entry.count==12);
    input(2,KEY_BACKSPACE,0); tick(&game);
    for (int i=0;i<12;i++) name_entry_update(&game.name_entry,true,.1f);
    input(1,KEY_BACKSPACE,0); tick(&game);
    assert(game.name_entry.count==0 && game.name_entry.draft[0]==0);
    text(&game,"João");
    click(&game,180,660); settle(&game);
    assert(game.screen==GAME_TRAINER_SELECT && strcmp(game.name_entry.draft,"João")==0);
    key(&game,KEY_ENTER); settle(&game);
    assert(strcmp(game.name_entry.draft,"João")==0 && game.name_entry.focus==0);
    key(&game,KEY_TAB); key(&game,KEY_TAB); assert(game.name_entry.focus==2);
    key(&game,KEY_ENTER); check_complete_intro(&game, key); assert(game.screen==GAME_EXPLORATION);
    if (!setup_only) check_greenhouse(&game);
    /* Botão Voltar focado por teclado e soltura de mouse mantido na entrada. */
    game.screen=GAME_TRAINER_SELECT; game.input_blocked=false;
    name_entry_init(&game.name_entry);
    input(7,820,360); input(6,0,0); tick(&game);
    key(&game,KEY_ENTER); settle(&game);
    assert(game.screen==GAME_NAME_ENTRY && game.name_entry.input_blocked);
    text(&game,"Não"); assert(game.name_entry.count==0);
    input(5,0,0); tick(&game); tick(&game);
    text(&game,"João");
    key(&game,KEY_TAB); assert(game.name_entry.focus==1);
    input(2,KEY_ENTER,0); tick(&game); settle(&game);
    assert(game.screen==GAME_TRAINER_SELECT && game.input_blocked);
    input(1,KEY_ENTER,0); tick(&game); tick(&game);
    assert(!game.input_blocked);
    /* Capturas temporais controladas: relógios avançam com o mesmo dt da transição. */
    RenderTexture2D target=LoadRenderTexture(1280,720);
    for (int gender=0;gender<2;gender++) {
        trainer_select_screen_init(&game.trainer_select);
        game.trainer_select.selected=(Trainer)gender;
        name_entry_init(&game.name_entry);
        game.screen=GAME_TRAINER_SELECT;
        capture(&game,target,TextFormat("build/name_entry_preview/selection_%d.ppm",gender));
        game.screen=GAME_NAME_ENTRY;
        name_entry_begin(&game.name_entry,true);
        for (int i=0;i<=12;i++) {
            Rectangle card=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
            assert(card.x>=160 && card.width>=300 && card.width<=320);
            capture(&game,target,TextFormat("build/name_entry_preview/forward_%d_%02d.ppm",gender,i));
            if (i<12) {
                trainer_select_screen_update(&game.trainer_select,false,.05f);
                name_entry_update(&game.name_entry,false,.05f);
            }
        }
        Rectangle last=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
        Rectangle desired=name_entry_portrait_bounds();
        assert(memcmp(&last,&desired,sizeof(last))==0 && game.name_entry.progress==1);
        capture(&game,target,TextFormat("build/name_entry_preview/empty_%d.png",gender));
        append(&game.name_entry,gender ? "João" : "Antônio");
        capture(&game,target,TextFormat("build/name_entry_preview/final_%d.png",gender));
        name_entry_begin(&game.name_entry,false);
        for (int i=0;i<=12;i++) {
            capture(&game,target,TextFormat("build/name_entry_preview/reverse_%d_%02d.ppm",gender,i));
            if (i<12) {
                trainer_select_screen_update(&game.trainer_select,false,.05f);
                name_entry_update(&game.name_entry,false,.05f);
            }
        }
        last=name_entry_transition_bounds(&game.name_entry,(Trainer)gender);
        desired=trainer_select_card_bounds((Trainer)gender);
        assert(memcmp(&last,&desired,sizeof(last))==0 && game.name_entry.progress==0);
        game.screen=GAME_TRAINER_SELECT;
        capture(&game,target,TextFormat("build/name_entry_preview/selection_end_%d.ppm",gender));
    }
    UnloadRenderTexture(target); game_shutdown(&game); CloseWindow();
    puts("PASS: UTF-8/acentos, 12 codepoints, Backspace multibyte/repetido, nomes vazios, foco, fila nativa, descarte, entrada mantida, sessão, rascunho, dois sentidos/personagens e desenho imutável.");
    return 0;
}
