/* Carrega tipografia com acentos e desenha painéis e textos com medidas consistentes. */
#include "ui_theme.h"
#include <math.h>
#include <string.h>

static Font load_font(const char *path, const int *codepoints, int count)
{
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "Fonte ausente: %s; usando a fonte de reserva.", path);
        return (Font){0};
    }
    Font font = LoadFontEx(path, 96, (int *)codepoints, count);
    if (font.texture.id == GetFontDefault().texture.id) return (Font){0};
    if (font.texture.id) SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    return font;
}

void ui_theme_load(UiTheme *ui)
{
    int codepoints[512];
    int count = 0;
    for (int value = 32; value <= 126; value++) codepoints[count++] = value;
    for (int value = 160; value <= 383; value++) codepoints[count++] = value;
    const int extra[] = {0x2013, 0x2014, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2026,
                         0x2190, 0x2191, 0x2192, 0x2193};
    for (unsigned int i = 0; i < sizeof(extra)/sizeof(extra[0]); i++) codepoints[count++] = extra[i];
    *ui = (UiTheme){
        .regular = load_font("assets/fonts/atkinson_hyperlegible_next_regular.ttf", codepoints, count),
        .semibold = load_font("assets/fonts/atkinson_hyperlegible_next_semibold.ttf", codepoints, count)
    };
}

void ui_theme_unload(UiTheme *ui)
{
    if (ui->regular.texture.id) UnloadFont(ui->regular);
    if (ui->semibold.texture.id) UnloadFont(ui->semibold);
    *ui = (UiTheme){0};
}

Font ui_font(const UiTheme *ui, bool bold)
{
    if (ui) {
        Font font = bold ? ui->semibold : ui->regular;
        if (font.texture.id) return font;
        font = bold ? ui->regular : ui->semibold;
        if (font.texture.id) return font;
    }
    return GetFontDefault();
}

static int measure(const UiTheme *ui, const char *text, int size, bool bold)
{
    return (int)ceilf(MeasureTextEx(ui_font(ui, bold), text, size, 0).x);
}

int ui_measure(const UiTheme *ui, const char *text, int size)
{
    return measure(ui, text, size, size >= 24);
}

int ui_measure_bold(const UiTheme *ui, const char *text, int size)
{
    return measure(ui, text, size, true);
}

void ui_text(const UiTheme *ui, const char *text, int x, int y, int size, Color color)
{
    DrawTextEx(ui_font(ui, size >= 24), text, (Vector2){x, y}, size, 0, color);
}

void ui_text_bold(const UiTheme *ui, const char *text, int x, int y, int size, Color color)
{
    DrawTextEx(ui_font(ui, true), text, (Vector2){x, y}, size, 0, color);
}

void ui_panel(Rectangle bounds)
{
    DrawRectangleRounded((Rectangle){bounds.x+2, bounds.y+3, bounds.width, bounds.height}, 0.25f, 8,
                         (Color){49, 35, 21, 65});
    DrawRectangleRounded(bounds, 0.25f, 8, (Color){127, 92, 56, 255});
    Rectangle inside = {bounds.x+2, bounds.y+2, bounds.width-4, bounds.height-4};
    DrawRectangleRounded(inside, 0.25f, 8, (Color){246, 235, 204, 255});
    DrawRectangleRoundedLinesEx(inside, 0.25f, 8, 1, (Color){255, 249, 225, 255});
}

int ui_text_wrap(const UiTheme *ui, const char *text, Rectangle bounds, int size, Color color, bool bold)
{
    char line[512] = {0};
    int length = 0, lines = 0;
    float y = bounds.y;
    const char *cursor = text;
    while (*cursor && y + size <= bounds.y + bounds.height) {
        if (*cursor == '\n') {
            DrawTextEx(ui_font(ui, bold), line, (Vector2){bounds.x, y}, size, 0, color);
            y += size * 1.3f;
            lines++;
            length = 0;
            line[0] = 0;
            cursor++;
            continue;
        }
        if (!length && *cursor == ' ') { cursor++; continue; }
        int bytes = 0;
        GetCodepointNext(cursor, &bytes);
        bool full = length + bytes >= (int)sizeof(line);
        if (!full) {
            memcpy(line+length, cursor, bytes);
            line[length+bytes] = 0;
        }
        if (length && (full || measure(ui, line, size, bold) > bounds.width)) {
            line[length] = 0;
            int split = length;
            while (split > 0 && line[split] != ' ') split--;
            if (split == 0) split = length;
            line[split] = 0;
            DrawTextEx(ui_font(ui, bold), line, (Vector2){bounds.x, y}, size, 0, color);
            y += size * 1.3f;
            lines++;
            int remaining = split < length ? length-split-1 : 0;
            if (remaining) memmove(line, line+split+1, remaining);
            length = remaining;
            line[length] = 0;
            continue;
        }
        length += bytes;
        cursor += bytes;
    }
    if (length && y + size <= bounds.y + bounds.height) {
        DrawTextEx(ui_font(ui, bold), line, (Vector2){bounds.x, y}, size, 0, color);
        lines++;
    }
    return lines;
}
