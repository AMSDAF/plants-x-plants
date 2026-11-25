/* Compara bosses solo contra equipes iniciais usando somente as regras reais de combate. */
#include "battle.h"
#include "plant_data.h"
#include "ai.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int play(int a, int b, int c, SpeciesId boss, unsigned int seed, int first_move_only)
{
    srand(seed);
    Team player = {0}, enemy = {0};
    Team_SelectSpecies(&player,0,PlantData_GetStarter(a)->id);
    Team_SelectSpecies(&player,1,PlantData_GetStarter(b)->id);
    Team_SelectSpecies(&player,2,PlantData_GetStarter(c)->id);
    Team_SelectSpecies(&enemy,0,boss);
    BattleState battle;
    Battle_Init(&battle,player,enemy);
    for (int turn = 0; turn < 300; turn++) {
        if (battle.state == BATTLE_STATE_PLAYER_WIN) {
            int living = 0;
            for (int i = 0; i < battle.player_team.count; i++) living += Plant_IsAlive(&battle.player_team.plants[i]);
            return living;
        }
        if (battle.state == BATTLE_STATE_ENEMY_WIN) return 0;
        if (battle.state == BATTLE_STATE_REPLACEMENT) {
            int reserve = -1;
            for (int i = 0; i < battle.player_team.count; i++)
                if (Battle_CanSwitch(&battle,i)) { reserve = i; break; }
            if (reserve < 0 || !Battle_SwitchPlant(&battle,reserve)) return -1;
        } else {
            const Plant *active = &battle.player_team.plants[battle.player_team.active_index];
            int best = -1;
            double best_score = -1;
            for (int i = 0; i < MAX_MOVES; i++) {
                const Move *move = &active->moves[i];
                if (move->effect != MOVE_EFFECT_DAMAGE ||
                    !Battle_CanUseMove(&battle,0,battle.player_team.active_index,i)) continue;
                double score = move->power * move->accuracy * (1.0+Battle_GetCriticalChance(active,move)/200.0);
                if (score > best_score) { best = i; best_score = score; }
                if (first_move_only) break;
            }
            int enemy_move = -1;
            if (AI_GetPlannedAction(&battle, &enemy_move) != AI_ACTION_MOVE) return -1;
            if (best < 0 || !Battle_ExecuteRound(&battle,
                    (BattleAction){BATTLE_ACTION_MOVE, best},
                    (BattleAction){BATTLE_ACTION_MOVE, enemy_move})) return -1;
        }
    }
    return -1;
}

/* Limites e cura são conferidos por ações reais, com sorteios desativados na fixture. */
static void check_limits(void)
{
    for (SpeciesId boss = SPECIES_SUNFLOWER; boss <= SPECIES_ALOE; boss++) {
        Team player = {0}, enemy = {0};
        Team_SelectSpecies(&player,0,SPECIES_CLOVER);
        Team_SelectSpecies(&player,1,SPECIES_CACTUS);
        Team_SelectSpecies(&enemy,0,boss);
        enemy.plants[0].current_hp -= 50;
        BattleState battle;
        Battle_Init(&battle,player,enemy);
        int own_turns = 0;
        while (Battle_CanUseMove(&battle,1,0,2)) {
            assert(own_turns < 16);
            assert(Battle_SwitchPlant(&battle,1-battle.player_team.active_index));
            assert(Battle_ExecuteMove(&battle,2));
            own_turns++;
        }
        const Plant *active = &battle.enemy_team.plants[0];
        int base = boss == SPECIES_ALOE ? active->base_defense : active->base_attack;
        int actual = boss == SPECIES_ALOE ? active->defense : active->attack;
        assert(actual == (int)(base*active->moves[2].effect_cap+0.5f));
        int healing = (active->max_hp*15+99)/100;
        int expected_hp = active->max_hp-50;
        if (boss == SPECIES_SUNFLOWER) expected_hp += (own_turns/3)*healing;
        if (expected_hp > active->max_hp) expected_hp = active->max_hp;
        assert(active->current_hp == expected_hp);
        assert(Battle_SwitchPlant(&battle,1-battle.player_team.active_index));
        BattleState before = battle;
        assert(!Battle_ExecuteMove(&battle,2));
        assert(memcmp(&before,&battle,sizeof(battle)) == 0);
        /* Erros também contam turnos para a cura periódica do Girassol. */
        if (boss == SPECIES_SUNFLOWER) {
            battle.enemy_team.plants[0].moves[0].accuracy = 0;
            do {
                assert(Battle_ExecuteMove(&battle,0));
                own_turns++;
                if (own_turns % 3 != 0)
                    assert(Battle_SwitchPlant(&battle,1-battle.player_team.active_index));
            } while (own_turns % 3 != 0);
            expected_hp += healing;
            if (expected_hp > active->max_hp) expected_hp = active->max_hp;
            assert(active->current_hp == expected_hp);
        }
    }
    Team player = {0}, enemy = {0};
    Team_SelectSpecies(&player,0,SPECIES_CLOVER);
    Team_SelectSpecies(&enemy,0,SPECIES_ALOE);
    player.plants[0].moves[0].accuracy = 100;
    player.plants[0].moves[0].critical_chance = 0;
    BattleState aloe;
    Battle_Init(&aloe,player,enemy);
    BattleState plain = aloe;
    plain.enemy_team.plants[0].id = SPECIES_PEPPER;
    assert(Battle_ExecuteMove(&plain,0));
    int damage = enemy.plants[0].max_hp-plain.enemy_team.plants[0].current_hp;
    assert(Battle_ExecuteMove(&aloe,0));
    assert(aloe.enemy_team.plants[0].current_hp == enemy.plants[0].max_hp-damage+damage/8);
}

int main(int argc, char **argv)
{
    check_limits();
    int first_move_only = argc > 1 && strcmp(argv[1],"--first-move") == 0;
    int report_only = argc > 1 && strcmp(argv[1],"--report") == 0;
    int failures = 0;
    for (SpeciesId boss = SPECIES_SUNFLOWER; boss <= SPECIES_ALOE; boss++) {
        int wins = 0, total = 0, stalled = 0, worst = 101, survivors = 0;
        int worst_team[3] = {0};
        for (int a = 0; a < STARTER_SPECIES_COUNT; a++)
        for (int b = 0; b < STARTER_SPECIES_COUNT; b++)
        for (int c = 0; c < STARTER_SPECIES_COUNT; c++) {
            if (a == b || a == c || b == c) continue;
            int team_wins = 0;
            for (unsigned int seed = 1; seed <= 100; seed++) {
                int result = play(a,b,c,boss,seed,first_move_only);
                team_wins += result > 0;
                if (result > 0) survivors += result;
                stalled += result < 0;
            }
            total += 100; wins += team_wins;
            if (team_wins < worst) {
                worst = team_wins;
                worst_team[0] = a; worst_team[1] = b; worst_team[2] = c;
            }
        }
        printf("%s: %d/%d vitorias (%.1f%%); pior ordem %s / %s / %s: %d%%; travamentos %d; plantas vivas por vitoria %.2f.\n",
            PlantData_GetSpecies(boss)->name_pt,wins,total,100.0*wins/total,
            PlantData_GetStarter(worst_team[0])->name_pt,PlantData_GetStarter(worst_team[1])->name_pt,
            PlantData_GetStarter(worst_team[2])->name_pt,worst,stalled,wins ? (double)survivors/wins : 0);
        if (stalled || worst < 75) failures++;
    }
    if (!report_only && failures) {
        fputs("FAIL: uma equipe basica venceu menos de 75% das sementes ou houve travamento.\n",stderr);
        return 1;
    }
    return 0;
}
