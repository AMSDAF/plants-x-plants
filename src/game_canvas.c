/* Mantém pixels físicos, enquadramento e entrada sem reduzir a cena para 720p. */
#include "game_canvas.h"
#include "game_view.h"
#include "rlgl.h"
#include <math.h>

bool game_canvas_resize(GameCanvas *canvas, int width, int height, int render_width, int render_height)
{
    if (width <= 0 || height <= 0 || render_width <= 0 || render_height <= 0) return false;
    float scale = fminf((float)width / GAME_SCREEN_WIDTH, (float)height / GAME_SCREEN_HEIGHT);
    Rectangle viewport = {floorf((width - GAME_SCREEN_WIDTH * scale) / 2),
                          floorf((height - GAME_SCREEN_HEIGHT * scale) / 2),
                          GAME_SCREEN_WIDTH * scale, GAME_SCREEN_HEIGHT * scale};
    int target_width = (int)fmaxf(1, roundf(viewport.width * render_width / width));
    int target_height = (int)fmaxf(1, roundf(viewport.height * render_height / height));
    if (canvas->target.texture.width != target_width || canvas->target.texture.height != target_height) {
        RenderTexture2D replacement = LoadRenderTexture(target_width, target_height);
        if (!IsRenderTextureValid(replacement)) {
            if (replacement.id) UnloadRenderTexture(replacement);
            return false;
        }
        SetTextureFilter(replacement.texture, TEXTURE_FILTER_POINT);
        if (canvas->target.id) UnloadRenderTexture(canvas->target);
        canvas->target = replacement;
    }
    canvas->viewport = viewport;
    return true;
}

void game_canvas_map_mouse(const GameCanvas *canvas)
{
    SetMouseOffset(-(int)canvas->viewport.x, -(int)canvas->viewport.y);
    SetMouseScale(GAME_SCREEN_WIDTH / canvas->viewport.width, GAME_SCREEN_HEIGHT / canvas->viewport.height);
}

void game_canvas_begin(const GameCanvas *canvas)
{
    BeginTextureMode(canvas->target);
    /* A projeção mantém a interface em 1280 x 720, mas rasteriza no alvo físico. */
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT, 0, 0, 1);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}

void game_canvas_present(const GameCanvas *canvas)
{
    EndTextureMode();
    ClearBackground(BLACK);
    Rectangle source = {0, 0, canvas->target.texture.width, -canvas->target.texture.height};
    DrawTexturePro(canvas->target.texture, source, canvas->viewport, (Vector2){0}, 0, WHITE);
}

void game_canvas_begin_scissor(Rectangle bounds)
{
    float scale_x = (float)rlGetFramebufferWidth() / GAME_SCREEN_WIDTH;
    float scale_y = (float)rlGetFramebufferHeight() / GAME_SCREEN_HEIGHT;
    int left = (int)floorf(bounds.x * scale_x);
    int top = (int)floorf(bounds.y * scale_y);
    int right = (int)ceilf((bounds.x + bounds.width) * scale_x);
    int bottom = (int)ceilf((bounds.y + bounds.height) * scale_y);
    BeginScissorMode(left, top, right - left, bottom - top);
}

void game_canvas_unload(GameCanvas *canvas)
{
    if (canvas->target.id) UnloadRenderTexture(canvas->target);
    *canvas = (GameCanvas){0};
}
