/* Simula duelos, equipes e bosses pelo motor real e publica métricas reproduzíveis. */
#include "battle.h"
#include "plant_data.h"
#include "ai.h"
#include <stdio.h>
#include <stdlib.h>

#define ROUND_LIMIT 300

typedef enum { POLICY_DAMAGE, POLICY_AI, POLICY_FIRST, POLICY_LOGIC } Policy;

typedef struct {
    int total, wins, losses, capped, errors, blocked, rounds, survivors, rejected_steps;
    double winner_hp, loser_enemy_hp;
    int histogram[ROUND_LIMIT + 1];
    long selections[SPECIES_COUNT][MAX_MOVES];
} Metrics;

static const char *policy_name(Policy policy)
{
    static const char *names[] = {"damage", "ai", "first", "logic"};
    return names[policy];
}

static Team make_team(const int *ids, int count)
{
    Team team = {0};
    for (int i = 0; i < count; i++) Team_SelectSpecies(&team, i, ids[i]);
    return team;
}

static Team *side_team(BattleState *battle, int side)
{
    return side == 0 ? &battle->player_team : &battle->enemy_team;
}

static int best_damage(const BattleState *battle, int side, int first)
{
    const Team *team = side == 0 ? &battle->player_team : &battle->enemy_team;
    const Plant *plant = &team->plants[team->active_index];
    int best = -1;
    double best_score = -1;
    for (int i = 0; i < MAX_MOVES; i++) {
        const Move *move = &plant->moves[i];
        if (move->effect != MOVE_EFFECT_DAMAGE ||
            !Battle_CanUseMove(battle, side, team->active_index, i)) continue;
        double score = Battle_GetMovePower(battle, side, i) * move->accuracy *
            (1.0 + Battle_GetCriticalChance(plant, move) / 200.0);
        if (score > best_score) { best = i; best_score = score; }
        if (first) break;
    }
    return best;
}

static BattleAction choose_action(BattleState *battle, int side, Policy policy,
                                   Metrics *metrics)
{
    int index = -1;
    BattleAction action = {BATTLE_ACTION_MOVE, -1};
    if (policy == POLICY_AI) {
        AIActionType type = AI_GetActionForSide(battle, side, &index);
        if (type == AI_ACTION_SWITCH) action.type = BATTLE_ACTION_SWITCH;
        else if (type != AI_ACTION_MOVE) return action;
    } else {
        index = best_damage(battle, side, policy == POLICY_FIRST);
        /* Um golpe lógico bloqueado e zero frutos podem deixar só um buff válido. */
        if (index < 0) {
            AIActionType type = AI_GetActionForSide(battle, side, &index);
            if (type == AI_ACTION_SWITCH) action.type = BATTLE_ACTION_SWITCH;
        }
    }
    action.index = index;
    Team *team = side_team(battle, side);
    if (action.type == BATTLE_ACTION_MOVE && index >= 0 && index < MAX_MOVES)
        metrics->selections[team->plants[team->active_index].id][index]++;
    return action;
}

static void configure_logic(BattleState *battle)
{
    /* Cenário opcional: cada planta recebeu um golpe lógico com esta fórmula. */
    for (int i = 0; i < battle->player_team.count; i++) {
        int old_active = battle->player_team.active_index;
        battle->player_team.active_index = i;
        int move = best_damage(battle, 0, 0);
        battle->player_team.active_index = old_active;
        battle->formulas[0][i] = (LogicFormula){
            LOGIC_OWN_LOW_HP, LOGIC_OWN_RESERVE, LOGIC_OR};
        battle->boosted_move_indices[0][i] = move;
    }
}

static void record_result(Metrics *metrics, const BattleState *battle, int rounds,
                          int error)
{
    metrics->total++;
    metrics->rounds += rounds;
    metrics->histogram[rounds]++;
    if (error == 2) { metrics->blocked++; return; }
    if (error) { metrics->errors++; return; }
    if (battle->state == BATTLE_STATE_PLAYER_WIN) {
        metrics->wins++;
        double remaining = 0, maximum = 0;
        for (int i = 0; i < battle->player_team.count; i++) {
            const Plant *plant = &battle->player_team.plants[i];
            metrics->survivors += Plant_IsAlive(plant);
            remaining += plant->current_hp;
            maximum += plant->max_hp;
        }
        metrics->winner_hp += remaining / maximum;
    } else if (battle->state == BATTLE_STATE_ENEMY_WIN) {
        metrics->losses++;
        double remaining = 0, maximum = 0;
        for (int i = 0; i < battle->enemy_team.count; i++) {
            remaining += battle->enemy_team.plants[i].current_hp;
            maximum += battle->enemy_team.plants[i].max_hp;
        }
        metrics->loser_enemy_hp += remaining / maximum;
    } else metrics->capped++;
}

