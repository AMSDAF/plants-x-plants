/* Declara rascunho UTF-8, foco, transição e ações da entrada de nome. */
#ifndef NAME_ENTRY_SCREEN_H
#define NAME_ENTRY_SCREEN_H
#include "trainer_select_screen.h"
#define PLAYER_NAME_MAX_CODEPOINTS 12
#define PLAYER_NAME_CAPACITY (PLAYER_NAME_MAX_CODEPOINTS*4+1)
#define NAME_ENTRY_TRANSITION_SECONDS 0.6f
typedef enum { NAME_ENTRY_NONE, NAME_ENTRY_BACK, NAME_ENTRY_CONFIRM, NAME_ENTRY_SELECTION } NameEntryAction;
typedef struct {
    char draft[PLAYER_NAME_CAPACITY];
    int count;
    int focus;
    int pressed;
    float progress;
    int direction;
    bool input_blocked;
    bool error;
    float caret_time;
    float backspace_time;
} NameEntryScreen;
void name_entry_init(NameEntryScreen *screen);
void name_entry_begin(NameEntryScreen *screen, bool forward);
bool name_entry_append(NameEntryScreen *screen, int codepoint);
void name_entry_backspace(NameEntryScreen *screen);
bool name_entry_confirm(NameEntryScreen *screen, char output[PLAYER_NAME_CAPACITY]);
NameEntryAction name_entry_update(NameEntryScreen *screen, bool accept_input, float dt);
Rectangle name_entry_portrait_bounds(void);
Rectangle name_entry_transition_bounds(const NameEntryScreen *screen, Trainer trainer);
void name_entry_draw(const NameEntryScreen *screen, const TrainerSelectScreen *selection,
                     const TrainerSelectAssets *assets);
#endif
