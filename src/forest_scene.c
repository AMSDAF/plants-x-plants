/* Compõe camadas de floresta e anima vento, nuvens e folhas sem alterar a interface. */
#include "forest_scene.h"
#include "selection_scene_data.h"
#include <math.h>

typedef struct {
    int sprite;
    Vector2 attachment;
    float width;
    Vector2 pivot;
    float angle;
    float amplitude;
    float phase;
} ForestPlacement;

typedef struct {
    int sprite;
    float initial_x;
    float y;
    float width;
    float speed;
} CloudPlacement;

/* Pivôs relativos ao recorte, em pontos encobertos pela madeira ou pelo solo. */
static const ForestPlacement CANOPIES[] = {
    {0, {85, 28}, 400, {0.08f, 0.13f}, -5, 0.65f, 0.2f},
    {1, {280, 32}, 310, {0.30f, 0.10f}, 3, 0.55f, 1.1f},
    {2, {1205, 24}, 440, {0.94f, 0.12f}, 5, 0.60f, 0.8f},
    {3, {48, 281}, 205, {0.08f, 0.11f}, -12, 0.80f, 1.8f},
    {5, {1225, 296}, 215, {0.88f, 0.12f}, 10, 0.75f, 2.4f}
};
static const ForestPlacement GROUND[] = {
    {0, {74, 754}, 265, {0.70f, 0.94f}, -4, 0.80f, 0.4f},
    {3, {164, 745}, 210, {0.50f, 0.94f}, 4, 0.65f, 1.5f},
    {2, {1215, 756}, 280, {0.30f, 0.94f}, 4, 0.85f, 1.0f},
    {5, {1110, 746}, 205, {0.49f, 0.94f}, -3, 0.60f, 2.1f}
};
static const CloudPlacement CLOUDS[FOREST_CLOUD_COUNT] = {
    {0, -80, 135, 280, 4.2f}, {3, 510, 178, 340, 5.8f}, {1, 1130, 108, 290, 3.6f}
};

static Texture2D load_layer(const char *path, int width, int height)
{
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "Camada opcional ausente: %s", path);
        return (Texture2D){0};
    }
    Texture2D texture = LoadTexture(path);
    if (texture.id && (texture.width != width || texture.height != height)) {
        TraceLog(LOG_WARNING, "Camada opcional com dimensoes invalidas: %s", path);
        UnloadTexture(texture);
        return (Texture2D){0};
    }
    if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

void forest_scene_assets_load(ForestSceneAssets *assets)
{
    *assets = (ForestSceneAssets){0};
    assets->base = load_layer("assets/sprites/selection/prepared/background_base.png", 1672, 941);
    assets->trunks = load_layer("assets/sprites/selection/prepared/tree_trunks.png", 1672, 941);
    assets->clouds = load_layer("assets/sprites/selection/prepared/clouds.png", 1024, 1536);
    assets->canopies = load_layer("assets/sprites/selection/prepared/canopies.png", 1536, 1024);
    assets->ground = load_layer("assets/sprites/selection/prepared/ground_foliage.png", 1536, 1024);
    assets->leaves = load_layer("assets/sprites/selection/prepared/leaf_particles.png", 2048, 1536);
}

void forest_scene_assets_unload(ForestSceneAssets *assets)
{
    Texture2D textures[] = {assets->base, assets->trunks, assets->clouds,
                            assets->canopies, assets->ground, assets->leaves};
    for (unsigned int i = 0; i < sizeof(textures) / sizeof(textures[0]); i++)
        if (textures[i].id) UnloadTexture(textures[i]);
    *assets = (ForestSceneAssets){0};
}

void forest_scene_init(ForestScene *scene)
{
    *scene = (ForestScene){0};
    for (int i = 0; i < FOREST_CLOUD_COUNT; i++) scene->cloud_x[i] = CLOUDS[i].initial_x;
    for (int i = 0; i < FOREST_LEAF_COUNT; i++) {
        scene->leaves[i].position = (Vector2){(float)((i * 397 + 73) % 1360 - 40),
                                            (float)((i * 173 + 25) % 820 - 60)};
        scene->leaves[i].rotation = i * 37.0f;
    }
}

