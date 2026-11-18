/* Carrega os atlas preparados e controla o treinador ancorado pelos pés. */
#include "player.h"
#include "ui_theme.h"
#include <math.h>

enum { BOTANICAL_CELL = 256, BOTANICAL_ANCHOR_Y = 240 };

void player_assets_load(PlayerAssets *assets)
{
    const char *paths[TRAINER_COUNT] = {
        "assets/sprites/trainers/male_botanical.png", "assets/sprites/trainers/female_botanical.png"
    };
    *assets = (PlayerAssets){0};
    for (int i = 0; i < TRAINER_COUNT; i++) {
        Texture2D texture = LoadTexture(paths[i]);
        if (texture.id && (texture.width != 3 * BOTANICAL_CELL || texture.height != 4 * BOTANICAL_CELL)) {
            UnloadTexture(texture);
            texture = (Texture2D){0};
        }
        if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        else TraceLog(LOG_WARNING, "Treinador botanico ausente/invalido: %s; usando marcador.", paths[i]);
        assets->botanical_sheets[i] = texture;
    }
}

void player_assets_unload(PlayerAssets *assets)
{
    for (int i = 0; i < TRAINER_COUNT; i++) {
        if (assets->botanical_sheets[i].id) UnloadTexture(assets->botanical_sheets[i]);
    }
    *assets = (PlayerAssets){0};
}

void player_init(Player *player, Trainer trainer, Vector2 feet)
{
    *player = (Player){.trainer = trainer, .feet = feet, .facing = FACING_SOUTH};
}

void player_update(Player *player, const World *world, float dt)
{
    Vector2 next = player->feet;
    int horizontal = (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    int vertical = (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
    Facing facing = player->facing;
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    bool running = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (horizontal || vertical) {
        float distance = (running ? 300.0f : 190.0f) * dt;
        /* Passos pequenos evitam saltar por cima da colisão em quadros longos. */
        int steps = 1 + (int)(distance / 5);
        float step = distance / steps;
        if (horizontal && vertical) step *= 0.70710678f;
        for (int i = 0; i < steps; i++) {
            if (horizontal)
                next = world_constrain(world, next, (Vector2){next.x + horizontal * step, next.y});
            if (vertical)
                next = world_constrain(world, next, (Vector2){next.x, next.y + vertical * step});
        }
        if (vertical && (next.y != player->feet.y || next.x == player->feet.x))
            facing = vertical < 0 ? FACING_NORTH : FACING_SOUTH;
        else if (horizontal)
            facing = horizontal < 0 ? FACING_WEST : FACING_EAST;
    }
    bool moved = next.x != player->feet.x || next.y != player->feet.y;
    if (!moved || facing != player->facing) player->walk_time = 0;
    float cycle = player->trainer == TRAINER_MALE ? PLAYER_MALE_WALK_CYCLE_SECONDS : 0.56f;
    if (moved) player->walk_time = fmodf(player->walk_time + dt * (running ? 1.5f : 1.0f), cycle);
    player->facing = facing;
    player->feet = next;
    player->moving = moved;
}

void player_draw(const UiTheme *ui, const Player *player, const PlayerAssets *assets, Camera2D camera)
{
    float x = roundf(player->feet.x);
    float y = roundf(player->feet.y);
    Texture2D botanical = assets->botanical_sheets[player->trainer];
    if (botanical.id) {
        static const int front_back[4] = {0, 1, 2, 1};
        static const int side[4] = {1, 0, 2, 0};
        bool profile = player->facing == FACING_WEST || player->facing == FACING_EAST;
        float cycle = player->trainer == TRAINER_MALE ? PLAYER_MALE_WALK_CYCLE_SECONDS : 0.56f;
        int phase = (int)(player->walk_time / cycle * 4) % 4;
        int frame = player->moving ? (profile ? side[phase] : front_back[phase]) : (profile ? 0 : 1);
        Rectangle source = {frame * BOTANICAL_CELL, player->facing * BOTANICAL_CELL,
                            BOTANICAL_CELL, BOTANICAL_CELL};
        Vector2 screen_feet = GetWorldToScreen2D(player->feet, camera);
        Vector2 feet = GetScreenToWorld2D((Vector2){roundf(screen_feet.x), roundf(screen_feet.y)}, camera);
        /* Aumenta o explorador nos interiores e compensa o zoom sem deformar o sprite. */
        float screen_scale = camera.zoom > 1.01f ? 0.36f : 0.23f;
        float scale = screen_scale / camera.zoom;
        Rectangle destination = {feet.x - BOTANICAL_CELL / 2.0f * scale,
                                 feet.y - BOTANICAL_ANCHOR_Y * scale,
                                 BOTANICAL_CELL * scale, BOTANICAL_CELL * scale};
        DrawEllipse((int)roundf(feet.x), (int)roundf(feet.y), 9 / camera.zoom, 3 / camera.zoom,
                    Fade(BLACK, 0.22f));
        DrawTexturePro(botanical, source, destination, (Vector2){0}, 0, WHITE);
        return;
    }
    DrawEllipse((int)x, (int)y - 1, 13, 4, Fade(BLACK, 0.22f));
    Color color = player->trainer == TRAINER_MALE ? SKYBLUE : ORANGE;
    DrawRectangle((int)x - 12, (int)y - 65, 24, 60, color);
    DrawCircle((int)x, (int)y - 76, 12, color);
    Vector2 tip = {x, y - 38};
    if (player->facing == FACING_WEST) tip.x -= 20;
    if (player->facing == FACING_EAST) tip.x += 20;
    if (player->facing == FACING_NORTH) tip.y -= 20;
    if (player->facing == FACING_SOUTH) tip.y += 20;
    DrawLineEx((Vector2){x, y - 38}, tip, 3, WHITE);
    ui_text(ui, "Sprite indisponivel", (int)x - 72, (int)y + 8, 16, WHITE);
}
