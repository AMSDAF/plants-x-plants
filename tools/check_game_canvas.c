/* Verifica resolução física, recorte, entrada e comparação do quadrante com OpenGL. */
#include "game_canvas.h"
#include "game_view.h"
#include "world.h"
#include "courtyard_detail_data.h"
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static Image read_canvas(const GameCanvas *canvas)
{
    Image image = LoadImageFromTexture(canvas->target.texture);
    ImageFlipVertical(&image);
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    return image;
}

static bool same_color(Color first, Color second)
{
    return first.r == second.r && first.g == second.g && first.b == second.b && first.a == second.a;
}

static void check_sizes(GameCanvas *canvas)
{
    const int sizes[][6] = {
        {1280, 720, 1280, 720, 1280, 720},
        {1920, 1080, 1920, 1080, 1920, 1080},
        {1920, 1200, 1920, 1200, 1920, 1080},
        {2560, 1440, 2560, 1440, 2560, 1440},
        {1024, 768, 1024, 768, 1024, 576},
        {1280, 720, 2560, 1440, 2560, 1440}
    };
    for (unsigned int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        const int *size = sizes[i];
        assert(game_canvas_resize(canvas, size[0], size[1], size[2], size[3]));
        assert(canvas->target.texture.width == size[4] && canvas->target.texture.height == size[5]);
        assert(fabsf(canvas->viewport.width / canvas->viewport.height - 16.0f/9) < 0.001f);
        unsigned int identity = canvas->target.id;
        assert(game_canvas_resize(canvas, size[0], size[1], size[2], size[3]));
        assert(canvas->target.id == identity);
    }
    assert(!game_canvas_resize(canvas, 0, 720, 0, 720));
    assert(IsRenderTextureValid(canvas->target));
    assert(game_canvas_resize(canvas, 1920, 1200, 1920, 1200));
    assert(canvas->viewport.y == 60);
    game_canvas_map_mouse(canvas);
    SetMousePosition(960, 600);
    Vector2 mouse = GetMousePosition();
    assert(fabsf(mouse.x-640) < 0.01f && fabsf(mouse.y-360) < 0.01f);
    SetMouseOffset(0, 0);
    SetMouseScale(1, 1);
}

static void check_native_pixels(GameCanvas *canvas)
{
    assert(game_canvas_resize(canvas, 1920, 1080, 1920, 1080));
    Image source = GenImageChecked(1920, 1080, 1, 1, WHITE, BLACK);
    Texture2D checker = LoadTextureFromImage(source);
    UnloadImage(source);
    BeginDrawing();
    game_canvas_begin(canvas);
    ClearBackground(BLACK);
    DrawTexturePro(checker, (Rectangle){0, 0, 1920, 1080},
                   (Rectangle){0, 0, 1280, 720}, (Vector2){0}, 0, WHITE);
    game_canvas_present(canvas);
    EndDrawing();
    Image result = read_canvas(canvas);
    for (int x = 300; x < 330; x++) {
        Color first = GetImageColor(result, x, 200);
        Color second = GetImageColor(result, x+1, 200);
        assert(!same_color(first, second));
        assert(first.r == 0 || first.r == 255);
    }
    assert(ExportImage(result, "build/courtyard_detail_preview/native_pixel_check.png"));
    UnloadImage(result);
    UnloadTexture(checker);

    BeginDrawing();
    game_canvas_begin(canvas);
    ClearBackground(BLACK);
    BeginMode2D((Camera2D){.target={20, 30}, .zoom=1});
    DrawRectangle(120, 230, 10, 10, GREEN);
    EndMode2D();
    game_canvas_begin_scissor((Rectangle){100, 100, 20, 20});
    DrawRectangle(0, 0, 1280, 720, RED);
    EndScissorMode();
    game_canvas_present(canvas);
    EndDrawing();
    result = read_canvas(canvas);
    assert(same_color(GetImageColor(result, 151, 151), RED));
    assert(same_color(GetImageColor(result, 149, 149), BLACK));
    assert(same_color(GetImageColor(result, 181, 181), BLACK));
    assert(same_color(GetImageColor(result, 153, 303), GREEN));
    UnloadImage(result);
}

static Image render_world(GameCanvas *canvas, const World *world, const WorldAssets *assets)
{
    World before = *world;
    BeginDrawing();
    game_canvas_begin(canvas);
    ClearBackground(BLACK);
    BeginMode2D(world_camera(world));
    world_draw(world, assets, (Vector2){576, 799});
    EndMode2D();
    game_canvas_present(canvas);
    EndDrawing();
    assert(memcmp(&before, world, sizeof(before)) == 0);
    return read_canvas(canvas);
}

