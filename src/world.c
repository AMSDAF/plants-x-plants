/* Desenha a estufa e resolve os obstáculos a partir das proposições do cenário. */
#include "world.h"
#include <math.h>
#include <stddef.h>

typedef enum {
    VALVE,
    POWER,
    ALARM,
    NORTH_CHANNEL,
    SOUTH_CHANNEL,
    DRAIN,
    ALOE,
    SUNFLOWER,
    CANAL_NOTE,
    ALARM_NOTE,
    MAINTENANCE_DOOR,
    DEPOT_DOOR,
    NURSERY_DOOR,
    BRIDGE_CONTROL_DOOR,
    INTERIOR_DOOR,
    LOCKED_HATCH,
    BASEMENT_HATCH,
    UPPER_LIFT,
    LOWER_LIFT,
    RELAY_PANEL,
    OBJECT_COUNT
} Object;

typedef struct {
    Vector2 position;
    WorldLocation location;
} MapObject;

static const MapObject objects[OBJECT_COUNT] = {
    [VALVE] = {{1070, 305}, WORLD_EXTERIOR},
    [POWER] = {{640, 320}, WORLD_BASEMENT},
    [ALARM] = {{445, 320}, WORLD_MAINTENANCE},
    [NORTH_CHANNEL] = {{450, 330}, WORLD_BRIDGE_CONTROL},
    [SOUTH_CHANNEL] = {{830, 330}, WORLD_BRIDGE_CONTROL},
    [DRAIN] = {{335, 530}, WORLD_EXTERIOR},
    [ALOE] = {{640, 320}, WORLD_DEPOT},
    [SUNFLOWER] = {{515, 340}, WORLD_NURSERY},
    [CANAL_NOTE] = {{1070, 605}, WORLD_EXTERIOR},
    [ALARM_NOTE] = {{445, 460}, WORLD_MAINTENANCE},
    [MAINTENANCE_DOOR] = {{290, 398}, WORLD_EXTERIOR},
    [DEPOT_DOOR] = {{535, 398}, WORLD_EXTERIOR},
    [NURSERY_DOOR] = {{1715, 398}, WORLD_EXTERIOR},
    [BRIDGE_CONTROL_DOOR] = {{580, 692}, WORLD_EXTERIOR},
    [INTERIOR_DOOR] = {{640, 565}, WORLD_MAINTENANCE},
    [LOCKED_HATCH] = {{826, 320}, WORLD_MAINTENANCE},
    [BASEMENT_HATCH] = {{640, 565}, WORLD_BASEMENT},
    [UPPER_LIFT] = {{190, 625}, WORLD_EXTERIOR},
    [LOWER_LIFT] = {{837, 410}, WORLD_BASEMENT},
    [RELAY_PANEL] = {{790, 340}, WORLD_NURSERY}
};

static const Rectangle exterior = {70, 170, 1900, 640};
static const Rectangle canal = {785, 170, 128, 640};
static const Rectangle facades[] = {
    {187, 229, 206, 152}, {428, 229, 214, 152}, {1532, 229, 366, 152}, {470, 530, 220, 145}
};
static const float passage_top = 365;
static const float passage_bottom = 475;
static const float interaction_radius = 55;

static const Color ink = {24, 49, 43, 255};
static const Color frame = {39, 74, 60, 255};
static const Color cream = {249, 240, 205, 255};
static const Color grass = {175, 199, 142, 255};
static const Color stone = {220, 209, 165, 255};
static const Color water_color = {58, 145, 164, 255};
static const Color path_edge = {139, 123, 87, 255};
static const Rectangle validate_relays = {82, 596, 350, 48};
static const Rectangle previous_relay_rule = {449, 596, 248, 48};
static const Rectangle close_relays = {714, 596, 218, 48};
enum { CANAL_RELAY_RULE = 2, ALARM_RELAY_RULE = 7, ROOM_HEADER_HEIGHT = 69 };
enum { NORTH_PRESSURE = 3, SOUTH_PRESSURE = 5, DRAIN_PRESSURE = 2, BRIDGE_TARGET_PRESSURE = 3 };

typedef struct {
    Rectangle bounds;
    const char *name;
    Color floor;
    Color trim;
} Sector;

static const Sector sectors[] = {
    {{70, 170, 715, 640}, "Pátio", {165, 191, 141, 255}, {84, 119, 77, 255}},
    {{913, 170, 575, 640}, "Canais", {152, 191, 177, 255}, {55, 114, 114, 255}},
    {{1488, 170, 482, 640}, "Jardim do viveiro", {189, 207, 138, 255}, {100, 132, 64, 255}}
};

static const Sector rooms[WORLD_LOCATION_COUNT] = {
    [WORLD_MAINTENANCE] = {{280, 191, 720, 420}, "Casa de manutenção", {210, 198, 162, 255}, {111, 93, 66, 255}},
    [WORLD_DEPOT] = {{280, 191, 720, 420}, "Depósito", {202, 173, 139, 255}, {134, 92, 69, 255}},
    [WORLD_BASEMENT] = {{280, 191, 720, 420}, "Subsolo", {151, 170, 169, 255}, {72, 105, 111, 255}},
    [WORLD_NURSERY] = {{280, 191, 720, 420}, "Viveiro", {192, 213, 153, 255}, {82, 119, 62, 255}},
    [WORLD_BRIDGE_CONTROL] = {{280, 191, 720, 420}, "Controle da ponte", {163, 199, 186, 255}, {59, 114, 109, 255}}
};

