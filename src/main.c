/*
 * main.c
 * Ponto de entrada do programa.
 * Abre a janela, executa o loop principal e encerra o jogo.
 */

#include "game.h"

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

    SetTargetFPS(GAME_TARGET_FPS);

    /* O fechamento por ESC sera tratado pelo estado do jogo. */
    SetExitKey(KEY_NULL);

    Game game;
    game_init(&game);

    while (!WindowShouldClose() && !game.exit_requested) {
        game_update(&game);

        BeginDrawing();
        game_draw(&game);
        EndDrawing();
    }

    game_shutdown(&game);
    CloseWindow();

    return 0;
}