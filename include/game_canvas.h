/* Renderiza coordenadas lógicas na resolução física do viewport. */
#ifndef GAME_CANVAS_H
#define GAME_CANVAS_H

#include "raylib.h"

typedef struct {
    RenderTexture2D target;
    Rectangle viewport;
} GameCanvas;

bool game_canvas_resize(GameCanvas *canvas, int width, int height, int render_width, int render_height);
void game_canvas_map_mouse(const GameCanvas *canvas);
void game_canvas_begin(const GameCanvas *canvas);
void game_canvas_present(const GameCanvas *canvas);
void game_canvas_begin_scissor(Rectangle bounds);
void game_canvas_unload(GameCanvas *canvas);

#endif
