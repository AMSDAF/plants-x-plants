/* Padroniza navegação e cliques canceláveis das interfaces, sem guardar estado global. */
#ifndef UI_INPUT_H
#define UI_INPUT_H
#include "raylib.h"

typedef struct {
    Vector2 origin;
    Vector2 release;
    bool armed;
    bool clicked;
} UiPointer;

static inline void ui_pointer_update(UiPointer *pointer, bool enabled)
{
    pointer->clicked = false;
    if (!enabled) { pointer->armed = false; return; }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        pointer->origin = GetMousePosition();
        pointer->armed = true;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        pointer->release = GetMousePosition();
        pointer->clicked = pointer->armed;
        pointer->armed = false;
    }
}

static inline bool ui_pointer_clicked(const UiPointer *pointer, Rectangle bounds)
{
    return pointer->clicked && CheckCollisionPointRec(pointer->origin,bounds) &&
        CheckCollisionPointRec(pointer->release,bounds);
}
static inline bool ui_nav_left(void) { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
static inline bool ui_nav_right(void) { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }
static inline bool ui_nav_up(void) { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static inline bool ui_nav_down(void) { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
static inline bool ui_confirm_pressed(void) { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); }
static inline bool ui_digit_pressed(int digit)
{
    return digit >= 0 && digit <= 9 &&
        (IsKeyPressed(KEY_ZERO+digit) || IsKeyPressed(KEY_KP_0+digit));
}
#endif
