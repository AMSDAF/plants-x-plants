/* Desenha formas na grade de pixels e protege a entrada nas transições. */
#include "selection_ui.h"
#include <math.h>
const Color SELECTION_CREAM = {247,237,198,255};
const Color SELECTION_INK = {24,52,30,255};
const Color SELECTION_BORDER = {53,76,43,255};
const Color SELECTION_ORANGE = {190,93,28,255};

void selection_ui_panel(Rectangle bounds, int corner, Color color)
{
    int x=(int)roundf(bounds.x), y=(int)roundf(bounds.y);
    int w=(int)roundf(bounds.width), h=(int)roundf(bounds.height);
    DrawRectangle(x+2*corner,y,w-4*corner,h,color);
    DrawRectangle(x+corner,y+corner,w-2*corner,h-2*corner,color);
    DrawRectangle(x,y+2*corner,w,h-4*corner,color);
}

void selection_ui_text(const char *text, int center, int y, int size, Color color)
{
    DrawText(text,center-MeasureText(text,size)/2,y,size,color);
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
