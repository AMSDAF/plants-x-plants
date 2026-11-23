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
#include "battle_screen.h"
#include "game_view.h"
#include "ui_theme.h"
#include "intro_screen.h"
#include "logic_setup_screen.h"
#include "relay_ui.h"
#include "audio.h"

#define GAME_TARGET_FPS 60
#define GAME_TITLE "Plants x Plants"

typedef enum {
    GAME_MENU, GAME_TRAINER_SELECT, GAME_NAME_ENTRY,
    GAME_EXPLORATION, GAME_BATTLE, GAME_INTRO
} GameScreen;

/* Sessão de campanha somente em memória; o rascunho permanece na tela de nome. */
typedef struct {
    Trainer trainer;
    char player_name[PLAYER_NAME_CAPACITY];
    bool has_team;
    bool has_formula;
    Team team;
    LogicFormula formulas[MAX_TEAM_SIZE];
    int boosted_move_indices[MAX_TEAM_SIZE];
} CampaignSession;

/* Estado e recursos compartilhados entre as telas. */
typedef struct {
    bool exit_requested;
    UiTheme ui;
    MenuScreen menu;
    GameScreen screen;
    bool input_blocked;
    PlayerAssets player_assets;
    TrainerSelectAssets trainer_select_assets;
    TrainerSelectScreen trainer_select;
    NameEntryScreen name_entry;
    IntroScreen intro;
    IntroAssets intro_assets;
    CampaignSession campaign;
    LogicSetupScreen logic_setup;
    WorldEncounter encounter;
    BattleScreen battle_screen;
    BattleScreenAssets battle_assets;
    Player player;
    World world;
    WorldAssets world_assets;
    RelayUiAssets relay_assets;
    GameAudio audio;
} Game;

void game_init(Game *game);
void game_update(Game *game);
void game_draw(const Game *game);
void game_shutdown(Game *game);

#endif /* GAME_H */
