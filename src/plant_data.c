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
    },
    {SPECIES_ALOE, "Babosa", 110, 42, 53,
        {{"Espinho Curto", 39, 100}, {"Corte de Aloe", 51, 85}}},
    {SPECIES_ROSE, "Rosa", 89, 54, 38,
        {{"Espinho Rubro", 42, 100}, {"Pétala Cortante", 55, 80}}},
    {SPECIES_IVY, "Hera", 97, 47, 45,
        {{"Cipó Ágil", 40, 100}, {"Cipó Feroz", 52, 85}}},
    {SPECIES_BAMBOO, "Bambu", 106, 48, 49,
        {{"Haste Rápida", 36, 100}, {"Bastão Verde", 49, 85}}},
    {SPECIES_FERN, "Samambaia", 99, 45, 47,
        {{"Fronda Rápida", 40, 100}, {"Fronda Pesada", 54, 80}}},
    {SPECIES_LAVENDER, "Lavanda", 88, 51, 43,
        {{"Ramo Violeta", 42, 100}, {"Ramo Perfurante", 53, 85}}},
    {SPECIES_ORCHID, "Orquídea", 86, 53, 40,
        {{"Pétala Rosa", 41, 100}, {"Corte Floral", 54, 85}}},
    {SPECIES_LOTUS, "Lótus", 101, 43, 50,
        {{"Folha Lisa", 39, 100}, {"Pétala Pesada", 52, 85}}},
    {SPECIES_MINT, "Hortelã", 92, 50, 42,
        {{"Folha Fresca", 40, 100}, {"Lâmina Verde", 52, 85}}},
    {SPECIES_CLOVER, "Trevo", 94, 47, 46,
        {{"Folha Tripla", 41, 100}, {"Trinca de Folhas", 55, 80}}}
};

const SpeciesData* PlantData_GetSpecies(SpeciesId id) {
    if (id < 0 || id >= SPECIES_COUNT) {
        return NULL;
    }
    return &SPECIES_CATALOG[id];
}
