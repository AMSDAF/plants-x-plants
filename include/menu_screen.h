/* Declara recursos, relógios de vegetação/mascote e entrada do título/menu. */
#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <stdbool.h>
#include "raylib.h"

#define MENU_OPTION_COUNT 3

typedef struct {
    Texture2D background;
    Texture2D grass_background;
    Texture2D grass_atlas;
    Texture2D tree_background;
    Texture2D tree_atlas;
    float grass_time;
    Texture2D logo;
    Texture2D mascot;
    Texture2D mascot_idle;
    float mascot_time;
    float ui_time;
    Rectangle logo_source;
    Rectangle mascot_source;
    bool menu_open;
    bool input_blocked;
    int selected_option;
    int pressed_option;
    int pending_option;
    float activation_time;
} MenuScreen;

typedef enum { MENU_ACTION_NONE, MENU_ACTION_CAMPAIGN, MENU_ACTION_EXIT } MenuAction;

void menu_screen_init(MenuScreen *screen);
MenuAction menu_screen_update(MenuScreen *screen, bool accept_input);
void menu_screen_reset_input(MenuScreen *screen);
void menu_screen_draw(const MenuScreen *screen);
void menu_screen_shutdown(MenuScreen *screen);

#endif
