/*
 * plant.c
 * Implementa a criação e a manipulação de plantas e equipes.
 * Não depende da raylib.
 */

#include "plant.h"
#include "plant_data.h"
#include <math.h>
#include <string.h>

Plant Plant_CreateFromSpecies(int species_id) {
    const SpeciesData *base = PlantData_GetSpecies((SpeciesId)species_id);
    Plant plant = {0};
    
    if (!base) {
        // Fallback seguro caso o ID seja inválido
        memset(&plant, 0, sizeof(Plant));
        return plant;
    }

    plant.id = base->id;
    strcpy(plant.name_pt, base->name_pt);
    
    // Usa diretamente os atributos definidos no catálogo da espécie
    plant.max_hp = base->base_hp;
    plant.current_hp = plant.max_hp; // Começa com HP cheio
    plant.attack = base->base_attack;
    plant.defense = base->base_defense;
    plant.speed = base->base_speed;
    plant.base_attack = base->base_attack;
    plant.base_defense = base->base_defense;
    plant.base_speed = base->base_speed;
    
    // Copia os golpes do catálogo para a instância
    for (int i = 0; i < MAX_MOVES; i++) {
        memcpy(&plant.moves[i], &base->moves[i], sizeof(Move));
    }

    if (plant.id == SPECIES_CLOVER) {
        Plant_AddStatus(&plant, PLANT_STATUS_LUCK, 0,
                        PLANT_STATUS_LUCK_CRITICAL_MULTIPLIER);
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

int Plant_HasStatus(const Plant *plant, PlantStatusType type)
{
    if (!plant || type < 0 || type >= PLANT_STATUS_COUNT ||
        plant->status_count < 0 || plant->status_count > PLANT_STATUS_COUNT) return 0;
    for (int i = 0; i < plant->status_count; i++)
        if (plant->statuses[i].type == type) return 1;
    return 0;
}

int Plant_GetStatusValue(const Plant *plant, PlantStatusType type, float *value)
{
    if (!plant || type < 0 || type >= PLANT_STATUS_COUNT ||
        plant->status_count < 0 || plant->status_count > PLANT_STATUS_COUNT) return 0;
    for (int i = 0; i < plant->status_count; i++) {
        if (plant->statuses[i].type != type) continue;
        if (value) *value = plant->statuses[i].value;
        return 1;
    }
    return 0;
}

int Plant_AddStatus(Plant *plant, PlantStatusType type, int remaining_turns, float value)
{
    if (!plant || type < 0 || type >= PLANT_STATUS_COUNT || remaining_turns < 0 ||
        !isfinite(value) || value <= 0 || plant->status_count < 0 ||
        plant->status_count >= PLANT_STATUS_COUNT || Plant_HasStatus(plant, type))
        return 0;

    plant->statuses[plant->status_count++] = (PlantStatus){type, remaining_turns, value};
    return 1;
}

int Plant_RemoveStatus(Plant *plant, PlantStatusType type)
{
    if (!plant || type < 0 || type >= PLANT_STATUS_COUNT ||
        plant->status_count < 0 || plant->status_count > PLANT_STATUS_COUNT) return 0;
    for (int i = 0; i < plant->status_count; i++) {
        if (plant->statuses[i].type != type) continue;
        for (int j = i; j + 1 < plant->status_count; j++)
            plant->statuses[j] = plant->statuses[j + 1];
        plant->status_count--;
        plant->statuses[plant->status_count] = (PlantStatus){0};
        return 1;
    }
    return 0;
}

void Plant_TickStatuses(Plant *plant)
{
    if (!plant || plant->status_count < 0 || plant->status_count > PLANT_STATUS_COUNT) return;
    for (int i = 0; i < plant->status_count;) {
        PlantStatus *status = &plant->statuses[i];
        if (status->remaining_turns == 0) {
            i++;
            continue;
        }
        status->remaining_turns--;
        if (status->remaining_turns == 0) {
            Plant_RemoveStatus(plant, status->type);
        } else {
            i++;
        }
    }
}

void Plant_ClearStatuses(Plant *plant)
{
    if (!plant) return;
    memset(plant->statuses, 0, sizeof(plant->statuses));
    plant->status_count = 0;
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
    team->plants[index] = Plant_CreateFromSpecies(species_id);
    if (index == team->count) team->count++;
    return 1;
}

int Team_RemoveReserve(Team *team, int index)
{
    if (team->count <= MIN_TEAM_SIZE || index <= 0 || index >= team->count)
        return 0;
    for (int i = index; i + 1 < team->count; i++)
        team->plants[i] = team->plants[i + 1];
    team->count--;
    return 1;
}

void Team_Restore(Team *team)
{
    team->active_index = 0;
    for (int i = 0; i < team->count; i++) {
        team->plants[i].current_hp = team->plants[i].max_hp;
        team->plants[i].attack = team->plants[i].base_attack;
        team->plants[i].defense = team->plants[i].base_defense;
        team->plants[i].speed = team->plants[i].base_speed;
        memset(team->plants[i].stat_debuff_amount, 0,
               sizeof(team->plants[i].stat_debuff_amount));
        team->plants[i].ability_turn_count = 0;
        team->plants[i].fruit_count = 0;
        Plant_ClearStatuses(&team->plants[i]);
        if (team->plants[i].id == SPECIES_CLOVER) {
            Plant_AddStatus(&team->plants[i], PLANT_STATUS_LUCK, 0,
                            PLANT_STATUS_LUCK_CRITICAL_MULTIPLIER);
        }
    }
}
