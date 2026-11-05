/*
 * game.c
 * Controla o fluxo geral do jogo.
 * Coordena a inicializacao, atualizacao e desenho do menu.
 */

#include "game.h"

void game_init(Game *game)
{
    *game = (Game){0};
    menu_screen_init(&game->menu);
}

void game_update(Game *game)
{
    game->exit_requested = menu_screen_update(&game->menu);
}

void game_draw(const Game *game)
{
    menu_screen_draw(&game->menu);
}

void game_shutdown(Game *game)
{
    menu_screen_shutdown(&game->menu);
}