/* Recursos e estado do cenário de floresta, independentes da navegação da seleção. */
#ifndef FOREST_SCENE_H
#define FOREST_SCENE_H

#include "raylib.h"

#define FOREST_CLOUD_COUNT 3
#define FOREST_LEAF_COUNT 10

typedef struct {
    Texture2D base;
    Texture2D trunks;
    Texture2D clouds;
    Texture2D canopies;
    Texture2D ground;
    Texture2D leaves;
} ForestSceneAssets;

typedef struct {
    Vector2 position;
    float rotation;
} ForestLeaf;

typedef struct {
    float wind_phase;
    float cloud_x[FOREST_CLOUD_COUNT];
    ForestLeaf leaves[FOREST_LEAF_COUNT];
} ForestScene;

void forest_scene_assets_load(ForestSceneAssets *assets);
void forest_scene_assets_unload(ForestSceneAssets *assets);
void forest_scene_init(ForestScene *scene);
void forest_scene_update(ForestScene *scene, float dt);
void forest_scene_draw(const ForestScene *scene, const ForestSceneAssets *assets);

#endif
