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

/* Confere os contratos novos sem depender de sorteios de dano ou status. */
static void check_refreshed_rules(void)
{
    Team player = {0}, enemy = {0};
    assert(Team_SelectSpecies(&player,0,SPECIES_PEPPER));
    assert(Team_SelectSpecies(&player,1,SPECIES_CLOVER));
    assert(Team_SelectSpecies(&enemy,0,SPECIES_ALOE));
    assert(Team_SelectSpecies(&enemy,1,SPECIES_SUNFLOWER));
    BattleState battle;
    Battle_Init(&battle,player,enemy);
    assert(battle.current_turn == 0);
    assert(Plant_HasStatus(&battle.player_team.plants[1],PLANT_STATUS_LUCK));
    assert(battle.boosted_move_indices[0][0] == -1);
    assert(Battle_CanUseMove(&battle,0,0,0));
    assert(Battle_GetMovePower(&battle,0,0) == player.plants[0].moves[0].power);
    battle.player_team.plants[0].current_hp = player.plants[0].max_hp / 2;
    assert(Battle_GetMovePower(&battle,0,0) == player.plants[0].moves[0].power);
    battle.player_team.plants[0].current_hp = player.plants[0].max_hp;
    battle.boosted_move_indices[0][0] = 0;
    assert(!Battle_CanUseMove(&battle,0,0,0));
    BattleState before = battle;
    assert(!Battle_ExecuteMove(&battle,0));
    assert(!Battle_ExecuteMove(&battle,2));
    assert(memcmp(&before,&battle,sizeof(before)) == 0);
    battle.formulas[0][1] = (LogicFormula){LOGIC_OWN_LOW_HP,LOGIC_ENEMY_LOW_HP,LOGIC_OR};
    battle.player_team.plants[1].current_hp = 1;
    assert(Battle_EvaluatePlantFormula(&battle,0,1,battle.formulas[0][1]).result);
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,battle.formulas[0][0]).result);
    int initial_attack = battle.player_team.plants[0].attack;
    for (int i = 0; i < 2; i++) {
        assert(Battle_ExecuteMove(&battle,1));
        assert(Battle_SwitchPlant(&battle,1-battle.enemy_team.active_index));
    }
    assert(battle.player_team.plants[0].attack == 2*initial_attack);
    assert(battle.player_team.plants[0].fruit_count == 1);
    assert(Battle_CanUseMove(&battle,0,0,2));
    before = battle;
    assert(!Battle_ExecuteMove(&battle,1));
    assert(memcmp(&before,&battle,sizeof(before)) == 0);
    assert(Battle_SwitchPlant(&battle,1));
    assert(battle.player_team.plants[0].attack == initial_attack);
    Team_Restore(&battle.player_team);
    for (int i = 0; i < battle.player_team.count; i++)
        assert(battle.player_team.plants[i].status_count ==
            (battle.player_team.plants[i].id == SPECIES_CLOVER ? 1 : 0));
    Battle_Init(&battle,enemy,player);
    assert(battle.current_turn == 1);
}

/* Condições de status, velocidade, ataque e defesa, além do bônus do operador E. */
static void check_logic_conditions(void)
{
    Team player = {0}, enemy = {0};
    assert(Team_SelectSpecies(&player,0,SPECIES_CACTUS));
    assert(Team_SelectSpecies(&player,1,SPECIES_CLOVER));
    assert(Team_SelectSpecies(&enemy,0,SPECIES_ALOE));
    BattleState battle;
    Battle_Init(&battle,player,enemy);
    Plant *own = &battle.player_team.plants[0];
    Plant *rival = &battle.enemy_team.plants[0];
    LogicFormula status_formula = {LOGIC_OWN_LOW_HP,LOGIC_ENEMY_STATUS,LOGIC_AND};
    LogicFormula faster_formula = {LOGIC_OWN_LOW_HP,LOGIC_ENEMY_FASTER,LOGIC_OR};
    LogicFormula attack_formula = {LOGIC_OWN_LOW_HP,LOGIC_OWN_HIGHER_ATTACK,LOGIC_OR};
    LogicFormula defense_formula = {LOGIC_OWN_LOW_HP,LOGIC_OWN_HIGHER_DEFENSE,LOGIC_OR};
    assert(Battle_GetFormulaPowerBonus(LOGIC_AND) == 70);
    assert(Battle_GetFormulaPowerBonus(LOGIC_OR) == 45);
    assert(Battle_GetFormulaPowerBonus(LOGIC_XOR) == 55);

    own->current_hp = own->max_hp / 2;
    battle.boosted_move_indices[0][0] = 0;
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,status_formula).result);
    assert(Battle_GetPlantMovePower(&battle,0,0,0) == own->moves[0].power);

    const PlantStatusType harmful[] = {PLANT_STATUS_POISON, PLANT_STATUS_BURN,
        PLANT_STATUS_HOST, PLANT_STATUS_PARALYZED};
    for (size_t i = 0; i < sizeof(harmful)/sizeof(harmful[0]); i++) {
        assert(Plant_AddStatus(rival,harmful[i],2,1.0f));
        LogicEvaluation value = Battle_EvaluatePlantFormula(&battle,0,0,status_formula);
        assert(value.right && value.result);
        assert(Battle_GetPlantMovePower(&battle,0,0,0) == own->moves[0].power + 70);
        assert(Plant_RemoveStatus(rival,harmful[i]));
        assert(!Battle_EvaluatePlantFormula(&battle,0,0,status_formula).result);
        assert(Battle_GetPlantMovePower(&battle,0,0,0) == own->moves[0].power);
    }

    own->current_hp = own->max_hp;
    assert(rival->speed > own->speed);
    assert(Battle_EvaluatePlantFormula(&battle,0,0,faster_formula).right);
    rival->speed = own->speed;
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,faster_formula).right);
    rival->speed = own->speed - 1;
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,faster_formula).right);
    rival->speed = own->speed + 1;
    assert(Battle_EvaluatePlantFormula(&battle,0,0,faster_formula).right);

    own->attack = rival->attack;
    own->defense = rival->defense;
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,attack_formula).right);
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,defense_formula).right);
    own->attack = rival->attack + 1;
    own->defense = rival->defense + 1;
    assert(Battle_EvaluatePlantFormula(&battle,0,0,attack_formula).right);
    assert(Battle_EvaluatePlantFormula(&battle,0,0,defense_formula).right);
    own->attack = rival->attack - 1;
    own->defense = rival->defense - 1;
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,attack_formula).right);
    assert(!Battle_EvaluatePlantFormula(&battle,0,0,defense_formula).right);
}