static int border_errors(GameCanvas *canvas, Texture2D texture, Image source)
{
    BeginDrawing();
    game_canvas_begin(canvas);
    ClearBackground(BLACK);
    /* A borda termina em meio pixel físico, reproduzindo a antiga linha horizontal. */
    DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                   (Rectangle){0, 0, 900, 535}, (Vector2){0}, 0, WHITE);
    game_canvas_present(canvas);
    EndDrawing();
    Image actual = read_canvas(canvas);
    int errors = 0;
    for (int x = 64; x < 1300; x += 137) {
        float position = (x + 0.5f) * source.width / 1350.0f - 0.5f;
        int left = (int)floorf(position);
        float fraction = position - left;
        Color first = GetImageColor(source, left, source.height-1);
        Color second = GetImageColor(source, left+1, source.height-1);
        Color pixel = GetImageColor(actual, x, 802);
        if (fabsf(pixel.r - (first.r*(1-fraction)+second.r*fraction)) > 2 ||
            fabsf(pixel.g - (first.g*(1-fraction)+second.g*fraction)) > 2 ||
            fabsf(pixel.b - (first.b*(1-fraction)+second.b*fraction)) > 2) errors++;
    }
    UnloadImage(actual);
    return errors;
}

static void check_detail(GameCanvas *canvas)
{
    WorldAssets assets;
    world_assets_load(&assets);
    assert(assets.courtyard.id && assets.courtyard_detail.id);
    assert(assets.courtyard_detail.width == COURTYARD_DETAIL_WIDTH);
    assert(assets.courtyard_detail.height == COURTYARD_DETAIL_HEIGHT);
    World world;
    world_init(&world);
    world.detail_enabled = false;
    Image original = render_world(canvas, &world, &assets);
    assert(ExportImage(original, "build/courtyard_detail_preview/original_1080p.png"));
    world.detail_enabled = true;
    Image detail = render_world(canvas, &world, &assets);
    assert(ExportImage(detail, "build/courtyard_detail_preview/detail_1080p.png"));
    size_t size = (size_t)detail.width * detail.height * 4;
    assert(memcmp(original.data, detail.data, size) != 0);
    const Vector2 cameras[] = {{0, 0}, {392, 0}, {0, 221}, {392, 221}};
    const Vector2 samples[] = {{200, 200}, {1250, 200}, {200, 750}, {1400, 750}};
    for (int corner = 0; corner < 4; corner++) {
        World sample = world;
        sample.camera_position = cameras[corner];
        sample.detail_enabled = false;
        Image old = render_world(canvas, &sample, &assets);
        sample.detail_enabled = true;
        Image current = render_world(canvas, &sample, &assets);
        assert(ExportImage(current, TextFormat("build/courtyard_detail_preview/corner_%d_1080p.png", corner)));
        int center_x = (int)((samples[corner].x-cameras[corner].x)*1.5f);
        int center_y = (int)((samples[corner].y-cameras[corner].y)*1.5f);
        int differences = 0;
        for (int y = center_y-12; y <= center_y+12; y++)
            for (int x = center_x-12; x <= center_x+12; x++)
                if (!same_color(GetImageColor(old, x, y), GetImageColor(current, x, y))) differences++;
        assert(differences > 0);
        UnloadImage(old);
        UnloadImage(current);
    }
    Image source = LoadImage("assets/maps/courtyard_detail/courtyard_complete.png");
    assert(border_errors(canvas, assets.courtyard_detail, source) == 0);
    SetTextureWrap(assets.courtyard_detail, TEXTURE_WRAP_REPEAT);
    assert(border_errors(canvas, assets.courtyard_detail, source) > 0);
    SetTextureWrap(assets.courtyard_detail, TEXTURE_WRAP_CLAMP);
    UnloadImage(source);
    WorldAssets missing = assets;
    missing.courtyard_detail.id = 0;
    Image fallback = render_world(canvas, &world, &missing);
    assert(memcmp(original.data, fallback.data, size) == 0);
    UnloadImage(fallback);
    /* A ponte continua dinâmica e é desenhada sobre a textura completa. */
    world.north_channel = true;
    world.drain_open = false;
    assert(world_bridge_open(&world));
    Image bridge = render_world(canvas, &world, &assets);
    assert(ExportImage(bridge, "build/courtyard_detail_preview/bridge_1080p.png"));
    assert(memcmp(bridge.data, detail.data, size) != 0);
    UnloadImage(bridge);
    UnloadImage(original);
    UnloadImage(detail);
    world_assets_unload(&assets);
    assert(!assets.courtyard_detail.id);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Verificacao do canvas e quadrante");
    assert(IsWindowReady());
    assert(DirectoryExists("build/courtyard_detail_preview") ||
           MakeDirectory("build/courtyard_detail_preview") == 0);
    GameCanvas canvas = {0};
    check_sizes(&canvas);
    check_native_pixels(&canvas);
    check_detail(&canvas);
    game_canvas_unload(&canvas);
    assert(!canvas.target.id);
    CloseWindow();
    puts("PASS: resolucao nativa, HiDPI, mouse, recorte, camera, quatro quadrantes, bordas, fallback e ponte.");
    return 0;
}
