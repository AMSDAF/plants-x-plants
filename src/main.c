/*
 * main.c
 * Ponto de entrada do programa.
 * Abre a janela, executa o loop principal e encerra o jogo.
 */

#include "game.h"
#include "game_canvas.h"
#include "input_map.h"

#include "raylib.h"

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

    /* Sem dispositivo de áudio, o jogo segue em silêncio. */
    InitAudioDevice();

    SetTargetFPS(GAME_TARGET_FPS);

    /* O fechamento por ESC sera tratado pelo estado do jogo. */
    SetExitKey(KEY_NULL);

    GameCanvas canvas = {0};
    if (!game_canvas_resize(&canvas, GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight())) {
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    Game game;
    game_init(&game);
    int result = 0;

    while (!WindowShouldClose() && !game.exit_requested) {
        if (IsKeyPressed(KEY_F11) || input_fullscreen_toggle_pressed()) ToggleBorderlessWindowed();
        if (!game_canvas_resize(&canvas, GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight())) {
            TraceLog(LOG_ERROR, "Nao foi possivel preparar a resolucao de exibicao.");
            result = 1;
            break;
        }
        game_canvas_map_mouse(&canvas);
        game_update(&game);

        BeginDrawing();
        game_canvas_begin(&canvas);
        game_draw(&game);
        game_canvas_present(&canvas);
        EndDrawing();
    }

    game_shutdown(&game);
    game_canvas_unload(&canvas);
    CloseAudioDevice();
    CloseWindow();

    return result;
}
