/* Desenha a estufa e resolve os obstáculos a partir das proposições do cenário. */
#include "world.h"
#include "dialogue_ui.h"
#include "ui_theme.h"
#include "game_view.h"
#include "courtyard_detail_data.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>

static void open_dialogue(World *world, const char *title, const char *first, const char *second);

/* Coordenadas do exterior medidas na imagem de referência. */
enum { MAP_WIDTH = 1672, MAP_HEIGHT = 941,
       TREE_CANOPY_X = 580, TREE_CANOPY_Y = 205,
       TREE_CANOPY_WIDTH = 279, TREE_CANOPY_HEIGHT = 207,
       TREE_CANOPY_WORLD_WIDTH = 155, TREE_CANOPY_WORLD_HEIGHT = 115,
       INTERIOR_CROP_X = 84, INTERIOR_CROP_Y = 13,
       INTERIOR_CROP_WIDTH = 1504, INTERIOR_CROP_HEIGHT = 907,
       PORTAL_CROP_X = 636, PORTAL_CROP_Y = 690,
       PORTAL_CROP_WIDTH = 400, PORTAL_CROP_HEIGHT = 230 };

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
    RELAY_PANEL,
    ENTRANCE_SIGN,
    LOGIC_SIGN,
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
    [NORTH_CHANNEL] = {{539, 350}, WORLD_BRIDGE_CONTROL},
    [SOUTH_CHANNEL] = {{742, 350}, WORLD_BRIDGE_CONTROL},
    [DRAIN] = {{733, 555}, WORLD_EXTERIOR},
    [ALOE] = {{640, 420}, WORLD_DEPOT},
    [SUNFLOWER] = {{640, 420}, WORLD_NURSERY},
    [CANAL_NOTE] = {{1271, 401}, WORLD_EXTERIOR},
    [ALARM_NOTE] = {{537, 300}, WORLD_MAINTENANCE},
    [MAINTENANCE_DOOR] = {{243, 645}, WORLD_EXTERIOR},
    [DEPOT_DOOR] = {{278, 379}, WORLD_EXTERIOR},
    [NURSERY_DOOR] = {{1396, 330}, WORLD_EXTERIOR},
    [BRIDGE_CONTROL_DOOR] = {{810, 561}, WORLD_EXTERIOR},
    [INTERIOR_DOOR] = {{640, 565}, WORLD_MAINTENANCE},
    [LOCKED_HATCH] = {{846, 416}, WORLD_MAINTENANCE},
    [BASEMENT_HATCH] = {{640, 565}, WORLD_BASEMENT},
    [RELAY_PANEL] = {{782, 315}, WORLD_NURSERY},
    [ENTRANCE_SIGN] = {{487, 787}, WORLD_EXTERIOR},
    [LOGIC_SIGN] = {{505, 448}, WORLD_EXTERIOR},
    [GARDENER] = {{630, 507}, WORLD_EXTERIOR},
    [TECHNICIAN] = {{868, 600}, WORLD_EXTERIOR},
    [RESEARCHER] = {{790, 485}, WORLD_NURSERY}
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
/* Área da copa da árvore central liberada dentro do círculo marcado. */
static const Vector2 tree_courtyard_path[] = {
    {585, 216}, {602, 208}, {718, 208}, {733, 218}, {739, 232},
    {739, 278}, {734, 294}, {730, 300}, {724, 305}, {721, 311}, {720, 316},
    {710, 310}, {590, 310}, {584, 316}, {578, 312}, {574, 306}, {578, 303}, {569, 292},
    {563, 274}, {562, 239}, {568, 220}
};
/* Pequenas extensões laterais da copa, dentro das duas novas marcações. */
static const Vector2 tree_left_side_path[] = {
    {569, 291}, {590, 291}, {590, 314}, {569, 314}
};
static const Vector2 tree_right_side_path[] = {
    {716, 291}, {735, 291}, {735, 314}, {716, 314}
};
static const Vector2 left_bed_path[] = {
    {454, 458}, {502, 455}, {508, 463}, {510, 499}, {454, 506}, {452, 484}
};

#define WALK_AREA(points) {points, sizeof(points) / sizeof(points[0])}
static const struct {
    const Vector2 *points;
    int count;
} walk_areas[] = {
    WALK_AREA(entry_path), WALK_AREA(courtyard_path), WALK_AREA(depot_path),
    WALK_AREA(maintenance_path), WALK_AREA(pergola_path), WALK_AREA(garden_loop_path), WALK_AREA(east_path),
    WALK_AREA(irrigation_path), WALK_AREA(tree_courtyard_path), WALK_AREA(tree_left_side_path),
    WALK_AREA(tree_right_side_path), WALK_AREA(left_bed_path)
};
#undef WALK_AREA

static const Rectangle crossing = {899, 525, 160, 38};
static const float passage_top = 525;
static const float passage_bottom = 563;
static const float interaction_radius = 55;
static const Rectangle maintenance_hatch_bounds = {792, 372, 111, 90};
static const float hatch_interaction_margin = 36;

static const Color ink = {24, 49, 43, 255};
static const Color frame = {39, 74, 60, 255};
static const Color cream = {249, 240, 205, 255};
static const Color stone = {220, 209, 165, 255};
static const Color water_color = {58, 145, 164, 255};
static const Color path_edge = {139, 123, 87, 255};
static const Rectangle validate_relays = {940, 631, 240, 44};
static const Rectangle close_relays = {100, 631, 210, 44};
enum { ROOM_HEADER_HEIGHT = 69 };
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
    {397, 270, 112, 78}, /* Painel e bancada de parede. */
    {574, 319, 160, 40}, /* Base da prateleira central. */
    {870, 260, 96, 97},  /* Caldeira e tubulações. */
    {831, 316, 45, 42},  /* Caixote junto às ferramentas. */
    {302, 392, 146, 52}, /* Bancada de trabalho. */
    {380, 455, 48, 48},  /* Banco, com a base alinhada aos pés. */
    {305, 493, 62, 63},  /* Caixote de madeira inferior esquerdo. */
    {360, 508, 75, 50},  /* Baú verde inferior. */
    {917, 423, 58, 132}  /* Estante lateral. */
};

/* Pegadas dos mÃ³veis sÃ³lidos dos interiores, medidas no espaÃ§o de cada sala. */
static const Rectangle depot_furniture[] = {
    {302, 270, 78, 125}, {300, 375, 160, 74}, {415, 270, 370, 73},
    {850, 265, 80, 125}, {925, 405, 55, 155}, {300, 495, 112, 68}
};
static const Rectangle basement_furniture[] = {
    {300, 260, 114, 82}, /* Parede de tijolos à esquerda, acima do piso livre. */
    {414, 320, 270, 45}, {658, 318, 245, 58},
    {910, 325, 67, 132}, {300, 426, 77, 130}, {918, 430, 58, 125},
    {300, 548, 300, 28}, {680, 548, 300, 28}
};
static const Rectangle nursery_furniture[] = {
    {300, 325, 168, 123}, {478, 302, 126, 48}, {690, 325, 150, 35},
    {806, 292, 102, 76}, {925, 325, 54, 113}, {300, 464, 188, 67},
    {770, 395, 145, 78}, {817, 476, 155, 62}
};
static const Rectangle bridge_control_furniture[] = {
    {305, 355, 160, 92}, {500, 325, 82, 59}, {586, 304, 113, 63},
    {708, 325, 82, 59}, {897, 325, 76, 125}, {302, 480, 105, 76},
    {900, 480, 75, 76}
};
static const Rectangle interior_south_walls[] = {
    {300, 555, 290, 24}, {690, 555, 290, 24}
};
static const Rectangle interior_exit_door = {590, 555, 100, 24};
static const Rectangle interior_portal_depth = {540, 510, 200, 90};

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