/* Hospedeiro acompanha a afetada, mas some quando a planta que o aplicou cai. */
static void check_host_lifetime(void)
{
    Team player = {0}, enemy = {0};
    assert(Team_SelectSpecies(&player,0,SPECIES_ORCHID));
    assert(Team_SelectSpecies(&player,1,SPECIES_CLOVER));
    assert(Team_SelectSpecies(&enemy,0,SPECIES_CACTUS));
    assert(Team_SelectSpecies(&enemy,1,SPECIES_CLOVER));
    BattleState battle;
    Battle_Init(&battle,player,enemy);
    int source = battle.player_team.active_index;
    int victim = battle.enemy_team.active_index;
    int source_side = -1, source_index = -1;
    assert(Battle_ApplyStatusFrom(&battle,0,source,1,victim,
                                  PLANT_STATUS_HOST,0,1.0f));
    assert(Plant_HasStatus(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST));
    assert(Plant_GetStatusSource(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST,
                                 &source_side,&source_index));
    assert(source_side == 0 && source_index == source);

    battle.current_turn = 1;
    assert(Battle_SwitchPlant(&battle,1-victim));
    assert(battle.enemy_team.active_index == 1-victim);
    assert(Plant_HasStatus(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST));

    battle.current_turn = 0;
    assert(Battle_SwitchPlant(&battle,1-source));
    assert(battle.player_team.active_index == 1-source);
    assert(Plant_HasStatus(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST));

    battle.enemy_team.plants[1-victim].current_hp = 0;
    Battle_CheckConditions(&battle);
    assert(Plant_HasStatus(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST));

    battle.player_team.plants[source].current_hp = 0;
    Battle_CheckConditions(&battle);
    assert(!Plant_HasStatus(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST));
    assert(!Plant_GetStatusSource(&battle.enemy_team.plants[victim],PLANT_STATUS_HOST,
                                  &source_side,&source_index));
}

int main(void)
{
    check_refreshed_rules();
    check_logic_conditions();
    check_host_lifetime();
    /* Um guardião sozinho encerra a luta ao cair, sem pedir reserva. */
    for (int id = SPECIES_SUNFLOWER; id <= SPECIES_ALOE; id++) {
        Team players = {0}, boss = {0};
        Team_SelectSpecies(&players,0,SPECIES_CACTUS);
        Team_SelectSpecies(&players,1,SPECIES_CLOVER);
        Team_SelectSpecies(&players,2,SPECIES_PEPPER);
        Team_SelectSpecies(&boss,0,id);
        BattleState solo;
        Battle_Init(&solo,players,boss);
        assert(solo.enemy_team.count == 1 && solo.player_team.count == 3);
        solo.player_team.plants[0].moves[0].power = 10000;
        solo.player_team.plants[0].moves[0].accuracy = 100;
        /* A ordem depende da velocidade; este caso testa apenas o nocaute. */
        solo.current_turn = 0;
        assert(Battle_ExecuteMove(&solo,0));
        assert(solo.state == BATTLE_STATE_PLAYER_WIN);
        BattleState ended = solo;
        assert(!Battle_ExecuteMove(&solo,0) && !Battle_SwitchPlant(&solo,0));
        assert(memcmp(&ended,&solo,sizeof(solo)) == 0);
    }
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
            encounter.formulas[0][0] = (LogicFormula){LOGIC_OWN_LOW_HP, LOGIC_ENEMY_STATUS, LOGIC_OR};
            encounter.player_team.plants[0].current_hp = player.plants[0].max_hp / 2;
            encounter.player_team.plants[0].fruit_count = 1;
            /* A ordem depende da velocidade; este caso testa apenas o golpe. */
            encounter.current_turn = 0;
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
            assert(plant.moves[i].name[0]);
            if (plant.moves[i].effect == MOVE_EFFECT_DAMAGE) assert(plant.moves[i].power > 0);
            else assert(plant.moves[i].effect_step > 0 && plant.moves[i].effect_cap > 0);
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
    battle.boosted_move_indices[0][0] = 1;
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
    puts("PASS: 8 especies, 6 iniciais sem bosses, atributos/golpes, IDs invalidos, equipes, acoes invalidas, trocas, origem de Hospedeiro, derrota, substituicao pela IA, vitoria e restauracao.");
    return 0;
}
