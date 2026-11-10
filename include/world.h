/* Mantém o cenário e o estado dos desafios lógicos da estufa. */
#ifndef WORLD_H
#define WORLD_H
#include "raylib.h"

enum { WORLD_RELAY_COUNT = 8, WORLD_RELAY_RULE_COUNT = 9 };

typedef enum {
    WORLD_ENCOUNTER_NONE,
    WORLD_ENCOUNTER_ALOE,
    WORLD_ENCOUNTER_SUNFLOWER
} WorldEncounter;

typedef enum {
    WORLD_EXTERIOR,
    WORLD_MAINTENANCE,
    WORLD_DEPOT,
    WORLD_BASEMENT,
    WORLD_NURSERY,
    WORLD_BRIDGE_CONTROL,
    WORLD_LOCATION_COUNT
} WorldLocation;

typedef struct {
    Rectangle bounds;
    Vector2 camera_position;
    WorldLocation location;
    bool water;
    bool light;
    bool maintenance_key;
    bool basement_visited;
    bool alarm;
    bool gate_unlocked;
    bool guardian_defeated;
    bool canal_note;
    bool alarm_note;
    bool north_channel;
    bool south_channel;
    bool drain_open;
    bool relay_panel_open;
    unsigned int relay_mask;
    unsigned int relay_attempts;
    int relay_step;
    bool relay_rejected;
    bool completed;
} World;

void world_init(World *world);
Vector2 world_spawn(void);
bool world_gate_open(const World *world);
bool world_gate_rule(const World *world);
bool world_bridge_open(const World *world);
int world_bridge_pressure(const World *world);
bool world_relays_match(unsigned int mask);
Vector2 world_constrain(const World *world, Vector2 previous, Vector2 feet);
WorldEncounter world_interact(World *world, Vector2 *feet);
void world_finish_encounter(World *world, WorldEncounter encounter);
void world_return_to_courtyard(World *world, Vector2 *feet);
void world_follow_camera(World *world, Vector2 feet, float dt);
void world_snap_camera(World *world, Vector2 feet);
Camera2D world_camera(const World *world);
void world_update_relay_panel(World *world);
void world_draw(const World *world, Vector2 feet);
void world_draw_interface(const World *world, Vector2 feet);
#endif