static Texture2D load_sprite_texture(const char *path)
{
    if (!FileExists(path)) return (Texture2D){0};
    Texture2D texture = LoadTexture(path);
    if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

/* Usa a mesma arte da arena e remove a margem transparente antes de desenhar. */
static Texture2D load_cropped_sprite_texture(const char *path)
{
    if (!FileExists(path)) return (Texture2D){0};
    Image image = LoadImage(path);
    if (!image.data) return (Texture2D){0};

    Rectangle source = GetImageAlphaBorder(image, 128.0f / 255.0f);
    if (source.width <= 0 || source.height <= 0) {
        UnloadImage(image);
        return (Texture2D){0};
    }
    ImageCrop(&image, source);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
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
            "assets/maps/maintenance_house_interior.png", 1672, 941),
        .courtyard_detail = load_map_texture("assets/maps/courtyard_detail/courtyard_complete.png",
                                            COURTYARD_DETAIL_WIDTH, COURTYARD_DETAIL_HEIGHT),
        .tree_canopy_foreground = load_map_texture(
            "assets/maps/tree_canopy_foreground.png", TREE_CANOPY_WIDTH, TREE_CANOPY_HEIGHT),
        .depot_interior = load_map_texture("assets/maps/depot_interior.png", 1672, 941),
        .basement_interior = load_map_texture("assets/maps/basement_interior.png", 1672, 941),
        .bridge_control_interior = load_map_texture(
            "assets/maps/bridge_control_interior.png", 1672, 941),
        .nursery_interior = load_map_texture("assets/maps/nursery_interior.png", 1672, 941),
        .interior_portal_foreground = {
            [WORLD_MAINTENANCE] = load_map_texture(
                "assets/maps/interior_foregrounds/maintenance_house_portal.png",
                PORTAL_CROP_WIDTH, PORTAL_CROP_HEIGHT),
            [WORLD_DEPOT] = load_map_texture(
                "assets/maps/interior_foregrounds/depot_portal.png",
                PORTAL_CROP_WIDTH, PORTAL_CROP_HEIGHT),
            [WORLD_NURSERY] = load_map_texture(
                "assets/maps/interior_foregrounds/nursery_portal.png",
                PORTAL_CROP_WIDTH, PORTAL_CROP_HEIGHT),
            [WORLD_BRIDGE_CONTROL] = load_map_texture(
                "assets/maps/interior_foregrounds/bridge_control_portal.png",
                PORTAL_CROP_WIDTH, PORTAL_CROP_HEIGHT)
        },
        .aloe_guard = load_cropped_sprite_texture("assets/sprites/plants/aloe_v2.png"),
        .sunflower_guard = load_cropped_sprite_texture("assets/sprites/plants/sunflower_v2.png"),
        .boss_ground = load_sprite_texture("assets/ui/battle/ground_v1.png")
    };
    if (assets->tree_canopy_foreground.id) {
        SetTextureFilter(assets->tree_canopy_foreground, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(assets->tree_canopy_foreground, TEXTURE_WRAP_CLAMP);
    }
    if (assets->courtyard_detail.id) {
        SetTextureFilter(assets->courtyard_detail, TEXTURE_FILTER_BILINEAR);
        /* Impede o filtro de buscar pixels da borda oposta nas margens transparentes. */
        SetTextureWrap(assets->courtyard_detail, TEXTURE_WRAP_CLAMP);
    }
    for (int i = WORLD_MAINTENANCE; i < WORLD_LOCATION_COUNT; i++) {
        if (assets->interior_portal_foreground[i].id)
            SetTextureWrap(assets->interior_portal_foreground[i], TEXTURE_WRAP_CLAMP);
    }
}

void world_assets_unload(WorldAssets *assets)
{
    if (assets->courtyard.id) UnloadTexture(assets->courtyard);
    if (assets->bridge_open.id) UnloadTexture(assets->bridge_open);
    if (assets->tiles.id) UnloadTexture(assets->tiles);
    if (assets->maintenance_interior.id) UnloadTexture(assets->maintenance_interior);
    if (assets->courtyard_detail.id) UnloadTexture(assets->courtyard_detail);
    if (assets->tree_canopy_foreground.id) UnloadTexture(assets->tree_canopy_foreground);
    if (assets->depot_interior.id) UnloadTexture(assets->depot_interior);
    if (assets->basement_interior.id) UnloadTexture(assets->basement_interior);
    if (assets->bridge_control_interior.id) UnloadTexture(assets->bridge_control_interior);
    if (assets->nursery_interior.id) UnloadTexture(assets->nursery_interior);
    for (int i = WORLD_MAINTENANCE; i < WORLD_LOCATION_COUNT; i++) {
        if (assets->interior_portal_foreground[i].id)
            UnloadTexture(assets->interior_portal_foreground[i]);
    }
    if (assets->aloe_guard.id) UnloadTexture(assets->aloe_guard);
    if (assets->sunflower_guard.id) UnloadTexture(assets->sunflower_guard);
    if (assets->boss_ground.id) UnloadTexture(assets->boss_ground);
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

/* Cada etapa é independente: ligar, combinar E e combinar OU com NÃO. */
static bool relay_stage_matches(unsigned int mask, int step)
{
    if (step < 0 || step >= WORLD_RELAY_RULE_COUNT || mask >= (1u << WORLD_RELAY_COUNT)) return false;
    bool a = relay_on(mask,0), b = relay_on(mask,1), c = relay_on(mask,2);
    if (step == 0) return a;
    if (step == 1) return a && b;
    return (a || b) && !c;
}

bool world_relays_match(unsigned int mask)
{
    return relay_stage_matches(mask,WORLD_RELAY_RULE_COUNT-1);
}

/* Os mesmos limites servem à entrada e ao desenho industrial. */
Rectangle world_relay_bounds(int index)
{
    return (Rectangle){195+305*index,370,280,196};
}

Rectangle world_relay_close_bounds(void) { return close_relays; }
Rectangle world_relay_validate_bounds(void) { return validate_relays; }

static bool clicked(Rectangle bounds)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), bounds);
}

