/* Compara duelos 1x1 das plantas iniciais contra cada boss por HP e turnos. */
#include "ai.h"
#include "battle.h"
#include "plant_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DUEL_SEEDS 500u
#define MAX_ROUNDS 200

typedef struct {
    SpeciesId species;
    int battles;
    int wins;
    int stalls;
    int invalid;
    long long plant_hp;
    long long boss_hp;
    long long rounds;
    long long rounds_to_win;
} MatchupResult;

static BattleAction planned_player_action(const BattleState *battle)
{
    const Team *team = &battle->player_team;
    const Plant *active = &team->plants[team->active_index];
    int best = -1;
    double best_score = -1.0;
    for (int i = 0; i < MAX_MOVES; i++) {
        const Move *move = &active->moves[i];
        if (move->effect != MOVE_EFFECT_DAMAGE ||
            !Battle_CanUseMove(battle, 0, team->active_index, i)) continue;
        double score = (double)move->power * move->accuracy *
                       (1.0 + Battle_GetCriticalChance(active, move) / 200.0);
        if (score > best_score) {
            best = i;
            best_score = score;
        }
    }
    if (best < 0) return (BattleAction){BATTLE_ACTION_NONE, -1};
    return (BattleAction){BATTLE_ACTION_MOVE, best};
}

static int simulate(SpeciesId species, SpeciesId boss, unsigned int seed, MatchupResult *result)
{
    srand(seed);
    Team player = {0}, enemy = {0};
    Team_SelectSpecies(&player, 0, species);
    Team_SelectSpecies(&enemy, 0, boss);

    BattleState battle;
    Battle_Init(&battle, player, enemy);
    int rounds = 0;
    while (battle.state == BATTLE_STATE_ONGOING || battle.state == BATTLE_STATE_REPLACEMENT) {
        if (rounds >= MAX_ROUNDS) {
            result->stalls++;
            return -1;
        }
        if (battle.state == BATTLE_STATE_REPLACEMENT) {
            result->invalid++;
            return 0;
        }

        int index = -1;
        AIActionType enemy_type = AI_GetPlannedAction(&battle, &index);
        BattleAction enemy_action = {BATTLE_ACTION_NONE, -1};
        if (enemy_type == AI_ACTION_MOVE) enemy_action = (BattleAction){BATTLE_ACTION_MOVE, index};
        if (enemy_type == AI_ACTION_SWITCH) enemy_action = (BattleAction){BATTLE_ACTION_SWITCH, index};

        BattleAction player_action = planned_player_action(&battle);
        if (!Battle_ExecuteRound(&battle, player_action, enemy_action)) {
            result->invalid++;
            return 0;
        }
        rounds++;
    }

    const Plant *plant = &battle.player_team.plants[0];
    const Plant *boss_plant = &battle.enemy_team.plants[0];
    result->battles++;
    result->plant_hp += plant->current_hp;
    result->boss_hp += boss_plant->current_hp;
    result->rounds += rounds;
    if (battle.state == BATTLE_STATE_PLAYER_WIN) {
        result->wins++;
        result->rounds_to_win += rounds;
    }
    return 1;
}

static double average(long long value, int count)
{
    return count ? (double)value / count : 0.0;
}

static double plant_hp_score(const MatchupResult *result)
{
    return average(result->plant_hp, result->battles);
}

static double boss_hp_score(const MatchupResult *result)
{
    return average(result->boss_hp, result->battles);
}

static double turns_to_win_score(const MatchupResult *result)
{
    return result->wins ? average(result->rounds_to_win, result->wins) : 1e9;
}

/* Ordem: menos HP do boss, mais HP da planta e menos turnos até vencer. */
static int better_matchup(const MatchupResult *a, const MatchupResult *b)
{
    if (boss_hp_score(a) != boss_hp_score(b)) return boss_hp_score(a) < boss_hp_score(b);
    if (plant_hp_score(a) != plant_hp_score(b)) return plant_hp_score(a) > plant_hp_score(b);
    if (turns_to_win_score(a) != turns_to_win_score(b))
        return turns_to_win_score(a) < turns_to_win_score(b);
    return a->species < b->species;
}

static int better_plant_hp(const MatchupResult *a, const MatchupResult *b)
{
    if (plant_hp_score(a) != plant_hp_score(b)) return plant_hp_score(a) > plant_hp_score(b);
    return a->species < b->species;
}

