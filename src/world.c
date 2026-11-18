/* Desenha a estufa e resolve os obstáculos a partir das proposições do cenário. */
#include "world.h"
#include "game_view.h"
#include "courtyard_detail_data.h"
#include <math.h>
#include <stddef.h>

/* Coordenadas do exterior medidas na imagem de referência. */
enum { MAP_WIDTH = 1672, MAP_HEIGHT = 941 };

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
    ENTRANCE_SIGN,
    BRIDGE_SIGN,
    GARDENER,
    TECHNICIAN,
    RESEARCHER,
    OBJECT_COUNT
} Object;

typedef struct {
    Vector2 position;
    WorldLocation location;
} MapObject;

static const MapObject objects[OBJECT_COUNT] = {
    [VALVE] = {{1168, 644}, WORLD_EXTERIOR},
    [POWER] = {{640, 320}, WORLD_BASEMENT},
    [ALARM] = {{445, 310}, WORLD_MAINTENANCE},
    [NORTH_CHANNEL] = {{450, 352}, WORLD_BRIDGE_CONTROL},
    [SOUTH_CHANNEL] = {{830, 352}, WORLD_BRIDGE_CONTROL},
    [DRAIN] = {{733, 555}, WORLD_EXTERIOR},
    [ALOE] = {{640, 320}, WORLD_DEPOT},
    [SUNFLOWER] = {{515, 340}, WORLD_NURSERY},
    [CANAL_NOTE] = {{1271, 401}, WORLD_EXTERIOR},
    [ALARM_NOTE] = {{537, 281}, WORLD_MAINTENANCE},
    [MAINTENANCE_DOOR] = {{243, 645}, WORLD_EXTERIOR},
    [DEPOT_DOOR] = {{278, 379}, WORLD_EXTERIOR},
    [NURSERY_DOOR] = {{1396, 330}, WORLD_EXTERIOR},
    [BRIDGE_CONTROL_DOOR] = {{810, 561}, WORLD_EXTERIOR},
    [INTERIOR_DOOR] = {{640, 565}, WORLD_MAINTENANCE},
    [LOCKED_HATCH] = {{846, 416}, WORLD_MAINTENANCE},
    [BASEMENT_HATCH] = {{640, 565}, WORLD_BASEMENT},
    [UPPER_LIFT] = {{429, 479}, WORLD_EXTERIOR},
    [LOWER_LIFT] = {{837, 410}, WORLD_BASEMENT},
    [RELAY_PANEL] = {{790, 340}, WORLD_NURSERY},
    [ENTRANCE_SIGN] = {{487, 787}, WORLD_EXTERIOR},
    [BRIDGE_SIGN] = {{770, 625}, WORLD_EXTERIOR},
    [GARDENER] = {{630, 507}, WORLD_EXTERIOR},
    [TECHNICIAN] = {{868, 600}, WORLD_EXTERIOR},
    [RESEARCHER] = {{882, 477}, WORLD_NURSERY}
};

static const Rectangle exterior = {0, 0, MAP_WIDTH, MAP_HEIGHT};
static const Rectangle canal = {907, 0, 138, MAP_HEIGHT};
static const Rectangle bridge_dest = {902, 485, 158, 113};

/* Caminhos retangulares para o exterior montado com peças, se faltar a ilustração. */
static const Rectangle fallback_paths[] = {
    {523, 600, 106, 218}, /* Entrada sul, do lado interno do portão. */
    {389, 440, 367, 194}, /* Pátio e dreno. */
    {730, 548, 190, 55},
    {888, 522, 222, 44},  /* Tabuleiro e acesso da margem leste. */
    {328, 530, 90, 98},
    {340, 464, 79, 113},
    {298, 430, 110, 58},
    {244, 365, 68, 81},   /* Depósito. */
    {249, 410, 113, 50},
    {221, 632, 150, 66},  /* Manutenção. */
    {315, 585, 112, 79},
    {462, 185, 108, 274}, /* Caminho junto ao pergolado. */
    {551, 187, 219, 37},
    {738, 211, 106, 49},
    {738, 250, 112, 157},
    {723, 384, 43, 75},
    {1049, 462, 86, 131},
    {1066, 429, 151, 61},
    {1157, 408, 303, 56},
    {1330, 318, 127, 107}, /* Viveiro. */
    {1240, 391, 113, 44}, /* Placa com a pista. */
    {1049, 555, 72, 173},
    {1076, 596, 152, 61}, /* Registro de água. */
    {1076, 636, 153, 96},
    {1199, 692, 366, 46}  /* Frente da estufa menor. */
};

