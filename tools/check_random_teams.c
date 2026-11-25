/* Estima a chance de vitoria de equipes aleatorias contra cada boss, com e sem buff logico. */
#include "ai.h"
#include "battle.h"
#include "plant_data.h"
#include <stdio.h>
#include <stdlib.h>

#define TEAM_TRIALS 2000u
#define SEEDS_PER_TEAM 5u
#define MAX_ROUNDS 200

typedef enum { POLICY_GREEDY, POLICY_TACTICAL, POLICY_LOGIC, POLICY_COUNT } PlayerPolicy;

typedef struct {
    int battles;
    int wins;
    int stalls;
    int invalid;
    int invalid_replacement;
    int invalid_action;
    long long rounds;
    long long survivors;
} TrialStats;

/* Condições estáveis: não mudam quando o boss age antes da planta do jogador. */
static const LogicCondition LOGIC_PAIRS[][2] = {
    {LOGIC_OWN_HIGHER_ATTACK, LOGIC_OWN_HIGHER_DEFENSE},
    {LOGIC_OWN_HIGHER_ATTACK, LOGIC_OWN_RESERVE},
    {LOGIC_OWN_HIGHER_DEFENSE, LOGIC_OWN_RESERVE}
};
static const LogicOperator LOGIC_OPERATORS[] = {LOGIC_OR};

static const char *policy_name(PlayerPolicy policy)
{
    if (policy == POLICY_GREEDY) return "Maior dano";
    if (policy == POLICY_TACTICAL) return "Tatico (buff/debuff)";
    return "Tatico + logico";
}

