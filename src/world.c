/* Desenha a estufa e resolve os obstáculos a partir das proposições do cenário. */
#include "world.h"

typedef enum {
    VALVE,
    LIGHT,
    KEY,
    NORTH_CHANNEL,
    SOUTH_CHANNEL,
    EXIT,
    OBJECT_COUNT
} Object;

static const Vector2 objects[OBJECT_COUNT] = {
    [VALVE] = {218, 305},
    [LIGHT] = {397, 305},
    [KEY] = {342, 536},
    [NORTH_CHANNEL] = {675, 305},
    [SOUTH_CHANNEL] = {675, 536},
    [EXIT] = {1080, 420}
};

static const Rectangle gate = {520, 170, 32, 470};
static const Rectangle canal = {785, 170, 128, 470};
static const float passage_top = 365;
static const float passage_bottom = 475;
static const float interaction_radius = 55;

static const Color ink = {24, 49, 43, 255};
static const Color frame = {39, 74, 60, 255};
static const Color cream = {249, 240, 205, 255};
static const Color grass = {175, 199, 142, 255};
static const Color stone = {220, 209, 165, 255};
static const Color water_color = {58, 145, 164, 255};

void world_init(World *world)
{
    *world = (World){.bounds = {70, 170, 1140, 470}};
}

Vector2 world_spawn(void)
{
    return (Vector2){146, 420};
}

bool world_gate_open(const World *world)
{
    return world->water && (world->light || world->key_collected);
}

bool world_bridge_open(const World *world)
{
    return world->north_channel != world->south_channel;
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
    if (blocks(box, gate, world_gate_open(world)) ||
        blocks(box, canal, world_bridge_open(world)))
        return previous;
    return feet;
}

static Object nearby_object(const World *world, Vector2 feet)
{
    for (int i = 0; i < OBJECT_COUNT; i++) {
        if (i == KEY && world->key_collected) continue;
        float dx = objects[i].x - feet.x;
        float dy = objects[i].y - feet.y;
        if (dx * dx + dy * dy <= interaction_radius * interaction_radius) return i;
    }
    return OBJECT_COUNT;
}

void world_interact(World *world, Vector2 feet)
{
    if (world->completed) return;
    switch (nearby_object(world, feet)) {
        case VALVE: world->water = !world->water; break;
        case LIGHT: world->light = !world->light; break;
        case KEY: world->key_collected = true; break;
        case NORTH_CHANNEL: world->north_channel = !world->north_channel; break;
        case SOUTH_CHANNEL: world->south_channel = !world->south_channel; break;
        case EXIT: world->completed = world_gate_open(world) && world_bridge_open(world); break;
        case OBJECT_COUNT: break;
    }
}

static void centered_text(const char *text, int x, int y, int size, Color color)
{
    DrawText(text, x - MeasureText(text, size) / 2, y, size, color);
}

static void draw_switch(Object object, const char *name, bool on)
{
    Vector2 p = objects[object];
    int x = (int)p.x;
    int y = (int)p.y;
    DrawEllipse(x, y + 17, 29, 9, Fade(ink, 0.25f));
    DrawRectangle(x - 24, y - 19, 48, 37, frame);
    DrawRectangle(x - 20, y - 15, 40, 29, ink);
    DrawCircle(x, y - 1, 10, on ? LIME : ORANGE);
    DrawCircle(x - 2, y - 4, 3, cream);
    centered_text(name, x, y + 25, 18, ink);
}

static void draw_key(void)
{
    int x = (int)objects[KEY].x;
    int y = (int)objects[KEY].y;
    DrawEllipse(x, y + 15, 27, 8, Fade(ink, 0.25f));
    DrawCircle(x - 13, y - 3, 12, GOLD);
    DrawCircle(x - 13, y - 3, 5, grass);
    DrawRectangle(x - 3, y - 6, 24, 7, GOLD);
    DrawRectangle(x + 13, y, 7, 8, GOLD);
    centered_text("Chave", x, y + 24, 18, ink);
}

