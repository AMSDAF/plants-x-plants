/* Compartilha paleta e formas pixeladas entre seleção e entrada de nome. */
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
#endif
