/* Apresenta conclusão da estufa e derrota, sem modificar a campanha. */
#ifndef RESULT_SCREEN_H
#define RESULT_SCREEN_H
#include "intro_screen.h"

typedef enum { RESULT_NONE, RESULT_PRIMARY, RESULT_SECONDARY } ResultAction;
typedef struct { int cursor; float elapsed; } ResultScreen;
void result_screen_begin(ResultScreen *screen);
ResultAction result_screen_update(ResultScreen *screen, bool accept_input, float dt);
Rectangle result_screen_button_bounds(int index);
void result_screen_draw(const UiTheme *ui, const SelectionUiAssets *fonts,
                        const TrainerBackgroundAssets *background, const IntroAssets *plants,
                        const ResultScreen *screen, const Team *team, const char *player_name,
                        bool defeated, const char *location);
#endif
