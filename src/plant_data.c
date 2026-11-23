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
        120,60,105,20,
        {
            DAMAGE_MOVE("Espinho Afiado", MOVE_CATEGORY_RANGED, 55, 90, 30, 0),
            DAMAGE_MOVE("Investida", MOVE_CATEGORY_PHYSICAL, 45, 95, 50, 1),
            ATTRIBUTE_MOVE("Quebra-Defesa", MOVE_TARGET_OPPONENT, MOVE_STAT_DEFENSE,
                           0.5f, 2.0f, MOVE_STAT_CHANGE_RATIO_DEBUFF)
        }
    },
    {
        SPECIES_SUNFLOWER,
        "Girassol",
        240,65,40,34,
        {
            DAMAGE_MOVE("Raio Solar", MOVE_CATEGORY_RANGED, 40, 30, 20, 0),
            DAMAGE_MOVE("Folha Navalha", MOVE_CATEGORY_RANGED, 40, 100, 10, 0),
            ATTRIBUTE_MOVE("Fotossíntese", MOVE_TARGET_SELF, MOVE_STAT_ATTACK,
                           0.25f, 1.5f, MOVE_STAT_CHANGE_ADD_TO_CAP)
        }
    },
    {SPECIES_ALOE, "Babosa", 275, 57, 60, 30,
        {DAMAGE_MOVE("Espinho", MOVE_CATEGORY_RANGED, 40, 100, 45, 0),
         DAMAGE_MOVE("Corte", MOVE_CATEGORY_PHYSICAL, 67, 85, 35, 0),
         ATTRIBUTE_MOVE("Casca Grossa", MOVE_TARGET_SELF, MOVE_STAT_DEFENSE,
                        1.0f, 3.0f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_POISON_IVY, "Hera Venenosa", 78, 40, 65, 40,
        {DAMAGE_MOVE("Cipó Ágil", MOVE_CATEGORY_PHYSICAL, 40, 100, 10, 1),
         DAMAGE_MOVE("Cipó Feroz", MOVE_CATEGORY_PHYSICAL, 64, 85, 18, 0),
         ATTRIBUTE_MOVE("Folhagem Densa", MOVE_TARGET_SELF, MOVE_STAT_DEFENSE,
                        0.5f, 2.5f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_PEPPER, "Pimenta", 92, 70, 65, 58,
        {DAMAGE_MOVE("Galho", MOVE_CATEGORY_PHYSICAL, 40, 100, 12, 0),
         ATTRIBUTE_MOVE("Crescer", MOVE_TARGET_SELF, MOVE_STAT_ATTACK,
                        0.5f, 2.0f, MOVE_STAT_CHANGE_ADD_TO_CAP),
         DAMAGE_MOVE("Fruto Incendiário", MOVE_CATEGORY_RANGED, 125, 100, 20, 0)}},
    {SPECIES_ORCHID, "Orquídea", 45, 40, 90, 65,
        {DAMAGE_MOVE("Pétala Rosa", MOVE_CATEGORY_RANGED, 67, 80, 14, 0),
         DAMAGE_MOVE("Raíz Perfurante", MOVE_CATEGORY_PHYSICAL, 40, 100, 62, 0),
         ATTRIBUTE_MOVE("Manto Floral", MOVE_TARGET_SELF, MOVE_STAT_DEFENSE,
                        0.5f, 2.5f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_CLOVER, "Trevo", 53, 65, 30, 60,
        {DAMAGE_MOVE("Folha Tripla", MOVE_CATEGORY_RANGED, 61, 100, 25, 0),
         DAMAGE_MOVE("ZAP", MOVE_CATEGORY_RANGED, 78, 70, 30, 0),
         ATTRIBUTE_MOVE("Passos Ligeiros", MOVE_TARGET_SELF, MOVE_STAT_SPEED,
                        0.5f, 2.0f, MOVE_STAT_CHANGE_ADD_TO_CAP)}},
    {SPECIES_CARNIVOROUS_PLANT, "Planta Carnívora", 101, 75, 50, 10,
        {DAMAGE_MOVE("Mordida", MOVE_CATEGORY_PHYSICAL, 42, 100, 10, 0),
         DAMAGE_MOVE("Mordida Forte", MOVE_CATEGORY_PHYSICAL, 90, 70, 18, 0),
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
