/* Desenha formas na grade de pixels e protege a entrada nas transições. */
#include "selection_ui.h"
#include "ui_theme.h"
#include <math.h>
const Color SELECTION_CREAM = {247,237,198,255};
const Color SELECTION_INK = {24,52,30,255};
const Color SELECTION_BORDER = {53,76,43,255};
const Color SELECTION_ORANGE = {190,93,28,255};

static const int font_sizes[SELECTION_FONT_COUNT] = {20, 22, 24, 28, 30, 36, 72};
static const Rectangle skin_sources[SELECTION_SKIN_COUNT] = {
    {64, 44, 920, 1390}, {64, 44, 920, 1390}, {60, 41, 1486, 888},
    {152, 158, 1868, 387}, {410, 552, 293, 401}
};

void selection_ui_update_fonts(SelectionUiAssets *assets, float scale)
{
    if (scale <= 0) return;
    bool unchanged = assets->font_scale > 0;
    for (int i = 0; i < SELECTION_FONT_COUNT; i++)
        if (roundf(font_sizes[i] * scale) != roundf(font_sizes[i] * assets->font_scale)) unchanged = false;
    if (unchanged) { assets->font_scale = scale; return; }
    int codepoints[352];
    for (int i = 0; i < 352; i++) codepoints[i] = i + 32;
    for (int i = 0; i < SELECTION_FONT_COUNT; i++) {
        if (assets->fonts[i].texture.id) UnloadFont(assets->fonts[i]);
        assets->fonts[i] = (Font){0};
        if (!FileExists("assets/fonts/pixelify_sans_regular.ttf")) continue;
        Font font = LoadFontEx("assets/fonts/pixelify_sans_regular.ttf",
                               (int)fmaxf(1, roundf(font_sizes[i] * scale)), codepoints, 352);
        if (font.texture.id && font.texture.id != GetFontDefault().texture.id) {
            SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
            assets->fonts[i] = font;
        }
    }
    assets->font_scale = scale;
}

void selection_ui_assets_load(SelectionUiAssets *assets)
{
    static const char *const paths[SELECTION_SKIN_COUNT] = {
        "assets/ui/trainer_setup/portrait_normal_v1.png",
        "assets/ui/trainer_setup/portrait_selected_v1.png",
        "assets/ui/trainer_setup/name_panel_v1.png",
        "assets/ui/plant_selection/footer_button_v1.png",
        "assets/ui/plant_selection/cursor_v1.png"
    };
    *assets = (SelectionUiAssets){0};
    for (int i = 0; i < SELECTION_SKIN_COUNT; i++) {
        if (!FileExists(paths[i])) continue;
        Texture2D texture = LoadTexture(paths[i]);
        Rectangle source = skin_sources[i];
        if (texture.id && (texture.width < source.x + source.width || texture.height < source.y + source.height)) {
            UnloadTexture(texture);
            continue;
        }
        if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        assets->skins[i] = texture;
    }
    selection_ui_update_fonts(assets, fminf(GetRenderWidth() / 1280.0f, GetRenderHeight() / 720.0f));
}

void selection_ui_assets_unload(SelectionUiAssets *assets)
{
    for (int i = 0; i < SELECTION_SKIN_COUNT; i++)
        if (assets->skins[i].id) UnloadTexture(assets->skins[i]);
    for (int i = 0; i < SELECTION_FONT_COUNT; i++)
        if (assets->fonts[i].texture.id) UnloadFont(assets->fonts[i]);
    *assets = (SelectionUiAssets){0};
}

static Font pixel_font(const UiTheme *ui, const SelectionUiAssets *assets, int size, float *draw_size)
{
    for (int i = 0; i < SELECTION_FONT_COUNT; i++) {
        if (font_sizes[i] == size && assets->fonts[i].texture.id) {
            *draw_size = assets->fonts[i].baseSize / assets->font_scale;
            return assets->fonts[i];
        }
    }
    *draw_size = (float)size;
    return ui_pixel_font(ui);
}

float selection_ui_measure(const UiTheme *ui, const SelectionUiAssets *assets, const char *text, int size)
{
    float draw_size;
    Font font = pixel_font(ui, assets, size, &draw_size);
    return MeasureTextEx(font, text, draw_size, 0).x;
}

