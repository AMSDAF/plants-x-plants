/*
 * plant.c
 * Implementa a criação e a manipulação de plantas e equipes.
 * Não depende da raylib.
 */

#include "plant.h"
#include "plant_data.h"
#include <string.h>

Plant Plant_CreateFromSpecies(int species_id, int level) {
    const SpeciesData *base = PlantData_GetSpecies((SpeciesId)species_id);
    Plant plant;
    
    if (!base) {
        // Fallback seguro caso o ID seja inválido
        memset(&plant, 0, sizeof(Plant));
        return plant;
    }

    plant.id = base->id;
    strcpy(plant.name_pt, base->name_pt);
    plant.level = level;
    
    // Cálculo simples de atributos baseado no nível
    plant.max_hp = base->base_hp + (level - 1) * 10;
    plant.current_hp = plant.max_hp; // Começa com HP cheio
    plant.attack = base->base_attack + (level - 1) * 3;
    plant.defense = base->base_defense + (level - 1) * 2;
    
    // Copia os golpes do catálogo para a instância
    for (int i = 0; i < MAX_MOVES; i++) {
        memcpy(&plant.moves[i], &base->moves[i], sizeof(Move));
    }

    return plant;
}

int Plant_IsAlive(const Plant *plant) {
    return plant->current_hp > 0;
}

void Plant_TakeDamage(Plant *plant, int damage) {
    plant->current_hp -= damage;  // diminui o HP da planta pelo dano recebido
    if (plant->current_hp < 0) {
        plant->current_hp = 0;
    }
}

Plant* Team_GetActivePlant(Team *team) {
    if (team == NULL || team->active_index < 0 || team->active_index >= team->count) {
        return NULL;
    }
    return &team->plants[team->active_index];
}

int Team_HasLivingPlants(const Team *team) {
    if (team == NULL) return 0;
    for (int i = 0; i < team->count; i++) {
        if (Plant_IsAlive(&team->plants[i])) {
            return 1; // Tem pelo menos uma viva
        }
    }
    return 0; // Todas mortas
}
