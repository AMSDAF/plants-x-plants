/* Touceiras e atlas gerados por tools/prepare_versus_grass.py. */
#ifndef VERSUS_GRASS_DATA_H
#define VERSUS_GRASS_DATA_H
#include "raylib.h"
#define VERSUS_GRASS_COUNT 20
#define VERSUS_GRASS_FRAMES 32
#define VERSUS_GRASS_COLUMNS 8
#define VERSUS_GRASS_PAGE_WIDTH 512
#define VERSUS_GRASS_PAGE_HEIGHT 152
typedef struct { Rectangle source; Vector2 origin; int fps; } VersusGrassRegion;
static const VersusGrassRegion VERSUS_GRASS_REGIONS[VERSUS_GRASS_COUNT] = {
    {{0, 0, 136, 56}, {169, 201}, 8},
    {{152, 56, 64, 32}, {499, 217}, 7},
    {{0, 128, 56, 24}, {738, 225}, 6},
    {{216, 0, 80, 40}, {1092, 181}, 8},
    {{216, 56, 64, 32}, {1216, 283}, 7},
    {{280, 56, 64, 32}, {365, 330}, 6},
    {{344, 56, 64, 32}, {321, 471}, 8},
    {{408, 56, 64, 32}, {933, 429}, 7},
    {{0, 96, 64, 32}, {1267, 457}, 6},
    {{296, 0, 88, 40}, {1399, 495}, 8},
    {{384, 0, 72, 40}, {1592, 514}, 7},
    {{64, 96, 64, 32}, {556, 605}, 6},
    {{128, 96, 64, 32}, {1110, 604}, 8},
    {{192, 96, 72, 32}, {87, 679}, 7},
    {{136, 0, 80, 48}, {199, 731}, 6},
    {{0, 56, 80, 40}, {15, 767}, 8},
    {{264, 96, 64, 32}, {481, 823}, 7},
    {{80, 56, 72, 40}, {1085, 808}, 6},
    {{328, 96, 64, 32}, {1387, 753}, 8},
    {{392, 96, 72, 32}, {1542, 711}, 7},
};
#endif
