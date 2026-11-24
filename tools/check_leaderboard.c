/* Verifica persistência CSV, ordenação e formatação dos recordes sem raylib. */
#include "leaderboard.h"
#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define check_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define check_mkdir(path) mkdir(path, 0755)
#endif

static bool make_directory(const char *path)
{
    if (check_mkdir(path) == 0) return true;
    return errno == EEXIST;
}

static void check_time_format(void)
{
    char text[32];
    leaderboard_format_time(0, text, sizeof(text));
    assert(strcmp(text, "0:00:000") == 0);
    leaderboard_format_time(45230, text, sizeof(text));
    assert(strcmp(text, "0:45:230") == 0);
    leaderboard_format_time(90061, text, sizeof(text));
    assert(strcmp(text, "1:30:061") == 0);
    leaderboard_format_time(-5, text, sizeof(text));
    assert(strcmp(text, "0:00:000") == 0);
}

static void check_date(const char *date)
{
    assert(strlen(date) == 10);
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) assert(date[i] == '-');
        else assert(date[i] >= '0' && date[i] <= '9');
    }
}

int main(void)
{
    const char *path = "build/leaderboard_check/data/leaderboard.csv";
    assert(make_directory("build"));
    assert(make_directory("build/leaderboard_check"));
    assert(make_directory("build/leaderboard_check/data"));
    remove(path);

    Leaderboard board;
    leaderboard_load_from(path, &board);
    assert(board.count == 0);

    assert(leaderboard_add_to(path, "Ana", 90061, 120));
    assert(leaderboard_add_to(path, "Bia", 45230, 80));
    assert(leaderboard_add_to(path, "João", 45230, 40));
    leaderboard_load_from(path, &board);
    assert(board.count == 3);
    assert(strcmp(board.entries[0].name, "Ana") == 0);
    assert(board.entries[0].time_ms == 90061 && board.entries[0].damage == 120);
    check_date(board.entries[0].date);

    LeaderboardEntry ranked[LEADERBOARD_MAX_ENTRIES];
    int count = 0;
    leaderboard_sort(&board, LEADERBOARD_ORDER_TIME, ranked, &count);
    assert(count == 3);
    assert(strcmp(ranked[0].name, "João") == 0);
    assert(strcmp(ranked[1].name, "Bia") == 0);
    assert(strcmp(ranked[2].name, "Ana") == 0);

    leaderboard_sort(&board, LEADERBOARD_ORDER_DAMAGE, ranked, &count);
    assert(count == 3);
    assert(strcmp(ranked[0].name, "João") == 0 && ranked[0].damage == 40);
    assert(strcmp(ranked[1].name, "Bia") == 0 && ranked[1].damage == 80);
    assert(strcmp(ranked[2].name, "Ana") == 0 && ranked[2].damage == 120);

    /* Nomes vindos da interface não contêm vírgulas, mas o arquivo as neutraliza. */
    assert(leaderboard_add_to(path, "A,B\nC", 1234, 5));
    FILE *file = fopen(path, "a");
    assert(file);
    fprintf(file, "linha,invalida\n");
    fclose(file);
    leaderboard_load_from(path, &board);
    assert(board.count == 4);
    assert(strcmp(board.entries[3].name, "A?B?C") == 0);

    check_time_format();
    assert(remove(path) == 0);
    puts("PASS: CSV, data, duas ordenações, empates, nomes seguros e min:seg:miliseg.");
    return 0;
}