void selection_ui_pixel_text(const UiTheme *ui, const SelectionUiAssets *assets, const char *text,
                             float x, float y, int size, Color color)
{
    float draw_size;
    Font font = pixel_font(ui, assets, size, &draw_size);
    float scale = assets->font_scale > 0 ? assets->font_scale : 1;
    DrawTextEx(font, text, (Vector2){roundf(x * scale) / scale, roundf(y * scale) / scale}, draw_size, 0, color);
}

/* Centraliza pelos limites da tinta, sem a margem superior dos glifos. */
void selection_ui_pixel_box(const UiTheme *ui, const SelectionUiAssets *assets, const char *text,
                            Rectangle box, int size, Color color)
{
    float draw_size;
    Font font = pixel_font(ui, assets, size, &draw_size);
    float width = MeasureTextEx(font, text, draw_size, 0).x;
    float top = (float)font.baseSize, bottom = 0;
    for (const char *cursor = text; *cursor;) {
        int bytes;
        int code = GetCodepointNext(cursor, &bytes);
        int index = GetGlyphIndex(font, code);
        if (code != ' ') {
            top = fminf(top, (float)font.glyphs[index].offsetY);
            bottom = fmaxf(bottom, font.glyphs[index].offsetY + font.recs[index].height);
        }
        cursor += bytes;
    }
    float factor = width > box.width ? box.width / width : 1;
    float scale = assets->font_scale > 0 ? assets->font_scale : 1;
    Vector2 position = {box.x + (box.width - width * factor) / 2,
                        box.y + box.height / 2 - (top + bottom) * draw_size * factor / font.baseSize / 2};
    position.x = roundf(position.x * scale) / scale;
    position.y = roundf(position.y * scale) / scale;
    DrawTextEx(font, text, position, draw_size * factor, 0, color);
}

void selection_ui_skin(const SelectionUiAssets *assets, SelectionSkin skin, Rectangle bounds, float alpha)
{
    Texture2D texture = assets->skins[skin];
    if (texture.id) {
        Rectangle source = skin_sources[skin];
        float scale = fminf(bounds.width / source.width, bounds.height / source.height);
        Rectangle target = {bounds.x + (bounds.width - source.width * scale) / 2,
                            bounds.y + (bounds.height - source.height * scale) / 2,
                            source.width * scale, source.height * scale};
        DrawTexturePro(texture, source, target, (Vector2){0}, 0, Fade(WHITE, alpha));
    } else if (skin == SELECTION_POINTER) {
        selection_ui_cursor((int)(bounds.x+bounds.width), (int)(bounds.y+bounds.height/2), 0, alpha);
    } else {
        selection_ui_panel(bounds, 4, Fade(SELECTION_ORANGE, alpha));
        selection_ui_panel((Rectangle){bounds.x+4, bounds.y+4, bounds.width-8, bounds.height-8},
                            4, Fade(SELECTION_CREAM, alpha));
    }
}

Rectangle selection_ui_footer_bounds(bool forward)
{
    return forward ? (Rectangle){832, 628, 350, 64} : (Rectangle){98, 628, 240, 64};
}

void selection_ui_button(const UiTheme *ui, const SelectionUiAssets *assets, Rectangle bounds,
                          const char *label, bool focused, bool pressed, float alpha)
{
    Color tint = pressed ? (Color){220, 225, 180, 255} : focused ? (Color){255, 244, 210, 255} : WHITE;
    if (assets->skins[SELECTION_FOOTER].id) {
        Rectangle source = skin_sources[SELECTION_FOOTER];
        /* Nove partes explícitas preservam a escala da altura nos cantos. */
        float scale = bounds.height / source.height;
        float sx[4] = {source.x, source.x+260, source.x+source.width-260, source.x+source.width};
        float sy[4] = {source.y, source.y+88, source.y+source.height-88, source.y+source.height};
        float dx[4] = {bounds.x, bounds.x+260*scale, bounds.x+bounds.width-260*scale, bounds.x+bounds.width};
        float dy[4] = {bounds.y, bounds.y+88*scale, bounds.y+bounds.height-88*scale, bounds.y+bounds.height};
        for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
            DrawTexturePro(assets->skins[SELECTION_FOOTER],
                (Rectangle){sx[col], sy[row], sx[col+1]-sx[col], sy[row+1]-sy[row]},
                (Rectangle){dx[col], dy[row], dx[col+1]-dx[col], dy[row+1]-dy[row]},
                (Vector2){0}, 0, Fade(tint, alpha));
    } else selection_ui_skin(assets, SELECTION_FOOTER, bounds, alpha);
    if (focused) DrawRectangleLinesEx((Rectangle){bounds.x+3, bounds.y+3, bounds.width-6, bounds.height-6},
                                       2, Fade(GOLD, alpha));
    selection_ui_pixel_box(ui, assets, label,
        (Rectangle){bounds.x+24, bounds.y+12, bounds.width-48, bounds.height-24}, 28, Fade(SELECTION_INK, alpha));
}

