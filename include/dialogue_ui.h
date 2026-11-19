/* Compõe a moldura botânica, o retrato e os textos do diálogo, sem alterar estado. */
#ifndef DIALOGUE_UI_H
#define DIALOGUE_UI_H

#include "ui_theme.h"

void dialogue_ui_draw(const UiTheme *ui, const char *speaker, const char *message, bool has_next, int visible_characters);

#endif