static const Rectangle paths[] = {
    {70, 397, 1900, 78},
    {304, 430, 62, 103}, {1039, 300, 62, 310}, {159, 430, 62, 198},
    {702, 430, 56, 293}, {550, 675, 208, 48},
    {260, 370, 60, 70}, {505, 370, 60, 70}, {1685, 370, 60, 70}
};

void world_init(World *world)
{
    *world = (World){.bounds = exterior, .alarm = true, .drain_open = true};
}

Vector2 world_spawn(void)
{
    return (Vector2){146, 420};
}

bool world_gate_open(const World *world)
{
    return world->gate_unlocked || world_gate_rule(world);
}

bool world_gate_rule(const World *world)
{
    return world->water && world->light && !world->alarm;
}

bool world_bridge_open(const World *world)
{
    return world->north_channel != world->south_channel &&
           world_bridge_pressure(world) == BRIDGE_TARGET_PRESSURE;
}

int world_bridge_pressure(const World *world)
{
    int pressure = NORTH_PRESSURE * world->north_channel +
                   SOUTH_PRESSURE * world->south_channel -
                   DRAIN_PRESSURE * world->drain_open;
    return pressure > 0 ? pressure : 0;
}

static bool relay_on(unsigned int mask, int index)
{
    return (mask & (1u << index)) != 0;
}

static int active_relays(unsigned int mask)
{
    int count = 0;
    for (int i = 0; i < WORLD_RELAY_COUNT; i++)
        count += relay_on(mask, i);
    return count;
}

static int first_failed_relay_rule(unsigned int mask, int last_rule)
{
    bool a = relay_on(mask, 0), b = relay_on(mask, 1);
    bool c = relay_on(mask, 2), d = relay_on(mask, 3);
    bool e = relay_on(mask, 4), f = relay_on(mask, 5);
    bool g = relay_on(mask, 6), h = relay_on(mask, 7);

    const bool matches[WORLD_RELAY_RULE_COUNT] = {
        a != b, c == (a && d), e == (b != f),
        g == (c || e), h == (d != g), !f || !h,
        !a || h, !d || c, active_relays(mask) == 4
    };
    for (int i = 0; i <= last_rule; i++)
        if (!matches[i]) return i;
    return -1;
}

bool world_relays_match(unsigned int mask)
{
    return mask < (1u << WORLD_RELAY_COUNT) &&
           first_failed_relay_rule(mask, WORLD_RELAY_RULE_COUNT - 1) < 0;
}

static Rectangle relay_button(int index)
{
    return (Rectangle){82 + 142 * index, 193, 130, 87};
}

static bool clicked(Rectangle bounds)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), bounds);
}

static bool relay_clue_available(const World *world)
{
    if (world->relay_step == CANAL_RELAY_RULE) return world->canal_note;
    if (world->relay_step == ALARM_RELAY_RULE) return world->alarm_note;
    return true;
}

void world_update_relay_panel(World *world)
{
    if (!world->relay_panel_open) return;
    if (IsKeyPressed(KEY_ESCAPE) || clicked(close_relays)) {
        world->relay_panel_open = false;
        return;
    }
    if (world->relay_step > 0 &&
        (IsKeyPressed(KEY_LEFT) || clicked(previous_relay_rule))) {
        world->relay_step--;
        world->relay_rejected = false;
        return;
    }

    for (int i = 0; i < WORLD_RELAY_COUNT; i++) {
        if (IsKeyPressed(KEY_ONE + i) || clicked(relay_button(i))) {
            world->relay_mask ^= 1u << i;
            world->relay_rejected = false;
        }
    }

    if ((IsKeyPressed(KEY_ENTER) || clicked(validate_relays)) && relay_clue_available(world)) {
        world->relay_attempts++;
        int failed = first_failed_relay_rule(world->relay_mask, world->relay_step);
        world->relay_rejected = failed >= 0;
        if (world->relay_rejected) {
            world->relay_step = failed;
        } else if (world->relay_step == WORLD_RELAY_RULE_COUNT - 1) {
            world->completed = true;
            world->relay_panel_open = false;
        } else {
            world->relay_step++;
        }
    }
}

static bool blocks(Rectangle feet, Rectangle obstacle, bool open)
{
    return CheckCollisionRecs(feet, obstacle) &&
           (!open || feet.y < passage_top || feet.y + feet.height > passage_bottom);
}

