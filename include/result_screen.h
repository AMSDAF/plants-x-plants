/* Apresenta conclusão da estufa e derrota, sem modificar a campanha. */
#ifndef RESULT_SCREEN_H
#define RESULT_SCREEN_H
#include "intro_screen.h"

typedef enum { RESULT_NONE, RESULT_PRIMARY, RESULT_SECONDARY, RESULT_CORRECT, RESULT_WRONG } ResultAction;
typedef struct {
    int cursor;
    float elapsed;
    bool quiz_open;
    bool campaign_lost;
    int question_index;
} ResultScreen;
void result_screen_begin(ResultScreen *screen);
void result_screen_begin_quiz(ResultScreen *screen, unsigned int *seen_mask);
ResultAction result_screen_update(ResultScreen *screen, bool accept_input, float dt);
Rectangle result_screen_button_bounds(int index);
void result_screen_draw(const UiTheme *ui, const SelectionUiAssets *fonts,
                        const TrainerBackgroundAssets *background, const IntroAssets *plants,
                        const ResultScreen *screen, const Team *team, const char *player_name,
                        bool defeated, const char *location);
#endif
