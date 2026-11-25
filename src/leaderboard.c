/* Guarda recordes em CSV simples e produz as duas ordens usadas nos rankings. */
#include "leaderboard.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#define leaderboard_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define leaderboard_mkdir(path) mkdir(path, 0755)
#endif

static bool ensure_parent_directory(const char *path)
{
    size_t length = strlen(path);
    size_t separator = length;
    for (size_t i = 0; i < length; i++)
        if (path[i] == '/' || path[i] == '\\') separator = i;
    if (separator == length || separator == 0 || separator >= 1024) return true;

    char parent[1024];
    memcpy(parent, path, separator);
    parent[separator] = '\0';
    if (leaderboard_mkdir(parent) == 0) return true;
    return errno == EEXIST;
}

static void sanitize_name(char *output, size_t capacity, const char *name)
{
    while (*name == ' ') name++;
    size_t length = strlen(name);
    while (length && name[length - 1] == ' ') length--;
    if (length >= capacity) length = capacity - 1;

    size_t written = 0;
    for (size_t i = 0; i < length; i++) {
        char character = name[i];
        if (character == ',' || character == '"' || character == '\r' || character == '\n')
            character = '?';
        output[written++] = character;
    }
    output[written] = '\0';
}

static bool current_date(char *output, size_t capacity)
{
    time_t now = time(NULL);
    if (now == (time_t)-1) return false;
    struct tm local;
#if defined(_WIN32)
    if (localtime_s(&local, &now) != 0) return false;
#else
    if (!localtime_r(&now, &local)) return false;
#endif
    return strftime(output, capacity, "%Y-%m-%d", &local) > 0;
}

static char *next_field(char **cursor)
{
    char *start = *cursor;
    if (!start) return NULL;
    char *comma = strchr(start, ',');
    if (!comma) {
        *cursor = NULL;
        return start;
    }
    *comma = '\0';
    *cursor = comma + 1;
    return start;
}

static bool parse_nonnegative(const char *text, long long *value)
{
    while (*text == ' ' || *text == '\t') text++;
    errno = 0;
    char *end = NULL;
    long long parsed = strtoll(text, &end, 10);
    while (*end == ' ' || *end == '\t') end++;
    if (errno != 0 || end == text || *end != '\0' || parsed < 0) return false;
    *value = parsed;
    return true;
}

void leaderboard_load_from(const char *path, Leaderboard *board)
{
    if (!board) return;
    *board = (Leaderboard){0};
    if (!path) return;

    FILE *file = fopen(path, "r");
    if (!file) return;
    char line[512];
    while (fgets(line, sizeof(line), file) && board->count < LEADERBOARD_MAX_ENTRIES) {
        size_t length = strlen(line);
        while (length && (line[length - 1] == '\n' || line[length - 1] == '\r'))
            line[--length] = '\0';
        if (!length) continue;

        char *cursor = line;
        char *name = next_field(&cursor);
        char *time_text = next_field(&cursor);
        char *date = next_field(&cursor);
        char *damage_text = next_field(&cursor);
        if (!name || !time_text || !date || !damage_text || cursor) continue;
        if (!*name || strcmp(name, "name") == 0) continue;
        if (strlen(name) >= LEADERBOARD_NAME_CAPACITY ||
            strlen(date) >= LEADERBOARD_DATE_CAPACITY) continue;

        long long time_ms = 0, damage = 0;
        if (!parse_nonnegative(time_text, &time_ms) || !parse_nonnegative(damage_text, &damage))
            continue;

        LeaderboardEntry *entry = &board->entries[board->count];
        strcpy(entry->name, name);
        entry->time_ms = time_ms;
        strcpy(entry->date, date);
        entry->damage = damage;
        board->count++;
    }
    fclose(file);
}

void leaderboard_load(Leaderboard *board)
{
    leaderboard_load_from(LEADERBOARD_DEFAULT_PATH, board);
}

bool leaderboard_add_to(const char *path, const char *name, long long time_ms, long long damage)
{
    if (!path || !name || time_ms < 0 || damage < 0) return false;
    if (!ensure_parent_directory(path)) return false;

    char safe_name[LEADERBOARD_NAME_CAPACITY];
    sanitize_name(safe_name, sizeof(safe_name), name);
    if (!safe_name[0]) return false;

    char date[LEADERBOARD_DATE_CAPACITY];
    if (!current_date(date, sizeof(date))) return false;

    FILE *file = fopen(path, "a");
    if (!file) return false;
    if (ftell(file) == 0) fprintf(file, "name,time_ms,date,damage\n");
    int written = fprintf(file, "%s,%lld,%s,%lld\n", safe_name, time_ms, date, damage);
    bool ok = written > 0;
    if (fclose(file) != 0) ok = false;
    return ok;
}

bool leaderboard_add(const char *name, long long time_ms, long long damage)
{
    return leaderboard_add_to(LEADERBOARD_DEFAULT_PATH, name, time_ms, damage);
}

static bool comes_before(const LeaderboardEntry *candidate, const LeaderboardEntry *current,
                         LeaderboardOrder order)
{
    if (order == LEADERBOARD_ORDER_TIME) {
        if (candidate->time_ms != current->time_ms) return candidate->time_ms < current->time_ms;
        if (candidate->damage != current->damage) return candidate->damage < current->damage;
    } else {
        if (candidate->damage != current->damage) return candidate->damage < current->damage;
        if (candidate->time_ms != current->time_ms) return candidate->time_ms < current->time_ms;
    }
    int date_order = strcmp(candidate->date, current->date);
    if (date_order != 0) return date_order < 0;
    return strcmp(candidate->name, current->name) < 0;
}

void leaderboard_sort(const Leaderboard *board, LeaderboardOrder order,
                      LeaderboardEntry output[LEADERBOARD_MAX_ENTRIES], int *count)
{
    if (!count) return;
    *count = 0;
    if (!board || !output) return;

    int total = board->count;
    if (total < 0) total = 0;
    if (total > LEADERBOARD_MAX_ENTRIES) total = LEADERBOARD_MAX_ENTRIES;
    for (int i = 0; i < total; i++) output[i] = board->entries[i];

    for (int i = 1; i < total; i++) {
        LeaderboardEntry key = output[i];
        int j = i - 1;
        while (j >= 0 && comes_before(&key, &output[j], order)) {
            output[j + 1] = output[j];
            j--;
        }
        output[j + 1] = key;
    }
    *count = total;
}

void leaderboard_format_time(long long time_ms, char *output, size_t capacity)
{
    if (!output || capacity == 0) return;
    if (time_ms < 0) time_ms = 0;
    long long minutes = time_ms / 60000;
    long long seconds = (time_ms / 1000) % 60;
    long long millis = time_ms % 1000;
    snprintf(output, capacity, "%lld:%02lld:%03lld", minutes, seconds, millis);
}
