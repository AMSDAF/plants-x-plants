/*
 * game.c
 * Controla as telas e o fluxo geral do jogo.
 * Nesta etapa, apresenta uma tela provisória.
 */

#include "game.h"

#include "raylib.h"

static void draw_centered_text(
    const char *text,
    int y,
    int font_size,
    Color color
);

void game_init(Game *game)
{
    *game = (Game){0};
}

void game_update(Game *game)
{
    if (IsKeyPressed(KEY_ESCAPE)) {
        game->exit_requested = true;
    }
}

void game_draw(const Game *game)
{
    /* O desenho ainda não precisa consultar o estado do jogo. */
    (void)game;

    ClearBackground((Color){24, 32, 28, 255});

    draw_centered_text(
        GAME_TITLE,
        240,
        64,
        (Color){135, 220, 120, 255}
    );

    draw_centered_text(
        "Prototipo em desenvolvimento",
        330,
        24,
        RAYWHITE
    );

    draw_centered_text(
        "Pressione ESC para sair",
        410,
        20,
        LIGHTGRAY
    );
}

void game_shutdown(Game *game)
{
    /*
     * Ainda nao existem recursos para liberar.
     * Fontes, texturas e sons serao liberados aqui quando adicionados.
     */
    (void)game;
}

static void draw_centered_text(
    const char *text,
    int y,
    int font_size,
    Color color
)
{
    int text_width = MeasureText(text, font_size);
    int x = (GetScreenWidth() - text_width) / 2;

    DrawText(text, x, y, font_size, color);
}