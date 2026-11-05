/*
 * game.h
 * Define o estado geral do jogo e declara as funções
 * de inicialização, atualização, desenho e encerramento.
 */

#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "menu_screen.h"

#define GAME_SCREEN_WIDTH 1280
#define GAME_SCREEN_HEIGHT 720
#define GAME_TARGET_FPS 60
#define GAME_TITLE "Plants x Plants"

/* Estado geral que será expandido nas próximas tarefas. */
typedef struct {
    bool exit_requested;
    MenuScreen menu;
} Game;

void game_init(Game *game);
void game_update(Game *game);
void game_draw(const Game *game);
void game_shutdown(Game *game);

#endif /* GAME_H */