/* Desenha a estufa e resolve os obstáculos a partir das proposições do cenário. */
#include "world.h"

typedef enum {
    VALVE,
    POWER,
    ALARM,
    NORTH_CHANNEL,
    SOUTH_CHANNEL,
    ALOE,
    SUNFLOWER,
    CANAL_NOTE,
    ALARM_NOTE,
    UPPER_LIFT,
    LOWER_LIFT,
    RELAY_PANEL,
    OBJECT_COUNT
} Object;

static const Vector2 objects[OBJECT_COUNT] = {
    [VALVE] = {1070, 305},
    [POWER] = {250, 890},
    [ALARM] = {230, 580},
    [NORTH_CHANNEL] = {675, 305},
    [SOUTH_CHANNEL] = {675, 536},
    [ALOE] = {625, 888},
    [SUNFLOWER] = {1760, 420},
    [CANAL_NOTE] = {1070, 605},
    [ALARM_NOTE] = {420, 580},
    [UPPER_LIFT] = {535, 300},
    [LOWER_LIFT] = {535, 743},
    [RELAY_PANEL] = {2150, 420}
};

static const Rectangle gate = {1515, 170, 32, 870};
static const Rectangle canal = {785, 170, 128, 870};
static const float passage_top = 365;
static const float passage_bottom = 475;
static const float interaction_radius = 55;

static const Color ink = {24, 49, 43, 255};
static const Color frame = {39, 74, 60, 255};
static const Color cream = {249, 240, 205, 255};
static const Color grass = {175, 199, 142, 255};
static const Color stone = {220, 209, 165, 255};
static const Color water_color = {58, 145, 164, 255};
static const Rectangle validate_relays = {82, 596, 350, 48};
static const Rectangle previous_relay_rule = {449, 596, 248, 48};
static const Rectangle close_relays = {714, 596, 218, 48};
enum { CANAL_RELAY_RULE = 2, ALARM_RELAY_RULE = 7 };

void world_init(World *world)
{
    *world = (World){.bounds = {70, 170, 2250, 870}, .alarm = true};
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
    return world->water && (world->light || world->key_collected) && !world->alarm;
}

bool world_bridge_open(const World *world)
{
    return world->north_channel != world->south_channel;
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
    if (blocks(box, gate, world_gate_open(world)) ||
        blocks(box, canal, world_bridge_open(world)))
        return previous;
    return feet;
}

static Object nearby_object(const World *world, Vector2 feet)
{
    for (int i = 0; i < OBJECT_COUNT; i++) {
        if ((i == ALOE && world->key_collected) || (i == SUNFLOWER && world->guardian_defeated)) continue;
        float dx = objects[i].x - feet.x;
        float dy = objects[i].y - feet.y;
        if (dx * dx + dy * dy <= interaction_radius * interaction_radius) return i;
    }
    return OBJECT_COUNT;
}

WorldEncounter world_interact(World *world, Vector2 *feet)
{
    if (world->completed || world->relay_panel_open) return WORLD_ENCOUNTER_NONE;
    switch (nearby_object(world, *feet)) {
        case VALVE: world->water = !world->water; break;
        case POWER: world->light = !world->light; break;
        case ALARM: world->alarm = !world->alarm; break;
        case NORTH_CHANNEL: world->north_channel = !world->north_channel; break;
        case SOUTH_CHANNEL: world->south_channel = !world->south_channel; break;
        case ALOE: return WORLD_ENCOUNTER_ALOE;
        case SUNFLOWER: return WORLD_ENCOUNTER_SUNFLOWER;
        case CANAL_NOTE: world->canal_note = true; break;
        case ALARM_NOTE: world->alarm_note = true; break;
        case UPPER_LIFT: if (!world->light) *feet = objects[LOWER_LIFT]; break;
        case LOWER_LIFT: if (!world->light) *feet = objects[UPPER_LIFT]; break;
        case RELAY_PANEL: world->relay_panel_open = world->guardian_defeated; break;
        case OBJECT_COUNT: break;
    }
    return WORLD_ENCOUNTER_NONE;
}

