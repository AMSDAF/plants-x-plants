/* Apresenta o pedido de ajuda, a equipe inicial e a viagem até a vila. */
#ifndef INTRO_SCREEN_H
#define INTRO_SCREEN_H

#include "trainer_select_screen.h"
#include "plant.h"

#define INTRO_STARTER_COUNT 6
#define INTRO_TEAM_SIZE 3

typedef enum { INTRO_LETTER, INTRO_ACCEPT, INTRO_STARTERS, INTRO_TRAVEL } IntroStage;
typedef enum { INTRO_NONE, INTRO_BACK, INTRO_FINISH } IntroAction;

typedef struct {
    IntroStage stage;
    int cursor;
    int selected[INTRO_TEAM_SIZE];
    int count;
    float elapsed;
    float reveal;
    bool input_blocked;
} IntroScreen;

void intro_screen_init(IntroScreen *screen);
IntroAction intro_screen_update(IntroScreen *screen, bool accept_input, float dt);
bool intro_screen_team(const IntroScreen *screen, Team *team);
void intro_screen_draw(const UiTheme *ui, const IntroScreen *screen,
                       const TrainerSelectAssets *assets, const char *player_name);

#endif
