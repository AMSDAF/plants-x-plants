/*
 * plant_data.h
 * Declara o acesso aos dados das espécies e dos golpes.
 * Não depende da raylib.
 */
#ifndef PLANT_DATA_H
#define PLANT_DATA_H

#include "plant.h"

/* Elenco definitivo: seis iniciais e os bosses Girassol e Babosa. */
typedef enum {
    SPECIES_CACTUS = 0,
    SPECIES_SUNFLOWER = 1,
    SPECIES_ALOE,
    SPECIES_CLOVER,
    SPECIES_POISON_IVY,
    SPECIES_PEPPER,
    SPECIES_ORCHID,
    SPECIES_VENUS_FLYTRAP,
    SPECIES_COUNT
} SpeciesId;

#define STARTER_SPECIES_COUNT 6

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

/* Ordem da seleção inicial, independente dos IDs e sem os bosses. */
const SpeciesData* PlantData_GetStarter(int index);

#endif
