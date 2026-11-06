/* Compartilha paleta e formas pixeladas entre seleção, entrada de nome e Versus. */
#ifndef SELECTION_UI_H
#define SELECTION_UI_H
#include "raylib.h"
extern const Color SELECTION_CREAM;
extern const Color SELECTION_INK;
extern const Color SELECTION_BORDER;
extern const Color SELECTION_ORANGE;
void selection_ui_panel(Rectangle bounds, int corner, Color color);
void selection_ui_text(const char *text, int center, int y, int size, Color color);
bool selection_ui_input_down(void);
void selection_ui_discard_text(void);
void selection_ui_accents(Rectangle bounds, float alpha);
void selection_ui_cursor(int right, int center_y, float elapsed, float alpha);
#endif