void selection_ui_progress(OpeningStep step, float alpha)
{
    /* O passo vem da tela atual; voltar nunca mantém progresso adiantado. */
    for (int i = 0; i < OPENING_STEP_COUNT; i++)
        DrawRectangleRounded((Rectangle){502 + i * 48, 103, 36, 7}, 0.5f, 3,
            Fade(i <= (int)step ? (Color){244,133,25,255} : (Color){253,245,216,255}, alpha));
}

void selection_ui_header(const UiTheme *ui, const SelectionUiAssets *assets,
                          const char *title, const char *subtitle, OpeningStep step, float alpha)
{
    Color cream = Fade((Color){253, 245, 216, 255}, alpha);
    selection_ui_pixel_box(ui, assets, title, (Rectangle){80, 24, 1120, 68}, 72, cream);
    selection_ui_progress(step, alpha);
    selection_ui_pixel_box(ui, assets, subtitle, (Rectangle){90, 116, 1100, 28}, 28, cream);
}

void selection_ui_panel(Rectangle bounds, int corner, Color color)
{
    int x=(int)roundf(bounds.x), y=(int)roundf(bounds.y);
    int w=(int)roundf(bounds.width), h=(int)roundf(bounds.height);
    DrawRectangle(x+2*corner,y,w-4*corner,h,color);
    DrawRectangle(x+corner,y+corner,w-2*corner,h-2*corner,color);
    DrawRectangle(x,y+2*corner,w,h-4*corner,color);
}

void selection_ui_text(const UiTheme *ui, const char *text, int center, int y, int size, Color color)
{
    ui_text(ui, text,center-ui_measure(ui, text,size)/2,y,size,color);
}

bool selection_ui_input_down(void)
{
    for (int key=32; key<=348; key++) if (IsKeyDown(key)) return true;
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

void selection_ui_discard_text(void)
{
    while (GetCharPressed()!=0) {}
}

void selection_ui_accents(Rectangle bounds, float alpha)
{
    int x = (int)bounds.x, y = (int)bounds.y;
    int w = (int)bounds.width, h = (int)bounds.height;
    for (int side = 0; side < 2; side++) {
        for (int bottom = 0; bottom < 2; bottom++) {
            int left = side ? x + w - 40 : x + 16;
            int top = bottom ? y + h - 40 : y + 16;
            DrawRectangle(left, bottom ? top + 18 : top, 24, 6, Fade(SELECTION_ORANGE,alpha));
            DrawRectangle(side ? left + 18 : left, top, 6, 24, Fade(SELECTION_ORANGE,alpha));
        }
    }
}

void selection_ui_cursor(int right, int center_y, float elapsed, float alpha)
{
    /* Pulso discreto na grade inteira, com o mesmo ritmo do menu. */
    float pulse = 1.0f + 0.1f * sinf(2.0f * PI * elapsed);
    int width = 2 * (int)roundf(7 * pulse) + 1;
    int left = right - width;
    for (int step = 0; step < 5; step++) {
        int x0 = left + (int)roundf(step * width / 5.0f);
        int x1 = left + (int)roundf((step + 1) * width / 5.0f);
        int half_height = (int)roundf((10 - step*2) * pulse);
        DrawRectangle(x0, center_y - half_height, x1 - x0, half_height*2 + 1, Fade(SELECTION_INK,alpha));
    }
}
