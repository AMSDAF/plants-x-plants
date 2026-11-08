/*
 * plant_data.c
 * Armazena os dados das espécies, estágios evolutivos e golpes.
 * Centraliza os atributos usados para criar as plantas.
 * Não depende da raylib.
 */
#include "plant_data.h"
#include <stddef.h>

// Catálogo usando arrays estáticos char[] nas structs
static const SpeciesData SPECIES_CATALOG[SPECIES_COUNT] = {
    {
        SPECIES_CACTUS,
        "Cacto",
        100,
        55,
        40,
        {
            {"Espinho Afiado", 40, 100},
            {"Investida", 35, 95}
        }
    },
    {
        SPECIES_SUNFLOWER,
        "Girassol",
        90,
        45,
        45,
        {
            {"Raio Solar", 50, 90},
            {"Folha Navalha", 40, 100}
        }
    }
};

const SpeciesData* PlantData_GetSpecies(SpeciesId id) {
    if (id < 0 || id >= SPECIES_COUNT) {
        return NULL;
    }
    return &SPECIES_CATALOG[id];
}