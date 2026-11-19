/* Percorre a abertura nos testes anteriores de menu, nome e exploração. */
#ifndef CHECK_INTRO_FLOW_H
#define CHECK_INTRO_FLOW_H

#include "game.h"
#include <assert.h>

static void check_complete_intro(Game *game, void (*press)(Game *, int))
{
    assert(game->screen == GAME_INTRO);
    for (int step = 0; step < 12 && game->screen == GAME_INTRO; step++) {
        if (game->intro.stage == INTRO_STARTERS) {
            press(game, KEY_SPACE);
            press(game, KEY_RIGHT);
            press(game, KEY_SPACE);
            press(game, KEY_RIGHT);
            press(game, KEY_SPACE);
        }
        press(game, KEY_ENTER);
    }
    assert(game->screen == GAME_EXPLORATION && game->campaign.team.count == INTRO_TEAM_SIZE);
}

#endif
