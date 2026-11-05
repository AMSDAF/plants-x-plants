/* Gerado por tools/generate_menu_wind_test.py: layout e coordenadas do vento. */
#ifndef MENU_WIND_TEST_DATA_H
#define MENU_WIND_TEST_DATA_H

#include "raylib.h"

#define WIND_REGION_COUNT 10
#define WIND_FRAME_COUNT 64
#define WIND_FRAME_COLUMNS 8
#define WIND_PLAYBACK_FPS 12.0f
#define WIND_ATLAS_WIDTH 2048
#define WIND_ATLAS_HEIGHT 1792
#define WIND_SOURCE_WIDTH 1672
#define WIND_SOURCE_HEIGHT 941

typedef struct {
    Vector2 origin;
    Vector2 frame_size;
    Vector2 atlas_origin;
} MenuWindRegion;

static const MenuWindRegion WIND_REGIONS[WIND_REGION_COUNT] = {
    {{447, 262}, {88, 48}, {896, 1344}}, /* Regiao: left_fern */
    {{1275, 442}, {48, 56}, {0, 1344}}, /* Regiao: right_fern */
    {{189, 11}, {64, 64}, {640, 704}}, /* Regiao: left_canopy */
    {{1297, 31}, {72, 88}, {0, 0}}, /* Regiao: right_canopy */
    {{249, 170}, {48, 72}, {1600, 0}}, /* Regiao: left_moss */
    {{282, 184}, {64, 80}, {576, 0}}, /* Regiao: left */
    {{1338, 112}, {64, 80}, {1088, 0}}, /* Regiao: right */
    {{686, 62}, {64, 56}, {384, 1344}}, /* Regiao: upper_twig */
    {{299, 257}, {96, 64}, {1152, 704}}, /* Regiao: left_twig */
    {{1401, 268}, {80, 80}, {0, 704}}, /* Regiao: right_twig */
};

#endif