Vector2 world_constrain(const World *world, Vector2 previous, Vector2 feet)
{
    Rectangle b = world->bounds;
    /* Caixa de 16 x 8 pixels centrada no ponto dos pés. */
    if (feet.x < b.x + 8) feet.x = b.x + 8;
    if (feet.x > b.x + b.width - 8) feet.x = b.x + b.width - 8;
    if (feet.y < b.y + 4) feet.y = b.y + 4;
    if (feet.y > b.y + b.height - 4) feet.y = b.y + b.height - 4;

    Rectangle box = {feet.x - 8, feet.y - 4, 16, 8};
    if (world->location == WORLD_EXTERIOR) {
        if (blocks(box, canal, world_bridge_open(world))) return previous;
        for (unsigned int i = 0; i < sizeof(facades) / sizeof(facades[0]); i++)
            if (CheckCollisionRecs(box, facades[i])) return previous;
    }
    return feet;
}

static Object nearby_object(const World *world, Vector2 feet)
{
    Object nearest = OBJECT_COUNT;
    float best_distance = interaction_radius * interaction_radius;
    for (int i = 0; i < OBJECT_COUNT; i++) {
        bool exit_door = i == INTERIOR_DOOR &&
                         (world->location == WORLD_DEPOT || world->location == WORLD_NURSERY ||
                          world->location == WORLD_BRIDGE_CONTROL);
        if (objects[i].location != world->location && !exit_door) continue;
        if ((i == ALOE && world->maintenance_key) || (i == SUNFLOWER && world->guardian_defeated)) continue;
        float dx = objects[i].position.x - feet.x;
        float dy = objects[i].position.y - feet.y;
        float distance = dx * dx + dy * dy;
        if (distance <= best_distance) {
            nearest = i;
            best_distance = distance;
        }
    }
    return nearest;
}

static void travel(World *world, Vector2 *feet, WorldLocation location, Vector2 destination)
{
    world->location = location;
    world->bounds = exterior;
    if (location != WORLD_EXTERIOR) {
        Rectangle room = rooms[location].bounds;
        world->bounds = (Rectangle){room.x + 5, room.y + ROOM_HEADER_HEIGHT,
                                    room.width - 10, room.height - ROOM_HEADER_HEIGHT - 5};
    }
    *feet = destination;
    world_snap_camera(world, destination);
}

void world_return_to_courtyard(World *world, Vector2 *feet)
{
    travel(world, feet, WORLD_EXTERIOR, world_spawn());
}

WorldEncounter world_interact(World *world, Vector2 *feet)
{
    if (world->completed || world->relay_panel_open) return WORLD_ENCOUNTER_NONE;
    Object object = nearby_object(world, *feet);
    const Vector2 entrance = {640, 533};
    switch (object) {
        case VALVE: world->water = !world->water; break;
        case POWER: world->light = !world->light; break;
        case ALARM: world->alarm = !world->alarm; break;
        case NORTH_CHANNEL: world->north_channel = !world->north_channel; break;
        case SOUTH_CHANNEL: world->south_channel = !world->south_channel; break;
        case DRAIN: world->drain_open = !world->drain_open; break;
        case ALOE: return WORLD_ENCOUNTER_ALOE;
        case SUNFLOWER: return WORLD_ENCOUNTER_SUNFLOWER;
        case CANAL_NOTE: world->canal_note = true; break;
        case ALARM_NOTE: world->alarm_note = true; break;
        case MAINTENANCE_DOOR: travel(world, feet, WORLD_MAINTENANCE, entrance); break;
        case DEPOT_DOOR: travel(world, feet, WORLD_DEPOT, entrance); break;
        case BRIDGE_CONTROL_DOOR: travel(world, feet, WORLD_BRIDGE_CONTROL, entrance); break;
        case NURSERY_DOOR:
            if (world_gate_open(world)) {
                world->gate_unlocked = true;
                travel(world, feet, WORLD_NURSERY, entrance);
            }
            break;
        case INTERIOR_DOOR: {
            Object door = MAINTENANCE_DOOR;
            if (world->location == WORLD_DEPOT) door = DEPOT_DOOR;
            if (world->location == WORLD_NURSERY) door = NURSERY_DOOR;
            if (world->location == WORLD_BRIDGE_CONTROL) door = BRIDGE_CONTROL_DOOR;
            travel(world, feet, WORLD_EXTERIOR, objects[door].position);
            break;
        }
        case LOCKED_HATCH:
            if (world->maintenance_key) {
                world->basement_visited = true;
                travel(world, feet, WORLD_BASEMENT, entrance);
            }
            break;
        case BASEMENT_HATCH:
            travel(world, feet, WORLD_MAINTENANCE, objects[LOCKED_HATCH].position);
            break;
        case UPPER_LIFT:
            if (world->basement_visited && !world->light)
                travel(world, feet, WORLD_BASEMENT, objects[LOWER_LIFT].position);
            break;
        case LOWER_LIFT:
            if (!world->light) travel(world, feet, WORLD_EXTERIOR, objects[UPPER_LIFT].position);
            break;
        case RELAY_PANEL: world->relay_panel_open = world->guardian_defeated; break;
        case OBJECT_COUNT: break;
    }
    return WORLD_ENCOUNTER_NONE;
}

void world_finish_encounter(World *world, WorldEncounter encounter)
{
    if (encounter == WORLD_ENCOUNTER_ALOE) world->maintenance_key = true;
    if (encounter == WORLD_ENCOUNTER_SUNFLOWER) world->guardian_defeated = true;
}

