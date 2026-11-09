/* Carrega os atlas preparados e controla o treinador ancorado pelos pés. */
#include "player.h"
#include <math.h>

/* Mesmas sequências documentadas e pré-visualizadas por prepare_trainers.py. */
static const int WALK[TRAINER_COUNT][4][4] = {
    {{0, 1, 3, 1}, {0, 1, 0, 3}, {0, 1, 0, 3}, {0, 1, 3, 1}},
    {{0, 1, 0, 1}, {0, 1, 0, 3}, {0, 1, 0, 3}, {0, 1, 3, 1}}
};
static const int IDLE[4] = {1, 1, 3, 1};

void player_assets_load(PlayerAssets *assets)
{
    const char *paths[TRAINER_COUNT] = {
        "assets/sprites/trainers/male.png", "assets/sprites/trainers/female.png"
    };
    *assets = (PlayerAssets){0};
    for (int i = 0; i < TRAINER_COUNT; i++) {
        Texture2D texture = LoadTexture(paths[i]);
        if (texture.id && (texture.width != 1280 || texture.height != 1280)) {
            UnloadTexture(texture);
            texture = (Texture2D){0};
        }
        if (!texture.id) {
            TraceLog(LOG_WARNING, "Treinador ausente/invalido: %s; usando marcador. Execute tools/prepare_trainers.py na raiz.", paths[i]);
        } else {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        }
        assets->sheets[i] = texture;
    }
    const char *experimental_path = "assets/sprites/trainers/male_walk6.png";
    if (FileExists(experimental_path)) {
        Texture2D texture = LoadTexture(experimental_path);
        if (texture.id && texture.width == 1920 && texture.height == 1280) {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
            assets->male_walk6 = texture;
        } else {
            if (texture.id) UnloadTexture(texture);
            TraceLog(LOG_WARNING, "Atlas masculino experimental invalido; mantendo o aprovado.");
        }
    } else {
        TraceLog(LOG_INFO, "Atlas masculino experimental ausente; mantendo o aprovado.");
    }
}

void player_assets_unload(PlayerAssets *assets)
{
    for (int i = 0; i < TRAINER_COUNT; i++)
        if (assets->sheets[i].id) UnloadTexture(assets->sheets[i]);
    if (assets->male_walk6.id) UnloadTexture(assets->male_walk6);
    *assets = (PlayerAssets){0};
}

void player_init(Player *player, Trainer trainer, Vector2 feet)
{
    *player = (Player){.trainer = trainer, .feet = feet, .facing = FACING_SOUTH};
}

void player_update(Player *player, const World *world, float dt)
{
    /* Comparação local reversível; repouso e personagem feminino não mudam. */
    if (player->trainer == TRAINER_MALE && IsKeyPressed(KEY_F6))
        player->use_walk6 = !player->use_walk6;
    Vector2 next = player->feet;
    bool requested = true;
    Facing facing = player->facing;
    /* Prioridade fixa: norte, sul, oeste, leste. Nunca soma eixos. */
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) facing = FACING_NORTH;
    else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) facing = FACING_SOUTH;
    else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) facing = FACING_WEST;
    else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) facing = FACING_EAST;
    else requested = false;
    if (dt < 0) dt = 0;
    if (dt > 0.1f) dt = 0.1f;
    if (requested) {
        float step = 190.0f * dt;
        if (facing == FACING_NORTH) next.y -= step;
        if (facing == FACING_SOUTH) next.y += step;
        if (facing == FACING_WEST) next.x -= step;
        if (facing == FACING_EAST) next.x += step;
        next = world_constrain(world, next);
    }
    bool moved = next.x != player->feet.x || next.y != player->feet.y;
    if (!moved || facing != player->facing) player->walk_time = 0;
    float cycle = player->trainer == TRAINER_MALE ? PLAYER_MALE_WALK_CYCLE_SECONDS : 0.56f;
    if (moved) player->walk_time = fmodf(player->walk_time + dt, cycle);
    if (requested) player->facing = facing;
    player->feet = next;
    player->moving = moved;
}

void player_draw(const Player *player, const PlayerAssets *assets)
{
    Texture2D texture = assets->sheets[player->trainer];
    bool experimental = player->trainer == TRAINER_MALE && player->moving
        && player->use_walk6 && assets->male_walk6.id != 0;
    if (experimental) texture = assets->male_walk6;
    float x = roundf(player->feet.x);
    float y = roundf(player->feet.y);
    DrawEllipse((int)x, (int)y - 1, 13, 4, Fade(BLACK, 0.22f));
    if (!texture.id) {
        Color color = player->trainer == TRAINER_MALE ? SKYBLUE : ORANGE;
        DrawRectangle((int)x - 12, (int)y - 65, 24, 60, color);
        DrawCircle((int)x, (int)y - 76, 12, color);
        Vector2 tip = {x, y - 38};
        if (player->facing == FACING_WEST) tip.x -= 20;
        if (player->facing == FACING_EAST) tip.x += 20;
        if (player->facing == FACING_NORTH) tip.y -= 20;
        if (player->facing == FACING_SOUTH) tip.y += 20;
        DrawLineEx((Vector2){x, y - 38}, tip, 3, WHITE);
        DrawText("Sprite indisponivel", (int)x - 72, (int)y + 8, 16, WHITE);
        return;
    }
    int frame = player->moving ? WALK[player->trainer][player->facing][(int)(player->walk_time / 0.14f) % 4]
                               : IDLE[player->facing];
    if (player->moving && player->trainer == TRAINER_MALE) {
        int count = experimental ? 6 : 4;
        int phase = (int)(player->walk_time / PLAYER_MALE_WALK_CYCLE_SECONDS * count) % count;
        frame = experimental ? phase : WALK[TRAINER_MALE][player->facing][phase];
    }
    /* Escala única 0,30: corpos de aproximadamente 84–89 pixels, sem esticar. */
    Rectangle source = {frame * 320.0f, player->facing * 320.0f, 320, 320};
    Rectangle destination = {x - 48, y - 91.2f, 96, 96};
    DrawTexturePro(texture, source, destination, (Vector2){0}, 0, WHITE);
}
