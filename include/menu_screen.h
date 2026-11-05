/*
 * menu_screen.h
 * Define os recursos e o estado das telas de inicio e menu.
 * Declara as funcoes de carregamento, entrada, animacao e desenho.
 */

#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <stdbool.h>

#include "raylib.h"

#define MENU_LEAF_COUNT 12
#define MENU_OPTION_COUNT 3

/* Feedback visual independente para cada botao. */
typedef struct {
    float scale;
    float offset_y;
    bool focused;
    bool pressed;
} MenuButtonAnimation;

/* Folha decorativa que cai no fundo das telas de inicio e menu. */
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float sway_phase;
    float sway_amplitude;
    float rotation;
    float rotation_speed;
    float size;
    int color_index;
} MenuLeaf;

typedef struct {
    Texture2D background;
    Texture2D foliage;
    Texture2D wind_background;
    Texture2D wind_atlas;
    bool wind_test_ready;
    float wind_playback_time;
    int wind_frame;
    Texture2D logo;
    Texture2D buttons;

    bool menu_open;
    int selected_option;
    int pressed_option;
    int pending_option;
    float activation_time;
    bool mouse_focus;
    float elapsed_time;

    /* Progresso da transicao: 0 na tela inicial e 1 no menu aberto. */
    float transition;

    MenuLeaf leaves[MENU_LEAF_COUNT];
    MenuButtonAnimation button_animations[MENU_OPTION_COUNT];
} MenuScreen;

void menu_screen_init(MenuScreen *screen);
typedef enum { MENU_ACTION_NONE, MENU_ACTION_CAMPAIGN, MENU_ACTION_EXIT } MenuAction;
MenuAction menu_screen_update(MenuScreen *screen, bool accept_input);
void menu_screen_reset_input(MenuScreen *screen);
void menu_screen_draw(const MenuScreen *screen);
void menu_screen_shutdown(MenuScreen *screen);

#endif
