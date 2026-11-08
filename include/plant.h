/*
 * plant.h
 * Define as estruturas de golpes, plantas e equipes.
 * Compartilhado pelas regras de combate e pela interface.
 * Não depende da raylib.
 */
#ifndef PLANT_H
#define PLANT_H

#define MAX_MOVES 2
#define MAX_TEAM_SIZE 4

// Estrutura de um golpe
typedef struct {
    char name[32];
    int power;
    int accuracy;
} Move;

// Estrutura de uma planta individual (instância em batalha)
typedef struct {
    int id;                  // Identificador da espécie
    char name_pt[32];        // Nome da planta em português
    
    int level;               // Nível atual da planta
    
    // Atributos de Combate
    int max_hp;
    int current_hp;
    int attack;
    int defense;
    
    Move moves[MAX_MOVES];   // Golpes que esta planta conhece
} Plant;

// Estrutura de uma equipe de plantas
typedef struct {
    Plant plants[MAX_TEAM_SIZE];
    int count;               // Quantas plantas tem na equipe atualmente
    int active_index;        // Índice da planta que está lutando agora
} Team;

// Funções utilitárias da planta e da equipe
int Plant_IsAlive(const Plant *plant);
void Plant_TakeDamage(Plant *plant, int damage);

Plant Plant_CreateFromSpecies(int species_id, int level);

#endif