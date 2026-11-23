/* Mantém o cenário e o estado dos desafios lógicos da estufa. */
#ifndef WORLD_H
#define WORLD_H
#include "raylib.h"
#include "ui_theme.h"
#include "exploration_hud.h"

enum {
    WORLD_RELAY_COUNT = 3, WORLD_RELAY_RULE_COUNT = 3,
    WORLD_NPC_COUNT = 3, WORLD_DIALOGUE_PAGE_COUNT = 3
};

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
    Texture2D courtyard;
    Texture2D bridge_open;
    Texture2D tiles;
    Texture2D maintenance_interior;
    Texture2D courtyard_detail;
    Texture2D tree_canopy_foreground;
    Texture2D depot_interior;
    Texture2D basement_interior;
    Texture2D bridge_control_interior;
    Texture2D nursery_interior;
    Texture2D interior_portal_foreground[WORLD_LOCATION_COUNT];
    Texture2D aloe_guard;
    Texture2D sunflower_guard;
} WorldAssets;

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
    bool detail_enabled;
    const char *dialogue_title;
    const char *dialogue_pages[WORLD_DIALOGUE_PAGE_COUNT];
    char dialogue_introduction[512];
    char dialogue_greeting[512];
    bool npc_introduced[WORLD_NPC_COUNT];
    bool npc_help_thanked[WORLD_NPC_COUNT];
    int dialogue_page;
    int dialogue_visible_characters;
    float dialogue_character_timer;
    bool logic_quiz_open;
    unsigned int logic_move_mask;
    bool relay_panel_open;
    unsigned int relay_mask;
    unsigned int relay_attempts;
    int relay_step;
    int relay_cursor;
    bool relay_rejected;
    bool completed;
} World;

void world_init(World *world);
void world_assets_load(WorldAssets *assets);
void world_assets_unload(WorldAssets *assets);
Vector2 world_spawn(void);
bool world_gate_open(const World *world);
bool world_gate_rule(const World *world);
bool world_bridge_open(const World *world);
int world_bridge_pressure(const World *world);
bool world_relays_match(unsigned int mask);
Vector2 world_constrain(const World *world, Vector2 previous, Vector2 feet);
WorldEncounter world_interact(World *world, Vector2 *feet, const char *player_name);
void world_finish_encounter(World *world, WorldEncounter encounter);
void world_return_to_courtyard(World *world, Vector2 *feet);
void world_follow_camera(World *world, Vector2 feet, float dt);
void world_snap_camera(World *world, Vector2 feet);
Camera2D world_camera(const World *world);
Rectangle world_relay_bounds(int index);
Rectangle world_relay_close_bounds(void);
Rectangle world_relay_validate_bounds(void);
void world_update_relay_panel(World *world, const char *player_name);
void world_update_dialogue(World *world, float dt);
void world_draw(const UiTheme *ui, const World *world, const WorldAssets *assets);
void world_draw_interior_foreground(const World *world, const WorldAssets *assets, Vector2 feet);
void world_draw_npcs(const World *world, const WorldAssets *assets, Vector2 feet, Camera2D camera, bool foreground);
void world_draw_tree_canopy(const World *world, const WorldAssets *assets, Vector2 feet);
HudInteraction world_interaction_view(const World *world, Vector2 feet);
void world_draw_interface(const UiTheme *ui, const World *world, Vector2 feet);
#endif
