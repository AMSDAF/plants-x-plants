/* Desenha diálogos de campanha com moldura reutilizável e retrato independente. */
#include "dialogue_ui.h"
#include "game_view.h"
#include <math.h>
#include <string.h>

static const Color INK = {24, 59, 48, 255};
static const Color PAPER = {250, 235, 203, 255};
static const Color WOOD = {142, 72, 34, 255};
static const Color WOOD_LIGHT = {197, 119, 57, 255};

static void draw_frame(const UiTheme *ui, Rectangle bounds)
{
    DrawRectangleRec(bounds, PAPER);
    Texture2D texture = ui ? ui->dialogue_frame : (Texture2D){0};
    if (!texture.id) {
        DrawRectangleLinesEx(bounds, 5, WOOD);
        DrawRectangleLinesEx((Rectangle){bounds.x+5, bounds.y+5, bounds.width-10, bounds.height-10}, 2, INK);
        return;
    }

    /* Bordas repetidas conservam a escala dos veios; os cantos não se deformam. */
    float sx = roundf(texture.width * 0.16f), sy = roundf(texture.height * 0.16f);
    const float corner = 44;
    float source_x[4] = {0, sx, texture.width-sx, texture.width};
    float source_y[4] = {0, sy, texture.height-sy, texture.height};
    float dest_x[4] = {bounds.x, bounds.x+corner, bounds.x+bounds.width-corner, bounds.x+bounds.width};
    float dest_y[4] = {bounds.y, bounds.y+corner, bounds.y+bounds.height-corner, bounds.y+bounds.height};
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            Rectangle source = {source_x[col], source_y[row], source_x[col+1]-source_x[col], source_y[row+1]-source_y[row]};
            Rectangle dest = {dest_x[col], dest_y[row], dest_x[col+1]-dest_x[col], dest_y[row+1]-dest_y[row]};
            if ((row == 1) != (col == 1)) {
                bool horizontal = row != 1;
                float scale = horizontal ? corner/sy : corner/sx;
                float length = horizontal ? dest.width : dest.height;
                float tile = (horizontal ? source.width : source.height)*scale;
                for (float offset = 0; offset < length; offset += tile) {
                    float part = fminf(tile, length-offset);
                    Rectangle segment = source, target = dest;
                    if (horizontal) {
                        segment.width = part/scale;
                        target.x += offset;
                        target.width = part;
                    } else {
                        segment.height = part/scale;
                        target.y += offset;
                        target.height = part;
                    }
                    DrawTexturePro(texture, segment, target, (Vector2){0}, 0, WHITE);
                }
            } else {
                DrawTexturePro(texture, source, dest, (Vector2){0}, 0, WHITE);
            }
        }
    }
}

static bool is_character(const char *speaker)
{
    return strcmp(speaker, "Lia") == 0 || strcmp(speaker, "Bento") == 0 ||
           strcmp(speaker, "Íris") == 0;
}

static void draw_portrait(const UiTheme *ui, const char *speaker, Rectangle bounds, bool framed)
{
    Rectangle inner = bounds;
    if (framed) {
        DrawRectangleRec(bounds, WOOD);
        inner = (Rectangle){bounds.x+5, bounds.y+5, bounds.width-10, bounds.height-10};
        DrawRectangleRec(inner, PAPER);
        DrawRectangleLinesEx(inner, 1, WOOD_LIGHT);
    }
    Texture2D portrait = {0};
    Rectangle source = {0};
    if (ui) {
        if (strcmp(speaker, "Lia") == 0) {
            portrait = ui->gardener_portrait;
            source = ui->gardener_portrait_source;
        } else if (strcmp(speaker, "Bento") == 0) {
            portrait = ui->technician_portrait;
            source = ui->technician_portrait_source;
        } else if (strcmp(speaker, "Íris") == 0) {
            portrait = ui->researcher_portrait;
            source = ui->researcher_portrait_source;
        }
    }
    if (portrait.id && source.width > 0 && source.height > 0) {
        /* Mantém todo o busto, sem cortar o chapéu nem aproximar o rosto. */
        float scale = fminf(inner.width / source.width, inner.height / source.height);
        Rectangle dest = {roundf(inner.x+(inner.width-source.width*scale)/2), inner.y+inner.height-source.height*scale,
                          source.width*scale, source.height*scale};
        DrawTexturePro(portrait, source, dest, (Vector2){0}, 0, WHITE);
    } else {
        /* Identificação neutra se o retrato estiver ausente, sem trocar identidades. */
        const char *initial = strcmp(speaker, "Bento") == 0 ? "B" :
                              strcmp(speaker, "Íris") == 0 ? "Í" : "L";
        int size = 48;
        ui_text_bold(ui, initial, (int)(inner.x+(inner.width-ui_measure_bold(ui, initial, size))/2),
                     (int)(inner.y+(inner.height-size)/2), size, INK);
    }
}

