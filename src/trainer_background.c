/* Compõe o fundo experimental com vento ancorado, nuvens e fallback completo. */
#include "trainer_background.h"
#include "trainer_background_data.h"
#include <math.h>

static Texture2D load_texture(const char *path, int width, int height)
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

void trainer_background_load(TrainerBackgroundAssets *assets)
{
    *assets = (TrainerBackgroundAssets){0};
    assets->original = load_texture("assets/sprites/trainer_selection_background.png", 1672, 941);
    assets->base = load_texture("assets/sprites/selection/background/base.png", 1672, 941);
    assets->atlas = load_texture("assets/sprites/selection/background/atlas.png", 2048,
                                 TRAINER_BACKGROUND_ATLAS_HEIGHT);
    if (!assets->base.id || !assets->atlas.id) {
        if (assets->base.id) UnloadTexture(assets->base);
        if (assets->atlas.id) UnloadTexture(assets->atlas);
        assets->base = assets->atlas = (Texture2D){0};
        TraceLog(LOG_WARNING, "Fundo animado da selecao indisponivel; usando fonte completa.");
    }
}

void trainer_background_unload(TrainerBackgroundAssets *assets)
{
    if (assets->original.id) UnloadTexture(assets->original);
    if (assets->base.id) UnloadTexture(assets->base);
    if (assets->atlas.id) UnloadTexture(assets->atlas);
    *assets = (TrainerBackgroundAssets){0};
}

void trainer_background_draw(const TrainerBackgroundAssets *assets, float elapsed)
{
    ClearBackground((Color){147, 155, 73, 255});
    bool animated = assets->base.id && assets->atlas.id;
    Texture2D base = animated ? assets->base : assets->original;
    if (!base.id) return;
    float scale = fmaxf(GetScreenWidth()/1672.0f, GetScreenHeight()/941.0f);
    Rectangle transform = {(GetScreenWidth()-1672*scale)/2, (GetScreenHeight()-941*scale)/2,
                           1672*scale, 941*scale};
    DrawTexturePro(base, (Rectangle){0,0,1672,941}, transform, (Vector2){0}, 0, WHITE);
    if (!animated) return;
    int frame = (int)(fmodf(elapsed,4.0f)*8) % 32;
    for (int i=0; i<TRAINER_BACKGROUND_REGIONS; i++) {
        const TrainerBackgroundRegion *region = &TRAINER_BACKGROUND_DATA[i];
        Rectangle source = region->poses[region->frames[frame]];
        float x = region->origin.x;
        if (region->cloud) {
            /* Período comum: cópia só reaparece após sair completamente da janela. */
            float span = 1672 + source.width;
            x = fmodf(x + source.width + elapsed/128.0f*span, span)-source.width;
            x = roundf(x);
        }
        Rectangle dest = {transform.x+x*scale, transform.y+region->origin.y*scale,
                          source.width*scale, source.height*scale};
        DrawTexturePro(assets->atlas, source, dest, (Vector2){0}, 0, WHITE);
    }
}
