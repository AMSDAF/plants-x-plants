/*
 * battle.c
 * Implementa as regras do combate: ataques, trocas,
 * controle de turnos e verificação do vencedor.
 * Não depende da raylib.
 */

#include "battle.h"
#include <stdlib.h> // Para rand() se formos usar variação/crítico no futuro
#include <string.h>
#include <stdio.h>  // Para snprintf

void Battle_Init(BattleState *battle, Team player_team, Team enemy_team) {
    battle->player_team = player_team;
    battle->enemy_team = enemy_team;
    battle->current_turn = 0; // 0 começa com o Jogador
    battle->state = BATTLE_STATE_ONGOING;
    strcpy(battle->last_message, "A batalha começou!");
}

int Battle_ExecuteMove(BattleState *battle, int move_index) {
    // Valida se a batalha ainda está acontecendo
    if (battle->state != BATTLE_STATE_ONGOING) {
        return 0;
    }

    Team *attacker_team;
    Team *defender_team;

    // Define quem ataca e quem defende baseado no turno atual
    if (battle->current_turn == 0) {
        attacker_team = &battle->player_team;
        defender_team = &battle->enemy_team;
    } else {
        attacker_team = &battle->enemy_team;
        defender_team = &battle->player_team;
    }

    Plant *attacker = Team_GetActivePlant(attacker_team);
    Plant *defender = Team_GetActivePlant(defender_team);

    if (!attacker || !defender || !Plant_IsAlive(attacker) || !Plant_IsAlive(defender)) {
        return 0;
    }

    // Valida o índice do golpe
    if (move_index < 0 || move_index >= MAX_MOVES) {
        return 0;
    }

    Move *move = &attacker->moves[move_index];

    // Fórmula de dano simples (Poder do golpe + Ataque do atacante - Defesa do defensor) provavelmente sera mudada para algo proximo a do pkm
    // Garantindo pelo menos 1 de dano
    int raw_damage = move->power + (attacker->attack - defender->defense / 2);
    if (raw_damage < 5) raw_damage = 5; // Dano mínimo garantido

    // Aplica o dano na planta defensora
    Plant_TakeDamage(defender, raw_damage);

    // Atualiza a mensagem da batalha
    // (Se preferir usar sprintf/snprintf, lembre-se de incluir <stdio.h>)
    snprintf(battle->last_message, sizeof(battle->last_message), 
             "%s usou %s e causou %d de dano!", 
             attacker->name_pt, move->name, raw_damage);

    // Verifica condições de vitória/derrota após o ataque
    Battle_CheckConditions(battle);

    // Se a batalha continuar, alterna o turno (0 vira 1, 1 vira 0)
    if (battle->state == BATTLE_STATE_ONGOING) {
        battle->current_turn = 1 - battle->current_turn;
    }

    return 1; // Ação executada com sucesso
}

void Battle_CheckConditions(BattleState *battle) {
    // Verifica se o time inimigo perdeu todas as plantas
    if (!Team_HasLivingPlants(&battle->enemy_team)) {
        battle->state = BATTLE_STATE_PLAYER_WIN;
        strcpy(battle->last_message, "Vitória! O time inimigo foi derrotado.");
        return;
    }

    // Verifica se o time do jogador perdeu todas as plantas
    if (!Team_HasLivingPlants(&battle->player_team)) {
        battle->state = BATTLE_STATE_ENEMY_WIN;
        strcpy(battle->last_message, "Derrota! Suas plantas desmaiaram.");
        return;
    }
}