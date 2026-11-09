/* Define a área provisória e a colisão dos pés do treinador. */
#ifndef WORLD_H
#define WORLD_H
#include "raylib.h"
typedef struct { Rectangle bounds; } World;
void world_init(World *world);
Vector2 world_constrain(const World *world, Vector2 feet);
void world_draw(const World *world);
#endif