void world_update_relay_panel(World *world, const char *player_name)
{
    if (!world->relay_panel_open || world->completed) return;
    if (IsKeyPressed(KEY_ESCAPE) || clicked(close_relays)) {
        world->relay_panel_open = false;
        return;
    }
    if (world->relay_step < 0 || world->relay_step >= WORLD_RELAY_RULE_COUNT) return;
    int count = WORLD_RELAY_COUNT;
    if (world->relay_cursor < 0 || world->relay_cursor >= count) world->relay_cursor = 0;
    Vector2 motion = GetMouseDelta();
    if (motion.x || motion.y)
        for (int i = 0; i < count; i++)
            if (CheckCollisionPointRec(GetMousePosition(),world_relay_bounds(i))) world->relay_cursor = i;
    if (IsKeyPressed(KEY_RIGHT)) world->relay_cursor = (world->relay_cursor+1)%count;
    if (IsKeyPressed(KEY_LEFT)) world->relay_cursor = (world->relay_cursor+count-1)%count;
    for (int i = 0; i < WORLD_RELAY_COUNT; i++) {
        if (IsKeyPressed(KEY_ONE + i) || clicked(world_relay_bounds(i)) ||
            (IsKeyPressed(KEY_SPACE) && world->relay_cursor == i)) {
            world->relay_cursor = i;
            world->relay_mask ^= 1u << i;
            world->relay_rejected = false;
        }
    }

    if (IsKeyPressed(KEY_ENTER) || clicked(validate_relays)) {
        world->relay_attempts++;
        world->relay_rejected = !relay_stage_matches(world->relay_mask,world->relay_step);
        if (world->relay_rejected) return;
        if (world->relay_step == WORLD_RELAY_RULE_COUNT - 1) {
            world->completed = true;
            world->relay_panel_open = false;
            const char *name = player_name && player_name[0] ? player_name : "visitante";
            snprintf(world->dialogue_greeting, sizeof(world->dialogue_greeting),
                     "%s, você conseguiu! A central passou em todos os testes. Obrigada por topar essa investigação comigo!", name);
            open_dialogue(world, "Íris", world->dialogue_greeting,
                          "Com a central resolvida, podemos voltar a cuidar das plantas. Obrigada por ajudar a mim, à Lia e ao Bento!");
        } else {
            world->relay_step++;
            world->relay_cursor = 0;
            world->relay_mask = 0;
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
    float edge_margin = world->location == WORLD_EXTERIOR ? 18.0f : 12.0f;
    if (feet.x < b.x + edge_margin) feet.x = b.x + edge_margin;
    if (feet.x > b.x + b.width - edge_margin) feet.x = b.x + b.width - edge_margin;
    if (feet.y < b.y + edge_margin) feet.y = b.y + edge_margin;
    if (feet.y > b.y + b.height - edge_margin) feet.y = b.y + b.height - edge_margin;

    /* Conserva os pés do exterior; os interiores usam o explorador ampliado. */
    Rectangle box = world->location == WORLD_EXTERIOR
        ? (Rectangle){feet.x - 8, feet.y - 4, 16, 8}
        : (Rectangle){feet.x - 12, feet.y - 7, 24, 14};
    /* A placa lógica ocupa o caminho: impede atravessar a tábua e o suporte. */
    if (world->location == WORLD_EXTERIOR) {
        Vector2 sign = objects[LOGIC_SIGN].position;
        if (CheckCollisionRecs(box,(Rectangle){sign.x-22,sign.y-38,44,38})) return previous;
    }
    const Rectangle *furniture = NULL;
    size_t furniture_count = 0;
    switch (world->location) {
        case WORLD_MAINTENANCE:
            furniture = maintenance_furniture;
            furniture_count = sizeof(maintenance_furniture) / sizeof(maintenance_furniture[0]);
            break;
        case WORLD_DEPOT:
            furniture = depot_furniture;
            furniture_count = sizeof(depot_furniture) / sizeof(depot_furniture[0]);
            break;
        case WORLD_BASEMENT:
            furniture = basement_furniture;
            furniture_count = sizeof(basement_furniture) / sizeof(basement_furniture[0]);
            break;
        case WORLD_NURSERY:
            furniture = nursery_furniture;
            furniture_count = sizeof(nursery_furniture) / sizeof(nursery_furniture[0]);
            break;
        case WORLD_BRIDGE_CONTROL:
            furniture = bridge_control_furniture;
            furniture_count = sizeof(bridge_control_furniture) / sizeof(bridge_control_furniture[0]);
            break;
        default: break;
    }
    for (size_t i = 0; i < furniture_count; i++)
        if (CheckCollisionRecs(box, furniture[i])) return previous;
    if (world->location != WORLD_EXTERIOR) {
        for (size_t i = 0; i < sizeof(interior_south_walls) / sizeof(interior_south_walls[0]); i++)
            if (CheckCollisionRecs(box, interior_south_walls[i])) return previous;
        if (world->location != WORLD_BASEMENT && CheckCollisionRecs(box, interior_exit_door))
            return previous;
    }

    for (Object npc = GARDENER; npc <= RESEARCHER; npc++) {
        if (objects[npc].location != world->location) continue;
        Vector2 base = objects[npc].position;
        if (CheckCollisionRecs(box, (Rectangle){base.x - 11, base.y - 6, 22, 12})) return previous;
    }
    if (world->location == WORLD_DEPOT && !world->maintenance_key) {
        Vector2 guard = objects[ALOE].position;
        if (CheckCollisionRecs(box, (Rectangle){guard.x - 28, guard.y - 18, 56, 28})) return previous;
    }
    if (world->location == WORLD_NURSERY && !world->guardian_defeated) {
        Vector2 guard = objects[SUNFLOWER].position;
        if (CheckCollisionRecs(box, (Rectangle){guard.x - 28, guard.y - 18, 56, 28})) return previous;
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

/* Presença compartilhada pela seleção próxima e pelos indicadores à distância. */
static bool object_present(const World *world, int index)
{
    bool exit_door = index == INTERIOR_DOOR &&
        (world->location == WORLD_DEPOT || world->location == WORLD_NURSERY ||
         world->location == WORLD_BRIDGE_CONTROL);
    if (objects[index].location != world->location && !exit_door) return false;
    if ((index == ALOE && world->maintenance_key) ||
        (index == SUNFLOWER && world->guardian_defeated)) return false;
    return true;
}

static Object nearby_object(const World *world, Vector2 feet)
{
    Object nearest = OBJECT_COUNT;
    float best_distance = interaction_radius * interaction_radius;
    for (int i = 0; i < OBJECT_COUNT; i++) {
        if (!object_present(world,i)) continue;
        float dx;
        float dy;
        if (i == LOCKED_HATCH && world->location == WORLD_MAINTENANCE) {
            float closest_x = fminf(fmaxf(feet.x, maintenance_hatch_bounds.x),
                                    maintenance_hatch_bounds.x + maintenance_hatch_bounds.width);
            float closest_y = fminf(fmaxf(feet.y, maintenance_hatch_bounds.y),
                                    maintenance_hatch_bounds.y + maintenance_hatch_bounds.height);
            dx = feet.x - closest_x;
            dy = feet.y - closest_y;
            if (dx * dx + dy * dy > hatch_interaction_margin * hatch_interaction_margin) continue;
        } else {
            dx = objects[i].position.x - feet.x;
            dy = objects[i].position.y - feet.y;
        }
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
        world->bounds = (Rectangle){room.x + 24, room.y + 136,
                                    room.width - 48, room.height - 174};
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
    world->dialogue_pages[2] = NULL;
    world->dialogue_page = 0;
    world->dialogue_visible_characters = 0;
    world->dialogue_character_timer = 0;
}

/* A apresentação precede as pistas, sem descartar as páginas da missão. */
static void open_character_dialogue(World *world, Object character, const char *player_name,
                                    const char *title, const char *first, const char *second)
{
    static const char *const introductions[WORLD_NPC_COUNT] = {
        "Você deve ser %s! Eu sou a Lia. Cuido desse jardim — e ele anda me dando trabalho por três. Ainda bem que você chegou.",
        "Ah, você é %s? Sou o Bento. Se alguma coisa aqui emperra, me chamam. Hoje resolveram emperrar todas juntas.",
        "%s, certo? Sou a Íris. Estudo as plantas daqui. Você precisa ver o que descobri sobre o Girassol… mas vamos por partes."
    };
    static const char *const greetings[WORLD_NPC_COUNT] = {
        "Olha quem voltou, %s! %s",
        "Ei, %s! %s",
        "%s! %s"
    };
    int index = character-GARDENER;
    const char *name = player_name && player_name[0] ? player_name : "visitante";
    open_dialogue(world, title, first, second);
    if (!world->npc_introduced[index]) {
        const char *format = introductions[index];
        if (character == RESEARCHER && world->npc_introduced[0])
            format = "%s, certo? A Lia comentou que você vinha! Sou a Íris. Estudo as plantas daqui. Você precisa ver o que descobri sobre o Girassol… mas vamos por partes.";
        snprintf(world->dialogue_introduction, sizeof(world->dialogue_introduction), format, name);
        world->dialogue_pages[2] = second;
        world->dialogue_pages[1] = first;
        world->dialogue_pages[0] = world->dialogue_introduction;
        world->npc_introduced[index] = true;
    } else {
        snprintf(world->dialogue_greeting, sizeof(world->dialogue_greeting), greetings[index], name, first);
        world->dialogue_pages[0] = world->dialogue_greeting;
    }
}

void world_update_dialogue(World *world, float dt)
{
    if (!world->dialogue_title) return;
    const char *message = world->dialogue_pages[world->dialogue_page];
    int total = message ? GetCodepointCount(message) : 0;
    if (IsKeyPressed(KEY_ESCAPE)) {
        world->dialogue_title = NULL;
        return;
    }
    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER)) {
        if (world->dialogue_visible_characters < total) {
            /* O toque que revela a fala nunca avança a página no mesmo quadro. */
            world->dialogue_visible_characters = total;
            world->dialogue_character_timer = 0;
        } else if (world->dialogue_page+1 < WORLD_DIALOGUE_PAGE_COUNT && world->dialogue_pages[world->dialogue_page+1]) {
            world->dialogue_page++;
            world->dialogue_visible_characters = 0;
            world->dialogue_character_timer = 0;
        } else {
            world->dialogue_title = NULL;
        }
        return;
    }
    if (world->dialogue_visible_characters < total && isfinite(dt) && dt > 0) {
        float characters = world->dialogue_character_timer + dt*40.0f;
        int added = (int)fminf(floorf(characters), total-world->dialogue_visible_characters);
        world->dialogue_visible_characters += added;
        world->dialogue_character_timer = world->dialogue_visible_characters < total ? characters-added : 0;
    }
}

static void talk_to_gardener(World *world, const char *player_name)
{
    const char *first;
    const char *second;
    if (world_gate_open(world)) {
        first = world->npc_help_thanked[0] ?
            "O viveiro voltou a receber visitas! Dá gosto ver esse jardim ganhando vida outra vez." :
            "Você conseguiu reabrir o viveiro! Obrigada por cuidar desse jardim comigo. As plantas ganharam uma boa companhia.";
        second = "Passe lá para ver a Íris, do outro lado do canal. Ela precisa de ajuda com a central e adora dividir uma descoberta.";
        world->npc_help_thanked[0] = true;
    } else if (!world->maintenance_key) {
        first = "Você me ajuda a cuidar do viveiro? O refletor apagou. A chave da manutenção ficou com a Babosa, no depósito.";
        second = "O depósito é o prédio grande de telhado terracota, acima de mim. Vença a Babosa e leve a chave até a casa de manutenção.";
    } else if (!world->light) {
        first = "Você trouxe a chave! Obrigada. Agora falta acender o refletor. Entre na manutenção e desça pela escotilha.";
        second = "No subsolo, ligue o refletor no painel. Depois, volte pela mesma escotilha; o caminho continua livre.";
    } else if (world->alarm) {
        first = "Olha o refletor aceso! As plantas já estavam sentindo falta. Pode me ajudar com mais uma coisa? O alarme ainda está ligado.";
        second = world->water ?
            "O painel fica dentro da casa de manutenção. Com água e luz prontas, só falta esse sossego para abrir o viveiro." :
            "O painel fica dentro da casa de manutenção. Desligue o alarme por lá; depois podemos cuidar da água.";
    } else if (!world_bridge_open(world)) {
        first = "Refletor aceso, alarme quietinho... Só falta a água! Você ajuda o Bento a liberar a ponte? Ele está perto do canal.";
        second = "A válvula fica do outro lado da ponte, perto do viveiro. Abra a água por lá e a porta do viveiro poderá ser liberada.";
    } else {
        first = "A ponte já está livre! Só falta abrir a válvula do outro lado para a água chegar às plantas. Pode cuidar disso para mim?";
        second = "Depois, entre no viveiro e procure a Íris. Ela vai gostar de conhecer quem nos ajudou a colocar tudo em ordem.";
    }
    open_character_dialogue(world, GARDENER, player_name, "Lia", first, second);
}

static void talk_to_technician(World *world, const char *player_name)
{
    const char *first;
    const char *second;
    if (world_bridge_open(world)) {
        first = world->npc_help_thanked[1] ?
            "A ponte continua firme. Viu? Ela só precisava de alguém com mais paciência do que eu." :
            "Olha só, você conseguiu! Ponte liberada. Obrigado pela força; essa teimosa já estava me tirando do sério.";
        if (!world->water)
            second = "Agora atravesse e abra a válvula do outro lado, perto do viveiro. A Lia precisa dessa água para as plantas.";
        else if (!world->light && !world->gate_unlocked)
            second = "A água já está correndo. Falta ligar o refletor: procure a Lia e passe pela casa de manutenção.";
        else if (world->alarm && !world->gate_unlocked)
            second = "Água e luz prontas! Só falta desligar o alarme na manutenção para entrar no viveiro. Esse barulho não vai fazer falta.";
        else
            second = "Pode seguir para o viveiro, do outro lado do canal. A Íris está lá dentro e precisa de ajuda com a central.";
        world->npc_help_thanked[1] = true;
    } else if (!world->north_channel && !world->south_channel) {
        first = "Você me ajuda a liberar a ponte? Precisamos de um canal aberto e pressão 3. Com os dois fechados, essa belezinha não sai do lugar.";
        second = "Os registros ficam na casa de controle, aqui ao lado. O dreno está do lado de fora: quando aberto, tira 2 da pressão.";
    } else if (world->north_channel && world->south_channel) {
        first = "Opa, os dois canais estão abertos! Essa ponte só aceita um de cada vez. Pode fechar um? A pressão também precisa ficar em 3.";
        second = "Dá uma passada na casa de controle para ajustar os registros. Depois confira o dreno, aqui fora. Sem pressa: a ponte espera.";
    } else {
        first = "Um canal só: isso! Falta acertar a pressão, que precisa ficar em 3. Dá uma olhada no dreno do lado de fora da casa de controle.";
        second = "Com o dreno aberto, a pressão cai 2. Se não bastar, troque o canal lá dentro. Pode testar; só não abre os dois de novo.";
    }
    open_character_dialogue(world, TECHNICIAN, player_name, "Bento", first, second);
}

static void talk_to_researcher(World *world, const char *player_name)
{
    const char *hint;
    if (!world->alarm_note && !world->canal_note)
        hint = "A central tem três etapas simples, uma por vez. As pistas dos canais e da manutenção são dicas extras, se precisar.";
    else if (!world->alarm_note)
        hint = "A pista dos canais explica o OU. Você já pode resolver a central; o bilhete da manutenção tem outra dica opcional.";
    else if (!world->canal_note)
        hint = "O bilhete explica o NÃO. A placa dos canais tem outra dica opcional, mas você já pode resolver a central.";
    else
        hint = "Você achou as duas pistas! Na central, vamos testar uma regra por vez. Ajuste os relés e use Enter para conferir.";

    const char *first;
    if (!world->guardian_defeated) {
        first = "Você me ajuda a investigar a central? O Girassol não deixa ninguém chegar ao painel. Uma batalha deve convencê-lo a sair da frente.";
    } else if (!world->npc_help_thanked[2]) {
        first = "Você passou pelo Girassol! Obrigada, eu já estava ensaiando uma conversa com as pétalas dele. Agora podemos cuidar da central.";
        world->npc_help_thanked[2] = true;
    } else if (world->relay_step > 0) {
        first = "Olha, algumas regras já passaram! Pode continuar de onde parou; o painel guardou os ajustes. Estamos chegando lá.";
    } else {
        first = "Agora vem a parte que eu adoro: descobrir como os relés combinam. Você me ajuda a testar a central?";
    }
    open_character_dialogue(world, RESEARCHER, player_name, "Íris", first, hint);
}

static HudInteractionState interaction_state(const World *world, Object object)
{
    if ((object == LOCKED_HATCH && !world->maintenance_key) ||
        (object == NURSERY_DOOR && !world_gate_open(world)) ||
        (object == RELAY_PANEL && !world->guardian_defeated)) return HUD_INTERACTION_BLOCKED;
    return HUD_INTERACTION_AVAILABLE;
}

WorldEncounter world_interact(World *world, Vector2 *feet, const char *player_name)
{
    if (world->completed || world->logic_quiz_open || world->relay_panel_open || world->dialogue_title) return WORLD_ENCOUNTER_NONE;
    Object object = nearby_object(world, *feet);
    const Vector2 entrance = {640, 533};
    if (object != OBJECT_COUNT && interaction_state(world, object) != HUD_INTERACTION_AVAILABLE)
        return WORLD_ENCOUNTER_NONE;
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
                          "Central de Relés — dica do OU: basta A ou B estar ligado. Também funciona com os dois ligados.", NULL);
            break;
        case ALARM_NOTE:
            world->alarm_note = true;
            open_dialogue(world, "Bilhete da manutenção",
                          "Central de Relés — dica do NÃO: NÃO C é verdadeiro quando C está desligado.", NULL);
            break;
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
            /* Aparece no piso abaixo da escotilha, fora do colisor do alçapão. */
            travel(world, feet, WORLD_MAINTENANCE,
                   (Vector2){objects[LOCKED_HATCH].position.x,
                             objects[LOCKED_HATCH].position.y + 54});
            break;
        case RELAY_PANEL: world->relay_panel_open = world->guardian_defeated; break;
        case LOGIC_SIGN:
            world->logic_quiz_open = true;
            break;
        case ENTRANCE_SIGN:
            world->entrance_sign_read = true;
            open_dialogue(world, "Placa do jardim",
                          "Depósito: acima e à esquerda do pátio. Manutenção: abaixo e à esquerda.\nPara começar, fale com Lia, no pátio central.", NULL);
            break;
        case GARDENER: talk_to_gardener(world, player_name); break;
        case TECHNICIAN: talk_to_technician(world, player_name); break;
        case RESEARCHER: talk_to_researcher(world, player_name); break;
        case OBJECT_COUNT: break;
    }
    return WORLD_ENCOUNTER_NONE;
}

void world_finish_encounter(World *world, WorldEncounter encounter)
{
    if (encounter == WORLD_ENCOUNTER_ALOE && !world->maintenance_key) {
        world->maintenance_key = true;
        open_dialogue(world,"Chave do alçapão",
            "Você derrotou a Babosa e recebeu a Chave da Manutenção! Com ela, já pode abrir o alçapão na casa de manutenção.",NULL);
    }
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
        float zoom = fmaxf(GAME_SCREEN_WIDTH / room.width,
                           GAME_SCREEN_HEIGHT / room.height);
        /* Mantém a Casa de Manutenção como cenário enquadrado, com margem ao redor. */
        if (world->location == WORLD_MAINTENANCE || world->location == WORLD_DEPOT ||
            world->location == WORLD_BASEMENT || world->location == WORLD_BRIDGE_CONTROL ||
            world->location == WORLD_NURSERY)
            zoom *= 0.68f;
        return (Camera2D){
            .offset = {GAME_SCREEN_WIDTH / 2.0f, GAME_SCREEN_HEIGHT / 2.0f},
            .target = {room.x + room.width / 2, room.y + room.height / 2},
            .zoom = zoom
        };
    }
    return (Camera2D){
        .target = {roundf(world->camera_position.x), roundf(world->camera_position.y)},
        .zoom = 1.0f
    };
}

