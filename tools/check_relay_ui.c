/* Verifica a central industrial, entradas reais, estados e capturas sem carregar a campanha inteira. */
#include "relay_ui.h"
#include "game_canvas.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void event(unsigned int type, int a, int b)
{
    PlayAutomationEvent((AutomationEvent){.type=type,.params={a,b,0,0}});
}

static void tick(World *world)
{
    world_update_relay_panel(world,"Teste");
    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
}

static void key(World *world, int code)
{
    event(2,code,0); tick(world);
    event(1,code,0); tick(world);
}

static void click(World *world, int x, int y)
{
    event(7,x,y); event(6,0,0); tick(world);
    event(5,0,0); tick(world);
}

static void capture(const UiTheme *ui, SelectionUiAssets *fonts, const RelayUiAssets *assets,
                     const World *world, Texture2D background, int width, int height, const char *name)
{
    GameCanvas canvas = {0};
    assert(game_canvas_resize(&canvas,width,height,width,height));
    selection_ui_update_fonts(fonts,canvas.target.texture.width/1280.0f);
    World before = *world;
    BeginDrawing(); game_canvas_begin(&canvas);
    ClearBackground((Color){23,47,35,255});
    if (background.id) DrawTexturePro(background,(Rectangle){0,0,background.width,background.height},
        (Rectangle){0,0,1280,720},(Vector2){0},0,WHITE);
    relay_ui_draw(ui,fonts,assets,world);
    game_canvas_present(&canvas); EndDrawing();
    assert(memcmp(&before,world,sizeof(before)) == 0);
    Image image = LoadImageFromTexture(canvas.target.texture);
    ImageFlipVertical(&image);
    assert(ExportImage(image,TextFormat("build/relay_ui_preview/%s.png",name)));
    UnloadImage(image); game_canvas_unload(&canvas);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN); SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280,720,"Verificação da central industrial");
    SetExitKey(KEY_NULL); SetTargetFPS(120); assert(IsWindowReady());
    assert(DirectoryExists("build/relay_ui_preview") || MakeDirectory("build/relay_ui_preview") == 0);
    UiTheme ui; ui_theme_load(&ui);
    SelectionUiAssets fonts; selection_ui_assets_load(&fonts);
    RelayUiAssets assets; relay_ui_load(&assets);
    for (int i = 0; i < RELAY_ART_COUNT; i++) assert(assets.textures[i].id);
    Texture2D background = LoadTexture("assets/maps/nursery_interior.png");
    World world; world_init(&world); world.location = WORLD_NURSERY; world.guardian_defeated = true;
    world.relay_panel_open = true;
    capture(&ui,&fonts,&assets,&world,background,1280,720,"stage_one");
    key(&world,KEY_THREE); assert(world.relay_mask == 4);
    key(&world,KEY_ENTER); assert(world.relay_step == 0 && world.relay_rejected);
    click(&world,335,468); assert(world.relay_mask == 5);
    click(&world,1060,653); assert(world.relay_step == 1 && world.relay_mask == 0);
    capture(&ui,&fonts,&assets,&world,background,1280,720,"stage_two");
    key(&world,KEY_RIGHT); key(&world,KEY_SPACE);
    assert(world.relay_cursor == 1 && world.relay_mask == 2);
    key(&world,KEY_ENTER); assert(world.relay_step == 1 && world.relay_rejected);
    click(&world,335,468); key(&world,KEY_THREE); key(&world,KEY_ENTER);
    assert(world.relay_step == 2 && world.relay_mask == 0);
    key(&world,KEY_THREE); key(&world,KEY_ENTER);
    assert(world.relay_rejected && !world.completed);
    capture(&ui,&fonts,&assets,&world,background,1280,720,"stage_three_error");
    key(&world,KEY_THREE); key(&world,KEY_ONE);
    capture(&ui,&fonts,&assets,&world,background,1920,1080,"stage_three_1080p");
    capture(&ui,&fonts,&assets,&world,background,1024,768,"stage_three_letterbox");
    RelayUiAssets empty = {0};
    capture(&ui,&fonts,&empty,&world,background,1280,720,"missing_assets");
    unsigned int attempts = world.relay_attempts;
    click(&world,200,653); assert(!world.relay_panel_open && world.relay_mask == 1);
    Vector2 feet = {782,315};
    assert(world_interact(&world,&feet,"Teste") == WORLD_ENCOUNTER_NONE);
    assert(world.relay_panel_open && world.relay_step == 2 && world.relay_attempts == attempts);
    click(&world,1060,653);
    assert(world.completed && !world.relay_panel_open && world.dialogue_title);
    World ended = world;
    key(&world,KEY_ONE);
    assert(memcmp(&ended,&world,sizeof(world)) == 0);
    relay_ui_unload(&assets);
    for (int i = 0; i < RELAY_ART_COUNT; i++) assert(!assets.textures[i].id);
    UnloadTexture(background); selection_ui_assets_unload(&fonts); ui_theme_unload(&ui); CloseWindow();
    puts("PASS: central industrial, 3 etapas, mouse/teclado, rejeição, progresso, conclusão, 720p/1080p/4:3, fallback e desenho imutável.");
    return 0;
}
