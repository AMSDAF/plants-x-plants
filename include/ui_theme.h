/* Compartilha fontes legíveis e os elementos creme da interface, sem estado global. */
#ifndef UI_THEME_H
#define UI_THEME_H

#include "raylib.h"

typedef struct {
    Font regular;
    Font semibold;
} UiTheme;

void ui_theme_load(UiTheme *ui);
void ui_theme_unload(UiTheme *ui);
Font ui_font(const UiTheme *ui, bool bold);
void ui_text(const UiTheme *ui, const char *text, int x, int y, int size, Color color);
int ui_measure(const UiTheme *ui, const char *text, int size);
void ui_text_bold(const UiTheme *ui, const char *text, int x, int y, int size, Color color);
int ui_measure_bold(const UiTheme *ui, const char *text, int size);
void ui_panel(Rectangle bounds);
int ui_text_wrap(const UiTheme *ui, const char *text, Rectangle bounds, int size, Color color, bool bold);

#endif
