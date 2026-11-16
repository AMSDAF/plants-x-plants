/*
 * main.c
 * Ponto de entrada do programa.
 * Abre a janela, executa o loop principal e encerra o jogo.
 */

#include "game.h"

#include "raylib.h"
#include <math.h>

static Rectangle game_viewport(void)
{
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    float scale = fminf(fmaxf(1.0f, width) / GAME_SCREEN_WIDTH,
                        fmaxf(1.0f, height) / GAME_SCREEN_HEIGHT);
    float view_width = GAME_SCREEN_WIDTH * scale;
    float view_height = GAME_SCREEN_HEIGHT * scale;
    return (Rectangle){floorf((width - view_width) / 2.0f),
                       floorf((height - view_height) / 2.0f), view_width, view_height};
}

int main(void)
{
    InitWindow(
        GAME_SCREEN_WIDTH,
        GAME_SCREEN_HEIGHT,
        GAME_TITLE
    );

    if (!IsWindowReady()) {
        return 1;
    }

    SetTargetFPS(GAME_TARGET_FPS);

    /* O fechamento por ESC sera tratado pelo estado do jogo. */
    SetExitKey(KEY_NULL);

    RenderTexture2D canvas = LoadRenderTexture(GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    if (!IsRenderTextureValid(canvas)) {
        CloseWindow();
        return 1;
    }
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    Rectangle canvas_source = {0, 0, GAME_SCREEN_WIDTH, -GAME_SCREEN_HEIGHT};

    Game game;
    game_init(&game);

    while (!WindowShouldClose() && !game.exit_requested) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        Rectangle viewport = game_viewport();
        float mouse_scale = GAME_SCREEN_WIDTH / viewport.width;
        SetMouseOffset(-(int)viewport.x, -(int)viewport.y);
        SetMouseScale(mouse_scale, mouse_scale);
        game_update(&game);

        BeginDrawing();
        BeginTextureMode(canvas);
        game_draw(&game);
        EndTextureMode();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.texture, canvas_source, viewport, (Vector2){0}, 0, WHITE);
        EndDrawing();
    }

    game_shutdown(&game);
    UnloadRenderTexture(canvas);
    CloseWindow();

    return 0;
}