/* Contornos do piso na arte nativa. Polígonos vizinhos se sobrepõem nas junções. */
static const Vector2 entry_path[] = {
    {522, 811}, {531, 781}, {547, 758}, {547, 651}, {538, 627}, {515, 608},
    {656, 601}, {626, 620}, {605, 648}, {605, 758}, {628, 785}, {634, 811}
};
static const Vector2 courtyard_path[] = {
    {372, 592}, {400, 552}, {400, 493}, {378, 463}, {420, 469}, {460, 448},
    {470, 404}, {564, 404}, {571, 428}, {729, 428}, {752, 400}, {832, 396},
    {850, 413}, {771, 448}, {757, 493}, {729, 502}, {709, 550}, {762, 552},
    {906, 553}, {926, 528}, {926, 566}, {895, 599}, {835, 617}, {673, 607},
    {637, 629}, {525, 632}, {503, 607}, {420, 607}, {394, 628}, {379, 651},
    {360, 671}, {365, 629}
};
static const Vector2 depot_path[] = {
    {252, 367}, {301, 367}, {308, 399}, {329, 414}, {395, 418}, {414, 438},
    {464, 454}, {488, 478}, {401, 501}, {377, 475}, {348, 453}, {278, 445},
    {259, 427}, {252, 404}
};
static const Vector2 maintenance_path[] = {
    {215, 631}, {266, 631}, {282, 651}, {321, 651}, {341, 631}, {354, 608},
    {374, 580}, {430, 573}, {413, 623}, {382, 646}, {365, 679}, {348, 697},
    {244, 697}, {224, 686}, {215, 668}
};
static const Vector2 pergola_path[] = {
    {470, 188}, {755, 188}, {776, 213}, {811, 230}, {810, 263}, {779, 275},
    {741, 231}, {714, 223}, {694, 209}, {601, 209}, {582, 226}, {573, 308},
    {563, 322}, {563, 431}, {544, 460}, {468, 468}, {472, 362}, {501, 341},
    {511, 310}, {510, 274}, {467, 253}
};
static const Vector2 garden_loop_path[] = {
    {716, 217}, {747, 222}, {778, 269}, {777, 331}, {779, 390}, {806, 416},
    {747, 441}, {729, 412}, {738, 391}, {738, 314}, {725, 284}, {712, 257}
};
static const Vector2 east_path[] = {
    {1028, 524}, {1055, 521}, {1071, 453}, {1149, 449}, {1198, 407},
    {1284, 403}, {1320, 382}, {1334, 351}, {1334, 323}, {1460, 323},
    {1461, 436}, {1444, 456}, {1265, 457}, {1229, 475}, {1200, 504},
    {1117, 507}, {1096, 531}, {1098, 583}, {1061, 589}, {1028, 563}
};
static const Vector2 irrigation_path[] = {
    {1057, 546}, {1095, 540}, {1104, 600}, {1215, 598}, {1215, 640},
    {1198, 682}, {1215, 714}, {1348, 714}, {1357, 686}, {1414, 686},
    {1425, 710}, {1568, 708}, {1568, 742}, {1090, 742}, {1070, 723}
};

#define WALK_AREA(points) {points, sizeof(points) / sizeof(points[0])}
static const struct {
    const Vector2 *points;
    int count;
} walk_areas[] = {
    WALK_AREA(entry_path), WALK_AREA(courtyard_path), WALK_AREA(depot_path),
    WALK_AREA(maintenance_path), WALK_AREA(pergola_path), WALK_AREA(garden_loop_path), WALK_AREA(east_path),
    WALK_AREA(irrigation_path)
};
#undef WALK_AREA

static const Rectangle crossing = {899, 525, 160, 38};
static const float passage_top = 525;
static const float passage_bottom = 563;
static const float interaction_radius = 55;

static const Color ink = {24, 49, 43, 255};
static const Color frame = {39, 74, 60, 255};
static const Color cream = {249, 240, 205, 255};
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
    {{0, 735, 907, 206}, "Jardim de entrada", {165, 191, 141, 255}, {84, 119, 77, 255}},
    {{0, 0, 907, 735}, "Pátio", {165, 191, 141, 255}, {84, 119, 77, 255}},
    {{1045, 465, 627, 476}, "Canais", {152, 191, 177, 255}, {55, 114, 114, 255}},
    {{1045, 0, 627, 465}, "Jardim do viveiro", {189, 207, 138, 255}, {100, 132, 64, 255}}
};

static const Sector rooms[WORLD_LOCATION_COUNT] = {
    [WORLD_MAINTENANCE] = {{280, 191, 720, 420}, "Casa de manutenção", {210, 198, 162, 255}, {111, 93, 66, 255}},
    [WORLD_DEPOT] = {{280, 191, 720, 420}, "Depósito", {202, 173, 139, 255}, {134, 92, 69, 255}},
    [WORLD_BASEMENT] = {{280, 191, 720, 420}, "Subsolo", {151, 170, 169, 255}, {72, 105, 111, 255}},
    [WORLD_NURSERY] = {{280, 191, 720, 420}, "Viveiro", {192, 213, 153, 255}, {82, 119, 62, 255}},
    [WORLD_BRIDGE_CONTROL] = {{280, 191, 720, 420}, "Controle da ponte", {163, 199, 186, 255}, {59, 114, 109, 255}}
};

/* Pegadas das mobílias da oficina, alinhadas à ilustração no espaço do mapa. */
static const Rectangle maintenance_furniture[] = {
    {307, 260, 56, 91},  /* Armário de vasos à esquerda. */
    {400, 277, 90, 71},  /* Painel e bancada de parede. */
    {580, 316, 146, 39}, /* Prateleira central. */
    {870, 260, 96, 97},  /* Caldeira e tubulações. */
    {302, 392, 146, 52}, /* Bancada de trabalho. */
    {361, 449, 38, 41},  /* Banco. */
    {306, 495, 75, 62},  /* Caixas inferiores. */
    {917, 423, 58, 132}  /* Estante lateral. */
};

