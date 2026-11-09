/*
 * game.h
 * Define o estado geral do jogo e declara as funções
 * de inicialização, atualização, desenho e encerramento.
 */

#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "menu_screen.h"
#include "trainer_select_screen.h"
#include "name_entry_screen.h"
#include "versus_screen.h"

#define GAME_SCREEN_WIDTH 1280
#define GAME_SCREEN_HEIGHT 720
#define GAME_TARGET_FPS 60
#define GAME_TITLE "Plants x Plants"

typedef enum { GAME_MENU, GAME_VERSUS_SELECT, GAME_TRAINER_SELECT, GAME_NAME_ENTRY, GAME_EXPLORATION } GameScreen;

/* Sessão de campanha somente em memória; o rascunho permanece na tela de nome. */
typedef struct {
    Trainer trainer;
    char player_name[PLAYER_NAME_CAPACITY];
    bool ready;
} CampaignSession;

/* Estado e recursos compartilhados entre as telas. */
typedef struct {
    bool exit_requested;
    MenuScreen menu;
    GameScreen screen;
    bool input_blocked;
    PlayerAssets player_assets;
    TrainerSelectAssets trainer_select_assets;
    TrainerSelectScreen trainer_select;
    NameEntryScreen name_entry;
    CampaignSession campaign;
    VersusAssets versus_assets;
    VersusScreen versus;
    VersusMode versus_mode;
    bool versus_mode_selected;
    Player player;
    World world;
} Game;

void game_init(Game *game);
void game_update(Game *game);
void game_draw(const Game *game);
void game_shutdown(Game *game);

#endif /* GAME_H */
