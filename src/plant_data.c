/*
 * plant_data.c
 * Centraliza o elenco, as opções iniciais e os atributos e golpes provisórios.
 * Não depende da raylib.
 */
#include "plant_data.h"
#include <stddef.h>

/* Atributos reaproveitados da base; golpes só causam dano, sem efeitos especiais. */
static const SpeciesData SPECIES_CATALOG[SPECIES_COUNT] = {
    [SPECIES_CACTUS] = {
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
    [SPECIES_SUNFLOWER] = {
        SPECIES_SUNFLOWER,
        "Girassol",
        90,
        45,
        45,
        {
            {"Raio Solar", 50, 90},
            {"Folha Navalha", 40, 100}
        }
    },
    [SPECIES_ALOE] = {SPECIES_ALOE, "Babosa", 110, 42, 53,
        {{"Espinho Curto", 39, 100}, {"Corte de Aloe", 51, 85}}},
    [SPECIES_POISON_IVY] = {SPECIES_POISON_IVY, "Hera Venenosa", 97, 47, 45,
        {{"Cipó Ágil", 40, 100}, {"Cipó Feroz", 52, 85}}},
    [SPECIES_PEPPER] = {SPECIES_PEPPER, "Pimenta", 89, 54, 38,
        {{"Investida", 42, 100}, {"Semente Ardente", 55, 80}}},
    [SPECIES_ORCHID] = {SPECIES_ORCHID, "Orquídea", 86, 53, 40,
        {{"Pétala Rosa", 41, 100}, {"Corte Floral", 54, 85}}},
    [SPECIES_VENUS_FLYTRAP] = {SPECIES_VENUS_FLYTRAP, "Planta Carnívora", 106, 48, 49,
        {{"Mordida", 36, 100}, {"Mandíbula Feroz", 49, 85}}},
    [SPECIES_CLOVER] = {SPECIES_CLOVER, "Trevo", 94, 47, 46,
        {{"Folha Tripla", 41, 100}, {"Trinca de Folhas", 55, 80}}}
};

static const SpeciesId STARTER_SPECIES[STARTER_SPECIES_COUNT] = {
    SPECIES_CLOVER, SPECIES_POISON_IVY, SPECIES_PEPPER,
    SPECIES_CACTUS, SPECIES_ORCHID, SPECIES_VENUS_FLYTRAP
};

const SpeciesData* PlantData_GetSpecies(SpeciesId id) {
    if (id < 0 || id >= SPECIES_COUNT) {
        return NULL;
    }
    return &SPECIES_CATALOG[id];
}

const SpeciesData* PlantData_GetStarter(int index)
{
    if (index < 0 || index >= STARTER_SPECIES_COUNT) return NULL;
    return PlantData_GetSpecies(STARTER_SPECIES[index]);
}