static Vector2 camera_target(const World *world, Vector2 feet)
{
    float x = feet.x - 640;
    float y = feet.y - 420;
    float max_x = fmaxf(0.0f, world->bounds.width - 1140);
    float max_y = fmaxf(0.0f, world->bounds.height - 470);
    if (x < 0) x = 0;
    if (x > max_x) x = max_x;
    if (y < 0) y = 0;
    if (y > max_y) y = max_y;
    return (Vector2){x, y};
}

void world_follow_camera(World *world, Vector2 feet, float dt)
{
    Vector2 target = camera_target(world, feet);
    float blend = 1.0f - expf(-10.0f * fminf(fmaxf(dt, 0.0f), 0.1f));
    world->camera_position.x += (target.x - world->camera_position.x) * blend;
    world->camera_position.y += (target.y - world->camera_position.y) * blend;
}

void world_snap_camera(World *world, Vector2 feet)
{
    world->camera_position = camera_target(world, feet);
}

Camera2D world_camera(const World *world)
{
    return (Camera2D){
        .target = {roundf(world->camera_position.x), roundf(world->camera_position.y)},
        .zoom = 1.0f
    };
}

static void centered_text(const char *text, int x, int y, int size, Color color)
{
    DrawText(text, x - MeasureText(text, size) / 2, y, size, color);
}

static void draw_sector(const Sector *sector)
{
    Rectangle b = sector->bounds;
    DrawRectangleRec(b, sector->floor);
    DrawRectangleLinesEx(b, 5, sector->trim);
    for (int y = (int)b.y + 30; y < b.y + b.height - 18; y += 62) {
        int offset = ((y - (int)b.y) / 62) % 2 == 0 ? 0 : 26;
        for (int x = (int)b.x + 24 + offset; x < b.x + b.width - 30; x += 66) {
            DrawRectangle(x, y, 22, 2, Fade(sector->trim, 0.30f));
            DrawRectangle(x + 7, y + 5, 7, 2, Fade(sector->trim, 0.22f));
        }
    }
}

