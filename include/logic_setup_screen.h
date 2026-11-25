/* Define as abas, rascunhos e controles do desafio de golpes lógicos. */
#ifndef LOGIC_SETUP_SCREEN_H
#define LOGIC_SETUP_SCREEN_H

#include "battle.h"
#include "intro_screen.h"
#include "ui_input.h"

typedef struct {
    LogicFormula formulas[MAX_TEAM_SIZE];
    UiPointer pointer;
    int moves[MAX_TEAM_SIZE];
    unsigned int initialized_mask;
    unsigned int confirmed_mask;
    unsigned int dirty_mask;
    unsigned int rejected_mask;
    unsigned int wrong_answer_count;
    unsigned int questions_seen_mask;
    unsigned int question_assigned_mask;
    int question_indices[MAX_TEAM_SIZE];
    int selected;
    int focus;
    int dropdown; /* -1 fechado; 0 ataque, 1 condição P, 2 condição Q. */
    int option;
    int dropdown_scroll;
    float elapsed;
    float success_time;
} LogicSetupScreen;

void logic_setup_begin(LogicSetupScreen *screen, const Team *team,
                       const LogicFormula formulas[MAX_TEAM_SIZE], const int moves[MAX_TEAM_SIZE]);
bool logic_setup_update(LogicSetupScreen *screen, const Team *team, unsigned int *solved,
                        LogicFormula formulas[MAX_TEAM_SIZE], int moves[MAX_TEAM_SIZE]);
void logic_setup_draw(const UiTheme *ui, const SelectionUiAssets *fonts, const IntroAssets *art,
                      const LogicSetupScreen *screen, const Team *team, unsigned int solved);

#endif
