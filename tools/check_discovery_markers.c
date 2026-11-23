/* Verifica descoberta à distância, itens lidos, câmera e transição à seta próxima. */
#include "world.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int query(const World *world, Vector2 feet, HudDiscoveryMarker *markers)
{
    World before = *world;
    int count = world_discovery_markers(world,feet,markers,32);
    assert(memcmp(&before,world,sizeof(before)) == 0);
    for (int i = 0; i < count; i++) {
        assert(markers[i].position.x >= 24 && markers[i].position.x <= 1256);
        assert(markers[i].position.y >= 32 && markers[i].position.y <= 704);

    }
    return count;
}
static void capture(UiTheme *ui, World *world, Vector2 feet, Texture2D background,
                     int width, int height, const char *name)
{
    GameCanvas canvas = {0}; assert(game_canvas_resize(&canvas,width,height,width,height));
    World before = *world;
    BeginDrawing(); game_canvas_begin(&canvas); ClearBackground(DARKGREEN);
    BeginMode2D(world_camera(world));
    if (world->location == WORLD_BRIDGE_CONTROL) {
        WorldAssets room = {.bridge_control_interior=background};
        world_draw(ui,world,&room);
    } else if (background.id) DrawTexturePro(background,(Rectangle){0,0,background.width,background.height},
        (Rectangle){0,0,1672,941},(Vector2){0},0,WHITE);
    EndMode2D(); world_draw_interface(ui,world,feet);
    game_canvas_present(&canvas); EndDrawing();
    assert(memcmp(&before,world,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture); ImageFlipVertical(&image);
    assert(ExportImage(image,TextFormat("build/discovery_preview/%s.png",name)));
    UnloadImage(image); game_canvas_unload(&canvas);
}
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Interações à distância"); SetExitKey(KEY_NULL); SetTargetFPS(120);
    assert(IsWindowReady());
    assert(DirectoryExists("build/discovery_preview") || MakeDirectory("build/discovery_preview") == 0);
    UiTheme ui; ui_theme_load(&ui);
    Texture2D background = LoadTexture("assets/maps/courtyard_detail/courtyard_complete.png");
    if (background.id) SetTextureFilter(background,TEXTURE_FILTER_POINT);
    World world; world_init(&world);
    Vector2 feet = {640,640}; world_snap_camera(&world,feet);
    HudDiscoveryMarker markers[32];
    int count = query(&world,feet,markers);
    assert(count > 0);
    int conversations = 0; for (int i = 0; i < count; i++) conversations += markers[i].conversation;
    assert(conversations == 2); /* Lia e Bento; Íris está no viveiro. */
    Vector2 saved_camera = world.camera_position;
    world.camera_position = (Vector2){10000,10000};
    assert(query(&world,feet,markers) == 0);
    world.camera_position = saved_camera;
    capture(&ui,&world,feet,background,1280,720,"courtyard");
    capture(&ui,&world,feet,background,1920,1080,"courtyard_1080p");
    capture(&ui,&world,feet,background,1024,768,"courtyard_letterbox");
    world.entrance_sign_read = true;
    assert(query(&world,feet,markers) == count-1);
    world.entrance_sign_read = false;
    feet = (Vector2){505,448}; world_snap_camera(&world,feet);
    assert(world_interaction_view(&world,feet).state == HUD_INTERACTION_AVAILABLE);
    count = query(&world,feet,markers);
    /* A placa lógica próxima é indicada apenas pela seta existente. */
    Vector2 logical = GetWorldToScreen2D((Vector2){505,406},world_camera(&world));
    for (int i = 0; i < count; i++)
        assert(markers[i].position.x != logical.x || markers[i].position.y != logical.y);
    capture(&ui,&world,feet,background,1280,720,"near_logical_sign");
    feet = (Vector2){1270,620}; world_snap_camera(&world,feet);
    count = query(&world,feet,markers); assert(count > 0);
    world.canal_note = true; assert(query(&world,feet,markers) == count-1);
    world.location = WORLD_MAINTENANCE; feet = (Vector2){650,450};
    count = query(&world,feet,markers); assert(count > 0);
    world.alarm_note = true; assert(query(&world,feet,markers) == count-1);
    assert(world_discovery_markers(&world,feet,markers,1) == 1);
    assert(world_discovery_markers(&world,feet,NULL,32) == 0);
    assert(world_discovery_markers(&world,feet,markers,0) == 0);
    world.location = WORLD_DEPOT; world.maintenance_key = false; feet = (Vector2){640,300};
    count = query(&world,feet,markers); assert(count > 0);
    world.maintenance_key = true; assert(query(&world,feet,markers) == count-1);
    world.location = WORLD_NURSERY; world.guardian_defeated = false;
    count = query(&world,feet,markers); world.guardian_defeated = true;
    assert(query(&world,feet,markers) == count-1);
    world.dialogue_title = "Aviso"; assert(query(&world,feet,markers) == 0);
    world.dialogue_title = NULL; world.logic_quiz_open = true; assert(query(&world,feet,markers) == 0);
    world.logic_quiz_open = false; world.relay_panel_open = true; assert(query(&world,feet,markers) == 0);
    world.relay_panel_open = false; world.completed = true; assert(query(&world,feet,markers) == 0);
    /* A placa bloqueia a passagem, preservando aproximação e interação. */
    world_init(&world);
    Vector2 previous = {505,475};
    Vector2 blocked = world_constrain(&world,previous,(Vector2){505,445});
    assert(blocked.x == previous.x && blocked.y == previous.y);
    Vector2 beside = world_constrain(&world,previous,(Vector2){540,465});
    assert(beside.x == 540 && beside.y == 465);
    Vector2 approach = world_constrain(&world,previous,(Vector2){505,465});
    assert(approach.x == 505 && approach.y == 465);
    assert(world_interaction_view(&world,approach).state == HUD_INTERACTION_AVAILABLE);
    world_interact(&world,&approach,"Teste"); assert(world.logic_quiz_open);
    world.logic_quiz_open = false; world.location = WORLD_MAINTENANCE;
    HudInteraction hatch = world_interaction_view(&world,(Vector2){846,416});
    assert(hatch.state == HUD_INTERACTION_BLOCKED && strcmp(hatch.message,"Escotilha trancada") == 0);
    world.location = WORLD_DEPOT;
    count = query(&world,(Vector2){640,500},markers);
    Vector2 aloe_top = GetWorldToScreen2D((Vector2){640,222},world_camera(&world));
    bool found_aloe = false;
    for (int i = 0; i < count; i++)
        if (markers[i].position.x == aloe_top.x && markers[i].position.y == aloe_top.y) found_aloe = true;
    assert(found_aloe);
    world.location = WORLD_BRIDGE_CONTROL;
    count = query(&world,(Vector2){640,500},markers);
    Vector2 lever_top = GetWorldToScreen2D((Vector2){539,274},world_camera(&world));
    bool found_lever = false;
    for (int i = 0; i < count; i++)
        if (markers[i].position.x == lever_top.x && markers[i].position.y == lever_top.y) found_lever = true;
    assert(found_lever);
    Texture2D bridge = LoadTexture("assets/maps/bridge_control_interior.png");
    if (bridge.id) SetTextureFilter(bridge,TEXTURE_FILTER_POINT);
    capture(&ui,&world,(Vector2){640,500},bridge,1280,720,"bridge_controls");
    if (bridge.id) UnloadTexture(bridge);
    if (background.id) UnloadTexture(background);
    ui_theme_unload(&ui); CloseWindow();
    puts("PASS: brilhos, NPCs, salas/câmera, leituras, bosses vencidos, seta próxima, painéis, resoluções e desenho imutável.");
    return 0;
}