static Texture2D load_map_texture(const char *path, int width, int height)
{
    if (!FileExists(path)) return (Texture2D){0};
    Texture2D texture = LoadTexture(path);
    if (texture.id && (texture.width != width || texture.height != height)) {
        UnloadTexture(texture);
        return (Texture2D){0};
    }
    if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

void world_assets_load(WorldAssets *assets)
{
    *assets = (WorldAssets){
        .courtyard = load_map_texture("assets/maps/botanical_courtyard.png", MAP_WIDTH, MAP_HEIGHT),
        .bridge_open = load_map_texture("assets/maps/botanical_bridge.png", 158, 113),
        .tiles = load_map_texture("assets/maps/botanical_tiles.png", 512, 512),
        .maintenance_interior = load_map_texture(
            "assets/maps/maintenance_house_interior_v2.png", 1672, 941),
        .courtyard_detail = load_map_texture("assets/maps/courtyard_detail/courtyard_complete.png",
                                            COURTYARD_DETAIL_WIDTH, COURTYARD_DETAIL_HEIGHT)
    };
    if (assets->courtyard_detail.id) {
        SetTextureFilter(assets->courtyard_detail, TEXTURE_FILTER_BILINEAR);
        /* Impede o filtro de buscar pixels da borda oposta nas margens transparentes. */
        SetTextureWrap(assets->courtyard_detail, TEXTURE_WRAP_CLAMP);
    }
}

void world_assets_unload(WorldAssets *assets)
{
    if (assets->courtyard.id) UnloadTexture(assets->courtyard);
    if (assets->bridge_open.id) UnloadTexture(assets->bridge_open);
    if (assets->tiles.id) UnloadTexture(assets->tiles);
    if (assets->maintenance_interior.id) UnloadTexture(assets->maintenance_interior);
    if (assets->courtyard_detail.id) UnloadTexture(assets->courtyard_detail);
    *assets = (WorldAssets){0};
}

void world_init(World *world)
{
    *world = (World){
        .bounds = exterior,
        .camera_position = {exterior.x, exterior.y},
        .alarm = true,
        .drain_open = true,
        .detail_enabled = true
    };
}

Vector2 world_spawn(void)
{
    return (Vector2){576, 799};
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
    for (Object npc = GARDENER; npc <= RESEARCHER; npc++) {
        if (objects[npc].location != world->location) continue;
        Vector2 base = objects[npc].position;
        if (CheckCollisionRecs(box, (Rectangle){base.x - 8, base.y - 4, 16, 8})) return previous;
    }
    if (world->location == WORLD_MAINTENANCE) {
        for (unsigned int i = 0; i < sizeof(maintenance_furniture) / sizeof(maintenance_furniture[0]); i++)
            if (CheckCollisionRecs(box, maintenance_furniture[i])) return previous;
    }
    if (world->location == WORLD_EXTERIOR) {
        if (blocks(box, canal, world_bridge_open(world))) return previous;
        if (CheckCollisionPointRec(feet, crossing)) return feet;
        for (unsigned int i = 0; i < sizeof(walk_areas) / sizeof(walk_areas[0]); i++)
            if (CheckCollisionPointPoly(feet, walk_areas[i].points, walk_areas[i].count)) return feet;
        return previous;
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

static void open_dialogue(World *world, const char *title, const char *first, const char *second)
{
    world->dialogue_title = title;
    world->dialogue_pages[0] = first;
    world->dialogue_pages[1] = second;
    world->dialogue_page = 0;
}

void world_update_dialogue(World *world)
{
    if (!world->dialogue_title) return;

    if (IsKeyPressed(KEY_ESCAPE)) {
        world->dialogue_title = NULL;
    } else if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER)) {
        if (world->dialogue_page == 0 && world->dialogue_pages[1]) world->dialogue_page = 1;
        else world->dialogue_title = NULL;
    }
}

static void talk_to_gardener(World *world)
{
    if (!world->maintenance_key) {
        open_dialogue(world, "Jardineira",
                      "A Babosa guarda a Chave da Manutenção no depósito.\nÉ o prédio de telhado terracota, a noroeste do pátio.",
                      "A chave abre a escotilha da casa de telhado verde.\nLá embaixo você pode alimentar o refletor do viveiro.");
    } else if (!world->light && !world->gate_unlocked) {
        open_dialogue(world, "Jardineira",
                      "Com a chave, você pode descer pela escotilha da manutenção.\nNo subsolo, redirecione a energia para o refletor.", NULL);
    } else if (world->alarm && !world->gate_unlocked) {
        open_dialogue(world, "Jardineira",
                      "Ainda falta desligar o alarme.\nO painel fica na casa de manutenção.", NULL);
    } else {
        open_dialogue(world, "Jardineira",
                      "Quando estiver pronto, siga para os canais.\nA válvula da água fica do outro lado da ponte.", NULL);
    }
}

static void talk_to_technician(World *world)
{
    if (world_bridge_open(world)) {
        open_dialogue(world, "Técnico",
                      "Pressão certa. A ponte está pronta para atravessar.", NULL);
    } else if (world->north_channel == world->south_channel) {
        open_dialogue(world, "Técnico",
                      "A ponte precisa de um único canal aberto e pressão 3.\nVocê pode alternar os canais na casa de telhado azul.",
                      "Cada canal gera uma pressão diferente.\nO dreno, do lado de fora, retira 2 unidades quando aberto.");
    } else {
        open_dialogue(world, "Técnico",
                      "Você já acertou o número de canais. Falta acertar a pressão.\nConfira o dreno ao lado da casa de controle.", NULL);
    }
}

static void talk_to_researcher(World *world)
{
    const char *hint = "Você já tem as duas pistas. Use o painel para testar os relés.";
    if (!world->alarm_note && !world->canal_note)
        hint = "Procure a placa nos canais e o bilhete na casa de manutenção.";
    else if (!world->alarm_note)
        hint = "Falta o bilhete perto do alarme, na casa de manutenção.";
    else if (!world->canal_note)
        hint = "Falta a pista da placa nos canais, no caminho até aqui.";

    open_dialogue(world, "Pesquisadora",
                  world->guardian_defeated ? hint : "O Girassol guarda a Central de Relés.\nVença o guardião para liberar o painel.",
                  world->guardian_defeated ? NULL : hint);
}

WorldEncounter world_interact(World *world, Vector2 *feet)
{
    if (world->completed || world->relay_panel_open || world->dialogue_title) return WORLD_ENCOUNTER_NONE;
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
        case CANAL_NOTE:
            world->canal_note = true;
            open_dialogue(world, "Placa dos canais",
                          "Pista registrada. Na central, confira o relé E.\nEle deve ter o mesmo valor de B XOR F.", NULL);
            break;
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
        case ENTRANCE_SIGN:
            open_dialogue(world, "Placa do jardim",
                          "Ao norte, pátio central e depósito.\nA oeste, casa de manutenção. A leste, canais e viveiro.", NULL);
            break;
        case BRIDGE_SIGN:
            open_dialogue(world, "Aviso da ponte",
                          "Para estender a ponte, abra exatamente um canal\ne regule a pressão para 3.",
                          "A casa azul controla os canais. O dreno fica do lado de fora.");
            break;
        case GARDENER: talk_to_gardener(world); break;
        case TECHNICIAN: talk_to_technician(world); break;
        case RESEARCHER: talk_to_researcher(world); break;
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
    if (world->location != WORLD_EXTERIOR) return (Vector2){0};
    Rectangle b = world->bounds;
    return (Vector2){
        fminf(fmaxf(feet.x - GAME_SCREEN_WIDTH / 2.0f, b.x), b.x + b.width - GAME_SCREEN_WIDTH),
        fminf(fmaxf(feet.y - GAME_SCREEN_HEIGHT / 2.0f, b.y), b.y + b.height - GAME_SCREEN_HEIGHT)
    };
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
    if (world->location != WORLD_EXTERIOR) {
        Rectangle room = rooms[world->location].bounds;
        return (Camera2D){
            .offset = {GAME_SCREEN_WIDTH / 2.0f, GAME_SCREEN_HEIGHT / 2.0f},
            .target = {room.x + room.width / 2, room.y + room.height / 2},
            .zoom = fmaxf(GAME_SCREEN_WIDTH / room.width, GAME_SCREEN_HEIGHT / room.height)
        };
    }
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

static void draw_interaction_marker(Object object)
{
    Vector2 p = objects[object].position;
    DrawCircleLines((int)p.x, (int)p.y, 36, GOLD);
    DrawCircleLines((int)p.x, (int)p.y, 37, GOLD);
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

static void draw_room_portal(Object object, const char *label, bool locked)
{
    Vector2 p = objects[object].position;
    int x = (int)p.x, y = (int)p.y;
    DrawRectangle(x - 31, y - 26, 62, 43, frame);
    DrawRectangle(x - 25, y - 20, 50, 31, locked ? MAROON : path_edge);
    DrawRectangle(x - 12, y - 9, 24, 5, cream);
    if (object == INTERIOR_DOOR || object == BASEMENT_HATCH)
        centered_text(label, x + 132, y - 11, 16, ink);
    else centered_text(label, x, y + 22, 18, ink);
}

static void draw_ground_tiles(Texture2D tiles, Rectangle source, Rectangle area, int size)
{
    int left = (int)area.x;
    int top = (int)area.y;
    int right = (int)(area.x + area.width);
    int bottom = (int)(area.y + area.height);
    for (int y = top; y < bottom;) {
        int row_offset = y % size;
        int height = size - row_offset;
        if (height > bottom - y) height = bottom - y;
        for (int x = left; x < right;) {
            int column_offset = x % size;
            int width = size - column_offset;
            if (width > right - x) width = right - x;
            Rectangle part = {source.x + column_offset * source.width / size,
                              source.y + row_offset * source.height / size,
                              width * source.width / size, height * source.height / size};
            DrawTexturePro(tiles, part, (Rectangle){x, y, width, height}, (Vector2){0}, 0, WHITE);
            x += width;
        }
        y += height;
    }
}

static void draw_map_sprite(Texture2D atlas, Rectangle source, Vector2 base, int center)
{
    Rectangle destination = {base.x - 2 * center, base.y - 2 * source.height,
                             2 * source.width, 2 * source.height};
    DrawTexturePro(atlas, source, destination, (Vector2){0}, 0, WHITE);
}

static void draw_canal_banks(Texture2D atlas, int tile_size)
{
    const int bank_width = 16;
    for (int y = 0; y < MAP_HEIGHT; y += tile_size) {
        int height = MAP_HEIGHT - y < tile_size ? MAP_HEIGHT - y : tile_size;
        Rectangle source = {48, 0, 8, 16.0f * height / tile_size};
        Rectangle bank = {canal.x, y, bank_width, height};
        DrawTexturePro(atlas, source, bank, (Vector2){0}, 0, WHITE);
        source.width = -source.width;
        bank.x = canal.x + canal.width - bank_width;
        DrawTexturePro(atlas, source, bank, (Vector2){0}, 0, WHITE);
    }
}

static void draw_tiled_exterior(const World *world, Texture2D atlas)
{
    enum { TILE_SIZE = 32 };
    draw_ground_tiles(atlas, (Rectangle){0, 0, 16, 16}, exterior, TILE_SIZE);
    for (unsigned int i = 0; i < sizeof(fallback_paths) / sizeof(fallback_paths[0]); i++) {
        Rectangle path = fallback_paths[i];
        DrawRectangleRec((Rectangle){path.x - 3, path.y - 3, path.width + 6, path.height + 6}, path_edge);
    }
    for (unsigned int i = 0; i < sizeof(fallback_paths) / sizeof(fallback_paths[0]); i++)
        draw_ground_tiles(atlas, (Rectangle){16, 0, 16, 16}, fallback_paths[i], TILE_SIZE);
    draw_ground_tiles(atlas, (Rectangle){32, 0, 16, 16}, canal, TILE_SIZE);
    draw_canal_banks(atlas, TILE_SIZE);
    if (world_bridge_open(world)) {
        draw_map_sprite(atlas, (Rectangle){0, 254, 79, 50}, (Vector2){976, 588}, 39);
    } else {
        draw_map_sprite(atlas, (Rectangle){88, 254, 10, 48}, (Vector2){908, 586}, 5);
        draw_map_sprite(atlas, (Rectangle){104, 254, 10, 48}, (Vector2){1045, 586}, 5);
    }

    static const struct {
        Rectangle source;
        Vector2 base;
    } decorations[] = {
        {{280, 316, 48, 33}, {576, 883}}, /* Portão sul. */
        {{64, 0, 32, 32}, {582, 568}},    /* Mosaico do pátio. */
        {{144, 316, 60, 46}, {640, 185}},
        {{336, 316, 27, 20}, {682, 664}},
        {{56, 316, 50, 60}, {677, 400}},
        {{56, 316, 50, 60}, {419, 368}},
        {{0, 316, 50, 60}, {139, 223}},
        {{0, 316, 50, 60}, {86, 770}},
        {{0, 316, 50, 60}, {208, 818}},
        {{0, 316, 50, 60}, {834, 802}},
        {{0, 316, 50, 60}, {1135, 345}},
        {{0, 316, 50, 60}, {1590, 480}},
        {{0, 316, 50, 60}, {1550, 845}},
        {{112, 316, 24, 22}, {475, 708}},
        {{112, 316, 24, 22}, {692, 760}},
        {{112, 316, 24, 22}, {1142, 402}},
        {{112, 316, 24, 22}, {1517, 762}},
        {{208, 136, 107, 96}, {1406, 700}}
    };
    for (unsigned int i = 0; i < sizeof(decorations) / sizeof(decorations[0]); i++) {
        Rectangle source = decorations[i].source;
        draw_map_sprite(atlas, source, decorations[i].base, (int)source.width / 2);
    }

    draw_map_sprite(atlas, (Rectangle){0, 32, 101, 91}, (Vector2){243, 637}, 52);
    draw_map_sprite(atlas, (Rectangle){112, 32, 132, 91}, (Vector2){278, 368}, 66);
    draw_map_sprite(atlas, (Rectangle){256, 32, 73, 70}, (Vector2){810, 552}, 32);
    draw_map_sprite(atlas, (Rectangle){0, 136, 188, 103}, (Vector2){1396, 318}, 96);
    draw_map_sprite(atlas, (Rectangle){216, 316, 23, 30}, objects[CANAL_NOTE].position, 11);
    draw_map_sprite(atlas, (Rectangle){248, 316, 18, 16}, objects[DRAIN].position, 9);
    draw_map_sprite(atlas, (Rectangle){376, 316, 25, 20}, objects[VALVE].position, 12);
    draw_map_sprite(atlas, (Rectangle){408, 316, 19, 23}, objects[UPPER_LIFT].position, 9);
    Vector2 drain = objects[DRAIN].position;
    DrawCircle((int)drain.x + 14, (int)drain.y - 27, 4, world->drain_open ? LIME : ORANGE);
    Vector2 valve = objects[VALVE].position;
    DrawCircle((int)valve.x + 22, (int)valve.y - 35, 4, world->water ? LIME : ORANGE);
    Vector2 lift = objects[UPPER_LIFT].position;
    bool lift_available = world->basement_visited && !world->light;
    DrawCircle((int)lift.x + 16, (int)lift.y - 37, 4, lift_available ? LIME : ORANGE);
}

static void draw_exterior(const World *world, const WorldAssets *assets)
{
    if (!assets->courtyard.id && assets->tiles.id) {
        draw_tiled_exterior(world, assets->tiles);
        return;
    }
    if (assets->courtyard.id) {
        DrawTexturePro(assets->courtyard, (Rectangle){0, 0, MAP_WIDTH, MAP_HEIGHT},
                       exterior, (Vector2){0}, 0, WHITE);
        if (world->detail_enabled && assets->courtyard_detail.id) {
            DrawTexturePro(assets->courtyard_detail,
                           (Rectangle){0, 0, assets->courtyard_detail.width, assets->courtyard_detail.height},
                           (Rectangle){0, 0, COURTYARD_DETAIL_WORLD_WIDTH, COURTYARD_DETAIL_WORLD_HEIGHT},
                           (Vector2){0}, 0, WHITE);
        }
    } else {
        DrawRectangleRec(exterior, (Color){164, 187, 125, 255});
        for (unsigned int i = 0; i < sizeof(fallback_paths) / sizeof(fallback_paths[0]); i++)
            DrawRectangleRec(fallback_paths[i], stone);
        DrawRectangleRec(canal, water_color);
    }

    if (world_bridge_open(world)) {
        if (assets->bridge_open.id) {
            Rectangle source = {0, 0, assets->bridge_open.width, assets->bridge_open.height};
            DrawTexturePro(assets->bridge_open, source, bridge_dest, (Vector2){0}, 0, WHITE);
        } else {
            DrawRectangleRec((Rectangle){canal.x, passage_top, canal.width, passage_bottom - passage_top}, BROWN);
        }
    }

    if (assets->courtyard.id && assets->tiles.id) {
        draw_map_sprite(assets->tiles, (Rectangle){408, 316, 19, 23}, objects[UPPER_LIFT].position, 9);
        DrawCircle(744, 538, 3, world->drain_open ? LIME : ORANGE);
        DrawCircle(1194, 638, 3, world->water ? LIME : ORANGE);
    } else {
        draw_switch(DRAIN, "Dreno", world->drain_open);
        draw_switch(UPPER_LIFT, "Elevador", world->basement_visited && !world->light);
    }
}

static void draw_maintenance_details(const World *world)
{
    /* Atualiza os indicadores desenhados sobre o painel e a escotilha da arte. */
    Color alarm_color = world->alarm ? (Color){207, 68, 48, 255}
                                     : (Color){91, 190, 78, 255};
    DrawCircle(433, 267, 7, (Color){45, 57, 48, 255});
    DrawCircle(433, 267, 4, alarm_color);
    DrawCircle(432, 266, 1, (Color){255, 238, 178, 255});

    if (world->alarm_note) {
        DrawCircle(558, 264, 5, (Color){94, 167, 80, 255});
        DrawCircle(557, 263, 2, (Color){210, 238, 155, 255});
    }

    Color hatch_color = world->maintenance_key ? (Color){91, 190, 78, 255}
                                               : (Color){214, 139, 55, 255};
    DrawCircle(850, 410, 6, (Color){45, 57, 48, 255});
    DrawCircle(850, 410, 3, hatch_color);
}

static void draw_interior(const World *world, const WorldAssets *assets)
{
    const Sector *room = &rooms[world->location];
    Rectangle b = room->bounds;

    if (world->location == WORLD_MAINTENANCE && assets->maintenance_interior.id) {
        /* Recorta a margem preta da referência e enquadra a sala no mapa. */
        DrawTexturePro(assets->maintenance_interior,
                       (Rectangle){84, 13, 1504, 907}, b, (Vector2){0}, 0, WHITE);
        draw_maintenance_details(world);
    } else {
        draw_sector(room);
        DrawRectangleRec((Rectangle){b.x, b.y, b.width, ROOM_HEADER_HEIGHT}, room->trim);
        DrawRectangle((int)b.x, (int)b.y + ROOM_HEADER_HEIGHT - 5, (int)b.width, 5,
                      Fade(ink, 0.55f));
        DrawRectangle(610, 261, 60, 347, Fade(stone, 0.62f));
    }

    switch (world->location) {
        case WORLD_MAINTENANCE:
            if (!assets->maintenance_interior.id) {
                draw_switch(ALARM, "Painel do alarme", world->alarm);
                draw_note(ALARM_NOTE, "Pista da cabine", world->alarm_note);
                draw_room_portal(LOCKED_HATCH,
                                 world->maintenance_key ? "Escotilha" : "Escotilha trancada",
                                 !world->maintenance_key);
            }
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
            DrawRectangle(450, 410, 380, 9, frame);
            DrawRectangle(450, 410, 190, 9, world->north_channel ? water_color : frame);
            DrawRectangle(640, 410, 190, 9, world->south_channel ? water_color : frame);
            draw_switch(NORTH_CHANNEL, TextFormat("Canal norte (+%d)", NORTH_PRESSURE), world->north_channel);
            draw_switch(SOUTH_CHANNEL, TextFormat("Canal sul (+%d)", SOUTH_PRESSURE), world->south_channel);
            centered_text(TextFormat("Dreno externo %s", world->drain_open ? "aberto" : "fechado"),
                          640, 443, 20, ink);
            break;
        default: break;
    }
    if (world->location != WORLD_BASEMENT &&
        !(world->location == WORLD_MAINTENANCE && assets->maintenance_interior.id))
        draw_room_portal(INTERIOR_DOOR, "Voltar ao pátio", false);
}

static void draw_npc(Texture2D atlas, int sprite, Vector2 base, Camera2D camera)
{
    Vector2 screen = GetWorldToScreen2D(base, camera);
    base = GetScreenToWorld2D((Vector2){roundf(screen.x), roundf(screen.y)}, camera);
    float scale = 1 / camera.zoom;
    DrawEllipse((int)roundf(base.x), (int)roundf(base.y), 14 * scale, 4 * scale, Fade(ink, 0.28f));
    if (atlas.id) {
        Rectangle source = {64 * sprite, 384, 48, 56};
        Rectangle destination = {base.x - 24 * scale, base.y - 53 * scale, 48 * scale, 56 * scale};
        DrawTexturePro(atlas, source, destination, (Vector2){0}, 0, WHITE);
    } else {
        DrawRectangleRec((Rectangle){base.x - 12 * scale, base.y - 26 * scale,
                                     24 * scale, 26 * scale}, frame);
        DrawCircleV((Vector2){base.x, base.y - 34 * scale}, 12 * scale, cream);
    }
}

void world_draw_npcs(const World *world, const WorldAssets *assets, Vector2 feet, Camera2D camera, bool foreground)
{
    Object speaking = world->dialogue_title ? nearby_object(world, feet) : OBJECT_COUNT;
    for (Object npc = GARDENER; npc <= RESEARCHER; npc++) {
        Vector2 base = objects[npc].position;
        /* Em conversa, o treinador não deve cobrir quem está falando. */
        bool in_front = npc == speaking || base.y > feet.y;
        if (objects[npc].location == world->location && in_front == foreground)
            draw_npc(assets->tiles, npc - GARDENER, base, camera);
    }
}

static void draw_sign(Texture2D atlas, Object object)
{
    Vector2 base = objects[object].position;
    if (atlas.id) {
        draw_map_sprite(atlas, (Rectangle){216, 316, 23, 30}, base, 11);
    } else {
        DrawRectangleRec((Rectangle){base.x - 3, base.y - 27, 6, 27}, BROWN);
        DrawRectangleRec((Rectangle){base.x - 17, base.y - 38, 34, 18}, BROWN);
        DrawRectangleRec((Rectangle){base.x - 14, base.y - 35, 28, 12}, stone);
    }
}

void world_draw(const World *world, const WorldAssets *assets, Vector2 feet)
{
    if (world->location == WORLD_EXTERIOR) {
        draw_exterior(world, assets);
        if (!assets->courtyard.id) draw_sign(assets->tiles, ENTRANCE_SIGN);
        draw_sign(assets->tiles, BRIDGE_SIGN);
    } else {
        draw_interior(world, assets);
    }

    Object nearby = nearby_object(world, feet);
    if (world->location != WORLD_EXTERIOR && !world->relay_panel_open && !world->dialogue_title &&
        !world->completed && nearby != OBJECT_COUNT)
        draw_interaction_marker(nearby);
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
    DrawRectangle(0, 0, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT, Fade(ink, 0.88f));
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

static void hud_card(Rectangle bounds, Color accent)
{
    DrawRectangleRec((Rectangle){bounds.x + 2, bounds.y + 3, bounds.width, bounds.height}, Fade(ink, 0.25f));
    DrawRectangleRec(bounds, Fade(ink, 0.92f));
    DrawRectangleLinesEx(bounds, 1, Fade(accent, 0.45f));
}

static void draw_condition(int x, int y, bool met, const char *label)
{
    Color color = met ? (Color){170, 224, 145, 255} : (Color){246, 198, 115, 255};
    DrawRectangleLines(x, y + 2, 13, 13, color);
    if (met) {
        DrawLine(x + 3, y + 8, x + 5, y + 10, color);
        DrawLine(x + 5, y + 10, x + 10, y + 5, color);
    } else {
        DrawLine(x + 4, y + 8, x + 9, y + 8, color);
    }
    DrawText(label, x + 22, y, 17, cream);
}

static Rectangle draw_challenge(const World *world, Object object, Vector2 feet)
{
    bool bridge = world->location == WORLD_BRIDGE_CONTROL || object == DRAIN ||
                  object == BRIDGE_CONTROL_DOOR ||
                  (world->location == WORLD_EXTERIOR && feet.x >= canal.x - 125 && feet.x <= canal.x + canal.width + 125 &&
                   feet.y >= passage_top - 125 && feet.y <= passage_bottom + 125);
    bool nursery = world->location == WORLD_EXTERIOR && feet.x >= 1270 && feet.y <= 475;
    if (!bridge && !nursery) return (Rectangle){0};

    int top = 60;
    Rectangle card = {916, top, 346, bridge ? 119 : 142};
    int left = (int)card.x + 13;
    int right = (int)(card.x + card.width) - 13;
    bool open = bridge ? world_bridge_open(world) : world_gate_open(world);
    Color status_color = open ? (Color){170, 224, 145, 255} : (Color){246, 198, 115, 255};
    hud_card(card, status_color);
    DrawText(bridge ? "Ponte" : "Porta do viveiro", left, top + 10, 21, cream);
    const char *status = bridge ? (open ? "Estendida" : "Recolhida") : (open ? "Aberta" : "Fechada");
    DrawText(status, right - MeasureText(status, 16), top + 13, 16, status_color);
    DrawLine(left, top + 35, right, top + 35, Fade(stone, 0.26f));

    const char *rule;
    if (bridge) {
        bool one_channel = world->north_channel != world->south_channel;
        int pressure = world_bridge_pressure(world);
        const char *channels = one_channel ? (world->north_channel ? "Só o canal norte aberto" : "Só o canal sul aberto")
            : world->north_channel ? "Dois canais abertos" : "Nenhum canal aberto";
        const char *level = pressure < BRIDGE_TARGET_PRESSURE ? "baixa"
            : pressure > BRIDGE_TARGET_PRESSURE ? "alta" : "correta";
        draw_condition(left, top + 44, one_channel, channels);
        draw_condition(left, top + 67, pressure == BRIDGE_TARGET_PRESSURE,
                       TextFormat("Pressão %d/%d  ·  %s", pressure, BRIDGE_TARGET_PRESSURE, level));
        rule = TextFormat("(Norte XOR Sul) E (Pressão = %d)", BRIDGE_TARGET_PRESSURE);
    } else {
        draw_condition(left, top + 44, world->water, world->water ? "Água liberada" : "Água fechada");
        draw_condition(left, top + 67, world->light, world->light ? "Refletor alimentado" : "Refletor sem energia");
        draw_condition(left, top + 90, !world->alarm, world->alarm ? "Alarme ligado" : "Alarme desligado");
        rule = world->gate_unlocked ? "Trava acionada · porta liberada" : "Água E Luz E NÃO Alarme";
    }
    DrawText(rule, left, top + (int)card.height - 24, 16, stone);
    return card;
}

static const char *current_objective(const World *world)
{
    if (!world->maintenance_key) return "Encontre a Chave da Manutenção";
    if (!world->basement_visited) return "Abra a escotilha da manutenção";
    if (!world->light && !world->gate_unlocked) return "Alimente o refletor no subsolo";
    if (world->alarm && !world->gate_unlocked) return "Desligue o alarme na manutenção";
    if (!world->water && !world->gate_unlocked) return world_bridge_open(world)
        ? "Atravesse a ponte e libere a água" : "Ajuste os controles da ponte";
    if (!world->gate_unlocked) return "Entre no viveiro";
    if (!world->guardian_defeated) return "Vença o Girassol guardião";
    if (!world->canal_note || !world->alarm_note) return "Reúna as duas pistas dos relés";
    return "Resolva a Central de Relés";
}

static const char *interaction_hint(const World *world, Object object)
{
    static const char *const prompts[OBJECT_COUNT] = {
        [VALVE] = "[E] Alternar água",
        [POWER] = "[E] Redirecionar energia",
        [ALARM] = "[E] Alternar alarme",
        [NORTH_CHANNEL] = "[E] Alternar canal norte",
        [SOUTH_CHANNEL] = "[E] Alternar canal sul",
        [DRAIN] = "[E] Alternar dreno",
        [ALOE] = "[E] Desafiar Babosa pela chave",
        [SUNFLOWER] = "[E] Desafiar Girassol",
        [CANAL_NOTE] = "[E] Registrar pista dos canais",
        [ALARM_NOTE] = "[E] Registrar pista da cabine",
        [MAINTENANCE_DOOR] = "[E] Entrar na casa de manutenção",
        [DEPOT_DOOR] = "[E] Entrar no depósito",
        [NURSERY_DOOR] = "[E] Entrar no viveiro",
        [BRIDGE_CONTROL_DOOR] = "[E] Entrar no controle da ponte",
        [INTERIOR_DOOR] = "[E] Voltar ao pátio",
        [LOCKED_HATCH] = "[E] Descer pela escotilha",
        [BASEMENT_HATCH] = "[E] Subir pela escotilha",
        [UPPER_LIFT] = "[E] Descer pelo elevador",
        [LOWER_LIFT] = "[E] Subir pelo elevador",
        [RELAY_PANEL] = "[E] Abrir central de relés",
        [ENTRANCE_SIGN] = "[E] Ler placa do jardim",
        [BRIDGE_SIGN] = "[E] Ler aviso da ponte",
        [GARDENER] = "[E] Falar com a jardineira",
        [TECHNICIAN] = "[E] Falar com o técnico",
        [RESEARCHER] = "[E] Falar com a pesquisadora"
    };
    if (object == DRAIN) return world->drain_open ? "[E] Fechar dreno" : "[E] Abrir dreno";
    if (object == POWER) return world->light ? "[E] Alimentar elevador" : "[E] Alimentar refletor";
    if (object == UPPER_LIFT && !world->basement_visited) return "Elevador travado. Use a escotilha primeiro.";
    if ((object == UPPER_LIFT || object == LOWER_LIFT) && world->light)
        return "Elevador sem energia. Volte pela escotilha.";
    if (object == LOCKED_HATCH && !world->maintenance_key)
        return "Escotilha trancada. A Babosa guarda a chave.";
    if (object == NURSERY_DOOR && !world_gate_open(world))
        return "Viveiro fechado. Confira as condições.";
    if (object == CANAL_NOTE && world->canal_note) return "[E] Reler a pista dos canais";
    if (object == ALARM_NOTE && world->alarm_note) return "Pista da cabine registrada";
    if (object == RELAY_PANEL && !world->guardian_defeated)
        return "Central bloqueada até você vencer o Girassol.";
    return prompts[object];
}

static void draw_interaction_hint(const World *world, Object object, Rectangle challenge)
{
    const char *hint = interaction_hint(world, object);
    bool actionable = hint[0] == '[';
    if (actionable) hint += 4; /* Remove o prefixo [E] da mensagem. */
    Vector2 position = GetWorldToScreen2D(objects[object].position, world_camera(world));
    float width = MeasureText(hint, 17) + (actionable ? 55 : 24);
    float left = position.x < GAME_SCREEN_WIDTH / 2.0f ? position.x + 42 : position.x - width - 42;
    Rectangle card = {
        roundf(fminf(fmaxf(left, 18), GAME_SCREEN_WIDTH - 18 - width)),
        roundf(fminf(fmaxf(position.y - 71, 92), GAME_SCREEN_HEIGHT - 76)), width, 34
    };
    if (challenge.width > 0 && CheckCollisionRecs(card, challenge))
        card.x = challenge.x - width - 10;
    hud_card(card, actionable ? stone : ORANGE);
    if (actionable) {
        DrawRectangle((int)card.x + 8, (int)card.y + 6, 23, 22, cream);
        DrawText("E", (int)card.x + 15, (int)card.y + 9, 16, ink);
    }
    DrawText(hint, (int)card.x + (actionable ? 42 : 12), (int)card.y + 8, 17, cream);
}

static void draw_dialogue(const World *world)
{
    Rectangle card = {48, 534, GAME_SCREEN_WIDTH - 96, 160};
    DrawRectangleRec((Rectangle){card.x + 4, card.y + 5, card.width, card.height}, Fade(ink, 0.46f));
    DrawRectangleRec(card, cream);
    DrawRectangleLinesEx(card, 4, frame);
    DrawText(world->dialogue_title, 72, 551, 26, ink);
    DrawLine(72, 584, GAME_SCREEN_WIDTH - 72, 584, stone);
    DrawText(world->dialogue_pages[world->dialogue_page], 72, 594, 22, ink);

    bool has_next = world->dialogue_page == 0 && world->dialogue_pages[1];
    const char *controls = has_next ? "E / Enter  Próxima     Esc  Fechar" : "E / Enter / Esc  Fechar";
    DrawText(controls, GAME_SCREEN_WIDTH - 72 - MeasureText(controls, 17), 666, 17, frame);
}

void world_draw_interface(const World *world, Vector2 feet)
{
    if (world->dialogue_title) {
        draw_dialogue(world);
        return;
    }
    if (world->relay_panel_open) {
        draw_relay_panel(world);
        return;
    }
    if (world->completed) {
        DrawRectangle(0, 0, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT, Fade(ink, 0.83f));
        DrawRectangle(310, 260, 660, 190, cream);
        DrawRectangleLinesEx((Rectangle){310, 260, 660, 190}, 4, frame);
        centered_text("Estufa concluída!", 640, 291, 36, ink);
        centered_text("Você venceu o guardião e validou os oito relés.", 640, 350, 22, ink);
        centered_text("ENTER ou ESC  Voltar ao menu", 640, 401, 20, ink);
        return;
    }

    const Sector *sector = world->location == WORLD_EXTERIOR ? sector_at(feet) : &rooms[world->location];
    const char *area = sector ? sector->name : "Travessia";
    const char *objective = current_objective(world);
    int objective_width = (int)fmaxf(MeasureText(area, 16), MeasureText(objective, 21));
    hud_card((Rectangle){18, 18, objective_width + 32, 61}, stone);
    DrawText(area, 34, 27, 16, stone);
    DrawText(objective, 34, 49, 21, cream);

    hud_card((Rectangle){1139, 18, 123, 32}, stone);
    DrawText("Pistas", 1151, 26, 16, stone);
    DrawText(TextFormat("%d/2", world->canal_note + world->alarm_note), 1220, 25, 18, cream);

    Object object = nearby_object(world, feet);
    Rectangle challenge = draw_challenge(world, object, feet);
    if (object != OBJECT_COUNT) draw_interaction_hint(world, object, challenge);

    const char *controls = "WASD / Setas  Mover   Shift  Correr   E  Interagir   Esc  Menu";
    hud_card((Rectangle){18, 674, MeasureText(controls, 15) + 24, 28}, stone);
    DrawText(controls, 30, 681, 15, stone);
}
