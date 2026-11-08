/*
 * plant_data.h
 * Declara o acesso aos dados das espécies e dos golpes.
 * Não depende da raylib.
 */
#ifndef PLANT_DATA_H
#define PLANT_DATA_H

#include "plant.h"

// Identificadores das espécies do protótipo
typedef enum {
    SPECIES_CACTUS = 0,
    SPECIES_SUNFLOWER = 1,
    SPECIES_COUNT
} SpeciesId;

// Estrutura para os dados base de uma espécie no catálogo
typedef struct {
    SpeciesId id;
    char name_pt[32];
    int base_hp;
    int base_attack;
    int base_defense;
    Move moves[MAX_MOVES];
} SpeciesData;

// Função para obter os dados base de uma espécie pelo ID
const SpeciesData* PlantData_GetSpecies(SpeciesId id);

#endif