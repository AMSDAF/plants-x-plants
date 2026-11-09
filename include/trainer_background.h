/* Declara recursos e desenho do fundo experimental da seleção. */
#ifndef TRAINER_BACKGROUND_H
#define TRAINER_BACKGROUND_H
#include "raylib.h"

typedef struct {
    Texture2D original;
    Texture2D base;
    Texture2D atlas;
} TrainerBackgroundAssets;

void trainer_background_load(TrainerBackgroundAssets *assets);
void trainer_background_unload(TrainerBackgroundAssets *assets);
void trainer_background_draw(const TrainerBackgroundAssets *assets, float elapsed);

#endif
