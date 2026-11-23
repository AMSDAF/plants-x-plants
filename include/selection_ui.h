/* Compartilha paleta e formas pixeladas entre seleção e entrada de nome. */
#ifndef SELECTION_UI_H
#define SELECTION_UI_H
#include "raylib.h"
#include "ui_theme.h"
#define SELECTION_FONT_COUNT 7
typedef enum {
    OPENING_CHARACTER, OPENING_NAME, OPENING_LETTER, OPENING_ACCEPT,
    OPENING_STARTERS, OPENING_TRAVEL, OPENING_STEP_COUNT
} OpeningStep;
typedef enum {
    SELECTION_PORTRAIT_NORMAL, SELECTION_PORTRAIT_SELECTED, SELECTION_NAME_PANEL,
    SELECTION_FOOTER, SELECTION_POINTER, SELECTION_SKIN_COUNT
} SelectionSkin;
typedef struct {
    Texture2D skins[SELECTION_SKIN_COUNT];
    Font fonts[SELECTION_FONT_COUNT];
    float font_scale;
} SelectionUiAssets;
void selection_ui_assets_load(SelectionUiAssets *assets);
void selection_ui_assets_unload(SelectionUiAssets *assets);
void selection_ui_update_fonts(SelectionUiAssets *assets, float scale);
Font selection_ui_font(const UiTheme *ui, const SelectionUiAssets *assets, int size, float *draw_size);
float selection_ui_measure(const UiTheme *ui, const SelectionUiAssets *assets, const char *text, int size);
void selection_ui_pixel_text(const UiTheme *ui, const SelectionUiAssets *assets, const char *text,
                             float x, float y, int size, Color color);
void selection_ui_pixel_box(const UiTheme *ui, const SelectionUiAssets *assets, const char *text,
                            Rectangle box, int size, Color color);
void selection_ui_skin(const SelectionUiAssets *assets, SelectionSkin skin, Rectangle bounds, float alpha);
Rectangle selection_ui_footer_bounds(bool forward);
void selection_ui_button(const UiTheme *ui, const SelectionUiAssets *assets, Rectangle bounds,
                          const char *label, bool focused, bool pressed, float alpha);
void selection_ui_header(const UiTheme *ui, const SelectionUiAssets *assets,
                          const char *title, const char *subtitle, OpeningStep step, float alpha);
void selection_ui_progress(OpeningStep step, float alpha);
extern const Color SELECTION_CREAM;
extern const Color SELECTION_INK;
extern const Color SELECTION_BORDER;
extern const Color SELECTION_ORANGE;
void selection_ui_panel(Rectangle bounds, int corner, Color color);
void selection_ui_text(const UiTheme *ui, const char *text, int center, int y, int size, Color color);
bool selection_ui_input_down(void);
void selection_ui_discard_text(void);
void selection_ui_accents(Rectangle bounds, float alpha);
void selection_ui_cursor(int right, int center_y, float elapsed, float alpha);
#endif
