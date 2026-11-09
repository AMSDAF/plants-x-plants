/* Nuvens e remoção localizada geradas por tools/prepare_versus_clouds.py. */
#ifndef VERSUS_CLOUD_DATA_H
#define VERSUS_CLOUD_DATA_H
#include "raylib.h"
#define VERSUS_CLOUD_COUNT 3
#define VERSUS_CLOUD_SECONDS 3072.0f
#define VERSUS_CLOUD_ATLAS_HEIGHT 302
#define VERSUS_CLOUD_SKY_HEIGHT 80
#define VERSUS_CLOUD_CORRECTION_WIDTH 200
#define VERSUS_CLOUD_CORRECTION_HEIGHT 89
typedef struct { Rectangle source; Vector2 origin; } VersusCloudPatch;
typedef struct { Rectangle source; Vector2 origin; float seconds; } VersusCloud;
static const VersusCloudPatch VERSUS_CLOUD_PATCHES[3] = {
    {{389, 0, 186, 80}, {306, 0}},
    {{1672, 169, 186, 58}, {756, 0}},
    {{200, 0, 189, 88}, {1166, 0}},
};
static const VersusCloud VERSUS_CLOUDS[3] = {
    {{0, 249, 149, 53}, {323, 9}, 512.0f},
    {{149, 249, 149, 43}, {774, 0}, 768.0f},
    {{1858, 169, 150, 57}, {1184, 10}, 1024.0f},
};
static const Rectangle VERSUS_CLOUD_OCCLUDERS[2] = {
    {0, 89, 1672, 80},
    {0, 169, 1672, 80},
};
static const VersusCloudPatch VERSUS_CLOUD_CORRECTION = {{0, 0, 200, 89}, {1336, 208}};
#endif
