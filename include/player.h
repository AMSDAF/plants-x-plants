/* Define recursos, movimento cardinal e desenho dos treinadores. */
#ifndef PLAYER_H
#define PLAYER_H
#include "world.h"
typedef enum { TRAINER_MALE, TRAINER_FEMALE, TRAINER_COUNT } Trainer;
typedef enum { FACING_SOUTH, FACING_WEST, FACING_EAST, FACING_NORTH } Facing;
/* Duração do ciclo masculino, independente da quantidade de poses. */
#ifndef PLAYER_MALE_WALK_CYCLE_SECONDS
#define PLAYER_MALE_WALK_CYCLE_SECONDS 0.56f
#endif
typedef struct {
    Texture2D sheets[TRAINER_COUNT];
    Texture2D male_walk6;
} PlayerAssets;
typedef struct {
    Vector2 feet;
    Trainer trainer;
    Facing facing;
    float walk_time;
    bool moving;
    bool use_walk6;
} Player;
void player_assets_load(PlayerAssets *assets);
void player_assets_unload(PlayerAssets *assets);
void player_init(Player *player, Trainer trainer, Vector2 feet);
void player_update(Player *player, const World *world, float dt);
void player_draw(const Player *player, const PlayerAssets *assets);
#endif