static void simulate(Team player, Team enemy, Policy player_policy, Policy enemy_policy,
                     unsigned int seed, Metrics *metrics)
{
    srand(seed);
    BattleState battle;
    Battle_Init(&battle, player, enemy);
    if (player_policy == POLICY_LOGIC) configure_logic(&battle);
    int rounds = 0, error = 0, events = 0;
    while (rounds < ROUND_LIMIT && events++ < ROUND_LIMIT * 3) {
        if (battle.state == BATTLE_STATE_PLAYER_WIN ||
            battle.state == BATTLE_STATE_ENEMY_WIN) break;
        if (battle.state == BATTLE_STATE_REPLACEMENT) {
            int index = -1;
            Team *team = side_team(&battle, battle.current_turn);
            for (int i = 0; i < team->count; i++)
                if (Battle_CanSideSwitch(&battle, battle.current_turn, i)) {
                    index = i; break;
                }
            if (index < 0 || !Battle_SwitchPlant(&battle, index)) { error = 1; break; }
            continue;
        }
        BattleAction left = choose_action(&battle, 0, player_policy, metrics);
        BattleAction right = choose_action(&battle, 1, enemy_policy, metrics);
        if (left.index < 0 || right.index < 0) {
            /* Não inventa uma espera quando o jogo não oferece nenhuma ação válida. */
            if (getenv("BALANCE_TRACE"))
                fprintf(stderr, "Sem acao: seed=%u equipe=%d/%d/%d boss=%d rodada=%d frutos=%d hp=%d buff=%d\n",
                    seed, player.plants[0].id, player.plants[1].id, player.plants[2].id,
                    enemy.plants[0].id, rounds,
                    battle.player_team.plants[battle.player_team.active_index].fruit_count,
                    battle.player_team.plants[battle.player_team.active_index].current_hp,
                    battle.player_team.plants[battle.player_team.active_index].attack);
            error = 2; break;
        }
        BattleRound round;
        int valid = Battle_PrepareRound(&battle, left, right, &round);
        if (valid) {
            for (int step = 0; step < 2; step++) {
                if (!Battle_ExecuteRoundStep(&battle, &round)) {
                    /* A tela cancela a ação e ainda finaliza a rodada (recuo/nocaute). */
                    metrics->rejected_steps++;
                    break;
                }
            }
            valid = Battle_FinishRound(&battle, &round);
        }
        if (!valid) {
            if (getenv("BALANCE_TRACE"))
                fprintf(stderr, "seed=%u round=%d state=%d turn=%d player=%d:%d hp=%d enemy=%d:%d hp=%d msg=%s\n",
                    seed, rounds, battle.state, battle.current_turn,
                    battle.player_team.plants[battle.player_team.active_index].id, left.index,
                    battle.player_team.plants[battle.player_team.active_index].current_hp,
                    battle.enemy_team.plants[battle.enemy_team.active_index].id, right.index,
                    battle.enemy_team.plants[battle.enemy_team.active_index].current_hp,
                    battle.last_message);
            error = 1; break;
        }
        rounds++;
    }
    record_result(metrics, &battle, rounds, error);
}

static void print_ids(const int *ids, int count)
{
    putchar('[');
    for (int i = 0; i < count; i++) printf("%s%d", i ? "," : "", ids[i]);
    putchar(']');
}

