/* Apresenta um ranking por vez e devolve jogar/voltar, com navegação por setas. */
#ifndef LEADERBOARD_SCREEN_H
#define LEADERBOARD_SCREEN_H

#include "leaderboard.h"
#include "selection_ui.h"
#include "trainer_background.h"

typedef enum {
    LEADERBOARD_SCREEN_NONE, LEADERBOARD_SCREEN_PLAY, LEADERBOARD_SCREEN_BACK
} LeaderboardScreenAction;

typedef struct {
    LeaderboardEntry time_entries[LEADERBOARD_MAX_ENTRIES];
    int time_count;
    LeaderboardEntry damage_entries[LEADERBOARD_MAX_ENTRIES];
    int damage_count;
    LeaderboardOrder order;
    int selected[2];
    int scroll[2];
    int pressed;
    float elapsed;
} LeaderboardScreen;

void leaderboard_screen_begin(LeaderboardScreen *screen);
LeaderboardScreenAction leaderboard_screen_update(LeaderboardScreen *screen, bool accept_input, float dt);
Rectangle leaderboard_screen_tab_bounds(int index);
Rectangle leaderboard_screen_row_bounds(int row);
Rectangle leaderboard_screen_button_bounds(int index);
void leaderboard_screen_draw(const UiTheme *ui, const SelectionUiAssets *assets,
                             const TrainerBackgroundAssets *background,
                             const LeaderboardScreen *screen);

#endif /* LEADERBOARD_SCREEN_H */