static void centered_text(const UiTheme *ui, const char *text, int x, int y, int size, Color color)
{
    ui_text(ui, text, x - ui_measure(ui, text, size) / 2, y, size, color);
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

static const Sector *sector_at(Vector2 feet)
{
    for (unsigned int i = 0; i < sizeof(sectors) / sizeof(sectors[0]); i++)
        if (CheckCollisionPointRec(feet, sectors[i].bounds)) return &sectors[i];
    return NULL;
}

static void draw_switch(const UiTheme *ui, Object object, const char *name, bool on)
{
    Vector2 p = objects[object].position;
    int x = (int)p.x;
    int y = (int)p.y;
    DrawEllipse(x, y + 17, 29, 9, Fade(ink, 0.25f));
    DrawRectangle(x - 24, y - 19, 48, 37, frame);
    DrawRectangle(x - 20, y - 15, 40, 29, ink);
    DrawCircle(x, y - 1, 10, on ? LIME : ORANGE);
    DrawCircle(x - 2, y - 4, 3, cream);
    centered_text(ui, name, x, y + 25, 18, ink);
}

static void draw_note(const UiTheme *ui, Object object, const char *name, bool collected)
{
    int x = (int)objects[object].position.x;
    int y = (int)objects[object].position.y;
    DrawRectangle(x - 17, y - 23, 34, 36, collected ? cream : GOLD);
    for (int i = 0; i < 3; i++) DrawRectangle(x - 10, y - 15 + i * 7, 20, 2, frame);
    centered_text(ui, name, x, y + 23, 18, ink);
}

static void draw_guard(Object object, Color color)
{
    int x = (int)objects[object].position.x;
    int y = (int)objects[object].position.y;
    DrawEllipse(x, y + 9, 27, 7, Fade(ink, 0.3f));
    DrawRectangle(x - 5, y - 19, 10, 30, DARKGREEN);
    DrawEllipse(x - 14, y - 3, 16, 7, DARKGREEN);
    DrawEllipse(x + 15, y - 8, 15, 7, DARKGREEN);
    DrawCircle(x, y - 23, 22, color);
    DrawCircle(x, y - 22, 11, frame);
}

/* Mesmo recorte de terra da arena; as raízes repousam no centro do canteiro. */
static void draw_boss_ground(Texture2D texture, Object object)
{
    Vector2 base = objects[object].position;
    float width = object == ALOE ? 180 : 156;
    float height = width*400/1825;
    Rectangle destination = {base.x-width/2,base.y-height*0.45f,width,height};
    if (texture.id && texture.width >= 2001 && texture.height >= 585)
        DrawTexturePro(texture,(Rectangle){176,185,1825,400},destination,(Vector2){0},0,WHITE);
    else DrawEllipse((int)base.x,(int)(base.y+3),width/2,height/2,(Color){156,89,48,255});
}

static void draw_aloe_guard(Texture2D texture, Object object)
{
    Vector2 base = objects[object].position;
    float time = (float)GetTime();
    float bob = sinf(time * 2.4f) * 1.6f;
    float sway = sinf(time * 1.7f) * 1.2f;
    const float height = 190.0f;
    const float width = height * (float)texture.width / (float)texture.height;
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    Rectangle destination = {base.x, base.y + bob, width, height};
    DrawTexturePro(texture, source, destination, (Vector2){width / 2, height}, sway, WHITE);
}

static void draw_sunflower_guard(Texture2D texture, Object object)
{
    Vector2 base = objects[object].position;
    float time = (float)GetTime();
    float sway = sinf(time * 1.6f) * 1.0f;
    const float height = 174.0f;
    const float width = height * (float)texture.width / (float)texture.height;
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    Rectangle destination = {base.x, base.y, width, height};
    DrawTexturePro(texture, source, destination, (Vector2){width / 2, height}, sway, WHITE);
}

static void draw_room_portal(const UiTheme *ui, Object object, const char *label, bool locked)
{
    Vector2 p = objects[object].position;
    int x = (int)p.x, y = (int)p.y;
    DrawRectangle(x - 31, y - 26, 62, 43, frame);
    DrawRectangle(x - 25, y - 20, 50, 31, locked ? MAROON : path_edge);
    DrawRectangle(x - 12, y - 9, 24, 5, cream);
    if (object == INTERIOR_DOOR || object == BASEMENT_HATCH)
        centered_text(ui, label, x + 132, y - 11, 16, ink);
    else centered_text(ui, label, x, y + 22, 18, ink);
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
    Vector2 drain = objects[DRAIN].position;
    DrawCircle((int)drain.x + 14, (int)drain.y - 27, 4, world->drain_open ? LIME : ORANGE);
    Vector2 valve = objects[VALVE].position;
    DrawCircle((int)valve.x + 22, (int)valve.y - 35, 4, world->water ? LIME : ORANGE);
}

static void draw_exterior(const UiTheme *ui, const World *world, const WorldAssets *assets)
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
        DrawCircle(744, 538, 3, world->drain_open ? LIME : ORANGE);
        DrawCircle(1194, 638, 3, world->water ? LIME : ORANGE);
    } else {
        draw_switch(ui, DRAIN, "Dreno", world->drain_open);
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

static void draw_interior(const UiTheme *ui, const World *world, const WorldAssets *assets)
{
    const Sector *room = &rooms[world->location];
    Rectangle b = room->bounds;

    Texture2D interior = assets->basement_interior;
    if (world->location == WORLD_MAINTENANCE) interior = assets->maintenance_interior;
    if (world->location == WORLD_DEPOT) interior = assets->depot_interior;
    if (world->location == WORLD_BRIDGE_CONTROL) interior = assets->bridge_control_interior;
    if (world->location == WORLD_NURSERY) interior = assets->nursery_interior;
    if ((world->location == WORLD_MAINTENANCE && assets->maintenance_interior.id) ||
        (world->location == WORLD_DEPOT && assets->depot_interior.id) ||
        (world->location == WORLD_BASEMENT && assets->basement_interior.id) ||
        (world->location == WORLD_BRIDGE_CONTROL && assets->bridge_control_interior.id) ||
        (world->location == WORLD_NURSERY && assets->nursery_interior.id)) {
        /* Recorta a margem preta da referência e enquadra a sala no mapa. */
        DrawTexturePro(interior,
                       (Rectangle){INTERIOR_CROP_X, INTERIOR_CROP_Y,
                                   INTERIOR_CROP_WIDTH, INTERIOR_CROP_HEIGHT},
                       b, (Vector2){0}, 0, WHITE);
        if (world->location == WORLD_MAINTENANCE) draw_maintenance_details(world);
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
                draw_switch(ui, ALARM, "Painel do alarme", world->alarm);
                draw_note(ui, ALARM_NOTE, "Bilhete da manutenção", world->alarm_note);
                draw_room_portal(ui, LOCKED_HATCH,
                                 world->maintenance_key ? "Escotilha" : "Escotilha trancada",
                                 !world->maintenance_key);
            }
            break;
        case WORLD_DEPOT:
            if (!assets->depot_interior.id) {
                for (int i = 0; i < 3; i++) {
                    DrawRectangle(330, 283 + 75 * i, 115, 44, Fade(room->trim, 0.45f));
                    DrawRectangle(835, 283 + 75 * i, 115, 44, Fade(room->trim, 0.45f));
                }
            }
            if (!world->maintenance_key) {
                draw_boss_ground(assets->boss_ground,ALOE);
                if (assets->aloe_guard.id)
                    draw_aloe_guard(assets->aloe_guard, ALOE);
                else
                    draw_guard(ALOE, (Color){73, 162, 120, 255});
            }
            break;
        case WORLD_BASEMENT:
            if (assets->basement_interior.id) {
                /* Indicador do painel que alimenta o refletor. */
                DrawCircle(615, 330, 5, world->light ? (Color){91, 190, 78, 255}
                                                      : (Color){207, 68, 48, 255});
            } else {
                draw_switch(ui, POWER, "Distribuidor de energia", world->light);
                draw_room_portal(ui, BASEMENT_HATCH, "Subir pela escotilha", false);
            }
            break;
        case WORLD_NURSERY:
            if (assets->nursery_interior.id) {
                for (int i = 0; i < WORLD_RELAY_COUNT; i++) {
                    Color led = !world->guardian_defeated ? (Color){90, 96, 79, 255}
                        : ((world->relay_mask & (1u << i)) ? (Color){109, 221, 103, 255}
                                                           : (Color){202, 164, 73, 255});
                    DrawRectangle(773 + (i % 4) * 9, 320 + (i / 4) * 7, 5, 4, led);
                }
            } else {
                DrawRectangle(306, 284, 116, 194, Fade(DARKGREEN, 0.27f));
                DrawRectangle(875, 284, 99, 194, Fade(DARKGREEN, 0.27f));
            }
            if (!world->guardian_defeated) {
                draw_boss_ground(assets->boss_ground,SUNFLOWER);
                if (assets->sunflower_guard.id)
                    draw_sunflower_guard(assets->sunflower_guard, SUNFLOWER);
                else
                    draw_guard(SUNFLOWER, GOLD);
            }
            if (!assets->nursery_interior.id)
                draw_switch(ui, RELAY_PANEL, "Central de relés", world->guardian_defeated);
            break;
        case WORLD_BRIDGE_CONTROL:
            if (assets->bridge_control_interior.id) {
                DrawCircle(539, 378, 6, world->north_channel ? (Color){91, 190, 78, 255}
                                                              : (Color){88, 94, 83, 255});
                DrawCircle(742, 378, 6, world->south_channel ? (Color){91, 190, 78, 255}
                                                              : (Color){88, 94, 83, 255});
            } else {
                DrawRectangle(450, 410, 380, 9, frame);
                DrawRectangle(450, 410, 190, 9, world->north_channel ? water_color : frame);
                DrawRectangle(640, 410, 190, 9, world->south_channel ? water_color : frame);
                draw_switch(ui, NORTH_CHANNEL, TextFormat("Canal norte (+%d)", NORTH_PRESSURE), world->north_channel);
                draw_switch(ui, SOUTH_CHANNEL, TextFormat("Canal sul (+%d)", SOUTH_PRESSURE), world->south_channel);
                centered_text(ui, TextFormat("Dreno externo %s", world->drain_open ? "aberto" : "fechado"),
                              640, 443, 20, ink);
            }
            break;
        default: break;
    }
    if (world->location != WORLD_BASEMENT &&
        !((world->location == WORLD_MAINTENANCE && assets->maintenance_interior.id) ||
          (world->location == WORLD_DEPOT && assets->depot_interior.id) ||
          (world->location == WORLD_BASEMENT && assets->basement_interior.id) ||
          (world->location == WORLD_BRIDGE_CONTROL && assets->bridge_control_interior.id) ||
          (world->location == WORLD_NURSERY && assets->nursery_interior.id)))
        draw_room_portal(ui, INTERIOR_DOOR, "Voltar ao pátio", false);
}

