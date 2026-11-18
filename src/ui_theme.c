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

static Texture2D load_pixel_marker(const char *path)
{
    Image image = LoadImage(path);
    if (!image.data) return (Texture2D){0};
    /* Importa a arte gerada na grade nativa, sem halo ou pixels semitransparentes. */
    ImageResizeNN(&image, 18, 12);
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = image.data;
    Color fill[18*12] = {0};
    for (int i = 0; i < 18*12; i++) {
        Color color = pixels[i];
        if (color.a >= 128 && color.r >= 140 && color.g >= 110)
            fill[i] = color.b < 120 ? (Color){240, 184, 56, 255} : (Color){255, 250, 218, 255};
    }
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 18; x++) {
            Color color = fill[y*18+x];
            if (!color.a) {
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x+dx, ny = y+dy;
                        if (nx >= 0 && nx < 18 && ny >= 0 && ny < 12 && fill[ny*18+nx].a)
                            color = (Color){25, 35, 48, 255};
                    }
            }
            pixels[y*18+x] = color;
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
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
    const char *marker_path = "assets/ui/interaction_chevron_pixel.png";
    if (FileExists(marker_path)) {
        ui->interaction_chevron = load_pixel_marker(marker_path);
        if (ui->interaction_chevron.id) {
            SetTextureFilter(ui->interaction_chevron, TEXTURE_FILTER_POINT);
            SetTextureWrap(ui->interaction_chevron, TEXTURE_WRAP_CLAMP);
        }
    }
}

void ui_theme_update(UiTheme *ui, float dt)
{
    if (isfinite(dt) && dt > 0) ui->marker_time = fmodf(ui->marker_time + dt, 1.4f);
}

void ui_theme_unload(UiTheme *ui)
{
    if (ui->regular.texture.id) UnloadFont(ui->regular);
    if (ui->semibold.texture.id) UnloadFont(ui->semibold);
    if (ui->interaction_chevron.id) UnloadTexture(ui->interaction_chevron);
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

static void pixel_box(Rectangle bounds, int step, Color color)
{
    DrawRectangleRec((Rectangle){bounds.x+step, bounds.y, bounds.width-2*step, bounds.height}, color);
    DrawRectangleRec((Rectangle){bounds.x, bounds.y+step, bounds.width, bounds.height-2*step}, color);
}

void ui_panel(Rectangle bounds)
{
    bounds.x = roundf(bounds.x);
    bounds.y = roundf(bounds.y);
    pixel_box((Rectangle){bounds.x+2, bounds.y+4, bounds.width, bounds.height}, 6, (Color){22, 31, 40, 255});
    pixel_box(bounds, 6, (Color){31, 44, 54, 255});
    pixel_box((Rectangle){bounds.x+2, bounds.y+2, bounds.width-4, bounds.height-4}, 4, (Color){108, 135, 142, 255});
    pixel_box((Rectangle){bounds.x+4, bounds.y+4, bounds.width-8, bounds.height-8}, 2, (Color){255, 250, 226, 255});
    pixel_box((Rectangle){bounds.x+6, bounds.y+6, bounds.width-12, bounds.height-12}, 2, (Color){246, 235, 204, 255});
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
