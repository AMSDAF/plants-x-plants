/* Valida atributos, fórmulas e regras compartilhadas do combate da campanha. */
#include "plant.h"
#include "plant_data.h"
#include "battle.h"
#include "ai.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_logic_truth_tables(BattleState battle)
{
    const int expected[LOGIC_OPERATOR_COUNT][4] = {
        [LOGIC_AND] = {0, 0, 0, 1},
        [LOGIC_OR] = {0, 1, 1, 1},
        [LOGIC_XOR] = {0, 1, 1, 0}
    };
    Plant *own = &battle.player_team.plants[0];
    Plant *enemy = &battle.enemy_team.plants[0];
    for (int op = 0; op < LOGIC_OPERATOR_COUNT; op++) {
        LogicFormula formula = {LOGIC_OWN_LOW_HP, LOGIC_ENEMY_LOW_HP, op};
        for (int row = 0; row < 4; row++) {
            own->current_hp = row / 2 ? own->max_hp / 2 : own->max_hp / 2 + 1;
            enemy->current_hp = row % 2 ? enemy->max_hp / 2 : enemy->max_hp / 2 + 1;
            LogicEvaluation value = Battle_EvaluateFormula(&battle, 0, formula);
            assert(value.left == row / 2 && value.right == row % 2);
            assert(value.result == expected[op][row]);
        }
    }
}