static void draw_npc(Texture2D atlas, int sprite, Vector2 base, Camera2D camera)
{
    Vector2 screen = GetWorldToScreen2D(base, camera);
    base = GetScreenToWorld2D((Vector2){roundf(screen.x), roundf(screen.y)}, camera);
    /* Mantém a NPC na escala visual do explorador em interiores e no exterior. */
    float character_screen_scale = camera.zoom > 1.01f ? 0.36f : 0.23f;
    float scale = (256.0f * character_screen_scale * 0.82f / 56.0f) / camera.zoom;
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

void world_draw_tree_canopy(const World *world, const WorldAssets *assets, Vector2 feet)
{
    if (world->location != WORLD_EXTERIOR || !world->detail_enabled ||
        !assets->courtyard_detail.id || !assets->tree_canopy_foreground.id ||
        feet.x < 550 || feet.x > 755 || feet.y > 350) return;

    /* Reutiliza as folhas originais com transparência ao redor da silhueta. */
    DrawTexturePro(assets->tree_canopy_foreground,
                   (Rectangle){0, 0, TREE_CANOPY_WIDTH, TREE_CANOPY_HEIGHT},
                   (Rectangle){TREE_CANOPY_X, TREE_CANOPY_Y,
                               TREE_CANOPY_WORLD_WIDTH, TREE_CANOPY_WORLD_HEIGHT},
                   (Vector2){0}, 0, WHITE);
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

void world_draw(const UiTheme *ui, const World *world, const WorldAssets *assets)
{
    if (world->location == WORLD_EXTERIOR) {
        draw_exterior(ui, world, assets);
        if (!assets->courtyard.id) {
            draw_sign(assets->tiles,ENTRANCE_SIGN);
            draw_sign(assets->tiles,LOGIC_SIGN);
        }
    } else {
        draw_interior(ui, world, assets);
    }
}

void world_draw_interior_foreground(const World *world, const WorldAssets *assets, Vector2 feet)
{
    if (world->location <= WORLD_EXTERIOR || world->location >= WORLD_LOCATION_COUNT ||
        !CheckCollisionPointRec(feet, interior_portal_depth)) return;

    bool has_room_art = false;
    switch (world->location) {
        case WORLD_MAINTENANCE: has_room_art = assets->maintenance_interior.id != 0; break;
        case WORLD_DEPOT: has_room_art = assets->depot_interior.id != 0; break;
        case WORLD_NURSERY: has_room_art = assets->nursery_interior.id != 0; break;
        case WORLD_BRIDGE_CONTROL: has_room_art = assets->bridge_control_interior.id != 0; break;
        default: break;
    }
    if (!has_room_art) return;

    Texture2D portal = assets->interior_portal_foreground[world->location];
    if (!portal.id) return;

    Rectangle room = rooms[world->location].bounds;
    const float scale_x = room.width / (float)INTERIOR_CROP_WIDTH;
    const float scale_y = room.height / (float)INTERIOR_CROP_HEIGHT;
    Rectangle destination = {
        room.x + (PORTAL_CROP_X - INTERIOR_CROP_X) * scale_x,
        room.y + (PORTAL_CROP_Y - INTERIOR_CROP_Y) * scale_y,
        PORTAL_CROP_WIDTH * scale_x,
        PORTAL_CROP_HEIGHT * scale_y
    };

    DrawTexturePro(portal,
                   (Rectangle){0, 0, PORTAL_CROP_WIDTH, PORTAL_CROP_HEIGHT},
                   destination, (Vector2){0}, 0, WHITE);
}

static void draw_condition(const UiTheme *ui, int x, int y, bool met, const char *label)
{
    Color color = met ? (Color){45,100,61,255} : (Color){139,89,33,255};
    DrawRectangleLinesEx((Rectangle){x,y+7,16,16}, 1, color);
    if (met) {
        DrawLine(x+3,y+15,x+6,y+18,color);
        DrawLine(x+6,y+18,x+12,y+11,color);
    } else {
        DrawRectangle(x+5,y+14,6,2,color);
    }
    DrawTextEx(ui_pixel_font(ui), label, (Vector2){x+28,y}, 24, 0, ink);
}

static Rectangle draw_challenge(const UiTheme *ui, const World *world, Object object, Vector2 feet)
{
    bool bridge = world->location == WORLD_BRIDGE_CONTROL || object == DRAIN ||
                  object == BRIDGE_CONTROL_DOOR ||
                  (world->location == WORLD_EXTERIOR && feet.x >= canal.x - 125 && feet.x <= canal.x + canal.width + 125 &&
                   feet.y >= passage_top - 125 && feet.y <= passage_bottom + 125);
    bool nursery = world->location == WORLD_EXTERIOR && feet.x >= 1270 && feet.y <= 475;
    if (!bridge && !nursery) return (Rectangle){0};

    const int top = 106;
    Rectangle card = {822,top,438,bridge ? 182 : 216};
    int left = (int)card.x+34;
    int right = (int)(card.x+card.width)-34;
    bool open = bridge ? world_bridge_open(world) : world_gate_open(world);
    Color status_color = open ? (Color){45,100,61,255} : (Color){139,89,33,255};
    Font font = ui_pixel_font(ui);
    ui_botanical_panel(ui, card);
    DrawTextEx(font, bridge ? "Ponte" : "Porta do viveiro", (Vector2){left,top+16}, 26, 0, ink);
    const char *status = bridge ? (open ? "Estendida" : "Recolhida") : (open ? "Aberta" : "Fechada");
    DrawTextEx(font, status, (Vector2){right-ceilf(MeasureTextEx(font,status,20,0).x),top+21}, 20, 0, status_color);
    DrawLine(left,top+56,right,top+56,Fade(frame,0.26f));

    const char *rule;
    if (bridge) {
        bool one_channel = world->north_channel != world->south_channel;
        int pressure = world_bridge_pressure(world);
        const char *channels = one_channel ? (world->north_channel ? "Só o canal norte aberto" : "Só o canal sul aberto")
            : world->north_channel ? "Dois canais abertos" : "Nenhum canal aberto";
        const char *pressure_label = pressure < BRIDGE_TARGET_PRESSURE ? "baixa"
            : pressure > BRIDGE_TARGET_PRESSURE ? "alta" : "correta";
        draw_condition(ui, left, top + 70, one_channel, channels);
        draw_condition(ui, left, top + 104, pressure == BRIDGE_TARGET_PRESSURE,
                       TextFormat("Pressão %d/%d  ·  %s", pressure, BRIDGE_TARGET_PRESSURE, pressure_label));
        rule = TextFormat("(Norte XOR Sul) E (Pressão = %d)", BRIDGE_TARGET_PRESSURE);
    } else {
        draw_condition(ui, left, top + 70, world->water, world->water ? "Água liberada" : "Água fechada");
        draw_condition(ui, left, top + 104, world->light, world->light ? "Refletor alimentado" : "Refletor sem energia");
        draw_condition(ui, left, top + 138, !world->alarm, world->alarm ? "Alarme ligado" : "Alarme desligado");
        rule = world->gate_unlocked ? "Trava acionada · porta liberada" : "Água E Luz E NÃO Alarme";
    }
    DrawTextEx(font, rule, (Vector2){left,top+card.height-39}, 20, 0, ink);
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
    return "Resolva a Central de Relés";
}

static const char *interaction_hint(const World *world, Object object)
{
    static const char *const prompts[OBJECT_COUNT] = {
        [VALVE] = "Alternar água",
        [POWER] = "Redirecionar energia",
        [ALARM] = "Alternar alarme",
        [NORTH_CHANNEL] = "Alternar canal norte",
        [SOUTH_CHANNEL] = "Alternar canal sul",
        [DRAIN] = "Alternar dreno",
        [ALOE] = "Desafiar a Babosa Guardiã",
        [SUNFLOWER] = "Desafiar o Girassol Guardião",
        [CANAL_NOTE] = "Registrar pista dos canais",
        [ALARM_NOTE] = "Ler bilhete da manutenção",
        [MAINTENANCE_DOOR] = "Entrar na casa de manutenção",
        [DEPOT_DOOR] = "Entrar no depósito",
        [NURSERY_DOOR] = "Entrar no viveiro",
        [BRIDGE_CONTROL_DOOR] = "Entrar no controle da ponte",
        [INTERIOR_DOOR] = "Voltar ao pátio",
        [LOCKED_HATCH] = "Descer pela escotilha",
        [BASEMENT_HATCH] = "Subir pela escotilha",
        [RELAY_PANEL] = "Abrir central de relés",
        [ENTRANCE_SIGN] = "Ler placa do jardim",
        [LOGIC_SIGN] = "Resolver desafio de golpe lógico",
        [GARDENER] = "Falar com Lia",
        [TECHNICIAN] = "Falar com Bento",
        [RESEARCHER] = "Falar com Íris"
    };
    if (object == DRAIN) return world->drain_open ? "Fechar dreno" : "Abrir dreno";
    if (object == POWER) return world->light ? "Desligar refletor" : "Ligar refletor";
    if (object == LOCKED_HATCH && !world->maintenance_key)
        return "Escotilha trancada";
    if (object == NURSERY_DOOR && !world_gate_open(world))
        return "Viveiro fechado. Confira as condições.";
    if (object == CANAL_NOTE && world->canal_note) return "Reler a pista dos canais";
    if (object == ALARM_NOTE && world->alarm_note) return "Reler o bilhete da manutenção";
    if (object == RELAY_PANEL && !world->guardian_defeated)
        return "Central bloqueada até você vencer o Girassol.";
    return prompts[object];
}

static Vector2 interaction_anchor(const World *world, Object object)
{
    /* Centro superior visível, independente do ponto usado para alcance e colisão. */
    static const Vector2 marker_anchors[OBJECT_COUNT] = {
        [VALVE]={1168, 615}, [POWER]={640, 301}, [ALARM]={433, 250},
        [NORTH_CHANNEL]={539, 274}, [SOUTH_CHANNEL]={742, 274}, [DRAIN]={733, 530},
        [ALOE]={640, 222}, [SUNFLOWER]={640, 238}, [CANAL_NOTE]={1273, 360},
        [ALARM_NOTE]={540, 273}, [MAINTENANCE_DOOR]={246, 574},
        [DEPOT_DOOR]={278, 301}, [NURSERY_DOOR]={1396, 250},
        [BRIDGE_CONTROL_DOOR]={810, 498}, [INTERIOR_DOOR]={640, 528},
        [LOCKED_HATCH]={846, 375}, [BASEMENT_HATCH]={640, 539},
        [RELAY_PANEL]={782, 296},
        [ENTRANCE_SIGN]={483, 747},
        [LOGIC_SIGN]={505, 406},
        [GARDENER]={630, 454}, [TECHNICIAN]={868, 547}, [RESEARCHER]={790, 432}
    };
    Vector2 anchor = marker_anchors[object];
    if (object == INTERIOR_DOOR && world->location != WORLD_MAINTENANCE) anchor.y = 539;
    if (object >= GARDENER && object <= RESEARCHER)
        anchor.y = objects[object].position.y - 53 / world_camera(world).zoom;
    return anchor;
}

HudInteraction world_interaction_view(const World *world, Vector2 feet)
{
    if (world->dialogue_title || world->logic_quiz_open || world->relay_panel_open || world->completed) return (HudInteraction){0};
    Object object = nearby_object(world, feet);
    if (object == OBJECT_COUNT) return (HudInteraction){0};
    static const char *const labels[OBJECT_COUNT] = {
        [VALVE]="Válvula da água", [POWER]="Distribuidor de energia", [ALARM]="Painel do alarme",
        [NORTH_CHANNEL]="Canal norte", [SOUTH_CHANNEL]="Canal sul", [DRAIN]="Dreno",
        [ALOE]="Babosa guardiã", [SUNFLOWER]="Girassol guardião", [CANAL_NOTE]="Pista dos canais",
        [ALARM_NOTE]="Bilhete da manutenção", [MAINTENANCE_DOOR]="Casa de manutenção",
        [DEPOT_DOOR]="Depósito", [NURSERY_DOOR]="Viveiro", [BRIDGE_CONTROL_DOOR]="Controle da ponte",
        [INTERIOR_DOOR]="Saída", [LOCKED_HATCH]="Escotilha de manutenção", [BASEMENT_HATCH]="Escotilha",
        [RELAY_PANEL]="Central de relés",
        [ENTRANCE_SIGN]="Placa do jardim", [LOGIC_SIGN]="Placa dos golpes lógicos", [GARDENER]="Lia",
        [TECHNICIAN]="Bento", [RESEARCHER]="Íris"
    };
    Vector2 anchor = interaction_anchor(world, object);
    /* Quando o jogador passa na frente, afasta a seta do chapéu sem deslocá-la de lado. */
    float zoom = world_camera(world).zoom;
    float horizontal = fabsf(feet.x-anchor.x) * zoom;
    float clearance = fminf(fmaxf((32-horizontal)/20, 0), 1);
    clearance = clearance*clearance*(3-2*clearance);
    float vertical = fminf(fmaxf((feet.y-anchor.y)*zoom/12+1.5f, 0), 1);
    clearance *= vertical*vertical*(3-2*vertical);
    float player_top = feet.y-58/zoom;
    anchor.y -= fmaxf(anchor.y-player_top, 0)*clearance;
    return (HudInteraction){.state=interaction_state(world, object), .context=labels[object],
        .message=interaction_hint(world, object), .target=objects[object].position, .marker_anchor=anchor};
}

/* Consulta somente a sala atual, sem mudar alcance, colisões ou progresso. */
int world_discovery_markers(const World *world, Vector2 feet, HudDiscoveryMarker *markers, int capacity)
{
    if (!markers || capacity <= 0 || world->dialogue_title || world->logic_quiz_open ||
        world->relay_panel_open || world->completed) return 0;
    Object selected = nearby_object(world,feet);
    Camera2D camera = world_camera(world);
    int count = 0;
    for (int i = 0; i < OBJECT_COUNT && count < capacity; i++) {
        if (!object_present(world,i) || i == (int)selected) continue;
        if ((i == CANAL_NOTE && world->canal_note) || (i == ALARM_NOTE && world->alarm_note) ||
            (i == ENTRANCE_SIGN && world->entrance_sign_read)) continue;
        Vector2 position = GetWorldToScreen2D(interaction_anchor(world,(Object)i),camera);
        /* Sem indicadores presos à borda para objetos fora do enquadramento. */
        if (position.x < 24 || position.x > GAME_SCREEN_WIDTH-24 ||
            position.y < 32 || position.y > GAME_SCREEN_HEIGHT-16) continue;
        markers[count++] = (HudDiscoveryMarker){
            .position = position, .conversation = i >= GARDENER && i <= RESEARCHER,
            .blocked = interaction_state(world,(Object)i) == HUD_INTERACTION_BLOCKED
        };
    }
    return count;
}

static void draw_dialogue(const UiTheme *ui, const World *world)
{
    bool has_next = world->dialogue_page+1 < WORLD_DIALOGUE_PAGE_COUNT && world->dialogue_pages[world->dialogue_page+1];
    dialogue_ui_draw(ui, world->dialogue_title, world->dialogue_pages[world->dialogue_page], has_next,
                     world->dialogue_visible_characters);
}

void world_draw_interface(const UiTheme *ui, const World *world, Vector2 feet)
{
    if (world->logic_quiz_open) return;
    if (world->dialogue_title) {
        draw_dialogue(ui, world);
        return;
    }
    if (world->relay_panel_open) return;
    /* A conclusão é apresentada pela tela final coordenada por game.c. */
    if (world->completed) return;
    const Sector *sector = world->location == WORLD_EXTERIOR ? sector_at(feet) : &rooms[world->location];
    const char *area = sector ? sector->name : "Travessia";
    HudInteraction interaction = world_interaction_view(world, feet);
    draw_challenge(ui, world, nearby_object(world, feet), feet);
    HudDiscoveryMarker markers[OBJECT_COUNT];
    int marker_count = world_discovery_markers(world,feet,markers,OBJECT_COUNT);
    exploration_discovery_draw(markers,marker_count,ui ? ui->marker_time : 0);
    exploration_hud_draw(ui, area, current_objective(world),
                         &interaction, world_camera(world));
    if (world->location == WORLD_BRIDGE_CONTROL) {
        /* Leitura em tamanho fixo, fora do manômetro e dos controles da arte. */
        Rectangle pressure_card = {1000,18,260,64};
        ui_botanical_panel(ui,pressure_card);
        Font font = ui_pixel_font(ui);
        const char *pressure = TextFormat("Pressão: %d / %d",world_bridge_pressure(world),BRIDGE_TARGET_PRESSURE);
        float width = MeasureTextEx(font,pressure,28,0).x;
        DrawTextEx(font,pressure,(Vector2){pressure_card.x+(pressure_card.width-width)/2,36},28,0,ink);
    }
}