static void draw_paths(void)
{
    for (unsigned int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        Rectangle p = paths[i];
        DrawRectangleRec((Rectangle){p.x - 3, p.y - 3, p.width + 6, p.height + 6}, path_edge);
    }
    for (unsigned int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
        DrawRectangleRec(paths[i], stone);
}

static void draw_sign(const char *label, int x, int y, int font, Color background)
{
    int width = MeasureText(label, font) + 26;
    Rectangle plaque = {x - width / 2, y, width, font + 14};
    DrawRectangleRec(plaque, background);
    DrawRectangleLinesEx(plaque, 2, Fade(ink, 0.52f));
    centered_text(label, x, y + 7, font, cream);
}

static void draw_floor_details(void)
{
    /* Canteiros e drenos são marcas no piso, fora das rotas. */
    const Color bed = {99, 139, 79, 255};
    DrawRectangle(92, 549, 55, 51, Fade(bed, 0.75f));
    DrawRectangle(281, 648, 122, 51, Fade(bed, 0.75f));
    DrawRectangle(1234, 519, 222, 65, Fade(water_color, 0.48f));
    DrawRectangle(1234, 643, 222, 65, Fade(water_color, 0.48f));
    DrawRectangle(1586, 515, 137, 69, Fade(bed, 0.80f));
    DrawRectangle(1786, 515, 137, 69, Fade(bed, 0.80f));
    for (int i = 0; i < 4; i++) {
        DrawRectangle(296 + 26 * i, 667, 16, 10, grass);
        DrawRectangle(1255 + 52 * i, 545, 27, 6, SKYBLUE);
        DrawRectangle(1255 + 52 * i, 669, 27, 6, SKYBLUE);
        DrawRectangle(1598 + 32 * i, 544, 16, 15, grass);
        DrawRectangle(1798 + 32 * i, 544, 16, 15, grass);
    }
}

static void draw_interaction_marker(Object object)
{
    Vector2 p = objects[object].position;
    DrawCircleLines((int)p.x, (int)p.y, 36, GOLD);
    DrawCircleLines((int)p.x, (int)p.y, 37, GOLD);
    DrawRectangle((int)p.x + 31, (int)p.y - 57, 24, 22, ink);
    centered_text("E", (int)p.x + 43, (int)p.y - 54, 16, cream);
}

static const Sector *sector_at(Vector2 feet)
{
    for (unsigned int i = 0; i < sizeof(sectors) / sizeof(sectors[0]); i++)
        if (CheckCollisionPointRec(feet, sectors[i].bounds)) return &sectors[i];
    return NULL;
}

static void draw_switch(Object object, const char *name, bool on)
{
    Vector2 p = objects[object].position;
    int x = (int)p.x;
    int y = (int)p.y;
    DrawEllipse(x, y + 17, 29, 9, Fade(ink, 0.25f));
    DrawRectangle(x - 24, y - 19, 48, 37, frame);
    DrawRectangle(x - 20, y - 15, 40, 29, ink);
    DrawCircle(x, y - 1, 10, on ? LIME : ORANGE);
    DrawCircle(x - 2, y - 4, 3, cream);
    centered_text(name, x, y + 25, 18, ink);
}

static void draw_note(Object object, const char *name, bool collected)
{
    int x = (int)objects[object].position.x;
    int y = (int)objects[object].position.y;
    DrawRectangle(x - 17, y - 23, 34, 36, collected ? cream : GOLD);
    for (int i = 0; i < 3; i++) DrawRectangle(x - 10, y - 15 + i * 7, 20, 2, frame);
    centered_text(name, x, y + 23, 18, ink);
}

static void draw_guard(Object object, const char *name, Color color)
{
    int x = (int)objects[object].position.x;
    int y = (int)objects[object].position.y;
    DrawEllipse(x, y + 9, 27, 7, Fade(ink, 0.3f));
    DrawRectangle(x - 5, y - 19, 10, 30, DARKGREEN);
    DrawEllipse(x - 14, y - 3, 16, 7, DARKGREEN);
    DrawEllipse(x + 15, y - 8, 15, 7, DARKGREEN);
    DrawCircle(x, y - 23, 22, color);
    DrawCircle(x, y - 22, 11, frame);
    centered_text(name, x, y + 22, 18, ink);
}

static void draw_facade(Rectangle b, const char *label, Color roof, bool locked)
{
    int x = (int)b.x;
    int y = (int)b.y;
    int w = (int)b.width;
    int bottom = (int)(b.y + b.height);
    DrawRectangleRec(b, stone);
    DrawRectangle(x, y, w, (int)b.height - 68, roof);
    for (int row = y + 12; row < bottom - 72; row += 14)
        DrawRectangle(x + 4, row, w - 8, 2, Fade(ink, 0.25f));
    DrawRectangle(x, bottom - 70, w, 8, frame);
    for (int side = 0; side < 2; side++) {
        int window = side == 0 ? x + 16 : x + w - 56;
        DrawRectangle(window, bottom - 51, 40, 28, frame);
        DrawRectangle(window + 4, bottom - 47, 32, 20, SKYBLUE);
        DrawRectangle(window + 18, bottom - 47, 3, 20, frame);
    }
    DrawRectangle(x + w / 2 - 24, bottom - 58, 48, 58, ink);
    DrawRectangle(x + w / 2 - 19, bottom - 52, 38, 52, locked ? MAROON : roof);
    DrawCircle(x + w / 2 + 10, bottom - 22, 3, GOLD);
    DrawRectangleLinesEx(b, 3, frame);
    draw_sign(label, x + w / 2, y + 16, 16, frame);
}

static void draw_room_portal(Object object, const char *label, bool locked)
{
    Vector2 p = objects[object].position;
    int x = (int)p.x, y = (int)p.y;
    DrawRectangle(x - 31, y - 26, 62, 43, frame);
    DrawRectangle(x - 25, y - 20, 50, 31, locked ? MAROON : path_edge);
    DrawRectangle(x - 12, y - 9, 24, 5, cream);
    centered_text(label, x, y + 22, 18, ink);
}

static void draw_bridge(bool open)
{
    DrawRectangleRec(canal, water_color);
    int x = (int)canal.x;
    int w = (int)canal.width;
    for (int y = (int)canal.y + 15; y + 10 <= canal.y + canal.height; y += 30) {
        DrawRectangle(x + 12, y, 23, 3, SKYBLUE);
        DrawRectangle(x + 73, y + 7, 36, 3, SKYBLUE);
    }
    if (open) {
        for (int px = x; px < x + w; px += 16) {
            DrawRectangle(px, (int)passage_top, 15, (int)(passage_bottom - passage_top), BROWN);
            DrawRectangle(px, (int)passage_top + 4, 14, (int)(passage_bottom - passage_top) - 8,
                          (Color){174, 128, 77, 255});
        }
    }
}

static void draw_exterior(const World *world)
{
    Rectangle b = world->bounds;
    DrawRectangleRec(b, grass);
    for (unsigned int i = 0; i < sizeof(sectors) / sizeof(sectors[0]); i++) {
        const Sector *s = &sectors[i];
        draw_sector(s);
        int label_x = (int)(s->bounds.x + fminf(s->bounds.width / 2, 180));
        draw_sign(s->name, label_x, (int)s->bounds.y + 22, 23, s->trim);
    }
    draw_floor_details();
    draw_paths();
    draw_bridge(world_bridge_open(world));
    draw_facade(facades[0], "Manutenção", (Color){145, 116, 79, 255}, false);
    draw_facade(facades[1], "Depósito", (Color){159, 101, 73, 255}, false);
    draw_facade(facades[2], "Viveiro", (Color){83, 143, 106, 255}, !world_gate_open(world));
    draw_facade(facades[3], "Controle da ponte", (Color){76, 131, 131, 255}, false);
    draw_sign("Canais >", 706, 427, 16, frame);
    draw_sign("< Pátio", 979, 427, 16, frame);
    draw_sign("Viveiro >", 1390, 427, 16, frame);
    draw_sign("Entrada pelo sul", 580, 496, 16, frame);
    draw_switch(VALVE, "Válvula da água", world->water);
    draw_switch(DRAIN, TextFormat("Dreno (-%d)", DRAIN_PRESSURE), world->drain_open);
    draw_switch(UPPER_LIFT, "Elevador de serviço", world->basement_visited && !world->light);
    draw_note(CANAL_NOTE, "Pista dos canais", world->canal_note);
    DrawRectangleLinesEx(b, 3, frame);
}

static void draw_interior(const World *world)
{
    const Sector *room = &rooms[world->location];
    Rectangle b = room->bounds;
    draw_sector(room);
    DrawRectangleRec((Rectangle){b.x, b.y, b.width, ROOM_HEADER_HEIGHT}, room->trim);
    centered_text(room->name, 640, 212, 29, cream);
    DrawRectangle(610, 261, 60, 347, Fade(stone, 0.62f));

    switch (world->location) {
        case WORLD_MAINTENANCE:
            draw_switch(ALARM, "Painel do alarme", world->alarm);
            draw_note(ALARM_NOTE, "Pista da cabine", world->alarm_note);
            draw_room_portal(LOCKED_HATCH, world->maintenance_key ? "Escotilha" : "Escotilha trancada",
                             !world->maintenance_key);
            break;
        case WORLD_DEPOT:
            for (int i = 0; i < 3; i++) {
                DrawRectangle(330, 283 + 75 * i, 115, 44, Fade(room->trim, 0.45f));
                DrawRectangle(835, 283 + 75 * i, 115, 44, Fade(room->trim, 0.45f));
            }
            if (!world->maintenance_key) draw_guard(ALOE, "Babosa guardiã", (Color){73, 162, 120, 255});
            else centered_text("Chave da manutenção obtida", 640, 325, 22, ink);
            break;
        case WORLD_BASEMENT:
            draw_switch(POWER, "Distribuidor de energia", world->light);
            draw_switch(LOWER_LIFT, "Elevador", !world->light);
            draw_room_portal(BASEMENT_HATCH, "Subir pela escotilha", false);
            break;
        case WORLD_NURSERY:
            DrawRectangle(306, 284, 116, 194, Fade(DARKGREEN, 0.27f));
            DrawRectangle(875, 284, 99, 194, Fade(DARKGREEN, 0.27f));
            if (!world->guardian_defeated) draw_guard(SUNFLOWER, "Girassol guardião", GOLD);
            draw_switch(RELAY_PANEL, "Central de relés", world->guardian_defeated);
            break;
        case WORLD_BRIDGE_CONTROL:
            DrawRectangle(450, 396, 380, 9, frame);
            DrawRectangle(450, 396, 190, 9, world->north_channel ? water_color : frame);
            DrawRectangle(640, 396, 190, 9, world->south_channel ? water_color : frame);
            draw_switch(NORTH_CHANNEL, TextFormat("Canal norte (+%d)", NORTH_PRESSURE), world->north_channel);
            draw_switch(SOUTH_CHANNEL, TextFormat("Canal sul (+%d)", SOUTH_PRESSURE), world->south_channel);
            centered_text(TextFormat("Dreno externo %s", world->drain_open ? "aberto" : "fechado"),
                          640, 435, 20, ink);
            break;
        default: break;
    }
    if (world->location != WORLD_BASEMENT) draw_room_portal(INTERIOR_DOOR, "Voltar ao pátio", false);
}

void world_draw(const World *world, Vector2 feet)
{
    if (world->location == WORLD_EXTERIOR) draw_exterior(world);
    else draw_interior(world);

    Object nearby = nearby_object(world, feet);
    if (!world->relay_panel_open && !world->completed && nearby != OBJECT_COUNT)
        draw_interaction_marker(nearby);
}

static const char *truth(bool value)
{
    return value ? "V" : "F";
}

static void draw_formula(int x, const char *label, const char *evaluation, const char *status, bool open)
{
    DrawRectangle(x, 66, 560, 83, cream);
    DrawRectangle(x, 66, 6, 83, open ? LIME : ORANGE);
    DrawText(label, x + 15, 77, 20, ink);
    DrawText(evaluation, x + 15, 112, 20, ink);
    DrawText(status, x + 550 - MeasureText(status, 18), 113, 18, open ? DARKGREEN : MAROON);
}

static void draw_relay_action(Rectangle bounds, const char *label, bool enabled)
{
    bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
    DrawRectangleRec(bounds, enabled ? (hovered ? GOLD : stone) : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, frame);
    centered_text(label, (int)(bounds.x + bounds.width / 2), (int)bounds.y + 14, 20,
                  enabled ? ink : DARKGRAY);
}

static void draw_relay_panel(const World *world)
{
    static const struct {
        const char *formula;
        const char *guide;
    } clues[WORLD_RELAY_RULE_COUNT] = {
        {"A XOR B", "No XOR, exatamente um dos dois relés deve estar ligado."},
        {"C SE E SOMENTE SE (A E D)", "C deve ter o mesmo valor da expressão entre parênteses."},
        {"E SE E SOMENTE SE (B XOR F)", "Avalie o XOR entre B e F e compare o resultado com o relé E."},
        {"G SE E SOMENTE SE (C OU E)", "O OU é verdadeiro com um ou com os dois relés ligados."},
        {"H SE E SOMENTE SE (D XOR G)", "Compare H com o resultado do XOR entre D e G."},
        {"SE F, ENTÃO NÃO H", "A regra só falha se a primeira parte for V e a segunda for F."},
        {"SE A, ENTÃO H", "Se a primeira parte for F, a implicação já está satisfeita."},
        {"SE D, ENTÃO C", "Verifique a implicação junto com as equivalências que você já passou."},
        {"Exatamente 4 relés ligados", "Confira o total. As oito regras anteriores ainda precisam valer."}
    };
    const int step = world->relay_step;
    const bool available = relay_clue_available(world);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(ink, 0.88f));
    DrawRectangle(62, 40, 1160, 640, cream);
    DrawRectangleLinesEx((Rectangle){62, 40, 1160, 640}, 4, frame);
    DrawText("Central de relés", 82, 64, 36, ink);
    DrawText("Valide uma regra por vez. As regras anteriores continuam valendo.", 82, 117, 23, ink);
    DrawText("Use 1 a 8 ou clique nos relés. Ligado = verdadeiro.", 82, 153, 20, ink);

    for (int i = 0; i < WORLD_RELAY_COUNT; i++) {
        Rectangle bounds = relay_button(i);
        bool on = relay_on(world->relay_mask, i);
        int x = (int)bounds.x;
        int y = (int)bounds.y;
        DrawRectangleRec(bounds, on ? (Color){201, 225, 171, 255} : stone);
        DrawRectangleLinesEx(bounds, 2, frame);
        DrawText(TextFormat("%d", i + 1), x + 9, y + 8, 15, ink);
        centered_text(TextFormat("%c", 'A' + i), x + 65, y + 13, 30, ink);
        DrawCircle(x + 18, y + 65, 6, on ? DARKGREEN : MAROON);
        DrawText(on ? "Ligado" : "Desligado", x + 31, y + 56, 16, ink);
    }

    const Rectangle rule_panel = {82, 303, 1124, 266};
    DrawRectangleRec(rule_panel, (Color){237, 228, 190, 255});
    DrawRectangleLinesEx(rule_panel, world->relay_rejected ? 4 : 2, world->relay_rejected ? MAROON : frame);
    DrawText(TextFormat("Regra %02d de %02d", step + 1, WORLD_RELAY_RULE_COUNT), 104, 324, 23, ink);
    if (available) {
        DrawText(clues[step].formula, 104, 378, 31, ink);
        DrawText(clues[step].guide, 104, 429, 20, ink);
        if (world->relay_rejected)
            DrawText("Esta regra falhou. Ajuste os relés e tente novamente.", 104, 505, 22, MAROON);
        else if (step == WORLD_RELAY_RULE_COUNT - 1)
            DrawText("Valide para abrir a saída.", 104, 505, 21, ink);
        else
            DrawText("Valide para revelar a próxima regra.", 104, 505, 21, ink);
    } else {
        DrawText("Pista necessária", 104, 378, 31, ink);
        DrawText(step == CANAL_RELAY_RULE ? "Registre a pista nos canais para revelar esta regra."
                                        : "Registre a pista na cabine do alarme para revelar esta regra.",
                 104, 429, 20, ink);
    }

    draw_relay_action(validate_relays, "ENTER  Validar regra", available);
    draw_relay_action(previous_relay_rule, "ESQ  Regra anterior", step > 0);
    draw_relay_action(close_relays, "ESC  Fechar", true);
    DrawText(TextFormat("Relés ligados: %d", active_relays(world->relay_mask)), 950, 597, 18, ink);
    DrawText(TextFormat("Tentativas: %u", world->relay_attempts), 950, 625, 18, ink);
}

