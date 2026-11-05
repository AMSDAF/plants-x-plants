/* Geometria e parâmetros gerados por tools/prepare_menu_grass.py. */
#ifndef MENU_GRASS_DATA_H
#define MENU_GRASS_DATA_H
#include "raylib.h"
#define MENU_GRASS_REGION_COUNT 17
#define MENU_GRASS_FRAME_COUNT 32
#define MENU_GRASS_FPS 8.0f
#define MENU_GRASS_COLUMNS 8
#define MENU_GRASS_PAGE_WIDTH 512
#define MENU_GRASS_PAGE_HEIGHT 408
#define MENU_GRASS_SOURCE_WIDTH 1672
#define MENU_GRASS_SOURCE_HEIGHT 941
typedef struct { Rectangle source; Vector2 origin; Vector2 pivot; float amplitude; float phase; } MenuGrassRegion;
static const MenuGrassRegion MENU_GRASS_REGIONS[MENU_GRASS_REGION_COUNT] = {
    {{88, 360, 80, 40}, {459, 111}, {494.977f, 140.000f}, 2.500f, 0.439f}, /* far_left */
    {{240, 304, 96, 48}, {356, 202}, {385.850f, 241.000f}, 3.200f, 0.384f}, /* upper_left */
    {{336, 304, 88, 48}, {1202, 189}, {1249.745f, 226.000f}, 3.000f, 1.113f}, /* upper_right */
    {{424, 304, 88, 48}, {935, 157}, {978.510f, 193.000f}, 3.000f, 0.869f}, /* upper_middle */
    {{0, 224, 136, 80}, {267, 283}, {348.045f, 358.000f}, 5.000f, 0.337f}, /* middle_left */
    {{136, 224, 120, 80}, {1230, 316}, {1318.122f, 386.000f}, 5.200f, 1.184f}, /* middle_right */
    {{0, 360, 88, 48}, {359, 386}, {404.307f, 423.000f}, 3.100f, 0.455f}, /* rock_left */
    {{368, 224, 120, 56}, {1354, 397}, {1408.333f, 448.000f}, 4.000f, 1.322f}, /* trunk_right */
    {{168, 0, 160, 104}, {258, 488}, {326.613f, 585.000f}, 8.000f, 0.405f}, /* clearing_left_upper */
    {{168, 120, 152, 88}, {1209, 467}, {1251.188f, 550.000f}, 7.000f, 1.222f}, /* clearing_right_upper */
    {{328, 0, 128, 104}, {218, 609}, {266.042f, 707.000f}, 7.600f, 0.416f}, /* clearing_left */
    {{0, 120, 168, 104}, {1227, 688}, {1282.857f, 785.000f}, 8.500f, 1.320f}, /* clearing_right */
    {{0, 304, 112, 56}, {1111, 586}, {1139.000f, 633.000f}, 4.000f, 1.181f}, /* clearing_right_small */
    {{320, 120, 128, 88}, {438, 832}, {500.554f, 910.000f}, 7.000f, 0.689f}, /* bottom_left */
    {{0, 0, 168, 120}, {1108, 807}, {1130.000f, 916.000f}, 8.500f, 1.261f}, /* bottom_right */
    {{112, 304, 128, 56}, {360, 783}, {413.392f, 827.000f}, 4.200f, 0.603f}, /* lower_left_small */
    {{256, 224, 112, 64}, {1530, 522}, {1578.080f, 576.000f}, 4.300f, 1.521f}, /* edge_right */
};
#endif
