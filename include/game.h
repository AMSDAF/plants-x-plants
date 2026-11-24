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
#include "result_screen.h"
#include "leaderboard_screen.h"

#define GAME_TARGET_FPS 60
#define GAME_TITLE "Plants x Plants"

typedef enum {
    GAME_MENU, GAME_LEADERBOARD, GAME_TRAINER_SELECT, GAME_NAME_ENTRY,
    GAME_EXPLORATION, GAME_BATTLE, GAME_INTRO, GAME_ENDING, GAME_DEFEAT
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

/* Medição da partida: cronômetro da carta ao desfecho e dano acumulado. */
typedef struct {
    bool active;
    double elapsed;
    long long damage_taken;
} RunStats;

/* Estado e recursos compartilhados entre as telas. */
typedef struct {
    bool exit_requested;
    UiTheme ui;
    MenuScreen menu;
    LeaderboardScreen leaderboard;
    GameScreen screen;
    bool input_blocked;
    bool exit_confirm_open;
    int exit_confirm_cursor;
    RunStats run;
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
    ResultScreen result;
} Game;

void game_init(Game *game);
void game_update(Game *game);
void game_draw(const Game *game);
void game_shutdown(Game *game);

#endif /* GAME_H */