void world_draw_interface(const World *world, Vector2 feet)
{
    static const char *const prompts[OBJECT_COUNT] = {
        [VALVE] = "E  Alternar água",
        [POWER] = "E  Alternar energia entre o refletor e o elevador",
        [ALARM] = "E  Alternar alarme",
        [NORTH_CHANNEL] = "E  Alternar canal norte",
        [SOUTH_CHANNEL] = "E  Alternar canal sul",
        [DRAIN] = "E  Alternar dreno",
        [ALOE] = "E  Desafiar Babosa pela Chave da Manutenção",
        [SUNFLOWER] = "E  Desafiar Girassol para liberar a central",
        [CANAL_NOTE] = "E  Registrar pista dos canais",
        [ALARM_NOTE] = "E  Registrar pista da cabine",
        [MAINTENANCE_DOOR] = "E  Entrar na casa de manutenção",
        [DEPOT_DOOR] = "E  Entrar no depósito",
        [NURSERY_DOOR] = "E  Entrar no viveiro",
        [BRIDGE_CONTROL_DOOR] = "E  Entrar no controle da ponte",
        [INTERIOR_DOOR] = "E  Voltar ao pátio",
        [LOCKED_HATCH] = "E  Descer pela escotilha",
        [BASEMENT_HATCH] = "E  Subir pela escotilha",
        [UPPER_LIFT] = "E  Descer pelo elevador",
        [LOWER_LIFT] = "E  Subir pelo elevador",
        [RELAY_PANEL] = "E  Abrir central de relés"
    };
    bool gate_open = world_gate_open(world);
    bool bridge_open = world_bridge_open(world);
    bool one_channel = world->north_channel != world->south_channel;
    int pressure = world_bridge_pressure(world);
    const char *bridge_status = "Estendida";
    if (!one_channel) bridge_status = pressure == BRIDGE_TARGET_PRESSURE ? "XOR falso" : "XOR e pressão";
    else if (pressure < BRIDGE_TARGET_PRESSURE) bridge_status = "Pressão baixa";
    else if (pressure > BRIDGE_TARGET_PRESSURE) bridge_status = "Pressão alta";
    const Sector *sector = world->location == WORLD_EXTERIOR ? sector_at(feet) : &rooms[world->location];
    DrawText("Estufa lógica", 70, 19, 33, cream);
    draw_sign(TextFormat("Setor atual  %s", sector ? sector->name : "Travessia"),
              541, 19, 18, sector ? sector->trim : frame);
    DrawText(TextFormat("Pistas %d/2   |   Guardião %s", world->canal_note + world->alarm_note,
                        world->guardian_defeated ? "vencido" : "aguardando"), 826, 29, 18, cream);
    draw_formula(70, "Viveiro   Água E Luz E NÃO Alarme",
                 TextFormat("%s E %s E NÃO %s = %s", truth(world->water), truth(world->light),
                            truth(world->alarm), truth(world_gate_rule(world))),
                 world->gate_unlocked ? "Travada aberta" : gate_open ? "Aberta" : "Fechada", gate_open);
    draw_formula(650, TextFormat("Ponte   (Norte XOR Sul) E pressão em %d", BRIDGE_TARGET_PRESSURE),
                 TextFormat("N %s XOR S %s = %s   |   Pressão %d/%d", truth(world->north_channel),
                            truth(world->south_channel), truth(one_channel), pressure, BRIDGE_TARGET_PRESSURE),
                 bridge_status, bridge_open);

    Object object = nearby_object(world, feet);
    const char *hint = "Pátio: manutenção e depósito ao norte. Controle da ponte ao sul.";
    if (world->location == WORLD_MAINTENANCE) hint = "Manutenção: alarme e pista à esquerda, escotilha à direita.";
    if (world->location == WORLD_DEPOT) hint = world->maintenance_key
        ? "Chave obtida. Use a escotilha na casa de manutenção."
        : "A Babosa guarda a chave da escotilha.";
    if (world->location == WORLD_BASEMENT) hint = "Subsolo: distribuidor ao norte e escotilha ao sul.";
    if (world->location == WORLD_BRIDGE_CONTROL)
        hint = "Abra um canal e confira a pressão. O dreno fica do lado de fora.";
    if (world->location == WORLD_NURSERY) hint = world->guardian_defeated
        ? "Guardião vencido. Use as pistas na Central de Relés."
        : "Viveiro: vença o Girassol para liberar a central.";
    if (world->location == WORLD_EXTERIOR && feet.x > canal.x + canal.width)
        hint = "Canais: válvula ao norte, pista ao sul e viveiro a leste.";
    if (object != OBJECT_COUNT) {
        hint = prompts[object];
        if (object == DRAIN) hint = world->drain_open ? "Dreno aberto. E  Fechar dreno"
                                                     : "Dreno fechado. E  Abrir dreno";
        if (object == POWER) hint = world->light
            ? "Refletor energizado. E  Alimentar elevador" : "Elevador energizado. E  Alimentar refletor";
        if ((object == UPPER_LIFT || object == LOWER_LIFT) && world->light)
            hint = "Elevador sem energia. Use a escotilha da casa de manutenção.";
        if (object == UPPER_LIFT && !world->basement_visited)
            hint = "Elevador travado. Entre no subsolo pela escotilha primeiro.";
        if (object == LOCKED_HATCH && !world->maintenance_key)
            hint = "Escotilha trancada. A Babosa guarda a chave no depósito.";
        if (object == NURSERY_DOOR && !gate_open)
            hint = "Viveiro trancado. Confira as condições no painel acima.";
        if (object == CANAL_NOTE && world->canal_note) hint = "Pista dos canais registrada na central.";
        if (object == ALARM_NOTE && world->alarm_note) hint = "Pista da cabine registrada na central.";
        if (object == RELAY_PANEL && !world->guardian_defeated)
            hint = "Central bloqueada até você vencer o Girassol.";
    }
    DrawText(hint, 70, 650, 21, cream);
    DrawText("WASD / setas  Mover    SHIFT  Correr    E  Interagir    ESC  Menu", 70, 682, 17, RAYWHITE);

    if (world->relay_panel_open) draw_relay_panel(world);
    if (world->completed) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(ink, 0.83f));
        DrawRectangle(310, 260, 660, 190, cream);
        DrawRectangleLinesEx((Rectangle){310, 260, 660, 190}, 4, frame);
        centered_text("Estufa concluída!", 640, 291, 36, ink);
        centered_text("Você venceu o guardião e validou os oito relés.", 640, 350, 22, ink);
        centered_text("ENTER ou ESC  Voltar ao menu", 640, 401, 20, ink);
    }
}
