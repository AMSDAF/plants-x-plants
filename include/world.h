/* Mantém o cenário e o estado dos desafios lógicos da estufa. */
#ifndef WORLD_H
#define WORLD_H
#include "raylib.h"

typedef struct {
    Rectangle bounds;
    bool water;
    bool light;
    bool key_collected;
    bool north_channel;
    bool south_channel;
    bool completed;
} World;

void world_init(World *world);
Vector2 world_spawn(void);
bool world_gate_open(const World *world);
bool world_bridge_open(const World *world);
Vector2 world_constrain(const World *world, Vector2 previous, Vector2 feet);
void world_interact(World *world, Vector2 feet);
void world_draw(const World *world);
void world_draw_interface(const World *world, Vector2 feet);
#endif