void dialogue_ui_draw(const UiTheme *ui, const char *speaker, const char *message, bool has_next, int visible_characters)
{
    if (!speaker || !message) return;
    Rectangle card = {32, 504, GAME_SCREEN_WIDTH-64, 200};
    Font font = ui && ui->dialogue_font.texture.id ? ui->dialogue_font : ui_font(ui, false);
    bool character = is_character(speaker);
    bool skinned = character && ui && ui->dialogue_skin.id;
    if (skinned) {
        /* O alfa recorta as margens e preserva a transparência dos cantos. */
        DrawTexturePro(ui->dialogue_skin, ui->dialogue_skin_source, card, (Vector2){0}, 0, WHITE);
    } else {
        draw_frame(ui, card);
    }
    float text_right = card.x+card.width-32;
    float text_top = card.y+36;
    if (character) {
        float divider = card.x+card.width-244;
        Rectangle name;
        if (skinned) {
            /* Coordenadas medidas na arte: preservam inlays e enfeites da plaquinha. */
            draw_portrait(ui, speaker, (Rectangle){card.x+card.width*0.794f, card.y+33,
                                                   card.width*0.174f, 96}, false);
            name = (Rectangle){card.x+card.width*0.79f, card.y+140, card.width*0.183f, 34};
            divider = card.x+card.width*0.769f;
        } else {
            DrawRectangleRec((Rectangle){divider, card.y+10, 10, card.height-20}, WOOD);
            DrawRectangleRec((Rectangle){divider+2, card.y+10, 3, card.height-20}, WOOD_LIGHT);
            draw_portrait(ui, speaker, (Rectangle){divider+26, card.y+20, 192, 132}, true);
            name = (Rectangle){divider+20, card.y+156, 204, 30};
            DrawRectangleRec(name, PAPER);
            DrawRectangleLinesEx(name, 2, WOOD);
            DrawRectangleLinesEx((Rectangle){name.x+3, name.y+3, name.width-6, name.height-6}, 1, WOOD_LIGHT);
        }
        DrawTextEx(font, speaker, (Vector2){roundf(name.x+(name.width-MeasureTextEx(font, speaker, 28, 0).x)/2), name.y+2}, 28, 0, INK);
        text_right = divider-28;
    } else {
        DrawTextEx(font, speaker, (Vector2){card.x+48, card.y+24}, 28, 0, INK);
        text_top += 32;
    }

    bool typing = visible_characters < GetCodepointCount(message);
    ui_text_wrap_reveal(font, message, (Rectangle){card.x+48, text_top, text_right-card.x-48,
                                               card.y+card.height-58-text_top}, 28, INK, visible_characters);
    const char *controls = typing ? "E / Enter  Mostrar tudo     Esc  Fechar" : has_next ? "E / Enter  Avançar     Esc  Fechar" : "E / Enter / Esc  Fechar";
    int controls_right = (int)text_right - (has_next && !typing ? 28 : 0);
    DrawTextEx(font, controls, (Vector2){controls_right-ceilf(MeasureTextEx(font, controls, 20, 0).x), card.y+card.height-48}, 20, 0, INK);
    if (has_next && !typing) {
        float x = text_right-8, y = card.y+card.height-38;
        for (int row = 0; row < 5; row++)
            DrawRectangle((int)x-6+row, (int)y+row, 13-2*row, 1, INK);
    }
}