void world_finish_encounter(World *world, WorldEncounter encounter)
{
    if (encounter == WORLD_ENCOUNTER_ALOE) world->key_collected = true;
    if (encounter == WORLD_ENCOUNTER_SUNFLOWER) world->guardian_defeated = true;
}

void world_update_position(World *world, Vector2 feet)
{
    if (feet.x >= gate.x + gate.width + 8 && world_gate_rule(world))
        world->gate_unlocked = true;
}

Camera2D world_camera(const World *world, Vector2 feet)
{
    float x = feet.x - 640;
    float y = feet.y - 420;
    float max_x = world->bounds.width - 1140;
    float max_y = world->bounds.height - 470;
    if (x < 0) x = 0;
    if (x > max_x) x = max_x;
    if (y < 0) y = 0;
    if (y > max_y) y = max_y;
    return (Camera2D){.target = {x, y}, .zoom = 1.0f};
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

static void draw_note(Object object, const char *name, bool collected)
{
    int x = (int)objects[object].x;
    int y = (int)objects[object].y;
    DrawRectangle(x - 17, y - 23, 34, 36, collected ? cream : GOLD);
    for (int i = 0; i < 3; i++) DrawRectangle(x - 10, y - 15 + i * 7, 20, 2, frame);
    centered_text(name, x, y + 23, 18, ink);
}

static void draw_guard(Object object, const char *name, Color color)
{
    int x = (int)objects[object].x;
    int y = (int)objects[object].y;
    DrawEllipse(x, y + 9, 27, 7, Fade(ink, 0.3f));
    DrawRectangle(x - 5, y - 19, 10, 30, DARKGREEN);
    DrawEllipse(x - 14, y - 3, 16, 7, DARKGREEN);
    DrawEllipse(x + 15, y - 8, 15, 7, DARKGREEN);
    DrawCircle(x, y - 23, 22, color);
    DrawCircle(x, y - 22, 11, frame);
    centered_text(name, x, y + 22, 18, ink);
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
    DrawRectangleRec(b, grass);
    DrawRectangle(72, 698, 713, 340, (Color){143, 163, 139, 255});
    DrawRectangle(914, 171, 600, 867, (Color){183, 205, 163, 255});
    DrawRectangle(1548, 171, 770, 867, (Color){196, 212, 154, 255});
    for (int x = (int)b.x; x < b.x + b.width; x += 40)
        DrawLine(x, (int)b.y, x, (int)(b.y + b.height), (Color){161, 185, 132, 255});
    for (int y = (int)b.y; y < b.y + b.height; y += 40)
        DrawLine((int)b.x, y, (int)(b.x + b.width), y, (Color){161, 185, 132, 255});
    DrawRectangle((int)b.x, (int)passage_top, (int)b.width, (int)(passage_bottom - passage_top), stone);
    DrawRectangle(505, (int)passage_bottom, 62, 558, stone);
    DrawRectangle(186, 861, 526, 60, stone);
    DrawRectangle(1038, (int)passage_bottom, 64, 154, stone);

    centered_text("Pátio", 278, 192, 26, ink);
    centered_text("Canais de irrigação", 1118, 192, 24, ink);
    centered_text("Viveiro", 1740, 192, 24, ink);
    centered_text("Saída da estufa", 2150, 192, 24, ink);
    centered_text("Subsolo", 248, 722, 24, ink);
    centered_text("Depósito", 640, 785, 24, ink);
    draw_gate(world_gate_open(world));
    draw_bridge(world_bridge_open(world));
    draw_switch(VALVE, "Válvula da água", world->water);
    draw_switch(POWER, "Distribuidor de energia", world->light);
    draw_switch(ALARM, "Cabine do alarme", world->alarm);
    draw_switch(NORTH_CHANNEL, "Canal norte", world->north_channel);
    draw_switch(SOUTH_CHANNEL, "Canal sul", world->south_channel);
    draw_switch(UPPER_LIFT, "Elevador", !world->light);
    draw_switch(LOWER_LIFT, "Elevador", !world->light);
    draw_note(CANAL_NOTE, "Pista dos canais", world->canal_note);
    draw_note(ALARM_NOTE, "Pista da cabine", world->alarm_note);
    if (!world->key_collected) draw_guard(ALOE, "Babosa guardiã", (Color){73, 162, 120, 255});
    if (!world->guardian_defeated) draw_guard(SUNFLOWER, "Girassol guardião", GOLD);

    DrawRectangle(2125, 389, 50, 45, frame);
    DrawRectangle(2131, 395, 38, 32, world->guardian_defeated ? cream : stone);
    centered_text("?", 2150, 397, 26, ink);
    centered_text("Central de relés", 2150, 446, 18, ink);
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
        [ALOE] = "E  Desafiar Babosa pela chave",
        [SUNFLOWER] = "E  Desafiar Girassol para liberar a central",
        [CANAL_NOTE] = "E  Registrar pista dos canais",
        [ALARM_NOTE] = "E  Registrar pista da cabine",
        [UPPER_LIFT] = "E  Descer ao subsolo",
        [LOWER_LIFT] = "E  Subir ao pátio",
        [RELAY_PANEL] = "E  Abrir central de relés"
    };
    bool gate_open = world_gate_open(world);
    bool bridge_open = world_bridge_open(world);
    DrawText("Estufa lógica", 70, 19, 33, cream);
    DrawText(TextFormat("Pistas %d/2   |   Guardião %s", world->canal_note + world->alarm_note,
                        world->guardian_defeated ? "vencido" : "aguardando"), 826, 29, 18, cream);
    draw_formula(70, "Viveiro   Água E (Luz OU Chave) E NÃO Alarme",
                 TextFormat("%s E (%s OU %s) E NÃO %s = %s", truth(world->water), truth(world->light),
                            truth(world->key_collected), truth(world->alarm), truth(world_gate_rule(world))),
                 world->gate_unlocked ? "Travada aberta" : gate_open ? "Aberta" : "Fechada", gate_open);
    draw_formula(650, "Ponte   Norte XOR Sul",
                 TextFormat("%s XOR %s = %s", truth(world->north_channel), truth(world->south_channel),
                            truth(bridge_open)),
                 bridge_open ? "Estendida" : "Recolhida", bridge_open);

    Object object = nearby_object(world, feet);
    const char *hint = "Pátio: alarme ao sul, subsolo mais abaixo e canais a leste.";
    if (feet.x > gate.x + gate.width) {
        if (!world->guardian_defeated) hint = "Viveiro: vença o guardião para liberar a central.";
        else if (!world->canal_note || !world->alarm_note) hint = "Guardião vencido. Recolha as pistas que faltam.";
        else hint = "Guardião vencido e pistas reunidas. Siga até a central.";
    } else if (feet.x > canal.x + canal.width) hint = "Canais: válvula ao norte, pista ao sul e viveiro a leste.";
    else if (feet.y >= 698) hint = world->key_collected
        ? "Chave obtida. Volte ao pátio e siga para os canais."
        : "Subsolo: energia a oeste, chave com a Babosa a leste.";
    if (object != OBJECT_COUNT) {
        hint = prompts[object];
        if (object == POWER) hint = world->light
            ? "Refletor energizado. E  Alimentar elevador" : "Elevador energizado. E  Alimentar refletor";
        if ((object == UPPER_LIFT || object == LOWER_LIFT) && world->light)
            hint = "Elevador sem energia. Use o corredor ao sul do pátio.";
        if (object == CANAL_NOTE && world->canal_note) hint = "Pista dos canais registrada na central.";
        if (object == ALARM_NOTE && world->alarm_note) hint = "Pista da cabine registrada na central.";
        if (object == RELAY_PANEL && !world->guardian_defeated)
            hint = "Central bloqueada até você vencer o Girassol.";
    }
    DrawText(hint, 70, 650, 21, cream);
    DrawText("WASD / setas  Mover     E  Interagir     ESC  Menu", 70, 682, 17, RAYWHITE);

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
