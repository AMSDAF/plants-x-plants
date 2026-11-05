/* Desenha o chão provisório e limita a pequena caixa dos pés. */
#include "world.h"

void world_init(World *world)
{
    *world = (World){.bounds = {70, 170, 1140, 470}};
}

Vector2 world_constrain(const World *world, Vector2 feet)
{
    Rectangle b = world->bounds;
    /* Caixa de 16 x 8 pixels centrada no ponto dos pés. */
    if (feet.x < b.x + 8) feet.x = b.x + 8;
    if (feet.x > b.x + b.width - 8) feet.x = b.x + b.width - 8;
    if (feet.y < b.y + 4) feet.y = b.y + 4;
    if (feet.y > b.y + b.height - 4) feet.y = b.y + b.height - 4;
    return feet;
}

void world_draw(const World *world)
{
    Rectangle b = world->bounds;
    ClearBackground((Color){49, 59, 54, 255});
    DrawRectangleRec(b, (Color){112, 123, 103, 255});
    for (int x = (int)b.x; x < b.x + b.width; x += 40)
        DrawLine(x, (int)b.y, x, (int)(b.y + b.height), (Color){120, 131, 111, 255});
    for (int y = (int)b.y; y < b.y + b.height; y += 40)
        DrawLine((int)b.x, y, (int)(b.x + b.width), y, (Color){120, 131, 111, 255});
    DrawRectangleLinesEx(b, 3, (Color){192, 200, 170, 255});
    DrawText("Teste de exploracao", 70, 35, 30, RAYWHITE);
    DrawText("WASD / setas: mover | ESC: menu", 70, 675, 22, RAYWHITE);
}
