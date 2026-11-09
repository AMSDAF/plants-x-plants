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

int Team_SelectSpecies(Team *team, int index, int species_id)
{
    if (index < 0 || index > team->count || index >= MAX_TEAM_SIZE ||
        !PlantData_GetSpecies((SpeciesId)species_id))
        return 0;
    for (int i = 0; i < team->count; i++) {
        if (i != index && team->plants[i].id == species_id) {
            if (index == team->count) return 0;
            Plant previous = team->plants[index];
            team->plants[index] = team->plants[i];
            team->plants[i] = previous;
            return 1;
        }
    }
    team->plants[index] = Plant_CreateFromSpecies(species_id, 1);
    if (index == team->count) team->count++;
    return 1;
}

int Team_RemoveReserve(Team *team, int index)
{
    if (team->count <= MIN_VERSUS_TEAM_SIZE || index <= 0 || index >= team->count)
        return 0;
    for (int i = index; i + 1 < team->count; i++)
        team->plants[i] = team->plants[i + 1];
    team->count--;
    return 1;
}

void Team_Restore(Team *team)
{
    team->active_index = 0;
    for (int i = 0; i < team->count; i++)
        team->plants[i].current_hp = team->plants[i].max_hp;
}