int main(void)
{
    static const SpeciesId starters[STARTER_SPECIES_COUNT] = {
        SPECIES_CLOVER, SPECIES_POISON_IVY, SPECIES_PEPPER,
        SPECIES_CACTUS, SPECIES_ORCHID, SPECIES_CARNIVOROUS_PLANT
    };
    static const char *const names[STARTER_SPECIES_COUNT] = {
        "Trevo", "Hera Venenosa", "Pimenta", "Cacto", "Orquídea", "Planta Carnívora"
    };
    assert(SPECIES_COUNT == STARTER_SPECIES_COUNT + 2);
    assert(!PlantData_GetStarter(-1) && !PlantData_GetStarter(STARTER_SPECIES_COUNT));
    assert(!PlantData_GetSpecies((SpeciesId)-1) && !PlantData_GetSpecies(SPECIES_COUNT));
    for (int i = 0; i < STARTER_SPECIES_COUNT; i++) {
        const SpeciesData *starter = PlantData_GetStarter(i);
        assert(starter && starter->id == starters[i]);
        assert(strcmp(starter->name_pt, names[i]) == 0);
        assert(starter->id != SPECIES_SUNFLOWER && starter->id != SPECIES_ALOE);
        for (int previous = 0; previous < i; previous++)
            assert(starter->id != PlantData_GetStarter(previous)->id);
        /* Cada inicial pode entrar na equipe e usar os dois golpes contra um boss. */
        for (int move = 0; move < MAX_MOVES; move++) {
            if (starter->moves[move].effect != MOVE_EFFECT_DAMAGE) continue;
            Team player = {0}, enemy = {0};
            assert(Team_SelectSpecies(&player, 0, starter->id));
            assert(Team_SelectSpecies(&enemy, 0, SPECIES_ALOE));
            player.plants[0].moves[move].accuracy = 100;
            BattleState encounter;
            Battle_Init(&encounter, player, enemy);
            encounter.boosted_move_indices[0][0] = 2;
            assert(Battle_ExecuteMove(&encounter, move));
            assert(encounter.enemy_team.plants[0].current_hp < enemy.plants[0].current_hp);
        }
    }
    for (int id = 0; id < SPECIES_COUNT; id++) {
        const SpeciesData *data = PlantData_GetSpecies((SpeciesId)id);
        Plant plant = Plant_CreateFromSpecies(id);
        assert(plant.id == id && strcmp(plant.name_pt, data->name_pt) == 0);
        assert(plant.max_hp == data->base_hp && plant.current_hp == data->base_hp);
        assert(plant.attack == data->base_attack && plant.defense == data->base_defense);
        assert(plant.max_hp > 0 && plant.attack > 0 && plant.defense > 0);
        assert(plant.speed == data->base_speed);
        for (int i = 0; i < MAX_MOVES; i++) {
            assert(strcmp(plant.moves[i].name, data->moves[i].name) == 0);
            assert(plant.moves[i].power == data->moves[i].power);
            assert(plant.moves[i].accuracy == data->moves[i].accuracy);
            assert(plant.moves[i].name[0] && plant.moves[i].power > 0);
            assert(plant.moves[i].accuracy > 0 && plant.moves[i].accuracy <= 100);
            assert(plant.moves[i].critical_chance == data->moves[i].critical_chance);
            assert(plant.moves[i].priority == data->moves[i].priority);
        }
    }
    Plant empty = {0}, invalid = Plant_CreateFromSpecies(-1);
    assert(memcmp(&empty, &invalid, sizeof(empty)) == 0);
    invalid = Plant_CreateFromSpecies(SPECIES_COUNT);
    assert(memcmp(&empty, &invalid, sizeof(empty)) == 0);
    Team team = {0};
    assert(Team_SelectSpecies(&team, 0, SPECIES_CACTUS));
    assert(Team_SelectSpecies(&team, 1, SPECIES_SUNFLOWER));
    Team before_team = team;
    assert(!Team_SelectSpecies(&team, 2, SPECIES_COUNT));
    assert(memcmp(&team, &before_team, sizeof(team)) == 0);
    BattleState battle;
    Battle_Init(&battle, team, team);
    check_logic_truth_tables(battle);
    assert(Battle_SwitchPlant(&battle, 1));
    assert(battle.current_turn == 1 && battle.player_team.active_index == 1);
    assert(Battle_SwitchPlant(&battle, 1));
    assert(battle.current_turn == 0 && battle.enemy_team.active_index == 1);
    Battle_Init(&battle, team, team);
    BattleState before = battle;
    assert(!Battle_ExecuteMove(&battle, -1));
    assert(!Battle_ExecuteMove(&battle, MAX_MOVES));
    assert(!Battle_SwitchPlant(&battle, 0));
    assert(!Battle_SwitchPlant(&battle, MAX_TEAM_SIZE));
    assert(memcmp(&battle, &before, sizeof(battle)) == 0);
    battle.player_team.plants[0].moves[0].power = 10000;
    battle.player_team.plants[0].moves[0].accuracy = 100;
    assert(Battle_ExecuteMove(&battle, 0));
    assert(battle.state == BATTLE_STATE_REPLACEMENT && battle.current_turn == 1);
    assert(battle.enemy_team.plants[0].current_hp == 0);
    before = battle;
    assert(!Battle_ExecuteMove(&battle, 0) && !Battle_SwitchPlant(&battle, 0));
    assert(memcmp(&battle, &before, sizeof(battle)) == 0);
    assert(AI_PlayTurn(&battle));
    assert(battle.state == BATTLE_STATE_ONGOING && battle.current_turn == 1);
    assert(battle.enemy_team.active_index == 1);
    assert(AI_PlayTurn(&battle) && battle.current_turn == 0);
    assert(Battle_ExecuteMove(&battle, 0));
    assert(battle.state == BATTLE_STATE_PLAYER_WIN);
    before = battle;
    assert(!Battle_ExecuteMove(&battle, 0) && !Battle_SwitchPlant(&battle, 1));
    assert(!AI_PlayTurn(&battle));
    assert(memcmp(&battle, &before, sizeof(battle)) == 0);
    Team_Restore(&battle.enemy_team);
    assert(battle.enemy_team.active_index == 0);
    for (int i = 0; i < battle.enemy_team.count; i++)
        assert(battle.enemy_team.plants[i].current_hp == battle.enemy_team.plants[i].max_hp);
    puts("PASS: 8 especies, 6 iniciais sem bosses, atributos/golpes, IDs invalidos, equipes, acoes invalidas, trocas, derrota, substituicao pela IA, vitoria e restauracao.");
    return 0;
}