static unsigned int next_random(unsigned int *state)
{
    unsigned int value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static void random_team(unsigned int *rng, SpeciesId team[3])
{
    int used[STARTER_SPECIES_COUNT] = {0};
    for (int i = 0; i < 3; i++) {
        int pick = 0;
        do {
            pick = (int)(next_random(rng) % STARTER_SPECIES_COUNT);
        } while (used[pick]);
        used[pick] = 1;
        team[i] = (SpeciesId)PlantData_GetStarter(pick)->id;
    }
}

/* No início da batalha todas as plantas estão saudáveis e sem status. */
static int condition_true(const BattleState *battle, int plant_index, LogicCondition condition)
{
    LogicCondition anchor = condition == LOGIC_OWN_LOW_HP ? LOGIC_ENEMY_STATUS
                                                         : LOGIC_OWN_LOW_HP;
    LogicFormula formula = {condition, anchor, LOGIC_OR};
    return Battle_EvaluatePlantFormula(battle, 0, plant_index, formula).result;
}

static double estimated_uptime(int left, int right, LogicOperator op)
{
    if (op == LOGIC_AND) return left && right ? 1.0 : 0.0;
    if (op == LOGIC_OR) return left || right ? 1.0 : 0.0;
    if (op == LOGIC_XOR) return left != right ? 1.0 : 0.0;
    return 0.0;
}

static int best_damage_move(const BattleState *battle, int plant_index)
{
    const Plant *plant = &battle->player_team.plants[plant_index];
    int best = -1;
    double best_score = -1.0;
    for (int i = 0; i < MAX_MOVES; i++) {
        const Move *move = &plant->moves[i];
        if (move->effect != MOVE_EFFECT_DAMAGE) continue;
        double score = (double)move->power * move->accuracy *
                       (1.0 + Battle_GetCriticalChance(plant, move) / 200.0);
        if (score > best_score) {
            best = i;
            best_score = score;
        }
    }
    return best;
}

/* Escolhe a fórmula e o golpe fortalecido a partir dos atributos da equipe e do boss. */
static void configure_logic(BattleState *battle, const SpeciesId team[3])
{
    (void)team;
    for (int i = 0; i < battle->player_team.count; i++) {
        double best_value = -1.0;
        LogicFormula best_formula = {0};
        for (size_t p = 0; p < sizeof(LOGIC_PAIRS) / sizeof(LOGIC_PAIRS[0]); p++) {
            int left = condition_true(battle, i, LOGIC_PAIRS[p][0]);
            int right = condition_true(battle, i, LOGIC_PAIRS[p][1]);
            for (size_t o = 0; o < sizeof(LOGIC_OPERATORS) / sizeof(LOGIC_OPERATORS[0]); o++) {
                LogicOperator op = LOGIC_OPERATORS[o];
                double value = estimated_uptime(left, right, op) *
                               Battle_GetFormulaPowerBonus(op);
                /* OR com reserva viva mantém o golpe fortalecido por mais tempo. */
                if (LOGIC_PAIRS[p][0] == LOGIC_OWN_RESERVE && left) value += 1.0;
                if (LOGIC_PAIRS[p][1] == LOGIC_OWN_RESERVE && right) value += 1.0;
                if (value > best_value) {
                    best_value = value;
                    best_formula = (LogicFormula){LOGIC_PAIRS[p][0], LOGIC_PAIRS[p][1], op};
                }
            }
        }
        battle->formulas[0][i] = best_formula;
        battle->boosted_move_indices[0][i] = best_damage_move(battle, i);
    }
}

static BattleAction player_action(const BattleState *battle, PlayerPolicy policy)
{
    const Team *team = &battle->player_team;
    const Plant *active = &team->plants[team->active_index];
    const Plant *enemy = &battle->enemy_team.plants[battle->enemy_team.active_index];

    /* Politica tatica: usa buff/debuff enquanto as duas plantas estao saudaveis. */
    if (policy != POLICY_GREEDY && active && enemy &&
        active->current_hp * 10 > active->max_hp * 7 &&
        enemy->current_hp * 10 > enemy->max_hp * 3) {
        for (int i = 0; i < MAX_MOVES; i++) {
            if (active->moves[i].effect == MOVE_EFFECT_DAMAGE) continue;
            if (Battle_CanUseMove(battle, 0, team->active_index, i))
                return (BattleAction){BATTLE_ACTION_MOVE, i};
        }
    }

    int best = -1;
    double best_score = -1.0;
    for (int i = 0; i < MAX_MOVES; i++) {
        const Move *move = &active->moves[i];
        if (move->effect != MOVE_EFFECT_DAMAGE ||
            !Battle_CanUseMove(battle, 0, team->active_index, i)) continue;
        /* Com buff lógico, o poder efetivo depende da fórmula e do golpe fortalecido. */
        int power = Battle_GetPlantMovePower(battle, 0, team->active_index, i);
        double score = (double)power * move->accuracy *
                       (1.0 + Battle_GetCriticalChance(active, move) / 200.0);
        if (score > best_score) {
            best = i;
            best_score = score;
        }
    }
    if (best < 0) return (BattleAction){BATTLE_ACTION_NONE, -1};
    return (BattleAction){BATTLE_ACTION_MOVE, best};
}

static int choose_reserve(const BattleState *battle)
{
    int best = -1, best_hp = -1;
    for (int i = 0; i < battle->player_team.count; i++) {
        if (!Battle_CanSwitch(battle, i)) continue;
        int hp = battle->player_team.plants[i].current_hp;
        if (hp > best_hp) {
            best_hp = hp;
            best = i;
        }
    }
    return best;
}

static int simulate(const SpeciesId team[3], SpeciesId boss, unsigned int combat_seed,
                    PlayerPolicy policy, TrialStats *stats)
{
    srand(combat_seed);
    Team player = {0}, enemy = {0};
    for (int i = 0; i < 3; i++) Team_SelectSpecies(&player, i, team[i]);
    Team_SelectSpecies(&enemy, 0, boss);

    BattleState battle;
    Battle_Init(&battle, player, enemy);
    if (policy == POLICY_LOGIC) configure_logic(&battle, team);
    else for (int i = 0; i < MAX_TEAM_SIZE; i++) battle.boosted_move_indices[0][i] = -1;

    int rounds = 0;
    while (battle.state == BATTLE_STATE_ONGOING || battle.state == BATTLE_STATE_REPLACEMENT) {
        if (rounds >= MAX_ROUNDS) {
            stats->stalls++;
            return -1;
        }
        if (battle.state == BATTLE_STATE_REPLACEMENT) {
            if (battle.current_turn != 0) {
                stats->invalid++;
                stats->invalid_replacement++;
                return 0;
            }
            int reserve = choose_reserve(&battle);
            if (reserve < 0 || !Battle_SwitchPlant(&battle, reserve)) {
                stats->invalid++;
                stats->invalid_replacement++;
                return 0;
            }
            continue;
        }

        int index = -1;
        AIActionType enemy_type = AI_GetPlannedAction(&battle, &index);
        BattleAction enemy_action = {BATTLE_ACTION_NONE, -1};
        if (enemy_type == AI_ACTION_MOVE) enemy_action = (BattleAction){BATTLE_ACTION_MOVE, index};
        if (enemy_type == AI_ACTION_SWITCH) enemy_action = (BattleAction){BATTLE_ACTION_SWITCH, index};

        BattleAction action = player_action(&battle, policy);
        if (!Battle_ExecuteRound(&battle, action, enemy_action)) {
            stats->invalid++;
            stats->invalid_action++;
            return 0;
        }
        rounds++;
    }

    stats->battles++;
    stats->rounds += rounds;
    if (battle.state == BATTLE_STATE_PLAYER_WIN) {
        stats->wins++;
        for (int i = 0; i < player.count; i++)
            stats->survivors += Plant_IsAlive(&battle.player_team.plants[i]);
    }
    return 1;
}

int main(void)
{
    const SpeciesId bosses[] = {SPECIES_SUNFLOWER, SPECIES_ALOE};
    int failures = 0;

    for (size_t b = 0; b < sizeof(bosses) / sizeof(bosses[0]); b++) {
        TrialStats stats[POLICY_COUNT] = {{0}};
        unsigned int team_rng = 0x9E3779B9u ^ (unsigned int)bosses[b];

        for (unsigned int trial = 0; trial < TEAM_TRIALS; trial++) {
            SpeciesId team[3];
            random_team(&team_rng, team);
            for (unsigned int seed = 1; seed <= SEEDS_PER_TEAM; seed++) {
                unsigned int combat_seed = 0x1234567u + (unsigned int)bosses[b] * 7919u +
                                           trial * 131u + seed;
                for (int policy = 0; policy < POLICY_COUNT; policy++)
                    simulate(team, bosses[b], combat_seed, (PlayerPolicy)policy, &stats[policy]);
            }
        }

        printf("\n=== %s ===\n", PlantData_GetSpecies(bosses[b])->name_pt);
        printf("%u equipes aleatorias, %u sementes por equipe, %u duelos por politica.\n",
               TEAM_TRIALS, SEEDS_PER_TEAM, TEAM_TRIALS * SEEDS_PER_TEAM);
        printf("Politica                Vitorias       Chance   Turnos  Sobreviventes  Falhas\n");
        int recommended = 0;
        for (int policy = 0; policy < POLICY_COUNT; policy++) {
            const TrialStats *s = &stats[policy];
            printf("%-22s %6d/%-6d %7.2f%%   %5.1f      %5.2f       %d/%d/%d\n",
                   policy_name((PlayerPolicy)policy), s->wins, s->battles,
                   100.0 * s->wins / s->battles,
                   (double)s->rounds / s->battles,
                   (double)s->survivors / s->battles,
                   s->stalls, s->invalid_replacement, s->invalid_action);
            if (s->wins > stats[recommended].wins) recommended = policy;
            if (s->stalls || s->invalid) failures++;
        }
        printf("Politica recomendada: %s. (Falhas: travadas/troca/ação)\n", policy_name((PlayerPolicy)recommended));
    }

    if (failures) {
        fputs("FAIL: houve combate travado ou acao invalida na simulacao.\n", stderr);
        return 1;
    }
    return 0;
}
