/*
 * plant_data.c
 * Armazena os dados das espécies, estágios evolutivos e golpes.
 * Centraliza os atributos usados para criar as plantas.
 * Não depende da raylib.
 */
#include "plant_data.h"
#include <stddef.h>

// Catálogo usando arrays estáticos char[] nas structs
#define DAMAGE_MOVE(move_name, move_category, move_power, move_accuracy, move_critical, move_priority) \
    {.name = move_name, .category = move_category, .effect = MOVE_EFFECT_DAMAGE, \
     .power = move_power, .accuracy = move_accuracy, \
     .critical_chance = move_critical, .priority = move_priority}
#define ATTRIBUTE_MOVE(move_name, move_target, move_stat, step, cap, mode) \
    {.name = move_name, .category = MOVE_CATEGORY_STATUS, .effect = MOVE_EFFECT_ATTRIBUTE, \
     .target = move_target, .stat = move_stat, .stat_change_mode = mode, \
     .accuracy = 100, .effect_step = step, .effect_cap = cap}
static const SpeciesData SPECIES_CATALOG[SPECIES_COUNT] = {
    {
        SPECIES_CACTUS,
        "Cacto",
        100,
        55,
        40,
        45,
        {
            DAMAGE_MOVE("Espinho Afiado", MOVE_CATEGORY_RANGED, 40, 100, 10, 0),
            DAMAGE_MOVE("Investida", MOVE_CATEGORY_PHYSICAL, 35, 95, 5, 1),
            ATTRIBUTE_MOVE("Quebra-Defesa", MOVE_TARGET_OPPONENT, MOVE_STAT_DEFENSE,
                           0.5f, 2.0f, MOVE_STAT_CHANGE_RATIO_DEBUFF)
        }
    },
    {
        SPECIES_SUNFLOWER,
        "Girassol",
        225,
        50,
        45,
        38,
        {
            DAMAGE_MOVE("Raio Solar", MOVE_CATEGORY_RANGED, 50, 90, 20, 0),
            DAMAGE_MOVE("Folha Navalha", MOVE_CATEGORY_RANGED, 40, 100, 10, 0),
            ATTRIBUTE_MOVE("Fotossíntese", MOVE_TARGET_SELF, MOVE_STAT_ATTACK,
                           1.0f, 3.0f, MOVE_STAT_CHANGE_ADD_TO_CAP)
        }
    },
    {SPECIES_ALOE, "Babosa", 275, 46, 53, 30,
        {DAMAGE_MOVE("Espinho Curto", MOVE_CATEGORY_RANGED, 39, 100, 8, 0),
         DAMAGE_MOVE("Corte de Aloe", MOVE_CATEGORY_PHYSICAL, 51, 85, 16, 0),
         ATTRIBUTE_MOVE("Casca Grossa", MOVE_TARGET_SELF, MOVE_STAT_DEFENSE,
                        1.0f, 3.0f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_POISON_IVY, "Hera Venenosa", 97, 47, 45, 60,
        {DAMAGE_MOVE("Cipó Ágil", MOVE_CATEGORY_PHYSICAL, 40, 100, 10, 1),
         DAMAGE_MOVE("Cipó Feroz", MOVE_CATEGORY_PHYSICAL, 52, 85, 18, 0),
         ATTRIBUTE_MOVE("Folhagem Densa", MOVE_TARGET_SELF, MOVE_STAT_DEFENSE,
                        0.5f, 2.5f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_PEPPER, "Pimenta", 92, 50, 42, 58,
        {DAMAGE_MOVE("Arremesso", MOVE_CATEGORY_RANGED, 40, 100, 12, 0),
         ATTRIBUTE_MOVE("Crescer", MOVE_TARGET_SELF, MOVE_STAT_ATTACK,
                        0.5f, 2.0f, MOVE_STAT_CHANGE_ADD_TO_CAP),
         DAMAGE_MOVE("Fruto Incendiário", MOVE_CATEGORY_RANGED, 55, 90, 18, 0)}},
    {SPECIES_ORCHID, "Orquídea", 86, 53, 40, 55,
        {DAMAGE_MOVE("Pétala Rosa", MOVE_CATEGORY_RANGED, 41, 100, 14, 0),
         DAMAGE_MOVE("Corte Floral", MOVE_CATEGORY_PHYSICAL, 54, 85, 22, 0),
         ATTRIBUTE_MOVE("Manto Floral", MOVE_TARGET_SELF, MOVE_STAT_DEFENSE,
                        0.5f, 2.5f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_CLOVER, "Trevo", 94, 47, 46, 50,
        {DAMAGE_MOVE("Folha Tripla", MOVE_CATEGORY_RANGED, 41, 100, 15, 0),
         DAMAGE_MOVE("Trinca de Folhas", MOVE_CATEGORY_RANGED, 55, 80, 30, 0),
         ATTRIBUTE_MOVE("Passos Ligeiros", MOVE_TARGET_SELF, MOVE_STAT_SPEED,
                        0.5f, 2.0f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_CARNIVOROUS_PLANT, "Planta Carnívora", 101, 56, 50, 35,
        {DAMAGE_MOVE("Mordida", MOVE_CATEGORY_PHYSICAL, 42, 100, 10, 0),
         DAMAGE_MOVE("Mordida Forte", MOVE_CATEGORY_PHYSICAL, 54, 85, 18, 0),
         ATTRIBUTE_MOVE("Apetite Voraz", MOVE_TARGET_SELF, MOVE_STAT_ATTACK,
                        0.5f, 2.0f, MOVE_STAT_CHANGE_ADD_TO_CAP)}}
};
#undef DAMAGE_MOVE
#undef ATTRIBUTE_MOVE

static const SpeciesId PLAYABLE_SPECIES[PLAYABLE_SPECIES_COUNT] = {
    SPECIES_CLOVER, SPECIES_POISON_IVY, SPECIES_PEPPER,
    SPECIES_CACTUS, SPECIES_ORCHID, SPECIES_CARNIVOROUS_PLANT
};

const SpeciesData* PlantData_GetSpecies(SpeciesId id) {
    if (id < 0 || id >= SPECIES_COUNT) {
        return NULL;
    }
    return &SPECIES_CATALOG[id];
}

const SpeciesData* PlantData_GetPlayableSpecies(int index)
{
    if (index < 0 || index >= PLAYABLE_SPECIES_COUNT) return NULL;
    return PlantData_GetSpecies(PLAYABLE_SPECIES[index]);
}

const SpeciesData* PlantData_GetStarter(int index)
{
    return PlantData_GetPlayableSpecies(index);
}
