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
    for (int id = 0; id < SPECIES_COUNT; id++) {
        const SpeciesData *data = PlantData_GetSpecies((SpeciesId)id);
        Plant plant = Plant_CreateFromSpecies(id);
        assert(plant.id == id && strcmp(plant.name_pt, data->name_pt) == 0);
        assert(plant.max_hp == data->base_hp && plant.current_hp == data->base_hp);
        assert(plant.attack == data->base_attack && plant.defense == data->base_defense);
        for (int i = 0; i < MAX_MOVES; i++) {
            assert(strcmp(plant.moves[i].name, data->moves[i].name) == 0);
            assert(plant.moves[i].power == data->moves[i].power);
            assert(plant.moves[i].accuracy == data->moves[i].accuracy);
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
    assert(!Battle_ExecuteMove(&battle, BATTLE_LOGIC_MOVE_INDEX + 1));
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
    puts("PASS: 12 especies, atributos/golpes, IDs invalidos, equipes, acoes invalidas, trocas, derrota, substituicao pela IA, vitoria e restauracao.");
    return 0;
}
