/*
 * game_audio.h
 * Observa as mudanças de estado do jogo e escolhe efeitos e músicas.
 * Apenas lê o estado; regras de combate e telas permanecem intactas.
 */
#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <stdbool.h>
#include "game.h"

/* Cópia dos campos observados antes da atualização de um quadro. */
typedef struct {
    GameScreen screen;
    int result_cursor;
    unsigned int feedback_sequence;
    Trainer trainer_selected;
    int name_count;
    int name_focus;
    bool name_error;
    IntroStage intro_stage;
    int intro_cursor;
    int intro_count;
    WorldLocation location;
    Vector2 feet;
    bool dialogue_open;
    int dialogue_page;
    int dialogue_characters;
    unsigned int relay_mask;
    int relay_step;
    bool relay_rejected;
    bool relay_panel_open;
    bool logic_quiz_open;
    unsigned int world_controls;
    bool completed;
    int logic_selected;
    int logic_focus;
    int logic_dropdown;
    int logic_option;
    unsigned int logic_confirmed;
    unsigned int logic_rejected;
    BattlePendingAction pending_action;
    BattleMenu battle_menu;
    int battle_cursor;
    int setup_side;
    int setup_cursor;
    int setup_slot;
    int setup_plant;
    int setup_move;
    LogicFormula setup_draft;
    BattleStateEnum battle_state;
    int round_step;
    int round_message_index;
    int team_count[2];
    int active_index[2];
    int hp[2][MAX_TEAM_SIZE];
} GameAudioSnapshot;

GameAudioSnapshot game_audio_capture(const Game *game);
void game_audio_react(Game *game, const GameAudioSnapshot *before);

#endif /* GAME_AUDIO_H */