static void print_metrics(const char *suite, Policy policy, const int *left, int lc,
                           const int *right, int rc, const Metrics *metrics)
{
    int cumulative = 0, p95 = 0;
    for (int i = 0; i <= ROUND_LIMIT; i++) {
        cumulative += metrics->histogram[i];
        if (cumulative * 100 >= metrics->total * 95) { p95 = i; break; }
    }
    printf("{\"suite\":\"%s\",\"policy\":\"%s\",\"left\":", suite, policy_name(policy));
    print_ids(left, lc);
    printf(",\"right\":"); print_ids(right, rc);
    printf(",\"n\":%d,\"wins\":%d,\"losses\":%d,\"capped\":%d,\"errors\":%d,"
           "\"blocked\":%d,\"rounds\":%d,\"p95\":%d,\"survivors\":%d,\"rejected_steps\":%d,\"winner_hp_sum\":%.8f,"
           "\"loser_enemy_hp_sum\":%.8f,\"selections\":[",
           metrics->total, metrics->wins, metrics->losses, metrics->capped, metrics->errors,
           metrics->blocked, metrics->rounds, p95, metrics->survivors, metrics->rejected_steps, metrics->winner_hp,
           metrics->loser_enemy_hp);
    for (int id = 0; id < SPECIES_COUNT; id++) {
        printf("%s[%ld,%ld,%ld]", id ? "," : "", metrics->selections[id][0],
               metrics->selections[id][1], metrics->selections[id][2]);
    }
    puts("]}");
}

static Team permuted_team(const int *ids, int permutation)
{
    static const int orders[6][3] = {
        {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}};
    int ordered[3];
    for (int i = 0; i < 3; i++) ordered[i] = ids[orders[permutation % 6][i]];
    return make_team(ordered, 3);
}

int main(int argc, char **argv)
{
    int samples = argc > 1 ? atoi(argv[1]) : 1080;
    if (samples < 10 || samples > 100000) {
        fputs("Use entre 10 e 100000 sementes por confronto.\n", stderr);
        return 1;
    }
    int starters[STARTER_SPECIES_COUNT], teams[20][3], team_count = 0;
    for (int i = 0; i < STARTER_SPECIES_COUNT; i++) starters[i] = PlantData_GetStarter(i)->id;
    for (int a = 0; a < 6; a++) for (int b = a+1; b < 6; b++)
        for (int c = b+1; c < 6; c++) {
            teams[team_count][0] = starters[a];
            teams[team_count][1] = starters[b];
            teams[team_count++][2] = starters[c];
        }
    for (Policy policy = POLICY_DAMAGE; policy <= POLICY_AI; policy++) {
        for (int a = 0; a < 6; a++) for (int b = 0; b < 6; b++) {
            Metrics metrics = {0};
            for (int seed = 1; seed <= samples; seed++)
                simulate(make_team(&starters[a], 1), make_team(&starters[b], 1),
                         policy, policy, (unsigned int)seed, &metrics);
            print_metrics("duel", policy, &starters[a], 1, &starters[b], 1, &metrics);
        }
        for (int a = 0; a < team_count; a++) for (int b = 0; b < team_count; b++) {
            Metrics metrics = {0};
            for (int seed = 1; seed <= samples/10; seed++) {
                /* Ordens independentes, com as seis permutações representadas. */
                int permutation = (seed-1) % 36;
                simulate(permuted_team(teams[a], permutation % 6),
                         permuted_team(teams[b], permutation / 6), policy, policy,
                         (unsigned int)seed, &metrics);
            }
            print_metrics("team", policy, teams[a], 3, teams[b], 3, &metrics);
        }
        fprintf(stderr, "Duelos e equipes: politica %s concluida.\n", policy_name(policy));
    }
    for (Policy policy = POLICY_DAMAGE; policy <= POLICY_LOGIC; policy++) {
        for (int boss = SPECIES_SUNFLOWER; boss <= SPECIES_ALOE; boss++)
            for (int t = 0; t < team_count; t++) for (int order = 0; order < 6; order++) {
                Metrics metrics = {0};
                Team team = permuted_team(teams[t], order);
                int ids[3];
                for (int i = 0; i < 3; i++) ids[i] = team.plants[i].id;
                for (int seed = 1; seed <= samples; seed++)
                    simulate(team, make_team(&boss, 1), policy, POLICY_AI,
                             (unsigned int)seed, &metrics);
                print_metrics("boss", policy, ids, 3, &boss, 1, &metrics);
            }
        fprintf(stderr, "Bosses: politica %s concluida.\n", policy_name(policy));
    }
    return ferror(stdout) ? 1 : 0;
}