static int better_boss_hp(const MatchupResult *a, const MatchupResult *b)
{
    if (boss_hp_score(a) != boss_hp_score(b)) return boss_hp_score(a) < boss_hp_score(b);
    return a->species < b->species;
}

static int better_turns(const MatchupResult *a, const MatchupResult *b)
{
    if (turns_to_win_score(a) != turns_to_win_score(b))
        return turns_to_win_score(a) < turns_to_win_score(b);
    return a->species < b->species;
}

static void sort_results(MatchupResult results[], int count,
                         int (*better)(const MatchupResult *, const MatchupResult *))
{
    for (int i = 1; i < count; i++) {
        MatchupResult key = results[i];
        int j = i - 1;
        while (j >= 0 && better(&key, &results[j])) {
            results[j + 1] = results[j];
            j--;
        }
        results[j + 1] = key;
    }
}

static const MatchupResult *best_result(const MatchupResult results[], int count,
                                        int (*better)(const MatchupResult *, const MatchupResult *))
{
    const MatchupResult *best = &results[0];
    for (int i = 1; i < count; i++)
        if (better(&results[i], best)) best = &results[i];
    return best;
}

static void print_table(const MatchupResult results[], const char *boss_name)
{
    printf("\nDuelo 1x1 contra %s (%u sementes por planta)\n", boss_name, DUEL_SEEDS);
    printf("Ordem: menos HP do boss, depois mais HP da planta, depois menos turnos para vencer.\n");
    printf("Pos  Planta              HP planta  HP boss  Turnos  Turnos/vitoria  Vitorias\n");
    for (int i = 0; i < STARTER_SPECIES_COUNT; i++) {
        const MatchupResult *result = &results[i];
        char turns_to_win[16];
        if (result->wins) snprintf(turns_to_win, sizeof(turns_to_win), "%.1f", turns_to_win_score(result));
        else snprintf(turns_to_win, sizeof(turns_to_win), "-");
        printf("%-4d %-18s %8.1f   %7.1f   %6.1f      %8s      %4d/%-4u\n",
               i + 1, PlantData_GetSpecies(result->species)->name_pt,
               plant_hp_score(result), boss_hp_score(result),
               average(result->rounds, result->battles),
               turns_to_win, result->wins, DUEL_SEEDS);
    }
}

int main(void)
{
    const SpeciesId bosses[] = {SPECIES_SUNFLOWER, SPECIES_ALOE};
    int failures = 0;

    for (size_t b = 0; b < sizeof(bosses) / sizeof(bosses[0]); b++) {
        MatchupResult results[STARTER_SPECIES_COUNT];
        MatchupResult sorted[STARTER_SPECIES_COUNT];
        for (int i = 0; i < STARTER_SPECIES_COUNT; i++) {
            results[i] = (MatchupResult){0};
            results[i].species = (SpeciesId)PlantData_GetStarter(i)->id;
            for (unsigned int seed = 1; seed <= DUEL_SEEDS; seed++)
                simulate(results[i].species, bosses[b], seed, &results[i]);
        }

        const MatchupResult *best_plant_hp = best_result(results, STARTER_SPECIES_COUNT, better_plant_hp);
        const MatchupResult *best_boss_hp = best_result(results, STARTER_SPECIES_COUNT, better_boss_hp);
        const MatchupResult *best_turns = best_result(results, STARTER_SPECIES_COUNT, better_turns);
        memcpy(sorted, results, sizeof(results));
        sort_results(sorted, STARTER_SPECIES_COUNT, better_matchup);

        const char *boss_name = PlantData_GetSpecies(bosses[b])->name_pt;
        printf("\n=== %s ===\n", boss_name);
        print_table(sorted, boss_name);
        printf("Mais HP da planta: %s (%.1f). Menos HP do boss: %s (%.1f). "
               "Menos turnos por vitoria: %s (%.1f).\n",
               PlantData_GetSpecies(best_plant_hp->species)->name_pt, plant_hp_score(best_plant_hp),
               PlantData_GetSpecies(best_boss_hp->species)->name_pt, boss_hp_score(best_boss_hp),
               PlantData_GetSpecies(best_turns->species)->name_pt, turns_to_win_score(best_turns));

        for (int i = 0; i < STARTER_SPECIES_COUNT; i++)
            if (results[i].stalls || results[i].invalid) failures++;
    }

    if (failures) {
        fputs("FAIL: houve combate travado ou acao invalida na simulacao.\n", stderr);
        return 1;
    }
    return 0;
}
