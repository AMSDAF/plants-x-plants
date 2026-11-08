/*
 * battle.h
 * Define o estado da batalha e declara as funções de combate.
 * Não depende da raylib.
 */

#ifndef BATTLE_H
#define BATTLE_H

#include "plant.h"
#include <stdio.h>

// Estados possíveis da batalha
typedef enum {
    BATTLE_STATE_ONGOING = 0,
    BATTLE_STATE_PLAYER_WIN,
    BATTLE_STATE_ENEMY_WIN,
    BATTLE_STATE_FROZEN // para aguardar input ou visual/animação
} BattleStateEnum;

// Estrutura principal que guarda o contexto de uma luta
typedef struct {
    Team player_team;
    Team enemy_team;
    
    int current_turn;       // Quem joga: 0 para Jogador, 1 para Inimigo
    BattleStateEnum state;  // Estado atual (acontecendo, vitória, derrota)
    
    char last_message[128]; // Mensagem descritiva da última ação
} BattleState;

// Funções do motor de combate
void Battle_Init(BattleState *battle, Team player_team, Team enemy_team);
int Battle_ExecuteMove(BattleState *battle, int move_index);
void Battle_CheckConditions(BattleState *battle);

#endif