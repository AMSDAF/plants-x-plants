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
