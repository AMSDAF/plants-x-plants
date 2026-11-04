/*
 * menu_screen.h
 * Define os recursos e o estado das telas de inicio e menu.
 * Declara as funcoes de carregamento, entrada e desenho.
 */

#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <stdbool.h>

#include "raylib.h"

typedef struct {
    Texture2D background;
    Texture2D logo;
    Texture2D buttons;

    bool menu_open;
    int selected_option;
    int pressed_option;
    float elapsed_time;
} MenuScreen;

void menu_screen_init(MenuScreen *screen);
bool menu_screen_update(MenuScreen *screen);
void menu_screen_draw(const MenuScreen *screen);
void menu_screen_shutdown(MenuScreen *screen);

#endif