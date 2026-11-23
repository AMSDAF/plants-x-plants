/* Carrega e libera as artes da arena e desenha molduras com cantos preservados. */
#include "battle_screen.h"
#include <math.h>

static const Rectangle sources[BATTLE_ART_COUNT] = {
    {0,0,1672,941}, {0,0,1672,941}, {176,185,1825,400},
    {96,76,1961,510}, {35,150,2090,388}, {314,218,1528,289}
};

void battle_screen_assets_load(BattleScreenAssets *assets)
{
    static const char *const paths[BATTLE_ART_COUNT] = {
        "assets/maps/battle/depot_v1.png", "assets/maps/battle/nursery_v1.png",
        "assets/ui/battle/ground_v1.png", "assets/ui/battle/status_frame_v2.png",
        "assets/ui/battle/command_panel_v2.png", "assets/ui/battle/location_plaque_v1.png"
    };
    static const char *const bosses[] = {
        "assets/sprites/plants/aloe_v2.png", "assets/sprites/plants/sunflower_v2.png"
    };
    *assets = (BattleScreenAssets){0};
    for (int i = 0; i < BATTLE_ART_COUNT; i++) {
        if (!FileExists(paths[i])) continue;
        Texture2D texture = LoadTexture(paths[i]);
        Rectangle source = sources[i];
        if (texture.id && (texture.width < source.x+source.width || texture.height < source.y+source.height)) {
            UnloadTexture(texture);
            continue;
        }
        if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        assets->textures[i] = texture;
    }
    for (int i = 0; i < 2; i++) {
        if (!FileExists(bosses[i])) continue;
        Image image = LoadImage(bosses[i]);
        if (!image.data) continue;
        Rectangle source = GetImageAlphaBorder(image, 128.0f/255.0f);
        if (source.width > 0 && source.height > 0) {
            assets->bosses[i] = LoadTextureFromImage(image);
            assets->boss_sources[i] = source;
            if (assets->bosses[i].id) SetTextureFilter(assets->bosses[i], TEXTURE_FILTER_POINT);
        }
        UnloadImage(image);
    }
}

void battle_screen_assets_unload(BattleScreenAssets *assets)
{
    for (int i = 0; i < BATTLE_ART_COUNT; i++)
        if (assets->textures[i].id) UnloadTexture(assets->textures[i]);
    for (int i = 0; i < 2; i++)
        if (assets->bosses[i].id) UnloadTexture(assets->bosses[i]);
    *assets = (BattleScreenAssets){0};
}

void battle_screen_draw_art(const UiTheme *ui, const BattleScreenAssets *assets,
                             BattleArt art, Rectangle bounds)
{
    Texture2D texture = assets->textures[art];
    if (!texture.id) {
        if (art <= BATTLE_ART_NURSERY) ClearBackground((Color){195,181,142,255});
        else if (art == BATTLE_ART_GROUND)
            DrawEllipse((int)(bounds.x+bounds.width/2), (int)(bounds.y+bounds.height/2),
                         bounds.width/2, bounds.height/2, (Color){156,89,48,255});
        else ui_botanical_panel(ui, bounds);
        return;
    }
    Rectangle source = sources[art];
    if (art <= BATTLE_ART_NURSERY) {
        float scale = fmaxf(bounds.width/source.width, bounds.height/source.height);
        source.x += (source.width-bounds.width/scale)/2;
        source.y += (source.height-bounds.height/scale)/2;
        source.width = bounds.width/scale;
        source.height = bounds.height/scale;
    }
    if (art <= BATTLE_ART_GROUND) {
        DrawTexturePro(texture, source, bounds, (Vector2){0}, 0, WHITE);
        return;
    }
    /* Insets registrados no manifesto; cantos e folhas seguem a escala da altura. */
    float left = art == BATTLE_ART_STATUS ? 220 : art == BATTLE_ART_COMMANDS ? 150 : 190;
    float right = left;
    float top = art == BATTLE_ART_STATUS ? 175 : art == BATTLE_ART_COMMANDS ? 110 : 65;
    float bottom = art == BATTLE_ART_STATUS ? 80 : top;
    float scale = bounds.height/source.height;
    float sx[4] = {source.x,source.x+left,source.x+source.width-right,source.x+source.width};
    float sy[4] = {source.y,source.y+top,source.y+source.height-bottom,source.y+source.height};
    float dx[4] = {bounds.x,bounds.x+left*scale,bounds.x+bounds.width-right*scale,bounds.x+bounds.width};
    float dy[4] = {bounds.y,bounds.y+top*scale,bounds.y+bounds.height-bottom*scale,bounds.y+bounds.height};
    for (int row = 0; row < 3; row++) for (int col = 0; col < 3; col++)
        DrawTexturePro(texture, (Rectangle){sx[col],sy[row],sx[col+1]-sx[col],sy[row+1]-sy[row]},
                         (Rectangle){dx[col],dy[row],dx[col+1]-dx[col],dy[row+1]-dy[row]},
                         (Vector2){0},0,WHITE);
}
