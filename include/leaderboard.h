/* Persiste e ordena os recordes da campanha em CSV, sem depender da interface. */
#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <stdbool.h>
#include <stddef.h>

#define LEADERBOARD_DEFAULT_PATH "data/leaderboard.csv"
#define LEADERBOARD_NAME_CAPACITY 64
#define LEADERBOARD_DATE_CAPACITY 16
#define LEADERBOARD_MAX_ENTRIES 256
#define LEADERBOARD_VISIBLE_ROWS 10

typedef struct {
    char name[LEADERBOARD_NAME_CAPACITY];
    long long time_ms;
    char date[LEADERBOARD_DATE_CAPACITY];
    long long damage;
} LeaderboardEntry;

typedef struct {
    LeaderboardEntry entries[LEADERBOARD_MAX_ENTRIES];
    int count;
} Leaderboard;

typedef enum { LEADERBOARD_ORDER_TIME, LEADERBOARD_ORDER_DAMAGE } LeaderboardOrder;

/* As versões com caminho permitem isolar os testes sem tocar no arquivo do jogo. */
void leaderboard_load(Leaderboard *board);
void leaderboard_load_from(const char *path, Leaderboard *board);
bool leaderboard_add(const char *name, long long time_ms, long long damage);
bool leaderboard_add_to(const char *path, const char *name, long long time_ms, long long damage);

void leaderboard_sort(const Leaderboard *board, LeaderboardOrder order,
                      LeaderboardEntry output[LEADERBOARD_MAX_ENTRIES], int *count);
void leaderboard_format_time(long long time_ms, char *output, size_t capacity);

#endif /* LEADERBOARD_H */