void forest_scene_update(ForestScene *scene, float dt)
{
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    scene->wind_phase = fmodf(scene->wind_phase + dt * 0.45f, 2 * PI);
    for (int i = 0; i < FOREST_CLOUD_COUNT; i++) {
        scene->cloud_x[i] += CLOUDS[i].speed * dt;
        /* A troca acontece somente com a nuvem inteira fora da janela. */
        if (scene->cloud_x[i] - CLOUDS[i].width / 2 > 1280)
            scene->cloud_x[i] -= 1280 + CLOUDS[i].width;
    }
    for (int i = 0; i < FOREST_LEAF_COUNT; i++) {
        ForestLeaf *leaf = &scene->leaves[i];
        float depth = 0.55f + (i % 3) * 0.22f;
        leaf->position.x += dt * (10 + 7 * sinf(scene->wind_phase + i)) * depth;
        leaf->position.y += dt * (20 + i % 4 * 4) * depth;
        leaf->rotation = fmodf(leaf->rotation + dt * (12 + i % 3 * 8), 360);
        if (leaf->position.y > 760) leaf->position.y -= 820;
        if (leaf->position.x > 1320) leaf->position.x -= 1360;
    }
}

static void draw_placement(Texture2D texture, Rectangle source, ForestPlacement placement, float wind_phase)
{
    if (!texture.id) return;
    float height = placement.width * source.height / source.width;
    Rectangle dest = {placement.attachment.x, placement.attachment.y, placement.width, height};
    Vector2 origin = {placement.pivot.x * placement.width, placement.pivot.y * height};
    float wind = 0.7f * sinf(wind_phase) + 0.3f * sinf(wind_phase + placement.phase);
    DrawTexturePro(texture, source, dest, origin, placement.angle + placement.amplitude * wind, WHITE);
}

void forest_scene_draw(const ForestScene *scene, const ForestSceneAssets *assets)
{
    ClearBackground((Color){45, 60, 43, 255});
    if (assets->base.id) {
        float scale = fmaxf(1280.0f / assets->base.width, 720.0f / assets->base.height);
        float width = assets->base.width * scale, height = assets->base.height * scale;
        DrawTexturePro(assets->base, (Rectangle){0, 0, (float)assets->base.width, (float)assets->base.height},
                       (Rectangle){(1280 - width) / 2, (720 - height) / 2, width, height}, (Vector2){0}, 0, WHITE);
    }
    if (assets->clouds.id) {
        for (int i = 0; i < FOREST_CLOUD_COUNT; i++) {
            Rectangle source = CLOUDS_SOURCES[CLOUDS[i].sprite];
            float height = CLOUDS[i].width * source.height / source.width;
            DrawTexturePro(assets->clouds, source,
                           (Rectangle){scene->cloud_x[i] - CLOUDS[i].width / 2, CLOUDS[i].y, CLOUDS[i].width, height},
                           (Vector2){0}, 0, Fade(WHITE, 0.85f));
        }
    }
    if (assets->trunks.id) {
        float width = 1320, height = width * assets->trunks.height / assets->trunks.width;
        DrawTexturePro(assets->trunks, (Rectangle){0, 0, (float)assets->trunks.width, (float)assets->trunks.height},
                       (Rectangle){-20, 720 - height + 16, width, height}, (Vector2){0}, 0, WHITE);
        /* Sem troncos, evita mostrar galhos de dossel flutuando. */
        for (unsigned int i = 0; i < sizeof(CANOPIES) / sizeof(CANOPIES[0]); i++)
            draw_placement(assets->canopies, CANOPIES_SOURCES[CANOPIES[i].sprite], CANOPIES[i], scene->wind_phase);
    }
    for (unsigned int i = 0; i < sizeof(GROUND) / sizeof(GROUND[0]); i++)
        draw_placement(assets->ground, GROUND_FOLIAGE_SOURCES[GROUND[i].sprite], GROUND[i], scene->wind_phase);
    if (assets->leaves.id) {
        for (int i = 0; i < FOREST_LEAF_COUNT; i++) {
            float depth = 0.55f + (i % 3) * 0.22f;
            Rectangle source = LEAF_PARTICLES_SOURCES[(i * 5) % 12];
            float width = 16 * depth, height = width * source.height / source.width;
            DrawTexturePro(assets->leaves, source,
                           (Rectangle){scene->leaves[i].position.x, scene->leaves[i].position.y, width, height},
                           (Vector2){width / 2, height / 2}, scene->leaves[i].rotation, Fade(WHITE, 0.5f + depth * 0.35f));
        }
    }
}