static void draw_gate(bool open)
{
    int x = (int)gate.x;
    int w = (int)gate.width;
    DrawRectangle(x, (int)gate.y, w, (int)(passage_top - gate.y), frame);
    DrawRectangle(x, (int)passage_bottom, w, (int)(gate.y + gate.height - passage_bottom), frame);
    DrawRectangle(x - 6, (int)passage_top - 8, w + 12, 8, ink);
    DrawRectangle(x - 6, (int)passage_bottom, w + 12, 8, ink);
    if (!open) {
        DrawRectangle(x, (int)passage_top, w, (int)(passage_bottom - passage_top), ink);
        for (int y = (int)passage_top + 10; y < passage_bottom; y += 18)
            DrawRectangle(x + 3, y, w - 6, 5, ORANGE);
    }
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

void world_draw(const World *world)
{
    Rectangle b = world->bounds;
    ClearBackground(ink);
    DrawRectangleRec(b, grass);
    for (int x = (int)b.x; x < b.x + b.width; x += 40)
        DrawLine(x, (int)b.y, x, (int)(b.y + b.height), (Color){161, 185, 132, 255});
    for (int y = (int)b.y; y < b.y + b.height; y += 40)
        DrawLine((int)b.x, y, (int)(b.x + b.width), y, (Color){161, 185, 132, 255});
    DrawRectangle((int)b.x, (int)passage_top, (int)b.width, (int)(passage_bottom - passage_top), stone);
    DrawRectangle(305, (int)passage_bottom, 74, 82, (Color){190, 180, 135, 255});

    centered_text("01  Comporta", 295, 192, 24, ink);
    centered_text("02  Irrigação", 665, 192, 24, ink);
    centered_text("Saída", 1055, 192, 24, ink);
    draw_gate(world_gate_open(world));
    draw_bridge(world_bridge_open(world));
    draw_switch(VALVE, "Válvula", world->water);
    draw_switch(LIGHT, "Refletor", world->light);
    draw_switch(NORTH_CHANNEL, "Canal norte", world->north_channel);
    draw_switch(SOUTH_CHANNEL, "Canal sul", world->south_channel);
    if (!world->key_collected) draw_key();

    DrawRectangle(1055, 389, 50, 45, frame);
    DrawRectangle(1061, 395, 38, 32, cream);
    centered_text(">", 1080, 397, 26, ink);
    centered_text("Saída da estufa", 1080, 446, 18, ink);
    DrawRectangleLinesEx(b, 3, frame);
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

void world_draw_interface(const World *world, Vector2 feet)
{
    static const char *const prompts[OBJECT_COUNT] = {
        [VALVE] = "E  Alternar água",
        [LIGHT] = "E  Alternar luz",
        [KEY] = "E  Pegar chave",
        [NORTH_CHANNEL] = "E  Alternar canal norte",
        [SOUTH_CHANNEL] = "E  Alternar canal sul",
        [EXIT] = "E  Concluir missão"
    };
    bool gate_open = world_gate_open(world);
    bool bridge_open = world_bridge_open(world);
    DrawText("Estufa lógica", 70, 19, 33, cream);
    draw_formula(70, "Comporta   Água E (Luz OU Chave)",
                 TextFormat("%s E (%s OU %s) = %s", truth(world->water), truth(world->light),
                            truth(world->key_collected), truth(gate_open)),
                 gate_open ? "Aberta" : "Fechada", gate_open);
    draw_formula(650, "Ponte   Norte XOR Sul",
                 TextFormat("%s XOR %s = %s", truth(world->north_channel), truth(world->south_channel),
                            truth(bridge_open)),
                 bridge_open ? "Estendida" : "Recolhida", bridge_open);

    Object object = nearby_object(world, feet);
    const char *hint = "Abra a comporta. A água é necessária e basta luz ou chave.";
    if (feet.x > canal.x + canal.width) hint = "Atravesse até a placa de saída.";
    else if (feet.x > gate.x + gate.width) hint = "Para estender a ponte, deixe exatamente um canal aberto.";
    if (object != OBJECT_COUNT) hint = prompts[object];
    DrawText(hint, 70, 650, 21, cream);
    DrawText("WASD / setas  Mover     E  Interagir     ESC  Menu", 70, 682, 17, RAYWHITE);

    if (world->completed) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(ink, 0.83f));
        DrawRectangle(310, 260, 660, 190, cream);
        DrawRectangleLinesEx((Rectangle){310, 260, 660, 190}, 4, frame);
        centered_text("Estufa concluída!", 640, 291, 36, ink);
        centered_text("Você abriu a comporta e atravessou a ponte.", 640, 350, 22, ink);
        centered_text("ENTER ou ESC  Voltar ao menu", 640, 401, 20, ink);
    }
}
